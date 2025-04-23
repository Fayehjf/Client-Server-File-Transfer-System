#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define DEPLOY_SERVER_UDP_PORT 23981
#define MAX 1024

int main() {
    int udp_sock;
    struct sockaddr_in deploy_serv_addr, main_serv_addr;
    char buffer[MAX];
    socklen_t addr_len = sizeof(main_serv_addr);

    // Booting up message
    printf("Server D is up and running using UDP on port %d.\n", DEPLOY_SERVER_UDP_PORT);

    // Create UDP socket
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Define Deployment Server address
    memset(&deploy_serv_addr, 0, sizeof(deploy_serv_addr));
    deploy_serv_addr.sin_family = AF_INET;
    deploy_serv_addr.sin_addr.s_addr = INADDR_ANY;
    deploy_serv_addr.sin_port = htons(DEPLOY_SERVER_UDP_PORT);

    // Bind UDP socket
    if (bind(udp_sock, (struct sockaddr *)&deploy_serv_addr, sizeof(deploy_serv_addr)) < 0) {
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
        printf("Server D has received a deploy request from the main server.\n");

        // Write the received filenames to deployed.txt
        FILE *file = fopen("deployed.txt", "a");
        if (file == NULL) {
            perror("Failed to open deployed.txt");
            continue;
        }

        fprintf(file, "deployed files:\n%s\n", buffer);
        fclose(file);

        printf("Server D has deployed the user's repository.\n");

        // Send confirmation response back to Server M
        char response[MAX];
        snprintf(response, MAX, "Deployment successful.\n");
        if (sendto(udp_sock, response, strlen(response), 0, (struct sockaddr *)&main_serv_addr, addr_len) < 0) {
            perror("Failed to send deploy confirmation to Server M");
        }
    }

    close(udp_sock);
    return 0;
}

