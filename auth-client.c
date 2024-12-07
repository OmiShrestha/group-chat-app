#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>

// Taken from Dr. Bi's authentication.c for use in client
// get password from keyboard with no echo
// password: buffer for the password
// len: size of the buffer - max size of password
int getpasswd(char *pswd, int len) {
    struct termios current_settings;
    struct termios no_echo_settings;
    memset(pswd, 0, len);

    // get the current terminal settings
    if (tcgetattr(0, &current_settings)) {
        printf("Error: accessing keyboard\n");
        return -1;
    }

    // save the current terminal settings
    memcpy (&no_echo_settings, &current_settings, sizeof(struct termios));

    // disable echoing
    no_echo_settings.c_lflag &= ~(ICANON | ECHO);
    if (tcsetattr (0, TCSANOW, &no_echo_settings)) {
        printf("Error: accessing keyboard\n");
        return -1;
    }

    // read password with no echo
    int index = 0;
    char c;
    while (((c = fgetc(stdin))  != '\n') && (index < len - 1)) {
        pswd[index++] = c;
    }
    pswd[index] = 0; /* null-terminate   */

    // restore terminal settings
    if (tcsetattr (0, TCSANOW, &current_settings)) {
        printf("Error: accessing keyboard\n");
        return -1;
    }

    return index;
}