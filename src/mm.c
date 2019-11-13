#include "mm.h"
#include "arm/mmu.h"
#include "sched.h"

// Holds references to the memory pages.
static unsigned short mem_map[PAGING_PAGES] = {
    0,
};

unsigned long allocate_kernel_page() {
    unsigned long page = get_free_page();
    if (page == 0) {
        return 0;
    }
    return page + VA_START;
}

unsigned long allocate_user_page(struct task_struct *task, unsigned long va) {
    // page here is a physical pointer
    unsigned long page = get_free_page();

    if (page == 0) {
        return 0;
    }

    int ret = map_page(task, va, page);

    // If there was an error, we return 0. Note that we don't return -1 since
    // this is an unsigned long.
    // TODO: There's a lot of cleanup that we should be doing here.
    if (ret < 0) {
        return 0;
    }

    // By adding VA_START, we're effectively making this a virtual address in
    // Kernel space. For example, if get_free_page() returns 4096, then that is
    // the physical address, and VA_START + 4096 is the virtual address is
    // kernel space. So, technically, we can get any physical address and get
    // its virtual address in kernel space by adding VA_START (or determine a
    // physical address by subtracting VA_START from a kernel space virtual
    // address).
    return page + VA_START;
}

// This implementation differs with the implementation in boot.S in:
//   1. This is implemented in C.
//   2. This maps a single page to a single virtual address (not a range). In
//   boot.S, we map all of memory.
//   3. We're not using section mapping here (we use PMD and PTE).
// TODO: Validate the page counts are still within limit (potential buffer
// overflow).
int map_page(struct task_struct *task, unsigned long va, unsigned long page) {
    unsigned long pgd;
    if (!task->mm.pgd) {
        // This is a physical pointer.
        task->mm.pgd = get_free_page();
        if (task->mm.pgd < 0) {
            return -1;
        }
        task->mm.kernel_pages[++(task->mm.kernel_pages_count)] = task->mm.pgd;
    }
    pgd = task->mm.pgd;
    int new_table;
    unsigned long pud =
        map_table((unsigned long *)(pgd + VA_START), PGD_SHIFT, va, &new_table);
    if (pud < 0) {
        return -1;
    }
    if (new_table) {
        task->mm.kernel_pages[++(task->mm.kernel_pages_count)] = pud;
    }

    unsigned long pmd =
        map_table((unsigned long *)(pud + VA_START), PUD_SHIFT, va, &new_table);
    if (pmd < 0) {
        return -1;
    }
    if (new_table) {
        task->mm.kernel_pages[++(task->mm.kernel_pages_count)] = pmd;
    }

    unsigned long pte =
        map_table((unsigned long *)(pmd + VA_START), PMD_SHIFT, va, &new_table);
    if (pte < 0) {
        return -1;
    }
    if (new_table) {
        task->mm.kernel_pages[++(task->mm.kernel_pages_count)] = pte;
    }

    map_table_entry((unsigned long *)(pte + VA_START), va, page);

    // page is a physical address, va is the virtual address
    struct user_page user_page = {page, va};
    task->mm.user_pages[task->mm.user_pages_count++] = user_page;

    return 0;
}

// table is a virtual address in kernel space
// Note that creating a new table (or not) is decided solely by the virtual
// address (va) provided. This uses the va along with shift to get the index on
// the table. If the index is not set, then we get a new page to accomodate the
// next level table.
unsigned long map_table(unsigned long *table, unsigned long shift,
                        unsigned long va, int *new_table) {
    // use va and shift to compute the index in the table.
    unsigned long index = va >> shift;
    index = index & (PTRS_PER_TABLE - 1);

    // figure out if the table already has a valid mapping (set *new_table to 0
    // if yes and return the mapping). We have to do & PAGE_MASK because we use
    // the least significant bits to indicate the type of page we're pointing
    // to. We use the least significant bit to indicate that the descriptor is
    // valid.
    if (table[index]) {
        *new_table = 0;
        return table[index] & PAGE_MASK;
    }

    // create the mapping, store it in table[index], set *new_table to 1 and
    // return the mapping.
    *new_table = 1;
    unsigned long next_level_table = get_free_page();
    if (free_page < 0) {
        return -1;
    }
    unsigned long entry = next_level_table | MM_TYPE_PAGE_TABLE;
    table[index] = entry;
    // We return this since it doesn't have the "| MM_TYPE_PAGE_TABLE", but we
    // could also have retured table[index] & PAGE_MASK
    return next_level_table;
}

// Maps the PMD to the physical address. It uses the va to extract the index. We
// don't need the shift argument here because this function only deals with
// PTEs. pa stands for physical address
void map_table_entry(unsigned long *table, unsigned long va, unsigned long pa) {
    // the least significant 12 bits are the page offset (see the mm.h diagram).
    unsigned long index = (va >> PAGE_SHIFT) & (PTRS_PER_TABLE - 1);
    table[index] = pa | MMU_PTE_FLAGS;
}

// Returns a physical address to a free page.
unsigned long get_free_page() {
    // Iterate through all pages until we find one that is free. At that point,
    // we take it and calculate the offset of that page.
    for (int i = 0; i < PAGING_PAGES; i++) {
        if (mem_map[i] == 0) {
            mem_map[i] = 1;
            // We start at LOW_MEMORY and use the index as an offset of the page
            // size.
            unsigned long page = LOW_MEMORY + i * PAGE_SIZE;
            memzero(page + VA_START, PAGE_SIZE);
            return page;
        }
    }
    return 0;
}

void free_page(unsigned long p) {
    // p = LOW_MEMORY + i * PAGE_SIZE
    // (p - LOW_MEMORY) = i * PAGE_SIZE
    // (p - LOW_MEMORY)/PAGE_SIZE = i
    mem_map[(p - LOW_MEMORY) / PAGE_SIZE] = 0;
}

// Iterates through all user_pages from current and copies them to dst
// (allocates pages for dst).
int copy_virt_memory(struct task_struct *dst) {
    struct task_struct *src = current;

    for (int i = 0; i < src->mm.user_pages_count; i++) {
        unsigned long kernel_va =
            allocate_user_page(dst, src->mm.user_pages[i].virt_addr);
        if (kernel_va == 0) {
            return -1;
        }
        memcpy(kernel_va, src->mm.user_pages[i].virt_addr, PAGE_SIZE);
    }
    return 0;
}

// Not sure if this static ind is shared across all processes, or if it's unique
// per process.
static int ind = 1;

// Ensures that we're handling a translation fault, and then maps a new page to
// the requested address to the current process.
// addr = address that caused the page fault.
// esr = exception syndrome register
int do_mem_abort(unsigned long addr, unsigned long esr) {
    unsigned long dfs = (esr & 0b111111);

    // Verify that this is a translation fault. Page faults can happen for a
    // variety of reasons, including permissions, access, etc. More information
    // in the reference manual in page 2463.
    if ((dfs & 0b111100) != 0b100) {
        return -1;
    }

    unsigned long page = get_free_page();
    if (page == 0) {
        return -1;
    }
    map_page(current, addr & PAGE_MASK, page);
    ind++;

    // Seems like we only want to map 1 page at most.
    if (ind > 2) {
        return -1;
    }
    return 0;
}

unsigned long memcpy(unsigned long dst, unsigned long src, unsigned long n) {
    char *c1 = (char *)dst;
    char *c2 = (char *)src;
    for (unsigned long i = 0; i < n; i++) {
        c1[i] = c2[i];
    }
    return dst;
}