#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>

#define PORT ("3490")

int main(int argc, char * argv[]) {

  struct addrinfo hints, *result, *p;
  int sockfd;
  int numbytes;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_DGRAM;

  if (argc != 3) {
    fprintf(stderr, "Usage: command address Message\n");
    return -1;
  }

  if (getaddrinfo(argv[1], PORT, &hints, &result) == -1) {
    perror("Can't Get Address");
    return -2;
  }

  for (p = result; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("Sender: socket");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "Cannot get Address Object\n");
    return -3;
  }

  freeaddrinfo(result);

  printf("Sending to %s on port %s\n", argv[1], PORT);

  numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0, p->ai_addr, p->ai_addrlen);

  printf("Sent %d bytes to %s\n\n", numbytes, argv[1]);
}
