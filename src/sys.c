#include "fork.h"
#include "mm.h"
#include "printf.h"
#include "sched.h"
#include "utils.h"

// Dangerous! no bounds checking
void sys_write(char *buff) { printf(buff); }

void *malloc() { return (void *)get_free_page(); }

int sys_clone(unsigned long stack) {
    return copy_process(0, 0, 0, stack);
    return 0;
}

void sys_exit() { exit_process(); }

void *const sys_call_table[] = {sys_write, malloc, sys_clone, sys_exit};