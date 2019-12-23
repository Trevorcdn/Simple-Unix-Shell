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
	char *cmd_copy = NULL; //Save cmd into another variable because after calling parse, the address gets changed
	char *shArgs[16];
	cmd = (char*)malloc(maxSize * sizeof(char)); //allocate memory for cmd. Otherwise, SegFaults
	cmd_copy = (char*)malloc(maxSize * sizeof(char));
    for(int i = 0; i < 16; i++){
       shArgs[i] = (char*)malloc(maxSize * sizeof(char));
	} 
	
	while(1){
		fprintf(stdout, "sshell$ ");
		fgets(cmd, maxSize, stdin);
		cmd[strlen(cmd) - 1] = '\0';//removes endline from fgets
		strcpy(cmd_copy,cmd);
        parse(cmd, shArgs);
        
		pid = fork();
		if (pid == 0) {
			/* Child */
			execvp(shArgs[0], shArgs);
			if((cmd[0] != '&') && (cmd[0] != '>') && (cmd[0] != '<') && (cmd[0] != '|'))
				fprintf(stderr, "Error: command not found '%s'\n", cmd);
			exit(1);
		} else if(pid > 0){
			/* Parent */
			waitpid(-1, &status,0);
			if((cmd[0] != '&') && (cmd[0] != '>') && (cmd[0] != '<') && (cmd[0] != '|'))
				fprintf(stderr, "+ completed '%s' [%d]\n", cmd_copy, WEXITSTATUS(status));
			else
				fprintf(stderr, "Error: invalid command line \n");
		} else {
			perror("fork");
			exit(1);
		}
	
	}
	

	return EXIT_SUCCESS;
}

void parse(char * parseString, char * returnArray[]){
  int strIndx = 0;
  int counter = 0;
  char *tStr;
  
  tStr = strtok(parseString, " ");
  while(parseString[strIndx] != '\0'){ 
    returnArray[counter] = tStr;
    tStr = strtok(NULL," ");
    strIndx = strIndx + 1;
    counter++;
  }
  returnArray[counter + 1] = NULL;
  
}
