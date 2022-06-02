#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT "3490"
#define MAXLEN (50)

int main(int argc, char * argv[]) {
  struct addrinfo hints, *result, *p;
  struct sockaddr_storage their_addr;
  int numbytes;
  int sockfd;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  if (getaddrinfo(NULL, PORT, &hints, &result) == -1) {
    perror("Get Addr Info\n");
    return -1;
  }

  for (p = result; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("Listener: Socket\n");
      continue;
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("Listener: Bind\n");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "Cannot Find Address\n");
    return -2;
  }

  freeaddrinfo(result);

  char ip[INET6_ADDRSTRLEN];

  inet_ntop(AF_INET6, p->ai_addr, ip, INET6_ADDRSTRLEN);
  while (1) {
    printf("Listening from %s at port %s\n", ip, PORT);

    char buf[MAXLEN];

    socklen_t slen = sizeof(their_addr);

    numbytes = recvfrom(sockfd, buf, MAXLEN-1, 0, (struct sockaddr *)&their_addr, &slen);

    inet_ntop(AF_INET6, (struct sockaddr_in6 *)&their_addr, ip, INET6_ADDRSTRLEN);

    printf("Got %d bytes from %s\n", numbytes, ip);

    buf[numbytes] = '\0';

    printf("Received %s from %s\n", buf, ip);
  }
}
