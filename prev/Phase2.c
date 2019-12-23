#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>

int main(int argc, char *argv[])
{
	pid_t pid;
	int status;
	int maxSize = 512;
	char *cmd = NULL;
	cmd = (char*)malloc(maxSize * sizeof(char)); //allocate memory for cmd. Otherwise, SegFaults
	
	while(1){
		fprintf(stdout, "sshell$ ");
		fgets(cmd, maxSize, stdin);
		cmd[strlen(cmd) - 1] = '\0';//removes endline from fgets
		
		pid = fork();
		if (pid == 0) {
			/* Child */
			execvp(&cmd[0], argv);
			if(cmd[0] != '&')
				fprintf(stderr, "Error: command not found \n");
			exit(1);
		} else if(pid > 0){
			/* Parent */
			waitpid(-1, &status,0);
			if(cmd[0] == '&')
				fprintf(stderr, "Error: invalid command line \n");
			else 
				fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(status));
				
		} else {
			perror("fork");
			exit(1);
		}
	
	}
	

	return EXIT_SUCCESS;
}