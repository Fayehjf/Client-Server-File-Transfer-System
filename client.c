#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAIN_SERVER_TCP_PORT 25981
#define MAX 1024

// Function to encrypt password
void encrypt_password(char *password) {
    for (int i = 0; password[i] != '\0'; i++) {
        if (password[i] >= 'A' && password[i] <= 'Z') {
            password[i] = 'A' + (password[i] - 'A' + 3) % 26;
        } else if (password[i] >= 'a' && password[i] <= 'z') {
            password[i] = 'a' + (password[i] - 'a' + 3) % 26;
        } else if (password[i] >= '0' && password[i] <= '9') {
            password[i] = '0' + (password[i] - '0' + 3) % 10;
        }
    }
}

int main(int argc, char *argv[]) {
    int tcp_sock;
    struct sockaddr_in main_serv_addr;
    char buffer[MAX];
    char username[50], password[50];
    int is_guest = 0;

    // Booting up message
    printf("The client is up and running.\n");

    // Check command line arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <username> <password>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Get username and password from command line arguments
    strcpy(username, argv[1]);
    strcpy(password, argv[2]);

    // Encrypt the password if it's not the guest user
    if (strcmp(username, "guest") == 0 && strcmp(password, "guest") == 0) {
        is_guest = 1;
    } else {
        encrypt_password(password);
    }

    // Create TCP socket
    tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock < 0) {
        perror("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Define Main Server address
    memset(&main_serv_addr, 0, sizeof(main_serv_addr));
    main_serv_addr.sin_family = AF_INET;
    main_serv_addr.sin_port = htons(MAIN_SERVER_TCP_PORT);
    main_serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to Main Server
    if (connect(tcp_sock, (struct sockaddr *)&main_serv_addr, sizeof(main_serv_addr)) < 0) {
        perror("Connection to the main server failed");
        close(tcp_sock);
        exit(EXIT_FAILURE);
    }

    // Send encrypted credentials to the server for authentication
    snprintf(buffer, MAX, "%s %s", username, password);
    if (write(tcp_sock, buffer, strlen(buffer)) < 0) {
        perror("Failed to send credentials to server");
        close(tcp_sock);
        exit(EXIT_FAILURE);
    }

    // Receive authentication response from the server
    memset(buffer, 0, MAX);
    if (read(tcp_sock, buffer, MAX) < 0) {
        perror("Failed to receive authentication response from server");
        close(tcp_sock);
        exit(EXIT_FAILURE);
    }

    // Check authentication response
    if (strstr(buffer, "authenticated")) {
        printf("You have been granted %s access.\n", is_guest ? "guest" : "member");
        if (is_guest) {
            printf("Please enter the command: <lookup <username>>\n");
        } else {
            printf("Please enter the command: <lookup <username>>, <push <filename>>, <remove <filename>>, <deploy>, <log>.\n");
        }

        // Loop to allow the user to enter commands
        while (1) {
            char command[MAX];
            char target_username[50] = "";
            printf("Enter command: ");
            scanf(" %[^\n]s", command);

            // Guest can only use the lookup command
            if (is_guest) {
                if (strncmp(command, "lookup", 6) == 0) {
                    if (sscanf(command, "lookup %s", target_username) != 1) {
                        printf("username is not specified.\n Please enter the command: <lookup <username>>, <push <filename>>, <remove <filename>>, <deploy>, <log>.\n---Start a new request---\n");
                        continue;
                    }

                    // Send the lookup request to the Main Server
                    snprintf(buffer, MAX, "lookup %s", target_username);
                    printf("Guest sent a lookup request to the main server.\n");

                    if (write(tcp_sock, buffer, strlen(buffer)) < 0) {
                        perror("Failed to send lookup request to server");
                        close(tcp_sock);
                        exit(EXIT_FAILURE);
                    }

                    // Receive response from the Main Server
                    memset(buffer, 0, MAX);
                    if (read(tcp_sock, buffer, MAX) < 0) {
                        perror("Failed to receive lookup response from server");
                        close(tcp_sock);
                        exit(EXIT_FAILURE);
                    }

                    // Display the response from the server
                    printf("The client received the response from the main server using TCP over port %d.\n%s---Start a new request---\n", MAIN_SERVER_TCP_PORT, buffer);
                } else {
                    printf("Guests can only use the lookup command.\n");
                }
            } else {  // Member has more commands
                if (strncmp(command, "lookup", 6) == 0) {
                    if (sscanf(command, "lookup %s", target_username) != 1) {
                        printf("Username is not specified. Will lookup %s.\n", username);
                        strcpy(target_username, username);
                    }

                    // Send the lookup request to the Main Server
                    snprintf(buffer, MAX, "lookup %s", target_username);
                    printf("%s sent a lookup request to the main server.\n", username);

                    if (write(tcp_sock, buffer, strlen(buffer)) < 0) {
                        perror("Failed to send lookup request to server");
                        close(tcp_sock);
                        exit(EXIT_FAILURE);
                    }

                    // Receive response from the Main Server
                    memset(buffer, 0, MAX);
                    if (read(tcp_sock, buffer, MAX) < 0) {
                        perror("Failed to receive lookup response from server");
                        close(tcp_sock);
                        exit(EXIT_FAILURE);
                    }

                    // Display the response from the server
                    printf("The client received the response from the main server using TCP over port %d.\n%s---Start a new request---\n", MAIN_SERVER_TCP_PORT, buffer);
                } else if (strncmp(command, "push", 4) == 0) {
                    char filename[50];
                    if (sscanf(command, "push %s", filename) != 1) {
                        printf("filename is not specified.\n Please enter the command: <lookup <username>>, <push <filename>>, <remove <filename>>, <deploy>, <log>.\n---Start a new request---\n");
                        continue;
                    }

                    // Send the push request to the Main Server
                    snprintf(buffer, MAX, "push %s %s", username, filename);
                    printf("%s sent a push request to the main server.\n", username);

                    if (write(tcp_sock, buffer, strlen(buffer)) < 0) {
                        perror("Failed to send push request to server");
                        close(tcp_sock);
                        exit(EXIT_FAILURE);
                    }

                    // Receive response from the Main Server
                    memset(buffer, 0, MAX);
                    if (read(tcp_sock, buffer, MAX) < 0) {
                        perror("Failed to receive push response from server");
                        close(tcp_sock);
                        exit(EXIT_FAILURE);
                    }

                    // Handle Server R's response for overwriting confirmation
                    if (strstr(buffer, "overwrite confirmation")) {
                        printf("%s exists in %s's repository, do you want to overwrite (Y/N)? ", filename, username);
                        char response;

                        // Enforce Y or N input from user
                        while (1) {
                            scanf(" %c", &response);
                            if (response == 'Y' || response == 'N') {
                                break;
                            } else {
                                printf("Invalid input. Please enter Y or N: ");
                            }
                        }
                        snprintf(buffer, MAX, "%c", response);
                   
                        // Send the overwrite confirmation to Main Server
                        if (write(tcp_sock, buffer, strlen(buffer)) < 0) {
                            perror("Failed to send overwrite confirmation to server.");
                            close(tcp_sock);
                            exit(EXIT_FAILURE);
                        }

                        // Receive final response from the Main Server
                        memset(buffer, 0, MAX);
                        if (read(tcp_sock, buffer, MAX) < 0) {
                            perror("Failed to receive final response from server");
                            close(tcp_sock);
                            exit(EXIT_FAILURE);
                        }

                        // Display the final response
                        printf("The client received the response from the main server using TCP over port %d.\n%s\n", MAIN_SERVER_TCP_PORT, buffer);
                    } else if (strstr(buffer, "pushed successfully")) {
                        // Display the success message only once for new pushes
                        //printf("The client received the response from the main server using TCP over port %d.\n%s\n", MAIN_SERVER_TCP_PORT, buffer);
                        printf("%s pushed successfully.\n", filename); // Added newline character here
                    }
                    printf("---Start a new request---\n");
                } else if (strncmp(command, "remove", 6) == 0) {
                    char filename[50];
                    if (sscanf(command, "remove %s", filename) != 1) {
                        printf("filename is not specified.\n Please enter the command: <lookup <username>>, <push <filename>>, <remove <filename>>, <deploy>, <log>.\n---Start a new request---\n");
                        continue;
                    }

                    // Send the remove request to the Main Server
                    snprintf(buffer, MAX, "remove %s %s", username, filename);
                    printf("%s sent a remove request to the main server.\n", username);

                    if (write(tcp_sock, buffer, strlen(buffer)) < 0) {
                        perror("Failed to send remove request to server");
                        close(tcp_sock);
                        exit(EXIT_FAILURE);
                    }

                    // Receive response from the Main Server
                    memset(buffer, 0, MAX);
                    if (read(tcp_sock, buffer, MAX) < 0) {
                        perror("Failed to receive remove response from server");
                        close(tcp_sock);
                        exit(EXIT_FAILURE);
                    }

                    // Display the response from the server
                    printf("The remove request was successful.\n---Start a new request---\n");
                } else if (strncmp(command, "deploy", 6) == 0) {
                    // Send the deploy request to the Main Server
                    snprintf(buffer, MAX, "deploy %s", username);
                    printf("%s sent a deploy request to the main server.\n", username);

                    if (write(tcp_sock, buffer, strlen(buffer)) < 0) {
                        perror("Failed to send deploy request to server");
                        close(tcp_sock);
                        exit(EXIT_FAILURE);
                    }

                    // Receive response from the Main Server
                    memset(buffer, 0, MAX);
                    if (read(tcp_sock, buffer, MAX) < 0) {
                        perror("Failed to receive deploy response from server");
                        close(tcp_sock);
                        exit(EXIT_FAILURE);
                    }

                    // Display the response from the server (list of deployed files)
                    printf("The client received the response from the main server using TCP over port %d.\n%s---Start a new request---\n", MAIN_SERVER_TCP_PORT, buffer);
                } else if (strncmp(command, "log", 3) == 0) {
                    // Send the log request to the Main Server
                    snprintf(buffer, MAX, "log");
                    printf("%s sent a log request to the main server.\n", username);

                    if (write(tcp_sock, buffer, strlen(buffer)) < 0) {
                        perror("Failed to send log request to server");
                        close(tcp_sock);
                        exit(EXIT_FAILURE);
                    }

                    // Receive response from the Main Server
                    memset(buffer, 0, MAX);
                    if (read(tcp_sock, buffer, MAX) < 0) {
                        perror("Failed to receive log response from server");
                        close(tcp_sock);
                        exit(EXIT_FAILURE);
                    }

                    // Display the response from the server (log history)
                    printf("The client received the response from the main server using TCP over port %d.\n%s---Start a new request---\n", MAIN_SERVER_TCP_PORT, buffer);
                } else {
                    printf("Invalid command. Please try again.\n");
                }
            }
        }
    } else {
        // If authentication fails
        printf("The credentials are incorrect. Please try again.\n");
    }

    close(tcp_sock);
    return 0;
}

