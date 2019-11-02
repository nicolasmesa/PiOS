#ifndef _BOOT_H
#define _BOOT_H

extern void delay(unsigned long);
extern void put32(unsigned long, unsigned int);
extern unsigned int get32(unsigned long);
extern void branch_to_address(void*);
extern unsigned int get_cpuid();
extern unsigned int get_el();
extern void set_pgd(unsigned long);

#endif /*_BOOT_H */
