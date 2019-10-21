#include "arm/mmu.h"
#include "arm/sysregs.h"
#include "mm.h"
#include "peripherals/base.h"

.section ".text.boot"

.global _start
_start:
    mrs x0, mpidr_el1
    and x0, x0,#0xFF // We do this and to strip the last byte of the value coming from mpidr_el1
    cbz x0, master
    b  proc_hang

proc_hang:
    b proc_hang


.global master
master:
    // To allow UART boot to keep working, we first check what's the current Exception level. If
    // we're already in #1, it means that there was a kernel here before that loaded us into #1
    // so we skip this part.
    mrs x0, CurrentEL
    lsr x0, x0, #2
    cmp x0, #1
    beq el1_entry

    ldr x0, =SCTLR_VALUE_MMU_DISABLED
    msr sctlr_el1, x0

    ldr x0, =HCR_VALUE
    msr hcr_el2, x0
    ldr x0, =SCR_VALUE
    msr scr_el3, x0

    ldr x0, =SPSR_VALUE
    msr spsr_el3, x0

    adr x0, el1_entry
    msr elr_el3, x0

    eret

el1_entry:
    adr x0, bss_begin
    adr x1, bss_end
    sub x1, x1, x0
    bl  memzero // Branch with a link (when the function call is done, it should come back here)
    bl __create_page_tables
setup_stack:
    mrs x0, mpidr_el1
    and x0, x0, #0xFF
    mov x1, #STACK_SIZE
    mul x0, x0, x1 // x0 now holds the offset to use (0, 16KB, 32KB, 48KB) depending on the cpuid
    mov x1, #LOW_MEMORY
    sub x0, x1, x0 // x1 holds LOW_MEMORY. Here, we subtract from LOW_MEMORY (we're setting up our stacks downward).
    mov x1, #VA_START
    add x0, x0, x1

    // sp = VA_START + LOW_MEMORY (in CPU0). Note that this is a virtual address and, as a result, we can't make any
    // function calls or push anything onto the stack until we have enabled MMU.
    mov sp, x0 

    // Setup the page tables and configure MMU
    adrp x0, pg_dir
    msr ttbr1_el1, x0 // set pg_dir that we built before

    ldr x0, =(TCR_VALUE)
    msr tcr_el1, x0

    // Set the mair (more information in mm.h)
    ldr x0, =(MAIR_VALUE)
    msr mair_el1, x0

    // Here, we read the address of kernel_main, we enable the MMU and branch to the kernel_main address. Note that the
    // address at this time is physical (since we haven't enabled MMU yet), but, since we lied in the linker.ld file and
    // said that the kernel would be loaded at address 0xffff..., the kernel is accessible (since MMU is enabled).
    // This can be seen a bit easier running objdump on the kernel and looking at this instruction.
    ldr x2, =kernel_main
    mov x0, #SCTLR_MMU_ENABLED
    msr sctlr_el1, x0
    br x2


// Creates the PGD and PUD entries. Note that each call to the create_table_entry macro has the side-effect
// of updating the tbl argument.
.macro create_pgd_entry, tbl, virt, tmp1, tmp2
    create_table_entry \tbl, \virt, PGD_SHIFT, \tmp1, \tmp2
    create_table_entry \tbl, \virt, PUD_SHIFT, \tmp1, \tmp2
.endm

// This macro extracts the index of the virtual address into the table (tbl) by using the shift argument. Then,
// it computes and stores the address of the next table inside the index and marks the lower bits of the address
// to indicate that it is pointing to a table (not to a block). The reason we can use these lower bits is that
// they are always 0 (because they're page-aligned). So we can use them to store additional information and we
// ignore them when we need to derrefenrence the pointer. Finally, it increments the size of the table to 
// point to the next table so that it can be used by the next call. Sample pseudo-code:
//   index = compute_index(virt, shift)
//   tbl[index] = ((tbl + PAGE_SIZE) | #MM_TYPE_PAGE_TABLE)
//   tbl += #PAGE_SIZE
.macro create_table_entry, tbl, virt, shift, tmp1, tmp2
    // We extract the index of the virtual address
    lsr \tmp1, \virt, #\shift
    and \tmp1, \tmp1, #PTRS_PER_TABLE - 1 // table index
    add \tmp2, \tbl, #PAGE_SIZE
    orr \tmp2, \tmp2, #MM_TYPE_PAGE_TABLE // Set the lower bits to indicate that this points to a another table.
    str \tmp2, [\tbl, \tmp1, lsl #3]
    add \tbl, \tbl, #PAGE_SIZE // next level table page
.endm

// phys is the physical address to map, but it gets stored as a descriptor (see mm.h for the format) with
// flags, for example.
.macro create_block_map, tbl, phys, start, end, flags, tmp1
    // Here, we're computing the start and end indexes. We first shift start #SECTION_SHIFT (21) bits to the right.
    // Note that we are using Section Mapping instead of a normal 4KB (page) mapping. Section mapping maps
    // 2MB of contiguous memory which is 2^21. See mm.h for a more thorough explanation.
    lsr \start, \start, #SECTION_SHIFT
    and \start, \start, #PTRS_PER_TABLE - 1
    lsr \end, \end, #SECTION_SHIFT
    and \end, \end, #PTRS_PER_TABLE - 1

    // We clear the whole section area of the Physical Address and then set the flags. Note that the physical
    // address ends up unshifted again (the orr reverts the shift). This allows this macro to work with any
    // address, not just those that are aligned.
    lsr \phys, \phys, #SECTION_SHIFT
    mov \tmp1, #\flags // these are set to the lower attributes of the descriptor (see mm.h diagram)
    orr \phys, \tmp1, \phys, lsl #SECTION_SHIFT

// we shift 3 bits to the left because we want to set it every 8 bytes (2^3 = 1<<3 = 8).
// This loop is pretty much doing
// do {
//     index = start << 3; // start * 8
//     tbl[index] = phys;
//     phys += SECTION_SIZE;
// } while (start <= end);
9999: str \phys, [\tbl, \start, lsl #3] 
    add \start, \start, #1
    add \phys, \phys, #SECTION_SIZE
    cmp \start, \end
    b.ls 9999b
.endm

__create_page_tables:
    // Save the return address. As long as we don't use x29 for anything we should be fine
    mov x29, x30 
    // pg_dir (comes from linker.ld) is a relative pointer (relative to PC). adrp converts this to an asbolute address
    // that is page aligned while also removing the top  12 bits.
    // From https://www.element14.com/community/servlet/JiveServlet/previewBody/41836-102-1-229511/ARM.Reference_Manual.pdf (page 35):
    //   Address of Page: sign extends a 21-bit offset, shifts it left by 12 and adds it to the value of the PC with its
    //   bottom 12 bits cleared, writing the result to register Xd
    adrp x0, pg_dir
    mov x1, #PG_DIR_SIZE
    bl memzero

    // Note that x0 is modified by the create_pgd_entry macro and makes it point to the next
    // free page that we can conveniently use for the next table
    adrp x0, pg_dir
    mov x1, #VA_START
    create_pgd_entry x0, x1, x2, x3

    // Create the block map that holds the physical addresses.
    // We call create_block_map twice. The first time is to map all of memory up to DEVICE_BASE with the appropriate flags. Then,
    // we call it again for device memory with the flags that indicate that the memory is for devices. Note that the
    // create_block_map includes the end as well so we're not leaving any unmapped memory. Also note that this is all kernel
    // memory since VA_START has the most significant 16 bits set.
    //                 +-----------------------------------+
    // VA_START        | 0xffff000000000000                |
    //                 |                                   |
    //                 | All of this is mapped virtual     |
    //                 | memory for the kernel with        |
    //                 | MMU_FLAGS                         |
    //                 +-----------------------------------+
    // DEVICE_BASE     | 0xffff00003F000000                |
    //                 |                                   |
    //                 | All of this is mapped virtual     |
    //                 | memory for the kernel for devices |
    //                 | with MMU_DEVICE_FLAGS             |
    //                 +-----------------------------------+

    // Map memory
    mov x1, xzr
    mov x2, #VA_START
    ldr x3, =(VA_START + DEVICE_BASE - SECTION_SIZE)
    create_block_map x0, x1, x2, x3, MMU_FLAGS, x4

    // Map device memory
    mov x1, #DEVICE_BASE
    ldr x2, =(VA_START + DEVICE_BASE)
    ldr x3, =(VA_START + PHYS_MEMORY_SIZE - SECTION_SIZE)
    create_block_map x0, x1, x2, x3, MMU_DEVICE_FLAGS, x4

    mov x30, x29 // restore the return address (stored in the first instruction of this function)
    ret