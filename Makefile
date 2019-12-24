  
sshell: sshell.c
	gcc -std=c99 -o sshell sshell.c -Wall -Werror 

clean:
	rm sshell
