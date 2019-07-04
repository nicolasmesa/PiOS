#ifndef	_MM_H
#define	_MM_H

#define PAGE_SHIFT              12
#define TABLE_SHIFT             9
#define SECTION_SHIFT           (PAGE_SHIFT + TABLE_SHIFT)

#define PAGE_SIZE               (1 << PAGE_SHIFT)	
#define SECTION_SIZE            (1 << SECTION_SHIFT)	

// 4MB the kernel is at address 0, and the stack grows downward so we
// need to make sure that the stack doesn't overwrite the kernel.
#define LOW_MEMORY              (2 * SECTION_SIZE)

// 16KB
#define STACK_SIZE              (16384)

#ifndef __ASSEMBLER__

void memzero(unsigned long src, unsigned long n);

#endif

#endif  /*_MM_H */
