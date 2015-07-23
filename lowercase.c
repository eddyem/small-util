#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#define BUFFSIZE 1024

char *file;
char file1[BUFFSIZE];
char *basename;
char *newfile;


char to_lower(char ch){
    int ret = (int)ch;
    if (isupper(ret))
	ret = tolower(ret);
    if (ret > 223)
	ret = ret - 32;
    if (ret == 179)
	ret = 163;
    return (char)ret;
}


void usage(int err)
{
    switch (err){
    case 0: 	printf("\tYou have missed filename\n");
		break;
    case 1: 	printf("\tFile %s dosen't exists or permissions denied\n", file);
		break;
    default:	printf("\tToo many parameters\n");
		break;
    }
    printf("\t\tUsage:\n\t%s <filename>\n", basename);
    exit (1);
}

int main(int argc, char* argv[])
{
    basename = argv[0];
    if (argc < 2) usage(0);
    file = strdup(argv[1]);
    newfile = file;
    strncpy(file1, file, BUFFSIZE-1);
    if (argc > 2) usage(2);
    while (*file++)
    	*file = to_lower(*file);
    link(file1, newfile);
    unlink(file1);
    return 0;
}

