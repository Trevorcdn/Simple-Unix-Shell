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
  int isPipeline;
  int pipeIn;
  int pipeOut;
};

struct pipe{ 
  int pipe[2];
};

struct CommandLine pipeLine[512];
void clearCl(struct CommandLine *cl, int maxSize);
void newCL(struct CommandLine *cl, int maxSize);
void parse(struct CommandLine* parseString, char* returnArray[]);
void removeElement(char * returnArray[], int startPos, int endPos);

int main(int argc, char *argv[])
{
  pid_t pid;
  pid_t pid2;
  int status;
  int maxSize = 512;
  int pipeCount = 0;
  bool noCommand = false;
  char *cmd_copy = NULL; //Save cmd into another variable because after calling parse, the address gets changed
  char *cmd_copy2 = NULL; //Also saving cmd, for another strtok use that will change it.
  char * pipeStr;
  struct CommandLine input;
  struct pipe pipeArray[512];
  newCL(&input, maxSize);
  cmd_copy = (char*)malloc(maxSize * sizeof(char));
  cmd_copy2 = (char*)malloc(maxSize * sizeof(char));

  while(1){
    fprintf(stdout, "sshell$ ");
    fflush(stdout);
    fgets(input.cmd, maxSize, stdin);

    if(!isatty(STDIN_FILENO)){
      printf("%s", input.cmd);
      fflush(stdout);
      }//if(!isatty(STDIN_FILENO))

    input.cmd[strlen(input.cmd) - 1] = '\0';//removes endline from fgets
    strcpy(cmd_copy,input.cmd);
    strcpy(cmd_copy2, input.cmd);
    pipeStr = strchr(cmd_copy2, '|');
    input.isPipeline = 0;
    if((strcmp(cmd_copy,">") == 0) || (strcmp(cmd_copy,"<") == 0)){
      noCommand = true;
      }//if(strcmp(cmd, ">"), (cmd, "<"))
    else if(pipeStr == NULL){
      parse(&input, input.shArgs);
      }//else if(!pipeStr)
    else{
      /*Have cases that deal with output redir on first argument
        and deals with input redir on last argument. This
        will probably be after the while loop  */
      pipeStr = strtok(cmd_copy2, "|");
      input.isPipeline = 1;
      while(pipeStr != NULL){
        newCL(&pipeLine[pipeCount], maxSize);
        pipeLine[pipeCount].cmd = pipeStr;
        pipeCount++;
        pipeStr = strtok(NULL, "|");
        }
     
        for(int k = 0; k < pipeCount; k++){
          if(k < pipeCount - 1){
             pipe(pipeArray[k]);
             pipeLine[k+1].pipeIn = pipeArray[k].pipe[0];
             pipeLine[k].pipeOut = pipeArray[k].pipe[1];
            }
          parse(&pipeLine[k],pipeLine[k].shArgs);
         }
      }
      
  
      

/*When user just hits enter without inputing anything*/

   if(strcmp(cmd_copy,"") == 0){
     continue;
     }//if(strcmp(cmd, ""))

/*buildin commands for exit*/

   if(strcmp(cmd_copy,"exit") == 0){
     fprintf(stderr, "Bye...\n");
     exit(0);
     }//if(strcmp(cmd, "exit"))

/*buildin commands for cd. Note: cd does not rely on
  fork+exec+wait and that's why I used an if-else statement to check
  if first argument is cd and if not then run external programs*/

    if(strcmp(input.shArgs[0],"cd") == 0){
      int chdirPass = chdir(input.shArgs[1]);
      if(chdirPass == 0){
//if chdir return 0, then it pass while -1 is an error
        fprintf(stderr, "+ completed '%s' [0]\n", cmd_copy);
        }//if(chdirPass == 0)
      else{
        fprintf(stderr, "Error: no such directory \n");
        fprintf(stderr, "+ completed '%s' [1]\n", cmd_copy);
        }//else (if chdirPass != 0)
      }//if(strcmp(input.shArgs[0],"cd"))
    else{
/*fork+exec+wait*/
      pid = fork();

/*Child*/
      if(pid == 0){
/*------------------------Pipe fork-exec+wait here----------------------------------------*/        
        if(input.isPipeline == 1){
            int a = 0;
            for(; a < pipeCount; a++){
              if( pid2 > 0){
                /*parent pid2*/
                pid2 = fork();
              }
              else if(pid2 == 0){
                /*child pid2*/
                break;
              }
              else{
                /* neither parent or child*/
                pid2 = fork();
              }
            }
        if(pid2 == 0){
          printf("a = %d,   and pipeLine.cmd = %s\n",a,pipeLine[a-1].shArgs[1]);
            dup2(STDIN_FILENO, pipeLine[a - 1].pipeIn);
            dup2(STDOUT_FILENO, pipeLine[a - 1].pipeOut);
                execvp(pipeLine[a].shArgs[0], pipeLine[a].shArgs);
                if((pipeLine[a].cmd[0] != '&') && (pipeLine[a].cmd[0] != '>') && (pipeLine[a].cmd[0] != '<') && (pipeLine[a].cmd[0] != '|')){
                  fprintf(stderr, "Error: command not found '%s'\n", pipeLine[a].shArgs[0]);
                  exit(1);
                    }
          }
        else if(pid2 > 0){
          /*Parent of pid2*/
           for(int i = 0; i <= a; i++){
             waitpid(-1, &status,0); //change -1 so that parent wait for all children instead of just the first one finish
            }
          }
            
        }
        else{
/*------------------------Non-pipe fork-exec+wait here----------------------------------------*/  
/*Input redirection*/
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
	  if((input.cmd[0] != '&') && (input.cmd[0] != '>') && (input.cmd[0] != '<') && (input.cmd[0] != '|')){
	    fprintf(stderr, "Error: command not found '%s'\n", input.shArgs[0]);
	    exit(1);
        }
            }
      }

/*Parent*/
     else if(pid > 0){
       /* if(str has &){
         while(waitpid(-1, &status, WNOHANG) > 0)
       }else{}*/
	   waitpid(-1, &status,0); //change -1 so that parent wait for all children instead of just the first one finish
	   if(WEXITSTATUS(status) == 5);

           else if((input.cmd[0] != '&') && (input.cmd[0] != '>') && (input.cmd[0] != '<') && (input.cmd[0] != '|') && noCommand == false){
	     fprintf(stderr, "+ completed '%s' [%d]\n", cmd_copy, WEXITSTATUS(status));
	     }
	   else
	     fprintf(stderr, "Error: invalid command line \n");
             }
           else{
	     perror("fork");
	     exit(1);
	     }
	   }
	}
	return EXIT_SUCCESS;
}


void newCL(struct CommandLine *cl, int maxSize){
  cl->cmd = (char*)malloc(maxSize * sizeof(char));
  for(int i = 0; i < 16; i++){
    cl->shArgs[i] = (char*)malloc(maxSize * sizeof(char));
    }
  cl->redirIn = 0;
  cl->redirInFile = (char*)malloc(maxSize * sizeof(char));
  cl->isPipeline = 0;
  cl->pipeIn = 0;
  cl->pipeOut = 1;
}

void clearCl(struct CommandLine *cl, int maxSize){
  cl->redirIn = 0;
  memset(cl->redirInFile, '\0', maxSize * sizeof(char));
  cl->redirO = 0;
  memset(cl->redirOFile, '\0', maxSize * sizeof(char));
}

void parse(struct CommandLine* parseString, char * returnArray[]){
  int counter = 0;
  char *redirInStr;
  char *redirOStr;
  char *tStr;
  parseString->redirIn = 0;
  parseString->redirInFile = 0;
  parseString->redirO = 0;
  parseString->redirOFile = 0;

  tStr = strtok(parseString->cmd, " ");
  while(tStr != NULL){
    returnArray[counter] = tStr;
    tStr = strtok(NULL," ");
    counter++;
  }
  counter--;
  returnArray[counter + 1] = NULL;
  for(int i = 0; i <= counter; i++){
    redirInStr = strchr(returnArray[i], '<');
    redirOStr = strchr(returnArray[i], '>');

    /*Parse the input strings for "<" to make sure that both
      "grep toto<file" and "grep toto < file" is possible */
    if(redirInStr != NULL){
      parseString->redirIn = 1;
      if(strcmp(returnArray[i], "<") == 0){
        parseString->redirInFile = returnArray[i + 1];
        removeElement(returnArray, i, counter);
        removeElement(returnArray, i, counter);
        counter = counter - 2;
      }
      else if(strcmp(redirInStr,"<") == 0){
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
    if(redirOStr != NULL){
      parseString->redirO = 1;
      if(strcmp(returnArray[i], ">") == 0)
        {
        parseString->redirOFile = returnArray[i + 1];
        removeElement(returnArray, i, counter);
        removeElement(returnArray, i, counter);
        counter = counter - 2;
        }
      else if(strcmp(redirOStr,">") == 0){
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
//  fprintf(stderr, "Return Array [%d] is : %s \n", i, returnArray[i]);
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