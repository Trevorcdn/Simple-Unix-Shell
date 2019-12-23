#include<stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>
#include <fcntl.h>

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
void removeElement(char * returnArray[], int startPos, int endPos);

int main(int argc, char *argv[])
{
	pid_t pid;
	int status;
	int maxSize = 512;
	bool noCommand = false;
	char *cmd_copy = NULL; //Save cmd into another variable because after calling parse, the address gets changed
	struct CommandLine input;
        newCL(&input, maxSize);
	cmd_copy = (char*)malloc(maxSize * sizeof(char)); 
  
  	while(1){
		fprintf(stdout, "sshell$ ");
		fflush(stdout);
		fgets(input.cmd, maxSize, stdin);
		if (!isatty(STDIN_FILENO)) {
                   printf("%s", input.cmd);
                   fflush(stdout);
                }
                
		input.cmd[strlen(input.cmd) - 1] = '\0';//removes endline from fgets
		strcpy(cmd_copy,input.cmd);
		
		if((strcmp(cmd_copy,">") == 0) || (strcmp(cmd_copy,"<") == 0))
		   noCommand = true;
		  else
                parse(&input, input.shArgs);
      
      /*When user just hits enter without inputing anything*/          
      if(strcmp(cmd_copy,"") == 0){
                continue;
                }           
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
			                  /*Input Redirection*/
                        if(input.redirIn == 1){
                          int inFileDesc = open(input.redirInFile, O_RDONLY);
                          if((inFileDesc < 0) && (strcmp(input.shArgs[0], "grep") == 0)){
                            fprintf(stderr, "Error: cannot open input file\n");
                            exit(5);
                            }
                          else if((inFileDesc < 0) && (strcmp(input.shArgs[0], "cat") == 0)){
                            fprintf(stderr, "Error: no input file\n");
                            exit(5);
                            }
                          else{
                            dup2(inFileDesc, STDIN_FILENO);
                            }
                        }
                        
                        /*Output redirection*/
                         if(input.redirO == 1){
                          /*the 0666 is to give read and write permission for the O_CREAT*/
                          int oFileDesc = open(input.redirOFile,  O_CREAT | O_RDWR, 0666);
                          if((oFileDesc < 0) && (input.shArgs[1] == NULL)){ //echo >
                            fprintf(stderr, "Error: no ouput file\n");
                            exit(5);
                            }
                          if((oFileDesc < 0) && (strcmp(input.shArgs[0], "echo") == 0)){ //echo hack > ???
                            fprintf(stderr, "Error: cannot open output file\n");
                            exit(5);
                            }
                          else{
                            dup2(oFileDesc, STDOUT_FILENO);
                            close(oFileDesc);
                            }
                        }
                        
			execvp(input.shArgs[0], input.shArgs);
			if((input.cmd[0] != '&') && (input.cmd[0] != '>') && (input.cmd[0] != '<') && (input.cmd[0] != '|'))
				fprintf(stderr, "Error: command not found '%s'\n", input.shArgs[0]);
			exit(1);
		} else if(pid > 0){
			/* Parent */
			waitpid(-1, &status,0);
			if(WEXITSTATUS(status) == 5)
			  continue;
			else if((input.cmd[0] != '&') && (input.cmd[0] != '>') && (input.cmd[0] != '<') && (input.cmd[0] != '|') && noCommand == false){
				fprintf(stderr, "+ completed '%s' [%d]\n", cmd_copy, WEXITSTATUS(status));
			}
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
  cl->redirIn = 0;
  memset(cl->redirInFile, '\0', maxSize * sizeof(char));
  cl->redirO = 0;
  memset(cl->redirOFile, '\0', maxSize * sizeof(char));
}

void parse(struct CommandLine* parseString, char * returnArray[]){
  int counter = 0;
  char *tStr; 
  char *redirInStr;
  char *redirOStr;
  parseString->redirIn = 0;
  parseString->redirInFile = 0;
  parseString->redirO = 0;
  parseString->redirOFile = 0;
  
  tStr = strtok(parseString->cmd, " ");
  while(tStr != NULL){ 
    if(strcmp(tStr, "<") == 0){
      parseString->redirIn = 1;
      tStr = strtok(NULL, " ");
      parseString->redirInFile = tStr;
      tStr = strtok(NULL, " ");
      }
    else if(strcmp(tStr, ">") == 0){
      parseString->redirO = 1;
      tStr = strtok(NULL, " ");
      parseString->redirOFile = tStr;
      tStr = strtok(NULL, " ");
      }
    else{
      returnArray[counter] = tStr;
      tStr = strtok(NULL," ");
      counter++;
    }
  }
  counter--;
  returnArray[counter + 1] = NULL;
  for(int i = 0; i <= counter; i++){
    redirInStr = strchr(returnArray[i], '<');
    redirOStr = strchr(returnArray[i], '>');
    
    /*Parse the input strings for "<" to make sure that both
      "grep toto<file" and "grep toto < file" is possible */
    if(redirInStr != NULL)
      {
      parseString->redirIn = 1;
      if(strcmp(redirInStr,"<") == 0)
        {
        returnArray[i] = strtok(returnArray[i], "<");
        parseString->redirInFile = returnArray[i + 1];
        removeElement(returnArray, i + 1, counter);
        counter--;
        }
      else if(returnArray[i][0] == '<'){
        memmove(redirInStr, redirInStr+1, strlen(redirInStr));
        parseString->redirInFile = redirInStr;   
        removeElement(returnArray, i, counter);
        counter--; 
        }
      else{
        returnArray[i] = strtok(returnArray[i], "<");
        parseString->redirInFile = strtok(NULL, "\0");
        }
      }
    /*Parse the input strings for ">". Literally, the same code above
      except adapted to work for "echo Hello World!>file"*/  
    if(redirOStr != NULL)
      {
      parseString->redirO = 1;
      if(strcmp(redirOStr,">") == 0)
        {
        returnArray[i] = strtok(returnArray[i], ">");
        parseString->redirOFile = returnArray[i + 1];
        removeElement(returnArray, i + 1, counter);
        counter--;
        }
      else if(returnArray[i][0] == '>'){
        memmove(redirOStr, redirOStr+1, strlen(redirOStr));
        parseString->redirOFile = redirOStr;   
        removeElement(returnArray, i, counter);
        counter--; 
        }
      else{
        returnArray[i] = strtok(returnArray[i], ">");
        parseString->redirOFile = strtok(NULL, "\0");
        }
      }
   }
/*delete later V V V */   
for(int i = 0; i <= counter; i++){
  // fprintf(stderr, "Return Array [%d] is : %s \n", i, returnArray[i]); 
  }    
// fprintf(stderr, "%i \n", parseString->redirO);
// fprintf(stderr, "%s \n", parseString->redirOFile);
}

void removeElement(char * returnArray[], int startPos, int endPos){
  for(; startPos < endPos; startPos++){
     returnArray[startPos] = returnArray[startPos + 1];
     }
  returnArray[endPos] = NULL;
}