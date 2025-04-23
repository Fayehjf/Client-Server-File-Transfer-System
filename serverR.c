#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <ctype.h>

#define REPO_SERVER_UDP_PORT 22981
#define MAX 1024

// Function to trim whitespace from a string
void trim_whitespace(char *str) {
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    // If all spaces or empty string
    if (*str == 0) return;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator character
    *(end + 1) = '\0';
}

void handle_lookup_request(int udp_sock, struct sockaddr_in main_serv_addr, char *username, socklen_t addr_len) {
    printf("Server R has received a lookup request from the main server.\n");

    // Open the file to search for the username
    FILE *file = fopen("filenames.txt", "r");
    if (file == NULL) {
        perror("Failed to open filenames.txt");
        return;
    }

    // Prepare the response for the username lookup
    char response[MAX] = "";
    char line[MAX];
    int found = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        char file_username[50], filename[100];
        sscanf(line, "%s %s", file_username, filename);

        // Trim whitespace from file data
        trim_whitespace(file_username);
        trim_whitespace(filename);

        if (strcmp(username, file_username) == 0) {
            found = 1;
            if (strlen(response) + strlen(filename) + 2 < MAX) {  // +2 for '\n' and null terminator
                strncat(response, filename, MAX - strlen(response) - 1);
                strncat(response, "\n", MAX - strlen(response) - 1);
            } else {
                fprintf(stderr, "Warning: Response truncated due to buffer size.\n");
                break;
            }
        }
    }
    fclose(file);

    if (!found) {
        snprintf(response, MAX - 1, "Empty repository\n");
    }

    // Send the response back to Server M
    if (sendto(udp_sock, response, strlen(response), 0, (struct sockaddr *)&main_serv_addr, addr_len) < 0) {
        perror("Failed to send lookup response to Server M");
    }

    printf("Server R has finished sending the response to the main server.\n");
}

void handle_push_request(int udp_sock, struct sockaddr_in main_serv_addr, char *username, char *filename, socklen_t addr_len) {
    printf("Server R has received a push request from the main server.\n");
    FILE *file = fopen("filenames.txt", "r");
    FILE *temp_file = fopen("temp_filenames.txt", "w");
    if (file == NULL || temp_file == NULL) {
        perror("Failed to open filenames.txt or temp_filenames.txt");
        if (file) fclose(file);
        if (temp_file) fclose(temp_file);
        return;
    }

    char line[MAX];
    int found = 0;
    //int overwrite = 0;

    // Search for the filename in the member's repository and write to temp file
    while (fgets(line, sizeof(line), file) != NULL) {
        char file_username[50], file_filename[100];
        sscanf(line, "%s %s", file_username, file_filename);

        // Trim whitespace from file data
        trim_whitespace(file_username);
        trim_whitespace(file_filename);

        // Compare the username and filename
        if (strcmp(username, file_username) == 0 && strcmp(filename, file_filename) == 0) {
            found = 1;
            printf("%s exists in %s's repository; requesting overwrite confirmation.\n", filename, username);

            // Ask for overwrite confirmation
            char response[MAX];
            snprintf(response, MAX, "%s exists in %s's repository; requesting overwrite confirmation.", filename, username);
            if (sendto(udp_sock, response, strlen(response), 0, (struct sockaddr *)&main_serv_addr, addr_len) < 0) {
                perror("Failed to send overwrite confirmation request to Server M.");
                fclose(file);
                fclose(temp_file);
                return;
            }

            // Receive overwrite confirmation from Server M
            memset(response, 0, sizeof(response));
            if (recvfrom(udp_sock, response, sizeof(response), 0, (struct sockaddr *)&main_serv_addr, &addr_len) < 0) {
                perror("Failed to receive overwrite confirmation from Server M.");
                fclose(file);
                fclose(temp_file);
                return;
            }

            trim_whitespace(response);
            if (strcasecmp(response, "Y") == 0) {
                printf("User requested overwrite; overwrite successful.\n");
                //overwrite = 1;
                // Overwrite the line in temp file
                fprintf(temp_file, "%s %s\n", username, filename);
            } else {
                printf("Overwrite denied\n");
                fprintf(temp_file, "%s %s\n", file_username, file_filename);
            }
        } else {
            // Write the unchanged line to the temp file
            fprintf(temp_file, "%s %s\n", file_username, file_filename);
        }
    }

    // If filename was not found, add it to the repository
    if (!found) {
        fprintf(temp_file, "%s %s\n", username, filename);
        printf("%s uploaded successfully.\n", filename);
        
        // Send success response to Server M
        char response[MAX];
        snprintf(response, MAX, "%s pushed successfully.\n", filename);
        if (sendto(udp_sock, response, strlen(response), 0, (struct sockaddr *)&main_serv_addr, addr_len) < 0) {
            perror("Failed to send push response to Server M.");
        }
    }

    // Close both files
    fclose(file);
    fclose(temp_file);

    // Replace the original file with the updated temp file
    if (remove("filenames.txt") != 0) {
        perror("Failed to delete original filenames.txt");
        return;
    }
    if (rename("temp_filenames.txt", "filenames.txt") != 0) {
        perror("Failed to rename temp_filenames.txt to filenames.txt");
        return;
    }
}


void handle_remove_request(int udp_sock, struct sockaddr_in main_serv_addr, char *username, char *filename, socklen_t addr_len) {
    printf("Server R has received a remove request from the main server.\n");

    // Open the file to search for the username and filename
    FILE *file = fopen("filenames.txt", "r");
    if (file == NULL) {
        perror("Failed to open filenames.txt");
        return;
    }

    FILE *temp_file = fopen("temp_filenames.txt", "w");
    if (temp_file == NULL) {
        perror("Failed to open temporary file for writing");
        fclose(file);
        return;
    }

    char line[MAX];
    int found = 0;

    // Iterate over each line and write all lines except the one to be removed to the temp file
    while (fgets(line, sizeof(line), file) != NULL) {
        char file_username[50], file_filename[100];
        sscanf(line, "%s %s", file_username, file_filename);

        // Trim whitespace from file data
        trim_whitespace(file_username);
        trim_whitespace(file_filename);

        // Check if the current line matches the username and filename to be removed
        if (strcmp(username, file_username) == 0 && strcmp(filename, file_filename) == 0) {
            found = 1;
            continue; // Skip writing this line to the temp file
        }

        fprintf(temp_file, "%s", line);
    }

    fclose(file);
    fclose(temp_file);

    // Replace the old file with the new one
    if (found) {
        remove("filenames.txt");
        rename("temp_filenames.txt", "filenames.txt");

        // Send success response to Server M
        char response[MAX];
        snprintf(response, MAX, "%s removed successfully.\n", filename);
        if (sendto(udp_sock, response, strlen(response), 0, (struct sockaddr *)&main_serv_addr, addr_len) < 0) {
            perror("Failed to send remove response to Server M.");
        }
    } else {
        remove("temp_filenames.txt");
        // Send error response to Server M
        char response[MAX];
        snprintf(response, MAX, "Error: %s not found in %s's repository.\n", filename, username);
        if (sendto(udp_sock, response, strlen(response), 0, (struct sockaddr *)&main_serv_addr, addr_len) < 0) {
            perror("Failed to send remove error response to Server M.");
        }
    }

    printf("Server R has finished processing the remove request.\n");
}

void handle_deploy_request(int udp_sock, struct sockaddr_in main_serv_addr, char *username, socklen_t addr_len) {
    printf("Server R has received a deploy request from the main server.\n");

    // Open the file to search for the username
    FILE *file = fopen("filenames.txt", "r");
    if (file == NULL) {
        perror("Failed to open filenames.txt");
        return;
    }

    // Prepare the response with all filenames associated with the username
    char response[MAX] = "";
    char line[MAX];
    int found = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        char file_username[50], filename[100];
        sscanf(line, "%s %s", file_username, filename);

        trim_whitespace(file_username);
        trim_whitespace(filename);

        if (strcmp(username, file_username) == 0) {
            found = 1;
            if (strlen(response) + strlen(filename) + 2 < MAX) {
                strncat(response, filename, MAX - strlen(response) - 1);
                strncat(response, "\n", MAX - strlen(response) - 1);
            } else {
                fprintf(stderr, "Warning: Response truncated due to buffer size.\n");
                break;
            }
        }
    }
    fclose(file);

    if (!found) {
        snprintf(response, MAX - 1, "Empty repository\n");
    }

    // Send the response back to Server M
    if (sendto(udp_sock, response, strlen(response), 0, (struct sockaddr *)&main_serv_addr, addr_len) < 0) {
        perror("Failed to send deploy response to Server M");
    }

    printf("Server R has finished sending the response to the main server.\n");
}

int main() {
    int udp_sock;
    struct sockaddr_in repo_serv_addr, main_serv_addr;
    char buffer[MAX];
    socklen_t addr_len = sizeof(main_serv_addr);

    // Booting up message
    printf("Server R is up and running using UDP on port %d.\n", REPO_SERVER_UDP_PORT);

    // Create UDP socket
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Define Repository Server address
    memset(&repo_serv_addr, 0, sizeof(repo_serv_addr));
    repo_serv_addr.sin_family = AF_INET;
    repo_serv_addr.sin_addr.s_addr = INADDR_ANY;
    repo_serv_addr.sin_port = htons(REPO_SERVER_UDP_PORT);

    // Bind UDP socket
    if (bind(udp_sock, (struct sockaddr *)&repo_serv_addr, sizeof(repo_serv_addr)) < 0) {
        perror("UDP socket bind failed");
        close(udp_sock);
        exit(EXIT_FAILURE);
    }

    // Wait for requests from Server M
    while (1) {
        memset(buffer, 0, MAX);
        if (recvfrom(udp_sock, buffer, MAX, 0, (struct sockaddr *)&main_serv_addr, &addr_len) < 0) {
            perror("Failed to receive request from Server M");
            continue;
        }

        // Extract command and parameters from the received message
        char command[50];
        char username[50] = "";
        char filename[100] = "";
        sscanf(buffer, "%s %s %s", command, username, filename);

        // Trim whitespace
        trim_whitespace(command);
        trim_whitespace(username);
        trim_whitespace(filename);

        // Handle different commands
        if (strcmp(command, "lookup") == 0) {
            handle_lookup_request(udp_sock, main_serv_addr, username, addr_len);
        } else if (strcmp(command, "push") == 0) {
            handle_push_request(udp_sock, main_serv_addr, username, filename, addr_len);
        } else if (strcmp(command, "remove") == 0) {
            handle_remove_request(udp_sock, main_serv_addr, username, filename, addr_len);
        } else if (strcmp(command, "deploy") == 0) {
            handle_deploy_request(udp_sock, main_serv_addr, username, addr_len);
        } else {
            printf("Unknown command received: %s\n", command);
        }
    }

    close(udp_sock);
    return 0;
}

