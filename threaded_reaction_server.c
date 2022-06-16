#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#define PORT ("3490")
#define BACKLOG (10)
#define INITIAL_SIZE (3)
#define MESSAGE_LENGTH (256)
#define POLL_TIMEOUT_COUNT (5)

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

typedef struct sock_time {
  int socket;
  int wait_time;
} sock_time_t;

typedef struct sock_size {
  int socket;
  int * size;
} sock_size_t;

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

void * send_delay(void * arg) {
  sock_time_t * st = (sock_time_t *) arg;
  int socket = st->socket;
  int wait_time = st->wait_time;

  usleep(wait_time);

  char * send_buf = "Now!\n";
  int sendbytes = send(socket, send_buf, strlen(send_buf), 0);
  if (sendbytes == -1) {
    close(socket);
    perror("Sub Thread: Send");
    pthread_exit(NULL);
  }
  return NULL;
}

void decrement_size(int * size) {
  pthread_mutex_lock(&mutex1);
  (*size)--;
  pthread_mutex_unlock(&mutex1);
}

void * play_game(void * arg) {
  sock_size_t * sock_size = (sock_size_t *) arg;
  int socket = sock_size->socket;
  int * size = sock_size->size;
  struct pollfd pfds[1];
  pfds[0].fd = socket;
  pfds[0].events = POLLIN;
  char recv_buf[MESSAGE_LENGTH];
  char send_buf[MESSAGE_LENGTH];


  while (1) {
    int num_events = poll(pfds, 1, 500);
    if (num_events == -1) {
      perror("Thread: Polling");
      decrement_size(size);
      pthread_exit(NULL);
    }
    else {
      if (pfds[0].revents & POLLIN) {
        struct timeval post_sleep;
        struct timeval receive_input;
        int numbytes = recv(pfds[0].fd, recv_buf, MESSAGE_LENGTH, 0);
        if (numbytes <= -1) {
          close(pfds[0].fd);
          perror("Thread: Receive");
          decrement_size(size);
          pthread_exit(NULL);
        }
        else if (numbytes == 0) {
          close(pfds[0].fd);
          decrement_size(size);
          pthread_exit(NULL);
        }
        int sleep_time = rand_int(250000, 1750000); // Microseconds
        sock_time_t st = {.socket = pfds[0].fd, .wait_time = sleep_time};
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, send_delay, &st);
        gettimeofday(&post_sleep, NULL);

        numbytes = recv(pfds[0].fd, recv_buf, MESSAGE_LENGTH, 0);

        if (numbytes <= -1) {
          close(pfds[0].fd);
          perror("Thread: Receiving");
          decrement_size(size);
          pthread_exit(NULL);
        }
        else if (numbytes == 0) {
          close(pfds[0].fd);
          decrement_size(size);
          pthread_exit(NULL);
        }

        double time_taken = get_time_diff(&post_sleep, &receive_input) * 1000; // Seconds
        sleep_time *= 1e-3;
        if (time_taken <= (double) sleep_time) {
          strcpy(send_buf, "Too Early!\n");
        }
        else {
          snprintf(send_buf, MESSAGE_LENGTH - 1, "Reaction Time in Milliseconds: %.3lf\n", time_taken - sleep_time);
        }
        pthread_join(thread_id, NULL);
        int sendbytes = send(pfds[0].fd, send_buf, strlen(send_buf), 0);

        if (sendbytes == -1) {
          close(pfds[0].fd);
          perror("Thread: Sending");
          decrement_size(size);
          pthread_exit(NULL);
        }
      }
    }
  }

  return NULL;
}



void add_pthread(pthread_t * threads[], int * size, int * max_size, sock_size_t send) {
  if ((*size) == (*max_size)) {
    (*max_size) *= 2;
    pthread_t  * new_threads = (pthread_t *) malloc(*max_size * sizeof(pthread_t));
    for (int i = 0; i < *size; i++) {
      new_threads[i] = (*threads)[i];
    }
    free((*threads));
    (*threads) = new_threads;
  }

  pthread_create((*threads) + (*size), NULL, play_game, &send);
  if (pthread_detach(*((*threads) + (*size))) != 0) {
    fprintf(stderr, "Detach Thread");
  }
  pthread_mutex_lock(&mutex1);
  (*size)++;
  pthread_mutex_unlock(&mutex1);
}

int main() {
  srandom(time(NULL));
  struct timeval start;

  gettimeofday(&start, NULL);

  struct addrinfo hints, *result, *p;
  int listener_sockfd;

  struct sockaddr_storage theiraddr;
  socklen_t slen = sizeof(theiraddr);

  int thread_size = 0;
  int max_thread_size = 3;

  pthread_t * threads = malloc(max_thread_size * sizeof(pthread_t));

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

  char recv_buf[MESSAGE_LENGTH];

  int timeout_poll_counter = 0;

  while (1) {
    int num_events = poll(pfds, poll_size, 2500);
    if (num_events == -1) {
      perror("Poll");
      return -3;
    }
    else {
      if (pfds[0].revents & POLLIN) {
        if (pfds[0].fd == listener_sockfd) {
          int new_sock = accept(listener_sockfd, (struct sockaddr *) &theiraddr, &slen);
          if (new_sock == -1) {
            perror("Accept");
            return -4;
          }
          sock_size_t send = {new_sock, &thread_size};
          add_pthread(&threads, &thread_size, &max_thread_size, send);
        }
      }
    }

    if (thread_size == 0) {
      timeout_poll_counter++;
      if (timeout_poll_counter >= POLL_TIMEOUT_COUNT) {
        //Clean Up Memory Here
        close(pfds[0].fd);
        free(threads);
        free(pfds);
        return 0;
      }
    }
    else {
      timeout_poll_counter = 0;
    }
  }
}
