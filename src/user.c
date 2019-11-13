#include "user.h"
#include "user_sys.h"

void loop(char *str) {
    char buf[2];
    while (1) {
        char c;
        int i = 0;
        while ((c = str[i++]) != '\0') {
            buf[0] = c;
            buf[1] = '\0';
            call_sys_write(buf);
            user_delay(100000);
        }
    }
}

int fork_or_exit() {
    int pid = call_sys_fork();
    if (pid < 0) {
        call_sys_write("Error calling sys_fork\r\n");
        call_sys_exit();
        return -1;
    }
    return pid;
}

void fork_and_run_loop(char *str) {
    int pid = fork_or_exit();
    if (pid == 0) {
        loop(str);
    }
}

void user_process() {
    call_sys_write("User process started\n\r");

    fork_and_run_loop("abcde");
    fork_and_run_loop("12345");
    fork_and_run_loop("!@#$^&");
    fork_and_run_loop("wxyz");

    // Let the parent live and let the newly created process fall through to the
    // call_sys_exit
    int pid = fork_or_exit();
    if (pid > 0) {
        loop("mnopqr");
    }

    call_sys_write("\n\r\n\rExiting!\n\r\n\r");
    // We need to call this explicitly here because this process doesn't go
    // though thread_start in sys.S. If we don't call this, we will crash since
    // this will return to address 0x00 since set to 0 (using memzero) in
    // the move_to_user_mode function.
    call_sys_exit();
}
