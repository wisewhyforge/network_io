#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

void main() {
  struct timeval start, end;

  gettimeofday(&start, NULL);

  for (int i = 0; i < 1000; i++) {
    printf("%d\n", i);
  }

  gettimeofday(&end, NULL);

  double time_taken;

  time_taken = (end.tv_sec - start.tv_sec) * 1e6;
  time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;

  printf("Time: %lf\n", time_taken);
}
