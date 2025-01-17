#include "mdu.h"
#include "jstack.h"

struct task_manager
{
	pthread_cond_t cond;
	pthread_mutex_t mutex_stack_lock;
	pthread_mutex_t mutex_array_lock;
	pthread_mutex_t mutex_active_task_count_lock;
	pthread_mutex_t mutex_exit_code_lock;
	stack *task_stack;
	int active_threads;
	bool finished;
	long int *sum_array;
	int exit_code;
};

struct task_info
{
	// path to the dir or file to check and calculate
	char *path;
	// so the thread know which pos to write to when adding the sum to the total sum
	int sum_index;
};
/**
 * main()-
 * 
 * @argc: number of arguments
 * @argv: argument vector, array of char pointers
 * RETURNS: 0 on success, 1 on fail
 */
int main(int argc, char *argv[])
{
	int exit_code = 0;
	int thread_count = DEFAULT_THREADS;
	int opt;
	// use getopt to get flags and args
	while ((opt = getopt(argc, argv, "j:")) != -1)
	{
		switch (opt)
		{
		case 'j':
			thread_count = atoi(optarg);
			break;
		default:
			fprintf(stderr, "Usage: %s [-j number of threads]"
					" {file} [files...]\n",
				argv[0]);
			exit(EXIT_FAILURE);
			break;
		}
	}

	
	
	
	int i = optind;
	task_manager *task = malloc(sizeof(task_manager));
	if (i >= argc)
	{
		
		task->task_stack = stack_empty();
		task->sum_array = calloc(i, sizeof(long long));
		task->active_threads = 0;
		task->finished = false;
		task->exit_code = 0;

		pthread_mutex_init(&task->mutex_stack_lock, NULL);
		pthread_mutex_init(&task->mutex_array_lock, NULL);
		pthread_mutex_init(&task->mutex_active_task_count_lock, NULL);
		pthread_mutex_init(&task->mutex_exit_code_lock, NULL);
		pthread_cond_init(&task->cond, NULL);
		task_info *new_task = malloc(sizeof(task_info));
		new_task->path = strdup(".");
		new_task->sum_index = i;
		// push task onto stack
		pthread_mutex_lock(&task->mutex_stack_lock);
		task->task_stack = stack_push(task->task_stack, new_task);
		pthread_mutex_unlock(&task->mutex_stack_lock);
		pthread_mutex_lock(&task->mutex_active_task_count_lock);
		task->active_threads++;
		pthread_mutex_unlock(&task->mutex_active_task_count_lock);
	}
	while (i < argc)
	{
		task->task_stack = stack_empty();
		task->sum_array = calloc(argc, sizeof(long long));
		task->active_threads = 0;
		task->finished = false;
		task->exit_code = 0;

		pthread_mutex_init(&task->mutex_stack_lock, NULL);
		pthread_mutex_init(&task->mutex_array_lock, NULL);
		pthread_mutex_init(&task->mutex_active_task_count_lock, NULL);
		pthread_mutex_init(&task->mutex_exit_code_lock, NULL);
		pthread_cond_init(&task->cond, NULL);

		task_info *new_task = malloc(sizeof(task_info));
		new_task->path = strdup(argv[i]);

		new_task->sum_index = i;
		// push task onto stack
		pthread_mutex_lock(&task->mutex_stack_lock);
		task->task_stack = stack_push(task->task_stack, new_task);
		pthread_mutex_unlock(&task->mutex_stack_lock);
		pthread_mutex_lock(&task->mutex_active_task_count_lock);
		task->active_threads++;
		pthread_mutex_unlock(&task->mutex_active_task_count_lock);
		i++;
	}

	


	// crate all the threads and send them to the thread pool
	pthread_t threads[thread_count];

	for (int i = 0; i < thread_count; i++)
	{
		pthread_create(&threads[i], NULL, &thread_pool, task);
	}
	// wait for the threads
	for (int i = 0; i < thread_count; i++)
	{
		pthread_join(threads[i], NULL);
	}
	// print the total sizes here i guess
	for (int i = 0; i < thread_count; i++)
	{
		printf("%-7ld %s\n", task->sum_array[i], argv[i]);
	}
	//get the exit code before freeing everything
	exit_code = task->exit_code;
	// free everything here
	pthread_mutex_destroy(&task->mutex_stack_lock);
	pthread_mutex_destroy(&task->mutex_array_lock);
	pthread_mutex_destroy(&task->mutex_active_task_count_lock);
	pthread_mutex_destroy(&task->mutex_exit_code_lock);
	pthread_cond_destroy(&task->cond);
	
	// free the task manager
	stack_kill(task->task_stack);
	free(task->sum_array);
	free(task);
	exit(exit_code);
}
/**
 * thread_pool()- Function where all threads will wait for tasks to be 
 * pushed on the stack. If a task is found the thread will pop the task
 * from the stack and calculate the size of the file or directory.
 * 
 * @arg: a pointer to the task_manager struct
 * RETURNS: NULL
 */
void *thread_pool(void *arg)
{
	// get the task manager
	task_manager *task = (task_manager *)arg;
	while (1)
	{
		int found = 0;
		//get the stack
		pthread_mutex_lock(&task->mutex_stack_lock);
		stack *task_stack = task->task_stack;
		while (stack_is_empty(task_stack))
		{
			if (task->finished)
			{
				// if the stack is empty and the task count is 0 then all tasks are done
				pthread_mutex_unlock(&task->mutex_stack_lock);
				return NULL;
			}
			pthread_mutex_lock(&task->mutex_active_task_count_lock);
			if (task->active_threads == 0)
			{
				pthread_mutex_unlock(&task->mutex_active_task_count_lock);
				task->finished = true;
				//wake all threads so they can exit
				pthread_cond_broadcast(&task->cond);
				pthread_mutex_unlock(&task->mutex_stack_lock);
				return NULL;
			}
			else
			{
				//else wait for tasks to be pushed onto the stack
				pthread_mutex_unlock(&task->mutex_active_task_count_lock);
				pthread_cond_wait(&task->cond, &task->mutex_stack_lock);
			}
		}
		// if the stack is not empty then pop a task from the stack
		found = 1;
		task_info *task_inf;
		// allocate memmory for the copy since pop will free the task
		task_info *task_copy = malloc(sizeof(task_info));
		task_inf = stack_top(task->task_stack);
		task_copy->path = strdup(task_inf->path);
		task_copy->sum_index = task_inf->sum_index;
		// pop the task from the stack
		task->task_stack = stack_pop(task->task_stack);
		pthread_mutex_unlock(&task->mutex_stack_lock);
		// check if the task is a file or directory
		if (found)
		{
			if (is_file(task_copy->path))
			{
				calculate_size(task_copy, task, task_copy->path);
			}
			else
			{
				traverse_directory(task_copy, task);
			}
			pthread_mutex_lock(&task->mutex_active_task_count_lock);
			task->active_threads--;
			pthread_mutex_unlock(&task->mutex_active_task_count_lock);
		}
		//when done, free the task copy
		free(task_copy->path);
		free(task_copy);
	}
}


/**
 * traverse_directory()- Function that will traverse a directory and
 * push subdirectories onto the stack and calculate the size of the
 * files.
 * 
 * @task_inf: a pointer to the task_info struct
 * @task: a pointer to the task_manager struct
 * RETURNS: NULL
 */
void traverse_directory(task_info *task_inf, task_manager *task)
{
	// open the directory
	DIR *dir;
	struct dirent *entry;
	struct stat file_stat;
	dir = opendir(task_inf->path);
	if (dir == NULL)
	{
		fprintf(stderr, "mdu: cannot read directory \'%s\': ", task_inf->path);
		perror("");
		calculate_size(task_inf, task, task_inf->path);
		pthread_mutex_lock(&task->mutex_exit_code_lock);
		task->exit_code = 1;
		pthread_mutex_unlock(&task->mutex_exit_code_lock);
		return;
	}
	// read everything in the current directory
	while ((entry = readdir(dir)) != NULL)
	{
		//+2 for the "/" sign and the null terminator
		char *file_path = malloc((strlen(task_inf->path) + strlen(entry->d_name) + 2));
		// get the full path with the entry
		sprintf(file_path, "%s/%s", task_inf->path, entry->d_name);

		lstat(file_path, &file_stat);
		// check if it is a directory
		if (S_ISDIR(file_stat.st_mode))
		{
			// skip "." but count the size of the metadata
			if (strcmp(entry->d_name, ".") == 0)
			{
				calculate_size(task_inf, task, file_path);
				free(file_path);
				continue;
			}
			// skip ".."
			if (strcmp(entry->d_name, "..") == 0)
			{
				free(file_path);
				continue;
			}
			// allocate memory for the new task
			task_info *new_task = malloc(sizeof(task_info));
			// copy the path to the new task
			new_task->path = malloc((strlen(task_inf->path) + strlen(entry->d_name) + 2));
			strcpy(new_task->path, file_path);
			new_task->sum_index = task_inf->sum_index;
			// push the new task to the stack
			pthread_mutex_lock(&task->mutex_stack_lock);
			task->task_stack = stack_push(task->task_stack, new_task);
			pthread_cond_signal(&task->cond);
			pthread_mutex_unlock(&task->mutex_stack_lock);
			// update the task count
			pthread_mutex_lock(&task->mutex_active_task_count_lock);
			task->active_threads++;
			pthread_mutex_unlock(&task->mutex_active_task_count_lock);
			free(file_path);
		}
		// check for symbolic link
		else
		{
			calculate_size(task_inf, task, file_path);
			free(file_path);
		}
	}
	closedir(dir);
}
/**
 * calculate_size()- Function that will calculate the size of a file
 * and add it to the total sum. The total sum is stored in an array
 * that is shared between all threads. The index to write to is stored
 * in the task_info struct so the thread knows where to write to.
 * 
 * @task_inf: a pointer to the task_info struct
 * @task: a pointer to the task_manager struct
 * @file_path: a pointer to the path of the file
 * RETURNS: NULL
 */
void calculate_size(task_info *task_inf, task_manager *task, char *file_path)
{
	struct stat file_stat;
	lstat(file_path, &file_stat);
	// get the index to write to
	int index = task_inf->sum_index;
	pthread_mutex_lock(&task->mutex_array_lock);
	// get the array to read from and write to
	long int *sumarr = task->sum_array;
	long int sum = sumarr[index];
	sum += file_stat.st_blocks;
	sumarr[index] = sum;
	pthread_mutex_unlock(&task->mutex_array_lock);
}

bool is_file(char *path)
{
	struct stat file_stat;

	lstat(path, &file_stat);
	return S_ISREG(file_stat.st_mode);
}