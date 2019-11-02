#include "fork.h"
#include "mm.h"
#include "printf.h"
#include "sched.h"
#include "utils.h"

// Dangerous! no bounds checking
void sys_write(char *buff) { printf(buff); }

int sys_fork() { return copy_process(0, 0, 0); }

void sys_exit() { exit_process(); }

void *const sys_call_table[] = {sys_write, sys_fork, sys_exit};