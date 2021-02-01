#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define die(e) do { fprintf(stderr, "%s\n", e); exit(EXIT_FAILURE); } while (0);

int main() {
	fork();
	execvp("st", NULL);
}
