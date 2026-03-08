#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "vtsh.h"

#define MAX_INPUT 1024

int main () {
  (void)setvbuf(stdin, NULL, _IONBF, 0);

  int interactive = isatty(fileno(stdin));

  char input[MAX_INPUT];

  while (1) {
    if (!fgets(input, MAX_INPUT, stdin)) {
      break;
    }
  
    input[strcspn(input, "\n")] = 0;

    if (strcmp(input, "exit") == 0) {
      break;
    }
    
    prepare_commands(input, interactive);
  }
}
