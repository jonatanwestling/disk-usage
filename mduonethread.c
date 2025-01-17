#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <getopt.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

typedef struct args
{
	char **files;
	int num_files;
	int threads;
} args;



// int calculate_disk_usage();
int traverse_directory(const char *path, long long *total_size);
int file_or_directory(const char *path);
args *read_args(int argc, char *argv[]);
void print_info(args *argument_info);

int main(int argc, char *argv[])
{
	args *argument_info;
	argument_info = read_args(argc, argv);
	print_info(argument_info);
	long long total_size = 0;
	for (int i = optind; i < argc; i++)
	{
		traverse_directory(argv[i], &total_size);
		printf("Total size: %lli\n", total_size);
		total_size = 0;
	}

	
	return 0;
}


int traverse_directory(const char *path, long long *total_size)
{
	// open the directory
	DIR *dir;
	struct dirent *entry;
	struct stat file_stat;
	dir = opendir(path);
	if (dir == NULL)
	{
		perror("opendir");
		exit(EXIT_FAILURE);
	}

	while ((entry = readdir(dir)) != NULL)
	{
		char *file_path = malloc(sizeof(char) * (strlen(path) + strlen(entry->d_name) + 2));
		if (file_path == NULL)
		{
			perror("malloc failed");
			exit(EXIT_FAILURE);
		}

		sprintf(file_path, "%s/%s", path, entry->d_name);

		if (stat(file_path, &file_stat) == -1)
		{
			perror("stat failed");
			exit(EXIT_FAILURE);
		}

		if (S_ISDIR(file_stat.st_mode))
		{
			//kolla "."
			if (strcmp(entry->d_name, ".") == 0 )
			{
				*total_size += file_stat.st_blocks;
				continue;
			}
			
			if(strcmp(entry->d_name, "..") == 0)
			{
				continue;
			}

			traverse_directory(file_path, total_size);
		}
		else
		{
			//printf("%s\n", file_path);
			//printf("%ld\n\n", file_stat.st_blocks);
			*total_size += file_stat.st_blocks;
		}
	}

	return 0;
}
// allocate memmory for a struct that can hold necessary information about the arguments
args *read_args(int argc, char *argv[])
{
	// allocate memory for the struct
	args *argument_info = malloc(sizeof(args));
	if (argument_info == NULL)
	{
		perror("malloc");
		exit(EXIT_FAILURE);
	}
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

	int array_size = 0;
	int i = optind;
	while (i < argc)
	{
		array_size++;
		argument_info->files = realloc(argument_info->files, sizeof(char *) * array_size);
		if (argument_info->files == NULL)
		{
			perror("realloc");
			exit(EXIT_FAILURE);
		}
		argument_info->files[argument_info->num_files] = argv[i];
		argument_info->num_files++;
		i++;
	}
	return argument_info;
}

void print_info(args *argument_info)
{
	printf("Number of files: %d\n", argument_info->num_files);
	printf("Number of threads: %d\n", argument_info->threads);
	printf("________FILES________\n");

	for (int i = 0; i < argument_info->num_files; i++)
	{
		printf("file %d: %s\n", i, argument_info->files[i]);
	}
}