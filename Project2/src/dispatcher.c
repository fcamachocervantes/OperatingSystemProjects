#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dispatcher.h"
#include "shell_builtins.h"
#include "parser.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

/**
 * dispatch_external_command() - run a pipeline of commands
 *
 * @pipeline:   A "struct command" pointer representing one or more
 *              commands chained together in a pipeline.  See the
 *              documentation in parser.h for the layout of this data
 *              structure.  It is also recommended that you use the
 *              "parseview" demo program included in this project to
 *              observe the layout of this structure for a variety of
 *              inputs.
 *
 * Note: this function should not return until all commands in the
 * pipeline have completed their execution.
 *
 * Return: The return status of the last command executed in the
 * pipeline.
 */

// Redirect function for handling input
static void fileOpenRead(char *file_name)
{
	int input_fd = -1;
	input_fd = open(file_name, O_RDONLY);

	// Making sure the file opened correctly otherwise exiting and printing it failed to open file
	if (input_fd != -1) {
		dup2(input_fd, STDIN_FILENO);
		close(input_fd);
	} else {
		fprintf(stderr, "error: cannot open file or directory\n");
		exit(1);
	}
}

// Redirect function for handling output
static void fileOpenWrite(char *file_name, enum command_output_type output_type)
{
	int output_fd = -1;

	// Opening output file in correct mode, either truncate or append mode
	// Got help from here https://www.codequoi.com/en/handling-a-file-by-its-descriptor-in-c/
	switch (output_type) {
	case COMMAND_OUTPUT_FILE_TRUNCATE:
		// Opening output file if it exists
		output_fd = open(file_name, O_WRONLY | O_TRUNC);

		// Output file doesn't exist so we need to create it
		if (output_fd == -1) {
			output_fd = open(file_name,
					 O_WRONLY | O_TRUNC | O_CREAT,
					 S_IRUSR | S_IWUSR);
		}
		break;
	case COMMAND_OUTPUT_FILE_APPEND:
		// Opening output file if it exists
		output_fd = open(file_name, O_WRONLY | O_APPEND);

		// Output file doesn't exist so we need to create it
		if (output_fd == -1) {
			output_fd = open(file_name,
					 O_WRONLY | O_APPEND | O_CREAT,
					 S_IRUSR | S_IWUSR);
		}
		break;
	default:
		break;
	}

	// Making sure output file opened correctly or was created correctly
	if (output_fd != -1) {
		dup2(output_fd, STDOUT_FILENO);
		close(output_fd);
	} else {
		fprintf(stderr, "error: cannot open file or directory\n");
		exit(1);
	}
}

// Helper function using loop
static int execute_pipeline(struct command *pipeline)
{	
	int returnVal = 0;

	// Initializing values for pipe and previous read pipe end
	int pipe_fd[2];
	int previous_pipe_read_end = -1;

	// While loop that steps through each command
	while (pipeline != NULL) {
		if (pipeline->output_type == COMMAND_OUTPUT_PIPE) {
			if (pipe(pipe_fd) == -1) {
				fprintf(stderr, "error: failed to create pipe\n");
				return -1;
			}
		}

		pid_t returned_pid = fork();
		int status_ptr;

		// Child Process
		if (returned_pid == 0) {
			// Changing read end of process to write end of pipe created by previous command
			if (previous_pipe_read_end != -1) {
				dup2(previous_pipe_read_end, STDIN_FILENO);
				close(previous_pipe_read_end);
			}

			// Changing write end of process to write end of newly pipe
			if (pipeline->output_type == COMMAND_OUTPUT_PIPE) {
				dup2(pipe_fd[1], STDOUT_FILENO);
				close(pipe_fd[0]);
				close(pipe_fd[1]);
			}

			// Redirect handling for input
			if (pipeline->input_filename != NULL)
				fileOpenRead(pipeline->input_filename);

			// Redirect Handling for output
			if (pipeline->output_type ==
				    COMMAND_OUTPUT_FILE_APPEND ||
			    pipeline->output_type ==
				    COMMAND_OUTPUT_FILE_TRUNCATE)
				fileOpenWrite(pipeline->output_filename,
					      pipeline->output_type);

			// Executing command
			execvp(pipeline->argv[0], pipeline->argv);
			fprintf(stderr, "error: cannot find command pipe\n");
			exit(1);
		} else if (returned_pid < 0) {
			fprintf(stderr, "fork failed\n");
			return -1;
		} else {
			// Parent process

			// Closing read end of previous command pipe
			if(previous_pipe_read_end != -1) {
				close(previous_pipe_read_end);
			}

			// Closing write end of current pipe and setting previous pipe to current pipe's read end
			if(pipeline->output_type == COMMAND_OUTPUT_PIPE) {
				close(pipe_fd[1]);
				previous_pipe_read_end = pipe_fd[0];
			}

			// Wait for child process to finish
			waitpid(returned_pid, &status_ptr, WUNTRACED);

			// Checking if the child process exited
			if (WIFEXITED(status_ptr)) {
				// Any value that's not 0 means that the child didn't exit properly
				if (WEXITSTATUS(status_ptr) != 0) {
					returnVal = -1;
				} else {
					returnVal = 0;
				}
			}
			
			// Moving on to the next command in the pipeline
			pipeline = pipeline->pipe_to;
		}
	}

	return returnVal;
}

static int dispatch_external_command(struct command *pipeline)
{
	/*
	 * Note: this is where you'll start implementing the project.
	 *
	 * It's the only function with a "TODO".  However, if you try
	 * and squeeze your entire external command logic into a
	 * single routine with no helper functions, you'll quickly
	 * find your code becomes sloppy and unmaintainable.
	 *
	 * It's up to *you* to structure your software cleanly.  Write
	 * plenty of helper functions, and even start making yourself
	 * new files if you need.
	 *
	 * For D1: you only need to support running a single command
	 * (not a chain of commands in a pipeline), with no input or
	 * output files (output to stdout only).  In other words, you
	 * may live with the assumption that the "input_file" field in
	 * the pipeline struct you are given is NULL, and that
	 * "output_type" will always be COMMAND_OUTPUT_STDOUT.
	 *
	 * For D2: you'll extend this function to support input and
	 * output files, as well as pipeline functionality.
	 *
	 * Good luck!
	 */

	int returnVal = -1;

	// Calling loop helper function
	returnVal = execute_pipeline(pipeline);

	return returnVal;
}

/**
 * dispatch_parsed_command() - run a command after it has been parsed
 *
 * @cmd:                The parsed command.
 * @last_rv:            The return code of the previously executed
 *                      command.
 * @shell_should_exit:  Output parameter which is set to true when the
 *                      shell is intended to exit.
 *
 * Return: the return status of the command.
 */
static int dispatch_parsed_command(struct command *cmd, int last_rv,
				   bool *shell_should_exit)
{
	/* First, try to see if it's a builtin. */
	for (size_t i = 0; builtin_commands[i].name; i++) {
		if (!strcmp(builtin_commands[i].name, cmd->argv[0])) {
			/* We found a match!  Run it. */
			return builtin_commands[i].handler(
				(const char *const *)cmd->argv, last_rv,
				shell_should_exit);
		}
	}

	/* Otherwise, it's an external command. */
	return dispatch_external_command(cmd);
}

int shell_command_dispatcher(const char *input, int last_rv,
			     bool *shell_should_exit)
{
	int rv;
	struct command *parse_result;
	enum parse_error parse_error = parse_input(input, &parse_result);

	if (parse_error) {
		fprintf(stderr, "Input parse error: %s\n",
			parse_error_str[parse_error]);
		return -1;
	}

	/* Empty line */
	if (!parse_result)
		return last_rv;

	rv = dispatch_parsed_command(parse_result, last_rv, shell_should_exit);
	free_parse_result(parse_result);
	return rv;
}
