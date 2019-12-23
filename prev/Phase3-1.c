#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>

void parse(char * parseString, char* returnArray[]);
int main(int argc, char *argv[])
{
	pid_t pid;
	int status;
	int maxSize = 512;
	char *cmd = NULL;
	cmd = (char*)malloc(maxSize * sizeof(char)); //allocate memory for cmd. Otherwise, SegFaults
        char *shArgs[16];
        for(int i = 0; i < 16; i++){
          shArgs[i] = (char*)malloc(maxSize * sizeof(char));
	} 
	while(1){
		fprintf(stdout, "sshell$ ");
		fgets(cmd, maxSize, stdin);
		cmd[strlen(cmd) - 1] = '\0';//removes endline from fgets
                parse(cmd, shArgs);
		pid = fork();
		if (pid == 0) {
			/* Child */
			execvp(shArgs[0], argv);
			if(cmd[0] != '&')
				fprintf(stderr, "Error: command not found '%s'\n", cmd);
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

void parse(char * parseString, char * returnArray[]){
  int strIndx = 0;
  char tStr[512] = "";
  char tChr[2] = "";
  while(parseString[strIndx] != ' ' && parseString[strIndx] != '\0'){
    tChr[0] = parseString[strIndx];
    tChr[1] = 0;
    strcat(tStr, tChr);
    strIndx = strIndx + 1;
  }
  returnArray[0] = tStr;
}