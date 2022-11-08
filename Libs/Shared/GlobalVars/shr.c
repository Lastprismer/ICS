#include "shr.h"
int global = 0;
int add() {
  printf("PID:%d, global:%d\n", getpid(), global);
  global++;
  printf("PID:%d, global:%d\n", getpid(), global);
  return global;
}