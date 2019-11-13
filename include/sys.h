#ifndef _SYS_H
#define _SYS_H

#define __NR_syscalls 3

#ifndef __ASSEMBLER__

// Sys call implementations
void sys_write(char *buf);
int sys_fork();
int sys_exit();

#endif
#endif /*_SYS_H */