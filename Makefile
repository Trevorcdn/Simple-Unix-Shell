  
sshell: sshell.c
	gcc -o sshell sshell.c -Wall -Werror 

clean:
	rm sshell
