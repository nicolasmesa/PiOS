#include "user.h"
#include "user_sys.h"

void loop(char *str) {
    char buf[3];
    while (1) {
        char c;
        int i = 0;
        while ((c = str[i++]) != '\0') {
            buf[0] = c;
            buf[1] = '\n';
            buf[2] = '\0';
            call_sys_write(buf);
            user_delay(10000000);
        }
    }
}

void user_process() {
    call_sys_write("User process started\n\r");

    int pid = call_sys_fork();
    if (pid < 0) {
        call_sys_write("Error calling sys_fork\r\n");
        call_sys_exit();
        return;
    }

    if (pid == 0) {
        loop("abcde");
    } else {
        loop("12345");
    }

    // We need to call this explicitly here because this process doesn't go
    // though thread_start in sys.S. If we don't call this, we will crash since
    // this will return to address 0x00 since set to 0 (using memzero) in
    // the move_to_user_mode function.
    call_sys_exit();
}