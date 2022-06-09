#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <sys/time.h>

#define PORT ("3490")
#define BACKLOG (10)
#define INITIAL_SIZE (10)
#define MESSAGE_LENGTH (256)

void add_poll(struct pollfd * pfds[], int * size, int * max_size, int fd) {
  if ((*size) == (*max_size)) {
    (*max_size) *= 2;
    *pfds = reallocarray(*pfds, *max_size, *size);
  }

  (*pfds)[*size].fd = fd;
  (*pfds)[*size].events = POLLIN;
  (*size)++;
}

void remove_poll(struct pollfd pfds[], int index, int *size) {
  if (*size == 0) {
    return;
  }

  pfds[index].fd = pfds[*size].fd;
  pfds[index].events = pfds[*size].events;
  (*size)--;
}

int rand_int(int min, int max) {
  return min + (random() % (max - min + 1));
}

double get_time_diff(struct timeval * before, struct timeval * after) {
  gettimeofday(after, NULL);
  double time_taken = (after->tv_sec - before->tv_sec) * 1e6;
  time_taken = (time_taken + (after->tv_usec - before->tv_usec)) * 1e-6;
  return time_taken;
}

int main() {
  srandom(time(NULL));
  struct timeval start;

  gettimeofday(&start, NULL);

  struct addrinfo hints, *result, *p;
  int listener_sockfd;

  struct sockaddr_storage theiraddr;
  socklen_t slen = sizeof(theiraddr);

  int poll_size = 0;
  int max_size = 10;

  struct pollfd *pfds = malloc(sizeof(struct pollfd) * max_size);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if (getaddrinfo(NULL, PORT, &hints, &result) == -1) {
    perror("getaddrinfo");
    return -1;
  }

  for (p = result; p!= NULL; p = p->ai_next) {
    if ((listener_sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("socket");
      continue;
    }

    if (bind(listener_sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(listener_sockfd);
      perror("bind");
      continue;
    }

    break;
  }

  freeaddrinfo(result);

  if (p == NULL) {
    fprintf(stderr, "No Address!");
    return -2;
  }

  if (listen(listener_sockfd, BACKLOG) == -1) {
    perror("Listen");
    return -2;
  }

  add_poll(&pfds, &poll_size, &max_size, listener_sockfd);

  char buf[MESSAGE_LENGTH];
  char send_buf[MESSAGE_LENGTH];

  while (1) {
    int num_events = poll(pfds, poll_size, 2500);

    if (num_events == -1) {
      perror("Poll");
      return -3;
    }
    else {
      //printf("Begin Poll Check!\n");
      for (int i = 0; i < poll_size; i++) {
        //printf("Checking Socket %d at index %d\n", pfds[i].fd, i);
        if (pfds[i].revents & POLLIN) {
          //printf("Polled event!\n");
          if (pfds[i].fd == listener_sockfd) {
            int new_sock = accept(listener_sockfd, (struct sockaddr *) &theiraddr, &slen);
            if (new_sock == -1) {
              perror("Accept");
              return -4;
            }
            add_poll(&pfds, &poll_size, &max_size, new_sock);
            //printf("Added new client!\n");
          }
          else {
            //printf("Received Something!\n");
            int numbytes = recv(pfds[i].fd, buf, MESSAGE_LENGTH, 0);
            if (numbytes <= 0) {
              close(pfds[i].fd);
              remove_poll(pfds, i, &poll_size);
            }
            else {
              struct timeval time;
              usleep(rand_int(250000, 1750000));
              struct timeval post_sleep;
              strcpy(send_buf, "Now!\n");
              int send_bytes = send(pfds[i].fd, send_buf, strlen(send_buf), 0);
              gettimeofday(&post_sleep, NULL);
              numbytes = recv(pfds[i].fd, buf, MESSAGE_LENGTH, 0);
              if (numbytes == -1) {
                perror("Receive after send");
                return -4;
              }
              else {
                sprintf(send_buf, "Reaction Time in Milliseconds: %lf\n", (1000 * get_time_diff(&post_sleep, &time)));
                send_bytes = send(pfds[i].fd, send_buf, strlen(send_buf), 0);
              }
            }
          }
        }
      }
    }
  }
}

