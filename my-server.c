#include "server-helper.h"
#include "msg-list.h"
#include "user-list.h"
#include "authentication.h"

#define BACKLOG 10 // how many pending connections queue will hold

/**
 * Program name: my-server.c
 * Description:  This server program listens for client connections, processes incoming messages, 
 *               and maintains a list of messages sent by clients. It includes functionality to 
 *               send acknowledgments and handle client disconnections.
 * Compile:      gcc -o server my-server.c server-helper.c list.c
 * Run:          ./server <hostname> <port>
 */

// Function prototypes
void send_ack(int client_socket);
void send_error(int client_socket, const char *error_message);
void *start_subserver(void *session_data);
void freeMessages(MessageList *msgList);
void freeSession(Session *session);

/**
 * Sends an acknowledgment to the client. The acknowledgment is encapsulated in a s2c_send_ok_ack struct.
 *
 * param client_socket The socket descriptor for the client connection. 
 *                     Used to send acknowledgment data back to the client.
 */
void send_ack(int client_socket) {
    s2c_send_ok_ack server_ack;
    server_ack.type = ACK_TYPE;
    if (send(client_socket, &server_ack, sizeof(s2c_send_ok_ack), 0) == -1) {
        perror("Error sending acknowledgement to client\n");
    }
}

// OMI
void send_error(int client_socket, const char *error_message) {
    user_message server_error;
    server_error.type = ERROR_TYPE;
    server_error.name[0] = '\0'; // No user for error messages
    strncpy(server_error.message, error_message, BUFFER_SIZE - 1);
    server_error.message[BUFFER_SIZE - 1] = '\0';

    if (send(client_socket, &server_error, sizeof(user_message), 0) == -1) {
        perror("Error sending error message to client\n");
    }
}

void freeSession(Session *session) {
    // Free session data and user
    free(session);
}

// Added By: Daniel & Aedan
/**
 * Manages the client connection in a loop, processing incoming messages and appending them 
 * to the provided list. Sends acknowledgments after processing each message and handles client 
 * disconnections.
 * c2s_send_message struct is used to receive messages from the client.
 *
 * param client_socket The socket descriptor for the client connection.
 * param msgList       A pointer to a list where incoming client messages will be stored.
 */
void *start_subserver (void *session_data) {

    Session *session = (Session *) session_data;
    int client_socket = session->socketFd;
    MessageList *messageList = session->messageList;
    UserList *userList = session->userList;

    // Registration handling: Ensure user is registered before processing messages
    int isRegistered = 0;

    while (1) {

        // Initialize client message
        c2s_send_message client_message;

        // Receive message from client
        int bytes_received = recv(client_socket, &client_message, sizeof(client_message), 0);
        if (bytes_received == -1) {
            perror("Error receiving message from client\n");
            break;
        } else if (bytes_received == 0) {
            printf("Client disconnected. Waiting for a new connection...\n");
            if (session->user != NULL) {
                session->user->isOnline = 0; // Set user as offline
            }
            break;
        }
        
        // Process client message
        if (client_message.type == REGISTRATION_TYPE) {

            // Create user and append to userList
            char *email = strtok(client_message.message, " ");
            char *name = strtok(NULL, " ");
            char *raw_password = strtok(NULL, " ");

            // Encode password
            char* password = encode(raw_password);
            // DEBUG
            if (password == NULL) {
               perror("Error encoding password\n");
               break;
            }
            // Cheks if email already exists in the userList
            User *existing_user = userList->first;
            int email_exists = 0;
            while (existing_user != NULL) {
                if (strcmp(existing_user->email, email) == 0) {
                    email_exists = 1;
                    break;
                }
                existing_user = existing_user->next;
            }

            User *user = createUser(email, name, password, client_socket);
            if (email_exists) {
                printf("Email already exists: %s\n", email);
                send_error(client_socket, "Email already exists. Please try again.");
            } else if (user != NULL) {
                appendUser(userList, user);
                printf("Client registered with email: %s, name: %s\n", user->email, user->name);
                session->user = user; // Set user for session
                send_ack(client_socket);
                isRegistered = 1;
            } else {
                printf("Error creating user\n");
                send_error(client_socket, "Error creating user. Please try again.");
            }
            // free(password);
            // free(raw_password);

        } else if (client_message.type == LOGIN_TYPE) {
            // Handle login with mutex protection

            // Create user and append to userList
            char *email = strtok(client_message.message, " ");
            char *password = strtok(NULL, " ");

            // Cheks if email already exists in the userList
            User *existing_user = userList->first;
            int email_exists = 0;
            while (existing_user != NULL) {
                if (strcmp(existing_user->email, email) == 0) {
                    email_exists = 1;
                    break;
                }
                existing_user = existing_user->next;
            }

            if (!email_exists) {
                printf("Email does not exist: %s\n", email);
                send_error(client_socket, "Email does not exist. Please try again.");
            } else {
                // Check password
                if (authenticate(password, existing_user->password)) {
                    // Restore user's socket file descriptor
                    existing_user->socketFd = client_socket;
                    printf("Client logged in with email: %s\n", existing_user->email);
                    existing_user->isOnline = 1; // Set user as online
                    session->user = existing_user; // Set user for session
                    send_ack(client_socket);
                    isRegistered = 1;
                } else {
                    printf("Incorrect password for email: %s\n", existing_user->email);
                    send_error(client_socket, "Incorrect password. Please try again.");
                }
            }
            // Unlock mutex
        } else if (!isRegistered) {
            printf("Client is not registered. Ignoring message.\n");
        }
        else if (client_message.type == MESSAGE_TYPE) {
            printf("Client sent: %s\n", client_message.message);

            // Parse group name and message from the client message (Aedan)
            char group_name[BUFFER_SIZE];
            char msg_content[BUFFER_SIZE];

            sscanf(client_message.message, "%s %[^\n]", group_name, msg_content);

            // Check if user is in the group (Aedan)
            Group *current_group = session->user->groups;
            int in_group = 0;
            while (current_group != NULL) {
                if (strcmp(current_group->name, group_name) == 0) {
                    in_group = 1;
                    break;
                }
                current_group = current_group->next;
            }

            if (!in_group) {
                printf("User %s is not in group %s\n", session->user->name, group_name);
                send_error(client_socket, "You are not in this group.");
                continue;
            }

            // Create message and append to msgList
            char *msgString = strdup(client_message.message);
            Message *msg = createMessage(msgString, session->user);
            // DEBUG
            if (msg == NULL) {
                perror("Error creating message\n");
                break;
            }
            appendMessage(messageList, msg);

            // Send message to all users in the selected group (Aedan)
            User *user_ptr = userList->first;
            while (user_ptr != NULL) {
                if (user_ptr->isOnline) {
                    Group *user_group = user_ptr->groups;
                    while (user_group != NULL) {
                        if (strcmp(user_group->name, group_name) == 0) {
                            user_message msg_to_send;
                            msg_to_send.type = PRINT_MESSAGE_TYPE;
                            strncpy(msg_to_send.name, session->user->name, BUFFER_SIZE - 1);
                            msg_to_send.name[BUFFER_SIZE - 1] = '\0'; // Ensure null-termination
                            strncpy(msg_to_send.message, msg_content, BUFFER_SIZE - 1);
                            msg_to_send.message[BUFFER_SIZE - 1] = '\0'; // Ensure null-termination
                            
                            // Send the message to the user
                            if (send(user_ptr->socketFd, &msg_to_send, sizeof(user_message), 0) == -1) {
                                perror("Error sending message to client\n");
                            }
                            break;
                        }
                        user_group = user_group->next;
                    }
                }
                user_ptr = user_ptr->next;
            }

            send_ack(client_socket);
        } else if (client_message.type == EXIT_TYPE) {
            printf("Client requested to exit. Closing connection...\n");
            if (session->user != NULL) {
                session->user->isOnline = 0; // Set user as offline
            }
            break;
        }
        else if (client_message.type == REQUEST_ALL_MESSAGES_TYPE) {
            printf("Client requested all messages\n");

            // Loop through and send every message in the MessageList to the client
            Message *ptr = messageList->first;
            while (ptr != NULL) {
                // DEBUG
                if (ptr->sender == NULL || ptr->message == NULL) {
                    printf("Error: Null sender or message in message list\n");
                    break;
                }

                user_message msg_to_send;
                msg_to_send.type = PRINT_MESSAGE_TYPE;
                strncpy(msg_to_send.name, ptr->sender->name, BUFFER_SIZE - 1);
                msg_to_send.name[BUFFER_SIZE - 1] = '\0'; // Ensure null-termination
                strncpy(msg_to_send.message, ptr->message, BUFFER_SIZE - 1);
                msg_to_send.message[BUFFER_SIZE - 1] = '\0'; // Ensure null-termination

                // Debug
                printf("sending message from user: %s\n", session->user->name);

                if (send(client_socket, &msg_to_send, sizeof(user_message), 0) == -1) {
                    perror("Error sending message to client\n");
                    break;
                }
                ptr = ptr->next;
            }

            // Send an end-of-messages indicator
            user_message end_msg;
            end_msg.type = PRINT_MESSAGE_TYPE;
            snprintf(end_msg.message, BUFFER_SIZE, "END_OF_MESSAGES");
            end_msg.name[0] = '\0'; // No user for end of messages
            if (send(client_socket, &end_msg, sizeof(user_message), 0) == -1) {
                perror("Error sending end-of-messages indicator to client\n");
            }

            // Send acknowledgment to client
            send_ack(client_socket);
        } else if (client_message.type == JOIN_GROUP_TYPE) {
            printf("Client requested to join a group\n");

            // Parse the group name from the message
            char group_name[BUFFER_SIZE];
            sscanf(client_message.message, "%s", group_name);

            // Update the user's group information
            if (session->user != NULL) {
                // Check to see if the group is already joined by user
                Group *current_group = session->user->groups;
                int already_in_group = 0;
                while (current_group != NULL) {
                    if (strcmp(current_group->name, group_name) == 0) {
                        already_in_group = 1;
                        break;
                    }
                    current_group = current_group->next;
                }

                if(!already_in_group) {
                    // Add user to group
                    Group *new_group = (Group *) malloc(sizeof(Group));
                    if (new_group == NULL) {
                        perror("Error allocating memory for group\n");
                        send_error(client_socket, "Error joining group. Please try again.");
                        break;
                    }
                    new_group->name = strdup(group_name);
                    new_group->next = session->user->groups;
                    session->user->groups = new_group;
                    printf("User %s joined group %s\n", session->user->name, group_name);
                    send_ack(client_socket);
                } else {
                    printf("User %s is already in group %s\n", session->user->name, group_name);
                    send_error(client_socket, "You are already in this group.");
                }
            } else {
                printf("User is not registered. Cannot join group.\n");
                send_error(client_socket, "User is not registered. Cannot join group.");
            }
                
        } else {
            printf("Client sent invalid message type: %d\n", client_message.type);
        }
    }
    close(client_socket);
    printf("Client disconnected. Waiting for a new connection...\n");
    // Daniel: Freeing session causes userList and messageList to be freed prematurely
    // freeSession needs rework 
    // freeSession(session);
    pthread_exit(NULL);
    return 0;
}

/**
 * Main function to start the server and handle client connections.
 *
 * param argc Number of command-line arguments.
 * param argv Array of command-line arguments. The first argument should be the hostname,
 *            and the second argument should be the port number.
 * return 0 on successful execution.
 */
int main(int argc, char *argv[]) {
    int server_socket;  // http server socket
    int client_socket;  // client connection
    UserList userList;
    MessageList messageList;

    initUserList(&userList);
    initMessageList(&messageList);

    server_socket = start_server(argv[1], argv[2], BACKLOG);
    if (server_socket == -1) {
        printf("Error starting server\n");
        exit(1);
    }

    while (1) {
        // Accept connection from client
        if ((client_socket = accept_client(server_socket)) == -1) {
            continue;
        }

        // Added By: Daniel
        Session *session = (Session *) malloc(sizeof(Session));
        session->socketFd = client_socket;
        session->messageList = &messageList;
        session->userList = &userList;
        session->user = NULL; // Initialize user to NULL, later set by registration

        // Added By: Aedan
        // Creating a new thread for each client connection
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, start_subserver, (void *) session) != 0) {
            perror("Error creating thread\n");
            close(client_socket);
            free(session);
            continue;
        }

        // Detaching the thread allows thread resources to be auto released on termination
        pthread_detach(thread_id);
    }

    close(server_socket);
    freeMessageList(&messageList);
    freeUserList(&userList);
    return 0;
}
