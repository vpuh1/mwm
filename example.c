#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define die(e) do { fprintf(stderr, "%s\n", e); exit(EXIT_FAILURE); } while (0);

int main() {
	int link[2];
	pid_t pid;
	if(pipe(link) == -1)
		die("pipe");
	if((pid = fork()) == -1)
		die("fork");
	if(pid == 0) {
		dup2(link[1], STDOUT_FILENO);
		close(link[0]);
		close(link[1]);
		execl("/usr/bin/which", "which", "st", NULL);
		die("execl");
	} 
	close(link[1]);
	char output[4096];
	int nbytes = read(link[0], output, sizeof(output));
	int cnt = 0;
	for(int i = 0; i < 4096; i++){
		if(output[i] != '\0'){
			cnt++;
		}
		else break;
	}
	char result[cnt-1];
	for(int i = 0; i < cnt-1; i++){
		result[i] = output[i];
	}
	execl("/usr/local/st", "usr/local/st", NULL);
}
