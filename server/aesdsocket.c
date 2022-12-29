#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

// Ugly code, written in a rush... :-/

static int loopserv = 1;
static int sockfd;
static int newfd;

void signal_handler(int signal) {
  loopserv = 0;
  shutdown(sockfd, SHUT_RDWR);
  shutdown(newfd, SHUT_RDWR);
}

int main(int argc, char *argv[]) {
  int run_as_daemon = 0;
  int opt;
  while ((opt = getopt(argc, argv, "d")) != -1) {
    if (opt == 'd') {
      run_as_daemon = 1;
      fprintf(stdout, "Running as daemon...\n");
    }
  }

  // Open stream socket on port 9000

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  int status;
  struct addrinfo hints;
  struct addrinfo *res;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((status = getaddrinfo(NULL, "9000", &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo() failed!\n");
    exit(1);
  }

  sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (sockfd == -1) {
    fprintf(stderr, "socket() failed!\n");
    exit(1);
  }

  // Listen for and accept connection

  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    fprintf(stderr, "setsockopt() failed!\n");
    exit(1);
  }

  if (bind(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
    fprintf(stderr, "bind() failed!\n");
    exit(1);
  }

  freeaddrinfo(res);

  // Fork here if running as daemon

  if (run_as_daemon) {
    pid_t pid = fork();
    if (pid == -1) {
      fprintf(stderr, "fork() failed!\n");
      exit(1);
    }

    // Parent process can exit

    if (pid > 0) {
      return 0;
    }

    // We're in the child

    setsid();
    chdir("/");
  }

  if (listen(sockfd, 10) != 0) {
    fprintf(stderr, "listen() failed!\n");
    exit(1);
  }

  struct sockaddr_storage remote_addr;
  socklen_t remote_addrlen = sizeof remote_addr;

  while (loopserv) {
    newfd = accept(sockfd, (struct sockaddr *)&remote_addr, &remote_addrlen);
    if (newfd == -1) {
      if (loopserv) fprintf(stderr, "accept() failed!\n");
      continue;
    }

    // Log message to syslog: Accepted connection from XXX

    char ipstr[INET6_ADDRSTRLEN];
    if (remote_addr.ss_family == AF_INET) {
      struct sockaddr_in *s = (struct sockaddr_in *)&remote_addr;
      inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
    } else {
      struct sockaddr_in6 *s = (struct sockaddr_in6 *)&remote_addr;
      inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
    }

    syslog(LOG_DEBUG, "Accepted connection from %s", ipstr);

    // Recieve data from connection and append to /var/tmp/aesdsocketdata

    int conloop = 1;
    while (conloop && loopserv) {
      FILE *file = fopen("/var/tmp/aesdsocketdata", "a");
      if (file == NULL) {
        fprintf(stdout, "fopen() failed!");
        exit(1);
      }

      char buffer[20480];
      memset(&buffer, 0, 20480);

      size_t space = 20480;
      size_t offset = 0;

      // Read a message

      while (space > 0 && loopserv) {
        size_t len = recv(newfd, &buffer[offset], space, 0);
        if (len == -1) {
          fprintf(stderr, "recv() failed!\n");
          exit(1);
        }

        if (len == 0) {
          conloop = 0;
          break;
        }

        space -= len;
        offset += len;
        if (buffer[offset - 1] == '\n') break;
      }

      // Message complete or connection closed...

      if (conloop != 0) {
        fprintf(file, "%s", buffer);
        fclose(file);

        // Got a complete message, return full content
        // of /var/tmp/aesdsocketdata to client

        FILE *file2 = fopen("/var/tmp/aesdsocketdata", "r");

        int bufferLength = 20480;
        char linebuffer[bufferLength];

        while (fgets(linebuffer, bufferLength, file2)) {
          send(newfd, &linebuffer, strlen(linebuffer), 0);
        }

        fclose(file2);
      } else {
        fclose(file);
      }
    }

    // Log message to syslog: Closed connection from XXX

    syslog(LOG_DEBUG, "Closed connection from %s", ipstr);
    close(newfd);

    // Loop until SIGINT or SIGTERM
    // Graceful shutdown on signals
  }

  // Log message to syslog: Caught signal, exiting

  syslog(LOG_DEBUG, "Caught signal, exiting");

  // Delete /var/tmp/aesdsocketdata

  close(sockfd);
  remove("/var/tmp/aesdsocketdata");

  return 0;
}
