#ifndef _MMU_H
#define _MMU_H

// These set 2 bits. Bit 0 is always 1 (which means the descriptor is valid).
// Bit 1 is set to 1 when the descriptor points to a page table and 0 when it
// points to a block (physical page).
#define MM_TYPE_PAGE_TABLE 0x3
#define MM_TYPE_PAGE 0x3
#define MM_TYPE_BLOCK 0x1

// Accessing an address that doesn't have this set will cause a synchronous
// exception.
#define MM_ACCESS (0x1 << 10)
#define MM_ACCESS_PERMISSION (0x01 << 6)  // TODO: Not sure what this means

/*
 * Memory region attributes (more info on page 2609 of the AArch64 ref manual).
 *
 * Here, we're writing to two sections of the mair. In the first one (section
 * 0), we're setting everything to 0.
 *
 * In the second one (section 1), we'resetting it to 01000100.
 *  - The first 0100 piece: Normal memory, Outer Non-cacheable
 *  - The second 0100 piece: Normal memory, Inner Non-cacheable
 * (not sure what Inner/Outer means).
 *
 *   n = AttrIndx[2:0]
 *                      n    MAIR
 *   DEVICE_nGnRnE    000    00000000
 *   NORMAL_NC        001    01000100
 */
#define MT_DEVICE_nGnRnE 0x0  // Device memory (0th index of the mair)
#define MT_NORMAL_NC 0x1  // Normal non-cacheable memory (index 1 of the mair)
#define MT_DEVICE_nGnRnE_FLAGS 0x00  // Device memory value

// Normal memory, Outer Non-cacheable, Normal memory, Inner
// Non-cacheable (from page 2609 - 2610)
#define MT_NORMAL_NC_FLAGS 0x44

// This is the value that we will set the mair to.
#define MAIR_VALUE                                       \
    (MT_DEVICE_nGnRnE_FLAGS << (8 * MT_DEVICE_nGnRnE)) | \
        (MT_NORMAL_NC_FLAGS << (8 * MT_NORMAL_NC))

#define MMU_FLAGS (MM_TYPE_BLOCK | (MT_NORMAL_NC << 2) | MM_ACCESS)
#define MMU_DEVICE_FLAGS (MM_TYPE_BLOCK | (MT_DEVICE_nGnRnE << 2) | MM_ACCESS)
#define MMU_PTE_FLAGS \
    (MM_TYPE_BLOCK | (MT_NORMAL_NC << 2) | MM_ACCESS | MM_ACCESS_PERMISSION)

// Used for the Translation Control Register
#define TCR_T0SZ (64 - 48)
#define TCR_T1SZ ((64 - 48) << 16)
#define TCR_TG0_4K (0 << 14)
#define TCR_TG1_4K (2 << 30)

// Configures kernel and user page tables to use 4KB pages. Not sure how.
#define TCR_VALUE (TCR_T0SZ | TCR_T1SZ | TCR_TG0_4K | TCR_TG1_4K)

#endif