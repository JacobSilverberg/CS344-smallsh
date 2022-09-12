// Created by: Jacob Silverberg - silverbj@oregonstate.edu
//
// Citation for selected functions:
// 1/28/2022
//
// Initial Layout Inspiration and clearScreen()
// https://www.youtube.com/watch?v=z4LEuxMGGs8
//
// Switch flow in allOtherCommands()
// Module 4: Monitoring Child Processes
//
// SIGINT and SIGTSTP signal handling, handle_SIGTSTP()
// Module 5: Signal Handling API
//
// int to string conversion and concatenation
// silverbj's Assignment 2
//
// strtok tokenization in getInput()
// silverbj's Assignment 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

// global variables for max_input, max_args, loop_controller and both background toggles
const int max_input = 2048;
const int max_args = 512;
int loop_controller = 1;
int background_bool = 0;
int background_signal = 1;


/* Receives user input and parses input into correct variable locations  */

void getInput(char *input_array[], char input_file[], char output_file[], int pid)
{
	// initialize variables
	char *colon = ": ";
	char input[max_input];
	char *new_line = "\n";
	char *null_term = "\0";
	int i = 0;
	int j = 0;
	char *space = " ";
	char *token;
	char *dollar = "$";
	char *input_symbol = "<";
	char *output_symbol = ">";
	char *ampersand = "&";
	char *string_storage;
	char *string_storage_2;

	// flust stdout every loop
	fflush(stdout);

	// print colon and receive user input with fgets
	printf(colon);
	fgets(input, max_input, stdin);

	// replace new line with null terminator
	while (input[i] != *new_line)
	{
		i++;
	}
	input[i] = *null_term;
	i = 0;

	// tokenize input for usage in comparisons
	token = strtok(input, space);

	// loop to examine input for args, files and background flag
	for (i = 0; token; i++)
	{
		// check for input file symbol <
		if (strcmp(token, input_symbol) == 0)
		{
			// if found, advance to next token and copy string to input_file
			token = strtok(NULL, space);
			strcpy(input_file, token);
		}

		// check for output file symbol >
		else if (strcmp(token, output_symbol) == 0)
		{
			// if found, advance to next token and copy string to output_file
			token = strtok(NULL, space);
			strcpy(output_file, token);
		}

		// check for background process symbol &
		else if (strcmp(token, ampersand) == 0)
		{
			// if found, flip background_bool to 1
			background_bool = 1;
		}

		// if none of above is true, token is an argument so duplicate token into input_array
		else 
		{
			input_array[i] = strdup(token);

			// check for $$ expansion
			while(input_array[i][j+1] != *null_term)
			{
				if (input_array[i][j] == *dollar && input_array[i][j + 1] == *dollar)
				{
					// replace $$ in arg with pid
					string_storage = (char*)calloc(1, max_args);
					strncpy(string_storage, input_array[i], j);
					input_array[i][j] = *null_term;
					int length = snprintf( NULL, 0, "%d", pid );
					char* str = malloc( length + 1);
					snprintf( str, length + 1, "%d", pid);
					strcat(string_storage, str);
					snprintf(input_array[i], max_args, "%s", string_storage);
				
				}
				j++;
			}
		}
		// reset variable j
		j = 0;

		// advance token to next part of input
		token = strtok(NULL, space);
	}
}

/* Handles execution of other commands that are not built in */

void allOtherCommands(char *input_array[], char input_file[], char output_file[], int pid, int exit_status, struct sigaction SIGINT_action)
{
	// initialize variables
	pid_t spawnpid = -5;
	char *null_term = "\0";
	int input;
	int output;
	int result;
	pid_t new_pid;

	// fork and set spawnpid, establish switch
	spawnpid = fork();

	switch(spawnpid)
	{
		// error case
		case -1:
			perror("Failure in creating fork.\n");
			exit(1);
			break;

		// child execution
		case 0:
			// re-establish ctrl+c SIGINT signal
			SIGINT_action.sa_handler = SIG_DFL;
			sigaction(SIGINT, &SIGINT_action, NULL);

			// if input redirect exists
			if(strcmp(input_file, null_term) != 0)
			{
				// open file
				input = open(input_file, O_RDONLY);
				if(input == -1)
				{
					perror("Error opening input file.\n");
					exit(1);
				}
				// redirect file
				result = dup2(input, 0);
				if(result == -1)
				{
					perror("Error redirecting input file.\n");
					exit(1);
				}
				// close file
				fcntl(input, F_SETFD, FD_CLOEXEC);
			}

			// if output redirect exists
			if(strcmp(output_file, null_term) != 0)
			{
				// open or create file
				output = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
				if(output == -1)
				{
					perror("Error opening output file.\n");
					exit(1);
				}
				// redirect file
				result = dup2(output, 1);
				if(result == -1)
				{
					perror("Error redirecting output file.\n");
					exit(1);
				}
				// close file
				fcntl(output, F_SETFD, FD_CLOEXEC);
			}

			// execute command
			if(execvp(input_array[0], (char* const*)input_array) == 1)
			{
				printf("Cannot perform %s.\n", input_array[0]);
				fflush(stdout);
				exit(1);
			}

			// only returns if there is an error
			perror("Error with child. Now Terminating.");
			// exit_status = 1;
			exit(1);
			break;

		// parent execution, check for background command
		default:
			// if background command and SIGTSTP both 1
			if (background_bool == 1 && background_signal == 1)
			{
				new_pid = waitpid(spawnpid, &exit_status, WNOHANG);
			}
			else
			{
				new_pid = waitpid(spawnpid, &exit_status, 0);
			}

			fflush(stdout);
			break;
	}

	// print background pid and exit status
	while ((spawnpid = waitpid(-1, &exit_status, WNOHANG)) > 0)
	{
		printf("Background process terminated. PID: %d Exit Status: %d\n", spawnpid, exit_status);
	}

	fflush(stdout);
	return;
}

void printExitStatus(int exit_status)
{
	if (WIFEXITED(exit_status) == 1)
		{
			printf("Exit Value: %d\n", WEXITSTATUS(&exit_status));
		}
		else
		{
			printf("Terminated by: %d\n", WTERMSIG(&exit_status));
		}
}

/* Function supporting built-in commands of exit, cd, status and handling blank and commented lines
   Passes off any additional commands to allOtherCommands function */

void builtInCommands(char *input_array[], char input_file[], char output_file[], int pid, int exit_status, struct sigaction SIGINT_action)
{
	// initialize variables
	char *comment = "#";
	char *exit = "exit";
	char *cd = "cd";
	char *status = "status";

	// check for blank input, return if found
	if (input_array[0] == '\0')
	{
		return;
	}

	// check for commented line, return if found
	else if (input_array[0][0] == *comment)
	{
		return;
	}

	// check for exit command, return if found
	else if (strcmp(input_array[0], exit) == 0)
	{
		loop_controller = 0;
	}

	// check for cd command
	else if (strcmp(input_array[0], cd) == 0)
	{
		// if no additional arguments, change dir to home
		if (input_array[1] == '\0')
		{
			chdir(getenv("HOME"));
			return;
		}
		// if additional arguments, attempt to change to arg dir
		else
		{
			// if successful
			if(chdir(input_array[1]) != 0)
			{
				return;
			}
			// if unsuccessful
			else
			{
				return;
			}
		}
	}

	// check for status command
	else if (strcmp(input_array[0], status) == 0)
	{
		printExitStatus(exit_status);
	}

	// if not a built in command, send to other commands function
	else
	{
		allOtherCommands(input_array, input_file, output_file, pid, exit_status, SIGINT_action);
		return;
	}
}


/* Function to reset the necessary variables between shell commands */

void variableReset(char *input_array[], char input_file[], char output_file[])
{
	// initialize variables
	char *null_term = "\0";
	int i;

	// reset input_array
	for (i = 0; input_array[i]; i++)
	{
		input_array[i] = NULL;
	}

	// reset input and output files
	input_file[0] = *null_term;
	output_file[0] = *null_term;

	// reset background_bool
	background_bool = 0;

	return;
}


/* Main shell loop for controlling flow of functions */

void shellLoop(char *input_array[], char input_file[], char output_file[], int pid, int exit_status, struct sigaction SIGINT_action)
{
	// continuously loop while global variable remains true
	while(loop_controller == 1)
	{
		// get user input, execute commands and reset variable each loop
		getInput(input_array, input_file, output_file, pid);
		builtInCommands(input_array, input_file, output_file, pid, exit_status, SIGINT_action);
		variableReset(input_array, input_file, output_file);
	}
}


/* Function to clear terminal screen */

void clearScreen()
{
	static int first_time = 1;
	if (first_time) 
	{
		const char* CLEAR_SCREEN_ANSI = " \e[1;1H\e[2J";
		write(STDOUT_FILENO, CLEAR_SCREEN_ANSI, 12);
		first_time = 0;
	}
}

/* function toggling foreground only mode when SIGTSTP is signaled */

void handle_SIGTSTP()
{
	// if background allowed, turn off
	if (background_signal == 1)
	{
		char* message = "\nEntering foreground-only mode (& is now ignored)\n";
		write(STDOUT_FILENO, message, 49);
		background_signal = 0;
	}
	// if background not allowed, turn on
	else
	{
		char* message = "\nExiting foreground-only mode\n";
		write(STDOUT_FILENO, message, 29);
		background_signal = 1;
	}
	fflush(stdout);
}


/*
  Main function
*/
int main(){
	// initialize input_array, input file, output file, pid, background_bool, exit_status
	int i;
	char* input_array[max_args];
	char* input_file = calloc(1, sizeof(max_args));
	char* output_file = calloc(1, sizeof(max_args));
	int pid;
	int exit_status;
	
	// ignore ctrl+c SIGINT signal
	struct sigaction SIGINT_action = {0};
	SIGINT_action.sa_handler = SIG_IGN;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = 0;
	sigaction(SIGINT, &SIGINT_action, NULL);
	

	// catch ctrl+z SIGTSTP signal to toggle foreground only mode
	struct sigaction SIGTSTP_action = {0};
	SIGTSTP_action.sa_handler = handle_SIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = 0;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);


	// set pid, background_bool, exit_status and clear input array
	pid = getpid();
	background_bool = 0;
	exit_status = 0;

	for (i = 0; i < max_args; i++)
	{
		input_array[i] = NULL;
	}

	// call clear screen function
	clearScreen();

	// call main looping shell function. pass relevant arguments
	shellLoop(input_array, input_file, output_file, pid, exit_status, SIGINT_action); 
}