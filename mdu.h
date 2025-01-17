#ifndef __MDU_H
#define __MDU_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <getopt.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>


#define DEFAULT_THREADS 1

typedef struct args args;
typedef struct task_manager task_manager;
typedef struct task_info task_info;

void traverse_directory(task_info *task_inf, task_manager *task);
args *read_args(int argc, char *argv[]);
void *thread_pool(void *arg);
bool is_file(char *path);
void calculate_size(task_info *task_inf, task_manager *task, char *file_path);
task_manager *create_task_manager(args *argument_info);
void free_memory(task_manager *task, args *argument_info);
void create_tasks(task_manager *task, args *argument_info);
void print_result(task_manager *task, args *argument_info);
void handle_subdir(task_info *task_inf, task_manager *task, char *file_path, char* entry);
void start_task(task_manager *task, int found);

#endif