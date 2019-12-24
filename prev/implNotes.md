## Arguments in Struct ##
For each command the user inputs, have a struct object that holds it.
The struct contains several variables:
1. char* cmd
..* Holds the original, unparsed command.
2. char* shArgs[16]
..* Holds the command, separated into arguments for a execvp call.
3. char * redirInFile/redirOFile
..* If input or output redirection occurs, this holds the I/O file.
4. int isPipeline
..* If a pipe is present, this notifies the child node to act accordingly.
5. int pipeIn/pipeOut
..* For each command in a pipeline, holds the input and output pipelines.
..* newCL (our 'constructor') sets these values to be 0 and 1 initially.

## Parsing ##
First parse the entire command line to skip white space and store it into
returnArray[]. For example, if the user enters "echo hello world!", then the
return arguments would be returnArray[0] = echo, returnArray[1] = hello, and
returnArray[2] = world!. Return the array which will execute correctly
when execvp(input.shArgs[0], input.shArgs) is called.

## Pipe Design ##
Struct is created with the original cmd stored. Immediately after this, 
check for a pipeline (checking for a pipeline first makes it easier to parse
individual pieces of a pipeline). If a pipeline is found, set the original
struct's isPipeline value to be 1 (true). Then split the command up by
strtok'ing the '|' command, and pass the returned values in as cmd's for structs
in a 512 sized array, which stores the components of our pipeline in order.
Then, each of the elements in the array that have had their cmd value set are
parsed, so that each one would function just like a normal command passed to
our shell.Then, if the number of commands is k,   create k - 1 pipes, and
assign the pipeIn and pipeOut values of our structs accordingly. Then, if  
find that isPipeline is 1 in our child process,   would execute a for loop
that loops through the number of items in the pipeline array. For each one,
it creates a child process, and the child process then exits the loop. As the
child process exits the loop after it is created,   can use the loop's iterator
to track the value of each of our child processes. Then, each of the child
processes use dup2 to replace their stdin and stdout with pipeIn and pipeOut. In
the cases of the first and last pieces of the pipeline, pipeIn and pipeOut
should hold their original values of STDIN_FILENO and STDOUT_FILENO, making us
not need to check for these edge cases.   currently have sequential piping
only, as time constraints did not allow us to find a more optimal method.
Ho ver, my best guess is that it would utilize a buffer, where after a certain
amount of input had been passed to the pipe, or a certain amount of time had
elapsed, the next item in the pipe would be executed.