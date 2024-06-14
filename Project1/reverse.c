#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

// https://opensource.apple.com/source/BerkeleyDB/BerkeleyDB-18/db/clib/strdup.c.auto.html
//  I got a general outline on how strdup works from page and I used this information to
//  create my own version within my code so I could handle the possibility of malloc failing

// my struct which defines the nodes of my doubly linked list
struct Line
{
	char *strPtr;
	struct Line *prev;
	struct Line *next;
};

static int printReverse(FILE *inputFile, FILE *outputFile, int argc)
{
	// starting node for my doubly linked list
	struct Line *start = NULL;
	struct Line *prevLine = NULL;
	char *currentLine = NULL;

	// variable for retrieving each line from the file
	char *line = NULL;
	size_t len = 0;
	size_t max_len = 0;
	ssize_t read;

	// making sure that the file isn't empty
	if ((read = getline(&line, &len, inputFile)) != -1)
	{
		// initalizing starting node for the doubly linked list
		// behavior for this first node works similarly as the other nodes
		// which is explained below
		start = (struct Line *)malloc(sizeof(struct Line));

		// ensuring malloc did not fail
		if (start == NULL)
		{
			fprintf(stderr, "error: malloc failed");
			return 1;
		}

		// creating array to store a copy of the current line since the current line will be overwritten each iteration
		currentLine = (char *)malloc(len + 1);

		// ensuring malloc didn't fail
		if (currentLine == NULL)
		{
			fprintf(stderr, "error: malloc failed");
			return 1;
		}

		// storing max line length for later use
		if (len + 1 > max_len)
			max_len = len + 1;

		// copying data from line to currentLine and using strncpy to be memory safe
		strncpy(currentLine, line, len + 1);

		// updating strPtr to point to the newly allocated array with the correct data
		start->strPtr = currentLine;
		start->prev = NULL;
		start->next = NULL;

		// previous node for setting up doubly linked list correctly
		prevLine = start;

		// while loop for reading file contents line by line until end of file
		while ((read = getline(&line, &len, inputFile)) != -1)
		{
			// creating new lineNode and allocating memory for it
			struct Line *lineNode = (struct Line *)malloc(sizeof(struct Line));

			// ensuring malloc did not fail
			if (lineNode == NULL)
			{
				fprintf(stderr, "error: malloc failed");
				return 1;
			}

			// creating array to store a copy of the current line since the current line will be overwritten each iteration
			currentLine = (char *)malloc(len + 1);

			// ensuring malloc didn't fail
			if (currentLine == NULL)
			{
				fprintf(stderr, "error: malloc failed");
				return 1;
			}

			// storing max line length for later use
			if (len + 1 > max_len)
				max_len = len + 1;

			// copying data from line into current line using strncpy to be memory safe
			strncpy(currentLine, line, len + 1);

			// updating pointer in lineNode to point to the allocated array
			lineNode->strPtr = currentLine;

			// updating prev pointer to prevLine node
			lineNode->prev = prevLine;
			lineNode->next = NULL;

			// updating prevLine to point to current node
			prevLine->next = lineNode;

			// setting current node to be the next previous node
			prevLine = lineNode;
		}

		// https://www.ibm.com/docs/en/zos/2.5.0?topic=functions-getline-read-entire-line-from-stream
		// https://stackoverflow.com/questions/46013418/how-to-check-the-value-of-errno
		// got information on what errno is from the getline() documentation from IBM and how to check
		// it's value from this stack overflow thread
		if (errno == ENOMEM)
		{
			fprintf(stderr, "error: malloc failed to allocate space for the current line");
			return 1;
		}
	}

	// https://www.ibm.com/docs/en/zos/2.5.0?topic=functions-getline-read-entire-line-from-stream
	// https://stackoverflow.com/questions/46013418/how-to-check-the-value-of-errno
	// got information on what errno is from the getline() documentation from IBM and how to check
	// it's value from this stack overflow thread
	if (errno == ENOMEM)
	{
		fprintf(stderr, "error: malloc failed to allocate space for the current line");
		return 1;
	}

	// printing lines in reverse order
	struct Line *lineNode = prevLine;

	// having to do some jank here to make the behavior work as a expected
	// since the final line of the file doesn't have a new line I'm creating a
	// copy of the line and concatenating a new line character
	// I'm then freeing the memory of the old line and updating the final node
	// in my linked list to contain the updated string with the newline character

	/*char *updatedFinalLine = malloc(strnlen(prevLine->strPtr, 500) + 2);
	strcpy(updatedFinalLine, prevLine->strPtr);
	free(prevLine->strPtr);
	strcat(updatedFinalLine, "\n");

	prevLine->strPtr = updatedFinalLine;*/

	// moving through linked list in reverse and printing the lines
	while (lineNode != NULL)
	{
		// printing file contents to console when no output file is specified
		if (argc == 2)
		{
			printf("%s", lineNode->strPtr);
			lineNode = lineNode->prev;
		}
		// writing out file contents to output file that was specified
		else if (argc == 3)
		{
			fwrite(lineNode->strPtr, sizeof(char), strnlen(lineNode->strPtr, max_len) / sizeof(char), outputFile);
			lineNode = lineNode->prev;
		}
	}

	// closing file stream
	fclose(inputFile);

	// closing output file if one exists
	if (argc == 3)
		fclose(outputFile);

	// freeing allocated memory
	lineNode = start;
	while (lineNode != NULL)
	{
		// temporary variable to store the pointer to the next node since it will be lost when freeing memory
		struct Line *nextLine = lineNode->next;

		// freeing memory storing the string which corresponds to a line in the file
		free(lineNode->strPtr);

		// freeing memory storing each node
		free(lineNode);

		// moving on to the next node
		lineNode = nextLine;
	}

	// freeing memory allocated by getline
	free(line);

	return 0;
}

int main(int argc, char *argv[])
{
	// checking for correct amount of command line arguments
	if (argc < 2 || argc > 3)
	{
		// printing error about correct usage
		fprintf(stderr, "usage: reverse <input> <output>\n");
		return 1;
	}

	// comparing the file paths passed to make sure they're not identical
	if (argc == 3 && strncmp(argv[1], argv[2], FILENAME_MAX) == 0)
	{
		// printing error that files shouldn't be the same
		fprintf(stderr, "error: input and output file must differ\n");
		return 1;
	}

	// opening up input file with read mode
	FILE *inputFile;
	inputFile = fopen(argv[1], "r");

	// creating output file pointer to initially be NULL in the case none is passed
	FILE *outputFile = NULL;

	// making sure the input file opened correctly
	if (inputFile == NULL)
	{
		// printing error if file could not be opened
		fprintf(stderr, "error: cannot open file '%s'\n", argv[1]);
		return 1;
	}

	// opening up output file if 3 command line arguments were passed
	if (argc == 3)
	{
		// opening output file in write mode
		outputFile = fopen(argv[2], "w");

		// making sure the output file opened correctly
		if (outputFile == NULL)
		{
			// printing error if file could not be opened
			fprintf(stderr, "error: cannot open file '%s'\n", argv[2]);
			return 1;
		}
	}

	// function that handles reversing the file contents
	return printReverse(inputFile, outputFile, argc);
}
