#include <stdio.h>
#include <time.h>
#include <unistd.h>

void alive(void)
{
   printf("I am alive \n");
   sleep(1);
}

time_t now;

int main(void)
{

  for (;;) {
    now = time(NULL);
    if ((now % 14) == 0)
       alive();
  }
}

