#include "mdu.h"
#include "jstack.h"

/**
 * @brief Implementation of a minimal version of the unix command du that can run on multiple threads.
 *
 * The program will take a list of files and directories and calculate the size of each file and directory.
 * The program will then print the size of each file and directory in the unit blocks. The program will also take a flag -j if
 * the user wants to specify the number of threads to use. If no flag is given then the program will use 1 thread.
 *
 * @author Jonatan Westling (c22jwg)
 * @file mdu.c
 * @date 2023-10-18
 */
// struct to hold the arguments
struct args
{
	char **files;
	int num_files;
	int threads;
};
// struct that manage the tasks for the threads, and collect necessary information
struct task_manager
{
	// conditional variable to wake up threads when a task is pushed onto the stack
	pthread_cond_t cond;
	// mutexes to protect the stack, array, task count and exit code
	pthread_mutex_t mutex_stack_lock;
	pthread_mutex_t mutex_array_lock;
	pthread_mutex_t mutex_active_task_count_lock;
	pthread_mutex_t mutex_exit_code_lock;
	// stack that holds the tasks
	stack *task_stack;
	int active_threads;
	// boolean to tell if all tasks are done
	bool finished;
	// array that holds the total sizes
	long int *sum_array;
	int exit_code;
};
// struct that holds the information for each task
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
 * @param argc: number of arguments
 * @param argv: argument vector, array of char pointers
 * RETURNS: 0 on success, 1 on fail
 */
int main(int argc, char *argv[])
{
	int exit_code = 0;
	args *argument_info = read_args(argc, argv);
	// create an empty stack for the threadpool
	task_manager *task = create_task_manager(argument_info);
	// create taskinfos and put them onto the stack
	// create tasks for threadpool, put them onto the stack
	create_tasks(task, argument_info);
	// crate all the threads and send them to the thread pool
	pthread_t threads[argument_info->threads];

	for (int i = 0; i < argument_info->threads; i++)
	{
		if (pthread_create(&threads[i], NULL, &thread_pool, task) != 0)
		{
			perror("Failed to create thread");
			exit(EXIT_FAILURE);
		}
	}
	// wait for the threads
	for (int i = 0; i < argument_info->threads; i++)
	{
		if (pthread_join(threads[i], NULL) != 0)
		{
			perror("Failed to join thread");
			exit(EXIT_FAILURE);
		}
	}
	print_result(task, argument_info);
	// get the exit code before freeing everything
	exit_code = task->exit_code;
	// free everything here
	free_memory(task, argument_info);
	exit(exit_code);
}
/**
 * thread_pool()- Function where all threads will wait for tasks to be
 * pushed on the stack. If a task is found the thread will pop the task
 * from the stack and calculate the size of the file or directory.
 *
 * @param arg: a pointer to the task_manager struct
 * RETURNS: NULL
 */
void *thread_pool(void *arg)
{
	// get the task manager
	task_manager *task = (task_manager *)arg;
	while (1)
	{
		int found = 0;
		// get the stack
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
				// wake all threads so they can exit
				pthread_cond_broadcast(&task->cond);
				pthread_mutex_unlock(&task->mutex_stack_lock);
				return NULL;
			}
			else
			{
				// else wait for tasks to be pushed onto the stack
				pthread_mutex_unlock(&task->mutex_active_task_count_lock);
				pthread_cond_wait(&task->cond, &task->mutex_stack_lock);
			}
		}
		start_task(task, found);
	}
}

/**
 * traverse_directory()- Function that will traverse a directory and
 * push subdirectories onto the stack and calculate the size of the
 * files.
 *
 * @param task_inf: a pointer to the task_info struct
 * @param task: a pointer to the task_manager struct
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
		// if the directory cannot be opened (no permission) print to stderr
		fprintf(stderr, "mdu: cannot read directory \'%s\': ", task_inf->path);
		perror("");
		// calculate the size of the metadata
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
		if (file_path == NULL)
		{
			perror("malloc failed");
			exit(EXIT_FAILURE);
		}
		// get the full path with the entry
		sprintf(file_path, "%s/%s", task_inf->path, entry->d_name);
		if (lstat(file_path, &file_stat) == -1)
		{
			perror("lstat failed");
			exit(EXIT_FAILURE);
		}
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
			else
			{
				handle_subdir(task_inf, task, file_path, entry->d_name);
			}
		}
		else
		{
			// if it is a file or symbolic link then calculate the size
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
 * @param task_inf: a pointer to the task_info struct
 * @param task: a pointer to the task_manager struct
 * @param file_path: a pointer to the path of the file
 * RETURNS: NULL
 */
void calculate_size(task_info *task_inf, task_manager *task, char *file_path)
{
	struct stat file_stat;
	if (lstat(file_path, &file_stat) == -1)
	{
		perror("stat failed");
		exit(EXIT_FAILURE);
	}
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

/**
 * read_args()- Function that will read the arguments and flags from
 * the command line and store them in a struct. If no directory or file
 * is given then the current directory will be used.
 *
 * @param argc: number of arguments
 * @param argv: argument vector, array of char pointers
 * RETURNS: a pointer to the struct
 */
args *read_args(int argc, char *argv[])
{
	// allocate memory for the struct
	args *argument_info = malloc(sizeof(args));
	if (argument_info == NULL)
	{
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	// default amount of threads: 1
	argument_info->threads = DEFAULT_THREADS;
	argument_info->num_files = 0;
	argument_info->files = NULL;
	int opt;
	// use getopt to get flags and args
	while ((opt = getopt(argc, argv, "j:")) != -1)
	{
		switch (opt)
		{
		case 'j':
			argument_info->threads = atoi(optarg);
			break;
		default:
			fprintf(stderr, "Usage: %s [-j number of threads]"
					" {file} [files...]\n",
				argv[0]);
			exit(EXIT_FAILURE);
			break;
		}
	}
	argument_info->files = malloc((argc - optind) * sizeof(char *));
	if (argument_info->files == NULL)
	{
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	int i = optind;
	if (i >= argc)
	{
		// if no run current directory
		argument_info->files[argument_info->num_files] = strdup(".");
		if (argument_info->files[argument_info->num_files] == NULL)
		{
			perror("strdup");
			exit(EXIT_FAILURE);
		}
		argument_info->num_files++;
	}
	while (i < argc)
	{
		// add all arguments to the array in the struct
		argument_info->files[argument_info->num_files] = strdup(argv[i]);
		if (argument_info->files[argument_info->num_files] == NULL)
		{
			perror("strdup");
			exit(EXIT_FAILURE);
		}
		argument_info->num_files++;
		i++;
	}
	return argument_info;
}

/**
 * print_info()- Function that will print the info from the struct
 *
 * @param argument_info: a pointer to the struct
 * RETURNS: NULL
 */
bool is_file(char *path)
{
	struct stat file_stat;

	if (lstat(path, &file_stat))
	{
		perror("stat");
		exit(EXIT_FAILURE);
	}
	return S_ISREG(file_stat.st_mode);
}

/**
 * create_task_manager()- Function that will create a task_manager struct
 * and initialize all mutexes, conditional variables.
 *
 * @param argument_info: a pointer to the struct
 * RETURNS: a pointer to the struct
 */
task_manager *create_task_manager(args *argument_info)
{
	task_manager *task = malloc(sizeof(task_manager));
	if (task == NULL)
	{
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	task->task_stack = stack_empty();
	// allocate memory for the array that will hold the total sizes
	task->sum_array = calloc(argument_info->num_files, sizeof(long long));
	if (task->sum_array == NULL)
	{
		perror("calloc");
		exit(EXIT_FAILURE);
	}
	task->active_threads = 0;
	task->finished = false;
	task->exit_code = 0;
	// declare mutex and conditional variable

	pthread_mutex_init(&task->mutex_stack_lock, NULL);
	pthread_mutex_init(&task->mutex_array_lock, NULL);
	pthread_mutex_init(&task->mutex_active_task_count_lock, NULL);
	pthread_mutex_init(&task->mutex_exit_code_lock, NULL);
	pthread_cond_init(&task->cond, NULL);
	return task;
}

/**
 * free_memory()- Function that will free all memory that was allocated
 * for the struct and the files.
 *
 * @param argument_info: a pointer to the struct
 * RETURNS: NULL
 */
void free_memory(task_manager *task, args *argument_info)
{
	pthread_mutex_destroy(&task->mutex_stack_lock);
	pthread_mutex_destroy(&task->mutex_array_lock);
	pthread_mutex_destroy(&task->mutex_active_task_count_lock);
	pthread_mutex_destroy(&task->mutex_exit_code_lock);
	pthread_cond_destroy(&task->cond);
	// free the files
	if (argument_info->files != NULL)
	{
		for (int i = 0; i < argument_info->num_files; i++)
		{
			free(argument_info->files[i]);
		}
		free(argument_info->files);
	}
	free(argument_info);
	// free the task manager
	stack_kill(task->task_stack);
	free(task->sum_array);
	free(task);
}

/**
 * create_tasks()- Function that will create a task_info struct for each
 * directory and push it onto the stack.
 *
 * @param argument_info: a pointer to the struct
 * RETURNS: NULL
 */
void create_tasks(task_manager *task, args *argument_info)
{
	for (int i = 0; i < argument_info->num_files; i++)
	{
		task_info *new_task = malloc(sizeof(task_info));
		if (new_task == NULL)
		{
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		new_task->path = strdup(argument_info->files[i]);
		if (new_task->path == NULL)
		{
			perror("strdup");
			exit(EXIT_FAILURE);
		}

		new_task->sum_index = i;
		// push task onto stack
		pthread_mutex_lock(&task->mutex_stack_lock);
		task->task_stack = stack_push(task->task_stack, new_task);
		pthread_mutex_unlock(&task->mutex_stack_lock);
		pthread_mutex_lock(&task->mutex_active_task_count_lock);
		task->active_threads++;
		pthread_mutex_unlock(&task->mutex_active_task_count_lock);
	}
}

/**
 * print_result()- Function that will print the result from the array
 * that holds the total sizes.
 *
 * @param argument_info: a pointer to the struct
 * RETURNS: NULL
 */
void print_result(task_manager *task, args *argument_info)
{
	for (int i = 0; i < argument_info->num_files; i++)
	{
		printf("%-7ld %s\n", task->sum_array[i], argument_info->files[i]);
	}
}

/**
 * handle_subdir()- Function that will create a new task_info struct for
 * a subdirectory and push it onto the stack.
 *
 * @param task_inf: a pointer to the task_info struct
 * @param task: a pointer to the task_manager struct
 * @param file_path: a pointer to the path of the file
 * @param entry: a pointer to the name of the file
 * RETURNS: NULL
 */
void handle_subdir(task_info *task_inf, task_manager *task, char *file_path, char *entry)
{
	// allocate memory for the new task
	task_info *new_task = malloc(sizeof(task_info));
	if (new_task == NULL)
	{
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	// copy the path to the new task
	new_task->path = malloc((strlen(task_inf->path) + strlen(entry) + 2));
	if (new_task->path == NULL)
	{
		perror("malloc");
		exit(EXIT_FAILURE);
	}
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

/**
 * start_task()- Function that will pop a task from the stack and check
 * if it is a file or directory. If it is a file then calculate the size
 * of the file. If it is a directory then traverse the directory and
 * calculate the size of the files and push subdirectories onto the stack.
 *
 * @param task_inf: a pointer to the task_info struct
 * @param task: a pointer to the task_manager struct
 * @param found: a pointer to the name of the file
 * RETURNS: NULL
 */
void start_task(task_manager *task, int found)
{

	// if the stack is not empty then pop a task from the stack
	found = 1;
	task_info *task_inf;
	// allocate memmory for the copy since pop will free the task
	task_info *task_copy = malloc(sizeof(task_info));
	if (task_copy == NULL)
	{
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	task_inf = stack_top(task->task_stack);
	task_copy->path = strdup(task_inf->path);
	if (task_copy->path == NULL)
	{
		perror("strdup");
		exit(EXIT_FAILURE);
	}
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
	// when done, free the task copy
	free(task_copy->path);
	free(task_copy);
}
