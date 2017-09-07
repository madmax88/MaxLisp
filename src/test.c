#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  char *foo = "bar";
  char buff[10];

  strcpy(buff, foo);
  printf("%s\n", buff);

  return 0;
}
