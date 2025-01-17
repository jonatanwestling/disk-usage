#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>


int main(int argc, char *argv[])
{
	struct stat file_stat;
	int a = argc;
	printf("%d ", a);
	stat(argv[1], &file_stat);
	fprintf(stdout, "Size of file '%s': %ld bytes\n", argv[1], file_stat.st_size);
	exit(EXIT_SUCCESS);
}