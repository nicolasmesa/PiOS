#include "sys.h"

.global call_sys_write
call_sys_write:
    mov w8, #SYS_WRITE_NUMBER
    svc #0
    ret

.global call_sys_malloc
call_sys_malloc:
    mov w8, #SYS_MALLOC_NUMBER
    svc #0
    ret

.global call_sys_exit
call_sys_exit:
    mov w8, #SYS_EXIT_NUMBER
    svc #0
    ret

.global call_sys_clone
call_sys_clone:
    mov x10, x0 // fn
    mov x11, x1 // arg
    mov x12, x2 // stack

    // effectively doing sys_clone(stack)
    mov x0, x2
    mov w8, #SYS_CLONE_NUMBER
    svc #0

    // compare the return value (0 means I'm the child, so go to thread_start).
    // != 0 means I'm the parent and the call either failed (if < 0) or I got the
    // child pid (if > 0).
    cmp x0, #0
    beq thread_start
    ret

thread_start:
    mov x29, 0

    // x0 now holds the argument. call the function (stored in x10).
    mov x0, x11
    blr x10

    mov w8, #SYS_EXIT_NUMBER
    svc #0



