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
  int background;
};

void newCL(struct CommandLine *cl, int maxSize);
void parse(struct CommandLine* parseString, char* returnArray[]);
void removeElement(char * returnArray[], int startPos, int endPos);
void forkExec();

int main(int argc, char *argv[])
{
   forkExec();
   return 0;
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

void parse(struct CommandLine* parseString, char * returnArray[]){
  int counter = 0;
  char *redirInStr;
  char *redirOStr;
  char *tStr;
  parseString->redirIn = 0;
  parseString->redirInFile = 0;
  parseString->redirO = 0;
  parseString->redirOFile = 0;
  if(parseString->cmd[strlen(parseString->cmd) - 1] == '&'){
  parseString->cmd[strlen(parseString->cmd) - 1] = '\0';
  parseString->background = 1;
  };
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
}

void removeElement(char * returnArray[], int startPos, int endPos){
  for(; startPos < endPos; startPos++){
     returnArray[startPos] = returnArray[startPos + 1];
     }
  returnArray[endPos] = NULL;
}

void forkExec(){
   struct CommandLine pipeLine[512];
  pid_t pid;
  pid_t pid2 = 1;
  int fd[2];
  int status;
  int passedOnce = 2;
  int maxSize = 512;
  int completed = -1;
  int pipeCount = 0;
  bool noCommand = false;
  char *cmd_copy = NULL; //Save cmd into another variable because after calling parse, the address gets changed
  char *cmd_copy2 = NULL; //Also saving cmd, for another strtok use that will change it.
  char *storeBackground = NULL;
  char * pipeStr;
  char * pipeErrorO;
  struct CommandLine input;
  newCL(&input, maxSize);
  storeBackground = (char*)malloc(maxSize * sizeof(char));
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
    int pipeBrkOut = 0;
    int pipeBrkIn = 0;
    int exitArray[maxSize];
    if((strcmp(cmd_copy,">") == 0) || (strcmp(cmd_copy,"<") == 0) || (strcmp(cmd_copy,"&") == 0) || (strcmp(cmd_copy,"|") == 0)){
      noCommand = true;
      fprintf(stderr,"Error: invalid command line \n");
      continue;
      }//if(strcmp(cmd, ">"), (cmd, "<"))
    else if(pipeStr == NULL){
      parse(&input, input.shArgs);
      }//else if(!pipeStr)
    else{
      /*Have cases that deal with output redir on first argument
        and deals with input redir on last argument. This
        will probably be after the while loop  */
        for(int k = 1; k < pipeCount; k++){
        input.background = 1;
        }
      pipeStr = strtok(cmd_copy2, "|");
      input.isPipeline = 1;
      while(pipeStr != NULL){
        newCL(&pipeLine[pipeCount], maxSize);
        pipeLine[pipeCount].cmd = pipeStr;
        pipeCount++;
        pipeStr = strtok(NULL, "|");
        }

      /*error handling for output redirection on anywhere that is not the last pipe*/
      for(int j = 0; j < pipeCount-1; j++){
        pipeErrorO = strchr(pipeLine[j].cmd, '>');
        if(pipeErrorO != NULL){
          pipeBrkOut = 1;
          continue;
        }
      }

      /*error handling for Input redirection on anywhere that is not the first pipe*/
      for(int k = 1; k < pipeCount; k++){
        pipeErrorO = strchr(pipeLine[k].cmd, '<');
        if(pipeErrorO != NULL){
          pipeBrkIn = 1;
          continue;
        }
      }

        for(int l = 0; l < pipeCount; l++){
          if(l < pipeCount - 1){
             pipe(fd);
             pipeLine[l+1].pipeIn = fd[0];
             pipeLine[l].pipeOut = fd[1];
            }
          parse(&pipeLine[l],pipeLine[l].shArgs);
         }
      }

if(pipeBrkOut == 1){
  fprintf(stderr, "Error: mislocated output redirection\n");
  continue;
  }

if(pipeBrkIn == 1){
  fprintf(stderr, "Error: mislocated input redirection\n");
  continue;
  }

if(strchr(cmd_copy, '&') != NULL && cmd_copy[strlen(cmd_copy) - 1] != '&'){
  fprintf(stderr, "Error: mislocated background sign\n");
  continue;
  }
/*When user just hits enter without inputing anything*/

   if(strcmp(cmd_copy,"") == 0){
     continue;
     }//if(strcmp(cmd, ""))

/*buildin commands for exit*/

   if(strcmp(cmd_copy,"exit") == 0){
     if(input.background != 1){
       fprintf(stderr, "Bye...\n");
       exit(0);
       }
     else{
       fprintf(stderr, "Error: active jobs still running\n");
       fprintf(stderr, "+ completed 'exit' [1]\n");
       continue;
       }
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
              if(pid2 > 0){
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
          //printf("a = %d, and pipeLine.cmd = %s\n",a,pipeLine[a-1].shArgs[1]);
            /*Anything with grep is breaking it*/
             dup2(pipeLine[a - 1].pipeIn, STDIN_FILENO);
             if(a < pipeCount)
                   dup2(pipeLine[a - 1].pipeOut, STDOUT_FILENO);

            /*input redirection for PIPING*/
            if(input.redirIn == 1){
                int inFileDescPipe = open(pipeLine[a - 1].redirInFile, O_RDONLY);
                if((inFileDescPipe < 0) && (strcmp(pipeLine[a - 1].shArgs[0], "grep") == 0)){
                  fprintf(stderr, "Error: cannot open input file\n");
                  exit(5);
                  }
                else if((inFileDescPipe < 0) && (strcmp(pipeLine[a - 1].shArgs[0], "cat") == 0)){
                  fprintf(stderr, "Error: no input file\n");
                  exit(5);
                  }

                }

           /*Output redirection for PIPING*/
            if(pipeLine[a - 1].redirO == 1){
              /*the 0666 is to give read and write permission for the O_CREAT*/
              int oFileDescPipe = open(pipeLine[a - 1].redirOFile,  O_CREAT | O_RDWR, 0666);
              if((oFileDescPipe < 0) && (pipeLine[a - 1].shArgs[1] == NULL)){ //echo >
                fprintf(stderr, "Error: no ouput file");
                exit(5);
                }
              if((oFileDescPipe < 0) && (strcmp(pipeLine[a - 1].shArgs[0], "echo") == 0)){ //echo hack > ???
                fprintf(stderr, "Error: cannot open output file");
                exit(5);
                }
              else{
                dup2(oFileDescPipe, STDOUT_FILENO);
                close(oFileDescPipe);
                }
            }
               execvp(pipeLine[a-1].shArgs[0], pipeLine[a-1].shArgs);

                if((pipeLine[a].cmd[0] != '&') && (pipeLine[a].cmd[0] != '>') && (pipeLine[a].cmd[0] != '<') && (pipeLine[a].cmd[0] != '|')){
                  fprintf(stderr, "Error: command not found '%s'\n", pipeLine[a].shArgs[0]);
                  exit(1);
                    }
          }
        else if(pid2 > 0){
          /*Parent of pid2*/
          if(input.background == 0){
           for(int i = 0; i <= a; i++){
             waitpid(-1, &status,0); //change -1 so that parent wait for all children instead of just the first one finish
             if(i == 1 ){
                fprintf(stdout, "+ completed '%s'", cmd_copy);
             }
             if(WEXITSTATUS(status) == 0)
               exitArray[i] = 0;
             else
               exitArray[i+1] = i+1;
             if(i >= 1){
               printf("[%d]", exitArray[i]);
               }
             }
             printf("\n");
             exit(5);
           }
          else{
            waitpid(-1, &status, WNOHANG);
          }

         }
        else{
	          perror("fork");
	          exit(1);
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
       if(input.background == 0){
         waitpid(-1, &status,0); //change -1 so that parent wait for all children instead of just the first one finish
	        if(WEXITSTATUS(status) == 5);
          else if((input.cmd[0] != '&') && (input.cmd[0] != '>') && (input.cmd[0] != '<') && (input.cmd[0] != '|') && noCommand == false){
	          fprintf(stderr, "+ completed '%s' [%d]\n", cmd_copy, WEXITSTATUS(status));
	          }
	        else{
	          fprintf(stderr, "Error: invalid command line \n");
            }
       }else{
       if(input.background == 1){
         completed = waitpid(-1, &status, WNOHANG);
         if(passedOnce == 2){
           passedOnce = 0;
         }
         if(strcmp(storeBackground,"\0") == 0){
           strcpy(storeBackground,cmd_copy);
         }
       }
       if(input.background == 1 && completed > 0){
           input.background = 0;
           completed = -1;
           passedOnce = 2;
           fprintf(stdout, "+ completed '%s' [%d]\n", storeBackground, WEXITSTATUS(status));
           storeBackground = "\0";
           waitpid(-1, &status,0); //change -1 so that parent wait for all children instead of just the first one finish
             if(WEXITSTATUS(status) == 5);
             else if((input.cmd[0] != '&') && (input.cmd[0] != '>') && (input.cmd[0] != '<') && (input.cmd[0] != '|') && noCommand == false){
                fprintf(stderr, "+ completed '%s' [%d]\n", cmd_copy, WEXITSTATUS(status));
                }
             else{
               fprintf(stderr, "Error: invalid command line \n");
               }
        }
        else if(passedOnce == 1){
	        waitpid(-1, &status,0); //change -1 so that parent wait for all children instead of just the first one finish
	        if(WEXITSTATUS(status) == 5);
          else if((input.cmd[0] != '&') && (input.cmd[0] != '>') && (input.cmd[0] != '<') && (input.cmd[0] != '|') && noCommand == false){
	          fprintf(stderr, "+ completed '%s' [%d]\n", cmd_copy, WEXITSTATUS(status));
	          }
	        else{
	          fprintf(stderr, "Error: invalid command line \n");
            }
          }
        passedOnce = 1;
        }
     }
        else{
	        perror("fork");
	        exit(1);
	        }
	      }
      }
}
