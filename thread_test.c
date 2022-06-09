#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

void * myThreadFun() {
  char buf[256];
  scanf("%255s", buf);
  return NULL;
}

int main() {
  pthread_t thread_id;
  printf("Before Thread\n");
  pthread_create(&thread_id, NULL, myThreadFun, NULL);
  for (int i = 1; i <= 100; i++) {
    printf("%d\n", i);
  }
  pthread_join(thread_id, NULL);
  printf("After Thread\n");
  exit(0);
}

