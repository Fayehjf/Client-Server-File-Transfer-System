#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAIN_SERVER_UDP_PORT 24981
#define MAIN_SERVER_TCP_PORT 25981
#define AUTH_SERVER_UDP_PORT 21981
#define REPO_SERVER_UDP_PORT 22981
#define DEPLOY_SERVER_UDP_PORT 23981
#define MAX 1024

struct thread_args {
    int client_sock;
    int udp_sock;
    struct sockaddr_in auth_serv_addr;
    struct sockaddr_in repo_serv_addr;
    struct sockaddr_in deploy_serv_addr;
};

// Function to log operations to a file
void log_operation(const char *username, const char *operation, const char *details) {
    FILE *log_file = fopen("server_logs.txt", "a");
    if (log_file == NULL) {
        perror("Failed to open server_logs.txt");
        return;
    }
    if (details) {
        fprintf(log_file, "%s: %s %s\n", username, operation, details);
    } else {
        fprintf(log_file, "%s: %s\n", username, operation);
    }
    fclose(log_file);
}

// Function to send log history to the client
void send_log_to_client(int client_sock, const char *username) {
    FILE *log_file = fopen("server_logs.txt", "r");
    if (log_file == NULL) {
        perror("Failed to open server_logs.txt");
        char error_response[MAX] = "No log available.";
        write(client_sock, error_response, strlen(error_response));
        return;
    }

    char buffer[MAX] = "";
    char line[MAX];
    int count = 1;

    while (fgets(line, sizeof(line), log_file) != NULL) {
        char log_username[50], operation[50], details[100] = "";
        sscanf(line, "%[^:]: %s %[^\n]", log_username, operation, details);

        if (strcmp(username, log_username) == 0) {
            char entry[MAX];
            if (strlen(details) > 0) {
                snprintf(entry, sizeof(entry), "%d. %s %s\n", count++, operation, details);
            } else {
                snprintf(entry, sizeof(entry), "%d. %s\n", count++, operation);
            }
            strncat(buffer, entry, MAX - strlen(buffer) - 1);
        }
    }
    fclose(log_file);

    // Ensure that buffer has data, else send appropriate message
    if (strlen(buffer) == 0) {
        snprintf(buffer, sizeof(buffer), "No log data available for user %s.\n", username);
    }

    // Send the response back to the client
    if (write(client_sock, buffer, strlen(buffer)) < 0) {
        perror("Failed to send log response to client");
    } else {
        printf("The main server has sent the log response to the client.\n");
    }
}

void *handle_authentication_request(void *args) {
    struct thread_args *targs = (struct thread_args *)args;
    int client_sock = targs->client_sock;
    int udp_sock = targs->udp_sock;
    struct sockaddr_in auth_serv_addr = targs->auth_serv_addr;
    struct sockaddr_in repo_serv_addr = targs->repo_serv_addr;
    struct sockaddr_in deploy_serv_addr = targs->deploy_serv_addr;
    socklen_t addr_len = sizeof(repo_serv_addr);
    free(targs);
    
    char username[50];
    char password[50];
    char buffer[MAX];
    int authenticated = 0;
    
    // Receive credentials from the client
    memset(buffer, 0, MAX);
    if (read(client_sock, buffer, MAX) < 0) {
        perror("Failed to read from client");
        close(client_sock);
        pthread_exit(NULL);
    }

    // Extract username and password
    sscanf(buffer, "%s %s", username, password);
    
    // Display received username and password (masking password)
    printf("Server M has received username %s and password ****.\n", username);
    printf("Server M has sent authentication request to Server A.\n");

    // Forward authentication request to server A using UDP
    if (sendto(udp_sock, buffer, strlen(buffer), 0, (struct sockaddr *)&auth_serv_addr, addr_len) < 0) {
        perror("Failed to send authentication request to Server A");
        close(client_sock);
        pthread_exit(NULL);
    }
    
    // Receive response from server A
    memset(buffer, 0, MAX);
    if (recvfrom(udp_sock, buffer, MAX, 0, (struct sockaddr *)&auth_serv_addr, &addr_len) < 0) {
        perror("Failed to receive response from Server A");
        close(client_sock);
        pthread_exit(NULL);
    }

    // Display response from Server A
    printf("The main server has received the response from server A using UDP over port %d.\n", MAIN_SERVER_UDP_PORT);
    
    // Send response back to the client
    if (write(client_sock, buffer, strlen(buffer)) < 0) {
        perror("Failed to send response to client");
        close(client_sock);
        pthread_exit(NULL);
    }
    printf("The main server has sent the response from server A using TCP over port %d.\n", MAIN_SERVER_TCP_PORT);
    
    // Check if authenticated
    if (strstr(buffer, "authenticated")) {
        authenticated = 1;
    }
    
    // If authenticated, handle further commands
    if (authenticated) {
        while (1) {
            memset(buffer, 0, MAX);
            if (read(client_sock, buffer, MAX) < 0) {
                perror("Failed to read command from client");
                close(client_sock);
                pthread_exit(NULL);
            }
            
            // Extract command and target username
            char command[50], target_username[50];
            sscanf(buffer, "%s %s", command, target_username);
            
            // Handle different commands
            if (strcmp(command, "lookup") == 0) {
                // Log lookup request based on user type
                if (strstr(username, "guest")) {
                    printf("The main server has received a lookup request from Guest to lookup %s's repository using TCP over port %d.\n", target_username, MAIN_SERVER_TCP_PORT);
                } else {
                    printf("The main server has received a lookup request from %s to lookup %s's repository using TCP over port %d.\n", username, target_username, MAIN_SERVER_TCP_PORT);
                    log_operation(username, "lookup",target_username);
                }

                // Forward lookup request to Server R
                if (sendto(udp_sock, buffer, strlen(buffer), 0, (struct sockaddr *)&repo_serv_addr, addr_len) < 0) {
                    perror("Failed to send lookup request to Server R");
                    close(client_sock);
                    exit(EXIT_FAILURE);
                }
                printf("The main server has sent the lookup request to server R.\n");

                // Receive response from Server R
                memset(buffer, 0, MAX);
                if (recvfrom(udp_sock, buffer, MAX, 0, (struct sockaddr *)&repo_serv_addr, &addr_len) < 0) {
                    perror("Failed to receive response from Server R");
                    close(client_sock);
                    pthread_exit(NULL);
                }
                printf("The main server has received the response from server R using UDP over port %d.\n", MAIN_SERVER_UDP_PORT);

                // Send response back to the client
                if (write(client_sock, buffer, strlen(buffer)) < 0) {
                    perror("Failed to send response to client");
                    close(client_sock);
                    exit(EXIT_FAILURE);
                }
                printf("The main server has sent the response to the client.\n");

            } else if (strcmp(command, "push") == 0) {
                // Handle push request
                printf("The main server has received a push request from %s, using TCP over port %d.\n", username, MAIN_SERVER_TCP_PORT);
                log_operation(username, "push", target_username);

                // Forward push request to Server R
                if (sendto(udp_sock, buffer, strlen(buffer), 0, (struct sockaddr *)&repo_serv_addr, addr_len) < 0) {
                    perror("Failed to send push request to Server R.");
                    close(client_sock);
                    exit(EXIT_FAILURE);
                }
                printf("The main server has sent the push request to server R.\n");

                // Receive response from Server R
                memset(buffer, 0, MAX);
                if (recvfrom(udp_sock, buffer, MAX, 0, (struct sockaddr *)&repo_serv_addr, &addr_len) < 0) {
                    perror("Failed to receive response from Server R.");
                    close(client_sock);
                    pthread_exit(NULL);
                }

                if (strstr(buffer, "overwrite confirmation")) {
                    printf("The main server has received the response from server R using UDP over port %d, asking for overwrite confirmation from %s.\n", MAIN_SERVER_UDP_PORT, username);
                    
                    // Forward the overwrite request to the client
                    if (write(client_sock, buffer, strlen(buffer)) < 0) {
                        perror("Failed to send overwrite confirmation request to client.");
                        close(client_sock);
                        exit(EXIT_FAILURE);
                    }
                    printf("The main server has sent the overwrite confirmation request to the client.\n");

                    // Wait for overwrite confirmation from the client
                    memset(buffer, 0, MAX);
                    if (read(client_sock, buffer, MAX) < 0) {
                        perror("Failed to read overwrite confirmation from client.");
                        close(client_sock);
                        pthread_exit(NULL);
                    }
                    printf("The main server has received the overwrite confirmation response from %s using TCP over port %d.\n", username, MAIN_SERVER_TCP_PORT);

                    // Forward overwrite confirmation to Server R
                    if (sendto(udp_sock, buffer, strlen(buffer), 0, (struct sockaddr *)&repo_serv_addr, addr_len) < 0) {
                        perror("Failed to send overwrite confirmation to Server R.");
                        close(client_sock);
                        exit(EXIT_FAILURE);
                    }
                    printf("The main server has sent the overwrite confirmation response to server R.\n");

                } else {
                    printf("The main server has received the response from server R using UDP over port %d.\n", MAIN_SERVER_UDP_PORT);

                    // Send response back to the client
                    if (write(client_sock, buffer, strlen(buffer)) < 0) {
                        perror("Failed to send response to client");
                        close(client_sock);
                        exit(EXIT_FAILURE);
                    }
                    printf("The main server has sent the response to the client.\n");
                }

            } else if (strcmp(command, "remove") == 0) {
                // Handle remove request
                printf("The main server has received a remove request from member %s using TCP over port %d.\n", username, MAIN_SERVER_TCP_PORT);
                log_operation(username, "remove", target_username);

                // Forward remove request to Server R
                if (sendto(udp_sock, buffer, strlen(buffer), 0, (struct sockaddr *)&repo_serv_addr, addr_len) < 0) {
                    perror("Failed to send remove request to Server R");
                    close(client_sock);
                    exit(EXIT_FAILURE);
                }

                // Receive confirmation from Server R
                memset(buffer, 0, MAX);
                if (recvfrom(udp_sock, buffer, MAX, 0, (struct sockaddr *)&repo_serv_addr, &addr_len) < 0) {
                    perror("Failed to receive remove confirmation from Server R");
                    close(client_sock);
                    pthread_exit(NULL);
                }
                printf("The main server has received confirmation of the remove request done by server R.\n");
                // Send response back to the client
                if (write(client_sock, buffer, strlen(buffer)) < 0) {
                    perror("Failed to send response to client");
                    close(client_sock);
                    exit(EXIT_FAILURE);
                }
                //printf("The main server has sent the remove confirmation to the client.\n");
            } else if (strcmp(command, "deploy") == 0) {
                // Handle deploy request
                printf("The main server has received a deploy request from member %s using TCP over port %d.\n", username, MAIN_SERVER_TCP_PORT);
                log_operation(username, "deploy", NULL);

                // Forward deploy request to Server R
                if (sendto(udp_sock, buffer, strlen(buffer), 0, (struct sockaddr *)&repo_serv_addr, addr_len) < 0) {
                    perror("Failed to send deploy request to Server R");
                    close(client_sock);
                    exit(EXIT_FAILURE);
                }
                printf("The main server has sent the deploy request to server R.\n");

                // Receive response from Server R
                memset(buffer, 0, MAX);
                if (recvfrom(udp_sock, buffer, MAX, 0, (struct sockaddr *)&repo_serv_addr, &addr_len) < 0) {
                    perror("Failed to receive deploy response from Server R");
                    close(client_sock);
                    pthread_exit(NULL);
                }
                printf("The main server received the deploy response from server R.\n");

                // Forward deployment request to Server D
                if (sendto(udp_sock, buffer, strlen(buffer), 0, (struct sockaddr *)&deploy_serv_addr, addr_len) < 0) {
                    perror("Failed to send deploy request to Server D");
                    close(client_sock);
                    exit(EXIT_FAILURE);
                }
                printf("The main server has sent the deploy request to server D.\n");

                // Receive confirmation response from Server D
                memset(buffer, 0, MAX);
                if (recvfrom(udp_sock, buffer, MAX, 0, (struct sockaddr *)&deploy_serv_addr, &addr_len) < 0) {
                    perror("Failed to receive confirmation from Server D");
                    close(client_sock);
                    pthread_exit(NULL);
                }
                printf("The user %s's repository has been deployed at server D.\n", username);

                // Send confirmation response to the client
                if (write(client_sock, buffer, strlen(buffer)) < 0) {
                    perror("Failed to send response to client");
                    close(client_sock);
                    exit(EXIT_FAILURE);
                }
                printf("The main server has sent the deploy confirmation to the client.\n");

            } else if (strcmp(command, "log") == 0) {
                // Handle log request
                printf("The main server has received a log request from member %s over TCP port %d.\n", username, MAIN_SERVER_TCP_PORT);
                log_operation(username, "log", NULL);

                send_log_to_client(client_sock, username);
            } else {
                printf("Invalid command received. Ignoring.\n");
            }
        }
    }

    close(client_sock);
    pthread_exit(NULL);
}

int main() {
    int udp_sock, tcp_sock;
    struct sockaddr_in udp_serv_addr, tcp_serv_addr, auth_serv_addr, repo_serv_addr, deploy_serv_addr;
    char buffer[MAX];

    // Booting up message
    printf("Server M is up and running using UDP on port %d.\n", MAIN_SERVER_UDP_PORT);

    // Create UDP socket
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Define UDP server address
    memset(&udp_serv_addr, 0, sizeof(udp_serv_addr));
    udp_serv_addr.sin_family = AF_INET;
    udp_serv_addr.sin_addr.s_addr = INADDR_ANY;
    udp_serv_addr.sin_port = htons(MAIN_SERVER_UDP_PORT);

    // Bind UDP socket
    if (bind(udp_sock, (struct sockaddr *)&udp_serv_addr, sizeof(udp_serv_addr)) < 0) {
        perror("UDP socket bind failed");
        close(udp_sock);
        exit(EXIT_FAILURE);
    }

    // Define Authentication Server address
    memset(&auth_serv_addr, 0, sizeof(auth_serv_addr));
    auth_serv_addr.sin_family = AF_INET;
    auth_serv_addr.sin_port = htons(AUTH_SERVER_UDP_PORT);
    auth_serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Define Repository Server address
    memset(&repo_serv_addr, 0, sizeof(repo_serv_addr));
    repo_serv_addr.sin_family = AF_INET;
    repo_serv_addr.sin_port = htons(REPO_SERVER_UDP_PORT);
    repo_serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Define Deployment Server address
    memset(&deploy_serv_addr, 0, sizeof(deploy_serv_addr));
    deploy_serv_addr.sin_family = AF_INET;
    deploy_serv_addr.sin_port = htons(DEPLOY_SERVER_UDP_PORT);
    deploy_serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Listen for incoming connections on TCP socket
    tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock < 0) {
        perror("TCP socket creation failed");
        close(udp_sock);
        exit(EXIT_FAILURE);
    }
    
    // Set socket option to reuse the address
    int opt = 1;
    if (setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(tcp_sock);
        exit(EXIT_FAILURE);
    }
    
    tcp_serv_addr.sin_family = AF_INET;
    tcp_serv_addr.sin_addr.s_addr = INADDR_ANY;
    tcp_serv_addr.sin_port = htons(MAIN_SERVER_TCP_PORT);

    if (bind(tcp_sock, (struct sockaddr *)&tcp_serv_addr, sizeof(tcp_serv_addr)) < 0) {
        perror("TCP socket bind failed");
        close(tcp_sock);
        close(udp_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(tcp_sock, 3) < 0) {
        perror("TCP listen failed");
        close(tcp_sock);
        close(udp_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server M is now listening for incoming TCP connections on port %d.\n", MAIN_SERVER_TCP_PORT);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_sock = accept(tcp_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            perror("TCP accept failed");
            continue; // Instead of exiting, continue accepting other clients
        }

        // Allocate memory for thread arguments
        struct thread_args *targs = malloc(sizeof(struct thread_args));
        if (!targs) {
            perror("Failed to allocate memory for thread arguments");
            close(client_sock);
            continue; // Handle other clients
        }

        targs->client_sock = client_sock;
        targs->udp_sock = udp_sock;
        targs->auth_serv_addr = auth_serv_addr;
        targs->repo_serv_addr = repo_serv_addr;
        targs->deploy_serv_addr = deploy_serv_addr;

        // Create a new thread to handle the client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_authentication_request, targs) != 0) {
            perror("Failed to create thread");
            close(client_sock);
            free(targs);
            continue; // Handle other clients
        }

        // Detach the thread so it cleans up after itself
        pthread_detach(thread_id);
    }

    close(tcp_sock);
    close(udp_sock);
    return 0;
}

