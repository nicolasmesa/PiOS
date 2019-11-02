#ifndef _MM_H
#define _MM_H

#include "peripherals/base.h"
#include "sched.h"

// Virtual addresses in ARM 64 only take 48 bits into consideration. Here, the
// least significant 48 bits are set as 0, and the remaining 16 bits are marked
// as 1. The most significant 16 bits (all 1s) are ignored for address
// calculation purposes, but they are used to tell the CPU to use the
// `ttbr1_el1` register to get the PGD instead of `ttbr1_el0`. `ttbr1_el1`
// points to the kernel PGD, while `ttbr1_el0` points to the user space PGD. An
// address that has the most significant 16 bits set to 0 mean that `ttbr1_el0`
// should be used (user space).
#define VA_START 0xffff000000000000

// 1GB
#define PHYS_MEMORY_SIZE 0x40000000

//                       Virtual address (no Section mapping)
// +-----------------------------------------------------------------------+
// |         | PGD Index | PUD Index | PMD Index | PTE Index | Page offset |
// +-----------------------------------------------------------------------+
// 63        47          38          29          20          11            0

//                   Virtual address (with Section mapping)
// Note that we don't have PTE Index nor page offset. In this case, the section
// offset is used to address a larger portion of contiguous physical memory (2MB
// = 2^21).
// +-----------------------------------------------------------------------+
// |         | PGD Index | PUD Index | PMD Index |      Section offset     |
// +-----------------------------------------------------------------------+
// 63        47           38          29          20                        0

//                            Descriptor format
// This is what is stored in the translation tables. This is not how a virtual
// address looks like.
// +---------------------------------------------------------------------------+
// | Upper attrs | Address (47:12) | Lower attrs | Block/table bit | Valid bit |
// +---------------------------------------------------------------------------+
// 63               47             11            2                 1           0
//
// Valid Bit: Set to 1 if the descriptor is valid.
// Block/table bit: 1 if it's a

// We don't care about the last 12 bits since pages are 4KB (4096 = 2^12)
// aligned. See PAGE_SHIFT for more information.
#define PAGE_MASK 0xfffffffffffff000

// 12 because the first 12 bits are unused since a page is aligned at the 4096
// byte boundary and 2^12 = 4096 (pages are 4KB). See the Page offset in the
// virtual address diagram above.
#define PAGE_SHIFT 12

// 9 because that's the number of bits needed to address each address (8 Bytes)
// in a translation table. Take a look at any of the indexes in the Virtual
// address and you'll see that each of them takes 9 bits.
#define TABLE_SHIFT 9

// We add these two since the Section mapping doesn't need a PTE (which takes 9
// bits). We use the PTE and the page offset as the offset to a larger section
// of memory (2MB). See the "Virtual address  (with Section mapping)" diagram
// above.
#define SECTION_SHIFT (PAGE_SHIFT + TABLE_SHIFT)

// 4KB
#define PAGE_SIZE (1 << PAGE_SHIFT)

// 2MB
#define SECTION_SIZE (1 << SECTION_SHIFT)

// 4MB the kernel is at address 0, and the stack grows downward so we
// need to make sure that the stack doesn't overwrite the kernel.
#define LOW_MEMORY (2 * SECTION_SIZE)
#define HIGH_MEMORY DEVICE_BASE

// 16KB
#define STACK_SIZE (16384)

// Available memory going from LOW_MEMORY (where the stack starts (growing
// downwards)) and HIGH_MEMORY (where PBASE and all addresses above it are meant
// to be used by devices).
#define PAGING_MEMORY (HIGH_MEMORY - LOW_MEMORY)

// Number of memory pages.
#define PAGING_PAGES (PAGING_MEMORY / PAGE_SIZE)

// This ends up being 512. Each pointer is 8 Bytes, and every table is 1 Page
// (4KB). We can fit 512 pointers per table.
#define PTRS_PER_TABLE (1 << TABLE_SHIFT)

// See the Virtual address diagram. This is the number of bits that need to be
// shifted to the right to have the PGD in the least significant 9 bits. The
// same for all.
#define PGD_SHIFT (PAGE_SHIFT + 3 * TABLE_SHIFT)
#define PUD_SHIFT (PAGE_SHIFT + 2 * TABLE_SHIFT)
#define PMD_SHIFT (PAGE_SHIFT + TABLE_SHIFT)

// We have a PGD, PUD and the Block. Each takes 1 page.
#define PG_DIR_SIZE (3 * PAGE_SIZE)

// 1 for PGD, 1 for PUD, one for the actual Block. Note that the PMD is not used
// in this calculation because we're using Section mapping.
#define PGDIR_SIZE (3 * PAGE_SIZE)

#ifndef __ASSEMBLER__

unsigned long get_free_page();
void free_page(unsigned long p);
void memzero(unsigned long src, unsigned long n);
unsigned long memcpy(unsigned long dst, unsigned long src, unsigned long n);

unsigned long allocate_kernel_page();
unsigned long allocate_user_page(struct task_struct *task, unsigned long va);
int copy_virt_memory(struct task_struct *dst);
int map_page(struct task_struct *task, unsigned long va, unsigned long page);
unsigned long map_table(unsigned long *, unsigned long shift, unsigned long va,
                        int *new_table);
void map_table_entry(unsigned long *table, unsigned long va, unsigned long pa);
int do_mem_abort(unsigned long addr, unsigned long esr);

extern unsigned long pg_dir;

#endif

#endif /*_MM_H */
