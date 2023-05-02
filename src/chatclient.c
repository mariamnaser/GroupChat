/*******************************************************************************
 * Name          : chatClinet.c
 * Author        : Mariam Naser
 * Date          : May 2, 2023
 * Description   : UDP Group Chat 
 ******************************************************************************/

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/select.h>
#include "util.h"

#define STDIN_FILENO 0

int client_socket = -1;
char username[MAX_NAME_LEN + 1];
char inbuf[BUFLEN + 1];
char outbuf[MAX_MSG_LEN + 1];

int handle_stdin(int sockfd);
int handle_client_socket(void);

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server-ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &server_address.sin_addr) <= 0) {
        fprintf(stderr, "Error converting server IP address: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        if (errno != EINPROGRESS) {
            perror("Error connecting to server");
            exit(EXIT_FAILURE);
        }
    }
    printf("Enter username: ");
    fflush(stdout);
    if (fgets(username, MAX_NAME_LEN + 1, stdin) == NULL) {
        perror("Error reading username from stdin");
        exit(EXIT_FAILURE);
    }

    username[strcspn(username, "\n")] = '\0';
    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(client_socket, &write_fds);
    if (select(client_socket + 1, NULL, &write_fds, NULL, NULL) < 0) {
        perror("Error waiting for connection");
        exit(EXIT_FAILURE);
    }
    if (send(client_socket, username, strlen(username) + 1, 0) < 0) {
        perror("Error sending username to server");
        exit(EXIT_FAILURE);
    }
      while (1) {
          fd_set read_fds;
          FD_ZERO(&read_fds);
          FD_SET(STDIN_FILENO, &read_fds);
          FD_SET(client_socket, &read_fds);

          if (select(client_socket + 1, &read_fds, NULL, NULL, NULL) < 0) {
              perror("Error in select");
              exit(EXIT_FAILURE);
          }

          // Handle input from stdin
          if (FD_ISSET(STDIN_FILENO, &read_fds)) {
              if (handle_stdin(client_socket) == EXIT_FAILURE) {
                  exit(EXIT_FAILURE);
              }
          }
          if (FD_ISSET(client_socket, &read_fds)) {
              if (handle_client_socket() < 0) {
                  exit(EXIT_FAILURE);
              }
          }
          printf("\n");

      }
}

int handle_stdin(int sockfd) {
    char buf[BUFLEN];
    int c, i = 0;

    fd_set set;
    struct timeval timeout;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    int ready = select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);
    if (ready == -1) {
        perror("Error waiting for input from stdin");
        return EXIT_FAILURE;
    }

    if (ready == 1) {
        while ((c = getchar()) != EOF && c != '\n' && i < BUFLEN - 1) {
            buf[i++] = c;
        }
        buf[i] = '\0';

        if (i == 0) {
            return EXIT_SUCCESS;
        }
        if (send(sockfd, buf, strlen(buf) + 1, 0) < 0) {
            perror("Error sending message to server");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int handle_client_socket(void) {
    int nbytes;
    if ((nbytes = recv(client_socket, inbuf, BUFLEN, 0)) > 0) {
        inbuf[nbytes] = '\0';
        printf("%s", inbuf);
        return 1;
    } else if (nbytes == 0) {
        fprintf(stderr, "Server closed the connection\n");
        return -1;
    } else {
        fprintf(stderr, "Error receiving message from server: %s\n", strerror(errno));
        return -1;
    }
}
