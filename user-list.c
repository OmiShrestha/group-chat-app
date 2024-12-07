#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "protocol.h"

// Added By: Daniel

void initUserList(UserList *userList) {
    userList->first = NULL;
    userList->last = NULL;
    userList->count = 0;
}

void appendUser(UserList *userList, User *user) {
    if (userList->first == NULL) {
        userList->first = user;
        userList->last = user;
    } else {
        userList->last->next = user;
        userList->last = user;
    }
    user->next = NULL;
    userList->count++;
}

User *createUser(char *email, char *name, char *password, int socketFd) {
    User *user = (User *) malloc(sizeof(User));
    // DEBUG
    if (user == NULL) {
        perror("Error allocating memory for user");
        return NULL;
    }
    user->email = strdup(email);
    user->name = strdup(name);
    user->password = strdup(password);
    if (user->email == NULL || user->name == NULL || user->password == NULL) {
        if (user->email == NULL) {
            perror("Error allocating memory for email");
        } else if (user->name == NULL) {
            perror("Error allocating memory for name");
        } else {
            perror("Error allocating memory for password");
        }
    }
    user->socketFd = socketFd;
    user->isOnline = 1; // User will be online after creation
    user->next = NULL;

    // Auto add user to "CMPS" group (Aedan)
    Group *group = (Group *) malloc(sizeof(Group));
    if (group == NULL) {
        perror("Error allocating memory for group\n");
        free(user);
        return NULL;
    }
    group->name = strdup("CMPS");
    group->next = NULL;
    user->groups = group;

    return user;
}

void freeUserList(UserList *userList) {
    User *ptr = userList->first;
    for (int i = 0; i < userList->count; i++) {
        User *temp = ptr;
        ptr = ptr->next;
        free(temp->email);
        free(temp->name);
        free(temp->password);
        free(temp);
    }
    userList->first = NULL;
    userList->last = NULL;
    userList->count = 0;
}
