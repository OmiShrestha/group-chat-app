#define AUTH_CLIENT_H
#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include "authentication.h"


// Taken from Dr. Bi's authentication.c for use in client
// get password from keyboard with no echo
// password: buffer for the password
// len: size of the buffer - max size of password
int getpasswd(char *password, int len);