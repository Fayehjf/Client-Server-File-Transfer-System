#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define AUTH_SERVER_UDP_PORT 21981
#define MAX 1024

// Function to trim whitespace from a string
void trim_whitespace(char *str) {
    char *end;

    // Trim leading space
    while (isspace((unsigned char) *str)) str++;

    // If all spaces or empty string
    if (*str == 0) return;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char) *end)) end--;

    // Write new null terminator character
    *(end + 1) = '\0';
}

// Structure to hold thread arguments
struct thread_args {
    char buffer[MAX];
    struct sockaddr_in client_addr;
    socklen_t addr_len;
    int udp_sock;
};

// Thread function to handle client requests
void *handle_client_request(void *args) {
    struct thread_args *targs = (struct thread_args *)args;
    char username[50];
    char password[50];
    char response[MAX];
    char *buffer = targs->buffer;

    // Extract username and password from the received buffer
    sscanf(buffer, "%s %s", username, password);

    // Trim any whitespace from username and password
    trim_whitespace(username);
    trim_whitespace(password);

    // Display received credentials
    printf("Server A received username %s and password ******.\n", username);

    // Special case for guest authentication
    if (strcmp(username, "guest") == 0 && strcmp(password, "guest") == 0) {
        snprintf(response, MAX, "Guest %s has been authenticated", username);
        //printf("Guest %s has been authenticated\n", username);
    } else {
        // Authenticate using members.txt
        FILE *file = fopen("members.txt", "r");
        if (file == NULL) {
            perror("Failed to open members.txt");
            close(targs->udp_sock);
            free(targs);
            pthread_exit(NULL);
        }

        int authenticated = 0;
        char line[MAX];
        while (fgets(line, sizeof(line), file) != NULL) {
            // Remove any trailing newline characters from the line
            trim_whitespace(line);
            
            char file_username[50], file_password[50];
            sscanf(line, "%s %s", file_username, file_password);
            
            // Trim whitespace from file values
            trim_whitespace(file_username);
            trim_whitespace(file_password);

            if (strcmp(username, file_username) == 0 && strcmp(password, file_password) == 0) {
                authenticated = 1;
                break;
            }
        }
        fclose(file);

        // Prepare authentication response
        if (authenticated) {
            snprintf(response, MAX, "Member %s has been authenticated.", username);
            printf("Member %s has been authenticated.\n", username);
        } else {
            snprintf(response, MAX, "The username %s or password ****** is incorrect.", username);
            printf("The username %s or password ****** is incorrect.\n", username);
        }
    }

    // Send authentication response back to Server M
    if (sendto(targs->udp_sock, response, strlen(response), 0, (struct sockaddr *)&targs->client_addr, targs->addr_len) < 0) {
        perror("Failed to send authentication response to Server M.");
    }

    // Clean up
    free(targs);
    pthread_exit(NULL);
}

int main() {
    int udp_sock;
    struct sockaddr_in auth_serv_addr, client_addr;
    char buffer[MAX];
    socklen_t addr_len = sizeof(client_addr);

    // Booting up message
    printf("Server A is up and running using UDP on port %d.\n", AUTH_SERVER_UDP_PORT);

    // Create UDP socket
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Define Authentication Server address
    memset(&auth_serv_addr, 0, sizeof(auth_serv_addr));
    auth_serv_addr.sin_family = AF_INET;
    auth_serv_addr.sin_addr.s_addr = INADDR_ANY;
    auth_serv_addr.sin_port = htons(AUTH_SERVER_UDP_PORT);

    // Bind UDP socket
    if (bind(udp_sock, (struct sockaddr *)&auth_serv_addr, sizeof(auth_serv_addr)) < 0) {
        perror("UDP socket bind failed");
        close(udp_sock);
        exit(EXIT_FAILURE);
    }

    // Wait for authentication request from Server M
    while (1) {
        memset(buffer, 0, MAX);
        if (recvfrom(udp_sock, buffer, MAX, 0, (struct sockaddr *)&client_addr, &addr_len) < 0) {
            perror("Failed to receive authentication request from Server M");
            continue; // Instead of exiting, continue to receive new requests
        }

        // Allocate memory for thread arguments
        struct thread_args *targs = malloc(sizeof(struct thread_args));
        if (!targs) {
            perror("Failed to allocate memory for thread arguments");
            continue;
        }

        // Fill thread arguments
        strcpy(targs->buffer, buffer);
        targs->client_addr = client_addr;
        targs->addr_len = addr_len;
        targs->udp_sock = udp_sock;

        // Create a new thread to handle the client request
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client_request, targs) != 0) {
            perror("Failed to create thread");
            free(targs);
            continue;
        }

        // Detach the thread so it can clean up after itself
        pthread_detach(thread_id);
    }

    close(udp_sock);
    return 0;
}

