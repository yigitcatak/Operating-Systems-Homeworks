#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
	int rc, fd[2];
	printf("I'm SHELL process, with PID: %d - Main command is: man grep | grep -A4 -e-A -m1 > output.txt\n", getpid());
	if ((pipe(fd) == -1) || ((rc = fork()) < 0)) {
		fprintf(stderr, "Failed\n");
		exit(1);
    }   
	if (rc == 0) {
		printf("I'm MAN process, with PID: %d - My command is: man grep\n", getpid());
		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO); // direct MAN process output to pipe
		char *myargs[] = {"man", "grep", NULL}; 
		execvp(myargs[0], myargs);
	} else {
		waitpid(rc,NULL,0); // wait MAN process to exit, do not store the exit code (NULL)
		if ((rc = fork()) < 0) {
			fprintf(stderr, "Failed\n");
			exit(1);
		}
		if (rc == 0) {
			printf("I'm GREP process, with PID: %d - My command is: grep -A4 -e-A -m1 > output.txt\n", getpid());
			close(fd[1]); // close the writing end
			dup2(fd[0], STDIN_FILENO); // read the pipe
			close(STDOUT_FILENO);; // close the console output
			open("./output.txt", O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU); // open the text output in the place of the closed console output
			char *myargs[] = {"grep", "-A4", "-e-A", "-m1", NULL}; 
			execvp(myargs[0], myargs);
		} else {
			waitpid(rc,NULL,0);	// wait GREP process to exit
			printf("I'm SHELL process, with PID: %d - execution is completed, you can find the results in output.txt\n", getpid());
		}
	}
	return 0;
}