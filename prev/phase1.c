#include<stdio.h>
#include <stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>

int main(int argc, char *argv[])
{
	pid_t pid;
	int status;
	char *cmd[3] = { "date", "-u", NULL, };

	pid = fork();
	if (pid == 0) {
		/* Child */
		
		//print out date here
		printf("This is cmd[1]: %s \n", cmd[1]);
		execvp(cmd[0], cmd);
		perror("execvp");
		exit(1);
	} else if(pid > 0){
		/* Parent */
		waitpid(-1, &status,0);
		fprintf(stderr, "+ completed '%s' [%d]\n", cmd[0], WEXITSTATUS(status));
	} else {
		perror("fork");
		exit(1);
	}
	
	
	

	return EXIT_SUCCESS;
}