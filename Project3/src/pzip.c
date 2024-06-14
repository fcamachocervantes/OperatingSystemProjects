#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pzip.h"

struct ThreadData {
	// Passing pointer of list of characters to the thread
	char *input_chars;
	// Passing where the thread should start
	int start_index;
	// Passing how big the chunk of characters is for the thread
	int chuck_size;
	// Number of elements in local result array
	int zipped_char_count;
	// Array to hold the zipped local result
	struct zipped_char *local_result;
};

// Thread compression function
static void *compress(void *arg) {
	struct ThreadData *data = (struct ThreadData *)arg;
	char current_char;
	int local_result_index = 0;

	// Looping through section of string assigned to this thread
	for (int i = data->start_index; i < (data->start_index + data->chuck_size); i++) {
		// Saving current character
		current_char = data->input_chars[i];

		// Checking if the results array already is storing a character, if it is we check if current char and stored one are the same
		if(data->local_result[local_result_index].character == '\0' || data->local_result[local_result_index].character == current_char) {
			data->local_result[local_result_index].character = current_char;
			data->local_result[local_result_index].occurence++;
		} else {
			local_result_index++;
			data->local_result[local_result_index].character = current_char;
			data->local_result[local_result_index].occurence++;
		}
	}

	// Updating count for amount of zipped characters
	data->zipped_char_count = local_result_index;

	return NULL;
}

/**
 * pzip() - zip an array of characters in parallel
 *
 * Inputs:
 * @n_threads:		   The number of threads to use in pzip
 * @input_chars:		   The input characters (a-z) to be zipped
 * @input_chars_size:	   The number of characaters in the input file
 *
 * Outputs:
 * @zipped_chars:       The array of zipped_char structs
 * @zipped_chars_count:   The total count of inserted elements into the zippedChars array.
 * @char_frequency[26]: Total number of occurences
 *
 * NOTE: All outputs are already allocated. DO NOT MALLOC or REASSIGN THEM !!!
 *
 */
void pzip(int n_threads, char *input_chars, int input_chars_size,
	  struct zipped_char *zipped_chars, int *zipped_chars_count,
	  int *char_frequency)
{
	// Creating array that holds thread IDs
	pthread_t *thread_ids;
	thread_ids = (pthread_t *)malloc(n_threads * sizeof(pthread_t));

	if (thread_ids == NULL) {
		fprintf(stderr, "Failed to malloc thread_ids\n");
		exit(1);
	}

	// Creating array to hold all local results
	struct ThreadData *thread_data;
	thread_data = (struct ThreadData *)malloc(n_threads * sizeof(struct ThreadData));

	if (thread_data == NULL) {
		fprintf(stderr, "Failed to malloc thread_data\n");
		exit(1);
	}

	// Calculating chunk size for each thread
	int chunk_size = input_chars_size / n_threads;

	int start_index = 0;
	for(int i = 0; i < n_threads; i++) {
		// Setting up thread data object to be passed to each thread
		thread_data[i].input_chars = input_chars;
		thread_data[i].start_index = start_index;
		thread_data[i].chuck_size = chunk_size;
		thread_data[i].local_result = calloc(chunk_size, sizeof(struct zipped_char));

		if (thread_data[i].local_result == NULL) {
			fprintf(stderr, "Failed to malloc local_result\n");
			exit(1);
		}

		// Initializing thread
		pthread_create(&thread_ids[i], NULL, compress, (void *)&thread_data[i]);

		// Updating start index for next thread
		start_index += chunk_size; 
	}
	
	// Wait for threads to all finish
	for (int i = 0; i < n_threads; i++) {
		pthread_join(thread_ids[i], NULL);
	}

	// Moving through all the data tuples the threads populate
	for (int i = 0; i < n_threads; i++) {
		// Moving through the local result array for each data tuple
		for (int j = 0; j <= thread_data[i].zipped_char_count; j++) {
			// Updating zipped_chars array element with local_result tuple and moving to next element
			zipped_chars[*zipped_chars_count] = thread_data[i].local_result[j];
			// Updating occurence of character in the fequency array, a=97 and z=122 in Ascii to translate to index 0 and 25 we % 97
			char_frequency[thread_data[i].local_result[j].character % 97] += thread_data[i].local_result[j].occurence;
			*zipped_chars_count += 1;
		}
	}

	// Freeing all allocated memory starting from deepest to highest
	for (int i = 0; i < n_threads; i++) {
		free(thread_data[i].local_result);
	}
	// Freeing thread data structs
	free(thread_data);
	// Freeing thread id array
	free(thread_ids);
}
