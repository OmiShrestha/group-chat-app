#include "client-helper.h"
#include "protocol.h"
#include <pthread.h>
#include "msg-list.h"
#include "user-list.h"
#include "auth-client.h"

/**
 * Program name: my-client.c
 * Description:  This client program connects to a server to send messages, receive acknowledgments, 
 *               and exit the connection.
 * Compile:      gcc -o client my-client.c client-helper.c
 * Run:          ./client <hostname> <port>
 */

// Function prototypes
void send_registration(int server_socket, char *email, char *name, char *password); // Omi
void receive_ack(int server_socket);
void send_exit_message(int server_socket);
void request_all_messages(int server_socket); // Omi
void send_login(int server_socket, char *email, char *password); // Omi
void send_messege(int server_socket, char *message, char *group_name);

void send_login(int server_socket, char *email, char *password) {
    c2s_send_message login_msg;
    login_msg.type = LOGIN_TYPE;
    snprintf(login_msg.message, BUFFER_SIZE, "%s %s", email, password);
    login_msg.length = strlen(login_msg.message) + 1;

    if (send(server_socket, &login_msg, sizeof(c2s_send_message), 0) == -1) {
        perror("Error sending login to server\n");
    }
}

// Added By: Omi
/**
 * Sends a registration message to the server with email and name
 *
 * param server_socket The socket descriptor for the server connection.
 * param email The user's email address.
 * param name The user's name.
 */
void send_registration(int server_socket, char *email, char *name, char *password) {
    c2s_send_message regis_msg;
    regis_msg.type = REGISTRATION_TYPE;
    // Prepare for use with strtok() in server
    snprintf(regis_msg.message, BUFFER_SIZE, "%s %s %s", email, name, password);
    regis_msg.length = strlen(regis_msg.message) + 1;
    
    if (send(server_socket, &regis_msg, sizeof(c2s_send_message), 0) == -1) {
        perror("Error sending registration to server\n");
    }

}

/**
 * Sends an exit signal to the server to terminate the client session.
 * The exit signal is encapsulated in a c2s_send_exit struct.
 *
 * param server_socket The socket descriptor for the server connection.
 */
void send_exit_message(int server_socket) {
    c2s_send_exit exit_message;
    exit_message.type = EXIT_TYPE;
    if (send(server_socket, &exit_message, sizeof(c2s_send_exit), 0) == -1) {
        perror("Error sending exit message to server\n");
    }
}

// Added By: Omi
/**
 * Requests all messages from the server
 *
 * param server_socket The socket descriptor for the server connection.
 */
void request_all_messages(int server_socket) {
    c2s_send_message req_msg;
    req_msg.type = REQUEST_ALL_MESSAGES_TYPE;
    snprintf(req_msg.message, BUFFER_SIZE, "REQUEST_ALL_MESSAGES");
    req_msg.length = strlen(req_msg.message) + 1;
    if (send(server_socket, &req_msg, sizeof(c2s_send_message), 0) == -1) {
        perror("Error requesting messages from server\n");
        return;
    }
}

/**
 * Sends a message to the server. The message is encapsulated in a c2s_send_message struct.
 *
 * param server_socket The socket descriptor for the server connection. 
 *                     Used to transmit the message to the server.
 * param message       A character pointer to the message that will be sent to the server.
 */
void send_messege(int server_socket, char *message, char *group_name) {
    c2s_send_message client_message;
    client_message.type = MESSAGE_TYPE;
    snprintf(client_message.message, BUFFER_SIZE, "%s %s", group_name, message);
    client_message.length = strlen(client_message.message) + 1;

    if (send(server_socket, &client_message, sizeof(c2s_send_message), 0) == -1) {
        perror("Error sending message to server\n");
    }
}

/**
 * Receives an acknowledgment from the server and prints a confirmation message 
 * if the acknowledgment is valid. Otherwise, an error message is printed.
 * The acknowledgment is encapsulated in a s2c_send_ok_ack struct.
 *
 * param server_socket The socket descriptor for the server connection.
 */
void receive_ack(int server_socket) {
    s2c_send_ok_ack server_ack;
    if (recv(server_socket, &server_ack, sizeof(s2c_send_ok_ack), 0) == -1) {
        perror("Error receiving ack from server\n");
    } else if (server_ack.type == ACK_TYPE) {
        printf("Acknowledgement received from server\n");
    } else {
        perror("Invalid acknowledgement received from server\n");
    }
}

int receive_login_response(int server_socket) {
    user_message server_response;
    if (recv(server_socket, &server_response, sizeof(user_message), 0) == -1) {
        perror("Error receiving response from server\n");
        return 0;
    } else if (server_response.type == ACK_TYPE) {
        printf("Login successful\n");
        return 1;
    } else if (server_response.type == ERROR_TYPE) {
        printf("Error from server: %s\n", server_response.message);
        return 0;
    } else {
        printf("Invalid response received from server\n");
        return 0;
    }
}

int receive_registration_response(int server_socket) {
    user_message server_response;
    if (recv(server_socket, &server_response, sizeof(user_message), 0) == -1) {
        perror("Error receiving response from server\n");
        return 0;
    } else if (server_response.type == ACK_TYPE) {
        printf("Registration successful\n");
        return 1;
    } else if (server_response.type == ERROR_TYPE) {
        printf("Error from server: %s\n", server_response.message);
        return 0;
    } else {
        printf("Invalid response received from server\n");
        return 0;
    }
}

void *handle_receive(void *arg) {
    int server_socket = *((int *) arg);
    while (1) {
        // Receive messages from the server
        user_message server_message;
        int bytes_received = recv(server_socket, &server_message, sizeof(user_message), 0);
        if (bytes_received == -1) {
            perror("Error receiving message from server\n");
            break;
        } else if (bytes_received == 0) {
            printf("Server disconnected. Exiting...\n");
            break;
        }
        switch (server_message.type)
        {
        case MESSAGE_TYPE:
            printf("Message from server: %s\n", server_message.message);
            break;
        case PRINT_MESSAGE_TYPE:
            if (strcmp(server_message.message, "END_OF_MESSAGES") == 0) {
                printf("End of messages\n");
            } else {
                printf("Message from user (%s): %s\n", server_message.name, server_message.message);
            }
            break;
        case ERROR_TYPE:
            printf("Error from server: %s\n", server_message.message);
            break;
        case ACK_TYPE:
            printf("Acknowledgment from server received\n");
            break;
        default:
            break;
        }
        printf("Receive loop continues...\n");
    }
}

// Added By: Aedan
void join_group(int server_socket, char *group_name) {
    c2s_send_message join_msg;
    join_msg.type = JOIN_GROUP_TYPE;
    snprintf(join_msg.message, BUFFER_SIZE, "%s", group_name);
    join_msg.length = strlen(join_msg.message) + 1;

    if (send(server_socket, &join_msg, sizeof(c2s_send_message), 0) == -1) {
        perror("Error sending join group message to server\n");
    }
}

void *handle_send(void *arg) {
    int server_socket = *((int *) arg);
    int choice;

    while(1) {
        // Menu options
        printf("\nMenu:\n");
        printf("1. Send a message\n");
        printf("2. Request all messages\n");
        printf("3. Join a group\n");
        printf("4. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        getchar();

        switch (choice) {
            case 1: {
                // Send a message
                char message[BUFFER_SIZE];
                char group_name[BUFFER_SIZE];
                printf("Enter the group name: ");
                fgets(group_name, BUFFER_SIZE, stdin);
                group_name[strcspn(group_name, "\n")] = '\0'; // Remove newline character

                printf("Enter your message: ");
                fgets(message, BUFFER_SIZE, stdin);
                message[strcspn(message, "\n")] = '\0'; // Remove newline character

                send_messege(server_socket, message, group_name);
                break;
            }
            case 2:
                // Request all messages
                request_all_messages(server_socket);
                break;
            case 3:
                // Join a group
                printf("Select group to join\n");
                printf("1. CMPS340\n");
                printf("2. CMPS352\n");
                int group_choice;
                scanf("%d", &group_choice);
                getchar();
                if (group_choice == 1) {
                    join_group(server_socket, "CMPS340");
                } else if (group_choice == 2) {
                    join_group(server_socket, "CMPS352");
                } else {
                    printf("Invalid group choice. Try again.\n");
                }
                break;
            case 4:
                // Exit the client
                send_exit_message(server_socket);
                printf("Exiting...\n");
                close(server_socket);
                return 0;
            default:
                printf("Invalid choice. Try again.\n");
        }
    }
}

// Added By: Omi
/**
 * Main function to start the client and send messages to the server.
 *
 * param argc Number of command-line arguments.
 * param argv Array of command-line arguments. The first argument should be the hostname,
 *            and the second argument should be the port number.
 * return 0 on successful execution.
 */
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <hostname> <port>\n", argv[0]);
        exit(1);
    }

    // Initialize the server socket
    int server_socket = get_server_connection(argv[1], argv[2]);
    if (server_socket == -1) {
        printf("Error connecting to server\n");
        exit(1);
    }

    // Registration by email and name
    char email[BUFFER_SIZE];
    char name[BUFFER_SIZE];
    char password[BUFFER_SIZE];
    int regis_success = 0; // Initialize regis_success


    while (!regis_success) {
        printf("1. Login\n");
        printf("2. Register\n");
        printf("3. Exit\n");
        printf("Enter your choice: ");
        int choice;
        scanf("%d", &choice);
        getchar();

        switch (choice)
        {
        case 1:
            // Ask for login details
            printf("Enter your email: ");
            fgets(email, BUFFER_SIZE, stdin);
            email[strcspn(email, "\n")] = '\0'; // Remove newline character

            printf("Enter your password: ");
            getpasswd(password, BUFFER_SIZE);

            // Send login data to the server
            send_login(server_socket, email, password);

            // Wait for acknowledgment or error from server
            regis_success = receive_login_response(server_socket);
            break;
        case 2:
            // Ask for registration details
            printf("Enter your email: ");
            fgets(email, BUFFER_SIZE, stdin);
            email[strcspn(email, "\n")] = '\0'; // Remove newline character

            printf("Enter your name: ");
            fgets(name, BUFFER_SIZE, stdin);
            name[strcspn(name, "\n")] = '\0'; // Remove newline character

            printf("Enter your password: ");
            getpasswd(password, BUFFER_SIZE);

            // Send registration data to the server
            send_registration(server_socket, email, name, password);

            // Wait for acknowledgment or error from server
            regis_success = receive_registration_response(server_socket);
            break;
        case 3:
            // Exit the client
            send_exit_message(server_socket);
            printf("Exiting...\n");
            close(server_socket);
            return 0;
        default:
            break;
        }
    }

    pthread_t send_thread;
    pthread_t receive_thread;

    if (pthread_create(&send_thread, NULL, handle_send, &server_socket) != 0) {
        perror("Error creating send thread\n");
        exit(1);
    }

    if (pthread_create(&receive_thread, NULL, handle_receive, &server_socket) != 0) {
        perror("Error creating receive thread\n");
        exit(1);
    }

    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);
    return 0;
}

