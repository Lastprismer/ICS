#include "fun.h"
#include <stdio.h>
int main() {
  int a;

  a = fun1(3);
  printf("%d\n", a);

  a = fun2(a);
  printf("%d\n", a);

  return 0;
}