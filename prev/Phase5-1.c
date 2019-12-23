#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>

struct CommandLine{
  char *cmd;
  char *shArgs[16];
  int redirIn;
  char *redirInFile;
  int redirO;
  char *redirOFile;
};

void clearCl(struct CommandLine *cl, int maxSize);
void newCL(struct CommandLine *cl, int maxSize);
void parse(struct CommandLine* parseString, char* returnArray[]);

int main(int argc, char *argv[])
{
	pid_t pid;
	int status;
	int maxSize = 512;
	char *cmd_copy = NULL; //Save cmd into another variable because after calling parse, the address gets changed
	struct CommandLine input;
        newCL(&input, maxSize);
	cmd_copy = (char*)malloc(maxSize * sizeof(char)); 
  
  	while(1){
		fprintf(stdout, "sshell$ ");
		fgets(input.cmd, maxSize, stdin);
		input.cmd[strlen(input.cmd) - 1] = '\0';//removes endline from fgets
		strcpy(cmd_copy,input.cmd);
                parse(&input, input.shArgs);
                
		/*buildin commands for exit*/
    		if(strcmp(cmd_copy,"exit") == 0){
    		fprintf(stderr, "Bye...\n");
    		exit(0);
    		}

		/*buildin commands for cd. Note: cd does not rely on 
		fork+exec+wait and that's why I used an if-else statement to check 
		if first argument is cd and if not then run external programs*/
		if(strcmp(input.shArgs[0],"cd") == 0){
		  int chdirPass = chdir(input.shArgs[1]);
		  if(chdirPass == 0){ //if chdir return 0, then it pass while -1 is an error
		    fprintf(stderr, "+ completed '%s' [0]\n", cmd_copy);
 		  }else{
		    fprintf(stderr, "Error: no such directory \n");
		    fprintf(stderr, "+ completed '%s' [1]\n", cmd_copy);
		    }
		}else{
		/*fork+exec+wait*/
		pid = fork();
		if (pid == 0) {
			/* Child */
			execvp(input.shArgs[0], input.shArgs);
			if((input.cmd[0] != '&') && (input.cmd[0] != '>') && (input.cmd[0] != '<') && (input.cmd[0] != '|'))
				fprintf(stderr, "Error: command not found '%s'\n", input.shArgs[0]);
			exit(1);
		} else if(pid > 0){
			/* Parent */
			waitpid(-1, &status,0);
			if((input.cmd[0] != '&') && (input.cmd[0] != '>') && (input.cmd[0] != '<') && (input.cmd[0] != '|'))
				fprintf(stderr, "+ completed '%s' [%d]\n", cmd_copy, WEXITSTATUS(status));
			else
				fprintf(stderr, "Error: invalid command line \n");
		} else {
			perror("fork");
			exit(1);
		}
	   }
	
	}
	

	return EXIT_SUCCESS;
}


void newCL(struct CommandLine *cl, int maxSize){
  cl->cmd = (char*)malloc(maxSize * sizeof(char));
  for(int i = 0; i < 16; i++)
    {
    cl->shArgs[i] = (char*)malloc(maxSize * sizeof(char));
    }
  cl->redirIn = 0;
  cl->redirInFile = (char*)malloc(maxSize * sizeof(char));
}

void clearCl(struct CommandLine *cl, int maxSize){
  memset(cl->cmd, '\0', maxSize * sizeof(char));
  for(int i = 0; i < 16; i++)
    {
    memset(cl->shArgs[i], '\0', maxSize * sizeof(char));
    }
  cl->redirIn = 0;
  memset(cl->redirInFile, '\0', maxSize * sizeof(char));
  cl->redirO = 0;
  memset(cl->redirOFile, '\0', maxSize * sizeof(char));
}

void parse(struct CommandLine* parseString, char * returnArray[]){
  int counter = 0;
  char *tStr; 
  tStr = strtok(parseString->cmd, " ");
  while(tStr != NULL){ 
    if(strcmp(tStr, "<") == 0){
      parseString->redirIn = 1;
      tStr = strtok(NULL, " ");
      parseString->redirInFile = tStr;
      }
    else if(strcmp(tStr, ">") == 0){
      parseString->redirO = 1;
      tStr = strtok(NULL, ">");
      parseString->redirOFile = tStr; // returns the word after '>'
      }
    else{
      returnArray[counter] = tStr;
      tStr = strtok(NULL," ");
      counter++;
    }
  }
  counter--;
  returnArray[counter + 1] = NULL;
}
