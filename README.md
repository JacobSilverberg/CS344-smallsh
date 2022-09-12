# CS344-smallsh

smallsh is a shell created in C.  It implements a subset of features of well known shells as listed below.

To Compile and Run, enter the following commands in your terminal:

gcc --std=gnu99 -o smallsh main.c

./smallsh

Features:

Provide a prompt for running commands

Handle blank lines and comments, which are lines beginning with the # character

Provide expansion for the variable $$ into the process ID of the smallsh

Execute 3 commands exit, cd, and status via code built into the shell

Execute other commands by creating new processes using a function from the exec family of functions

Support input and output redirection

Support running commands in foreground and background processes

Implement custom handlers for 2 signals, SIGINT and SIGTSTP
