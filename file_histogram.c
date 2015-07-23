#define _GNU_SOURCE
#include <sys/types.h>	// opendir, closedir
#include <dirent.h>		// opendir, closedir
#include <string.h>		// strdup
#include <unistd.h>		// getcwd, chdir
#include <stdio.h>		// perror
#include <stdlib.h>		// exit, calloc
#include <sys/stat.h>	// stat
#include <sys/time.h>	// gettimeofday
#include <getopt.h>		// getopt_long
#include <math.h>		// log

extern const char *__progname;

long long *histogram;	// histogram of file sizes
long long StarAmount = 30LL; // max number of stars in histogram
int histosize = 15;	// size of histogram
double d_histosize; // -//-
double MAXsize;			// max file size

// simple indicator
char *indicator[] = {"|","/","-","\\"};
int indi = 0; // char in indicator
// keys
int
	 follow_links = 0		// follow symbolic links
	,show_histogram = 0		// show histogram
	,FS = 'H'				// file size format (default - human)
	,HS = 'l'				// histogram format (default - linear)
	;
// pointer to lstat or stat (depends on follow_links key)
int (*Stat)(const char *path, struct stat *buf);
// pointer to function, which will make histogram
int (*Hist)(off_t sz);

// file & directories counters
long long fcounter = 0LL, dcounter = 0LL, lcounter = 0LL;
char maxname[PATH_MAX]; // name of biggest file
off_t maxsize = 0; // size of biggest file

// returns current time (double value)
double dtime(){
	double t;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	t = tv.tv_sec + ((double)tv.tv_usec)/1e6;
	return t;
}

// linear histogram
int linhist(off_t sz){
	double s = (double)sz / MAXsize;
	int r = (int)(s * d_histosize-0.5);
	if(r < 0) r = 0;
	return (r > histosize) ? histosize : r;
}

// log histogram
int loghist(off_t sz){
	double s = log((double)sz / MAXsize * (M_E + 1.) + 1.);
	int r = (int)(s * d_histosize - 0.5);
	if(r < 0) r = 0;
	return (r < histosize) ? r : histosize - 1;
}

// help
void usage(){
	printf("\nUsage:\t%s [options] <path to directory>\n", __progname);
	printf("Options:\n");
	printf("\t-h,\t--help\t\t\tshow this help\n");
	printf("\t-l,\t--follow-link\t\tfollow symbolic links\n");
	printf("\t-s,\t--show-histogram\tshow simple histogram of file's size\n");
	printf("\t-L,\t--log\t\t\thistogram in log format\n");
	printf("\tFile size:\n");
	printf("\t-K,\t--kilobytes\t\tSize in Kilobytes\n");
	printf("\t-M,\t--megabytes\t\tSize in Megabytes\n");
	printf("\t-G,\t--gigabytes\t\tSize in Gigabytes\n");
	printf("\t-H,\t--human-readable\t(default) depends on file size\n");
	printf("\n");
	exit(0);
}

// process directory path, oldWD - old working directory
void proc_dir(char *path, char *oldWD){
	DIR *dir;
	struct dirent *de;
	struct stat statbuf;
	char msg[1025];
	char curname[PATH_MAX];
	off_t size;
	printf("work... %s\r", indicator[indi++]); // simple indicator
	if(indi>3) indi = 0;
	fflush(stdout);
	if(chdir(path)){
		perror("Can't change directory");
		return;
	}
	if(!(dir = opendir(path))){
		snprintf(msg, 1024, "Couldn't open dir %s", path);
		perror(msg);
		return;
	}
	while((de = readdir(dir)) != NULL){
		if(!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
		if(path[strlen(path)-1] == '/')
			snprintf(curname, PATH_MAX-1, "%s%s", path, de->d_name);
		else
			snprintf(curname, PATH_MAX-1, "%s/%s", path, de->d_name);
		if(Stat(de->d_name, &statbuf)){
			snprintf(msg, 1024, "Can't STAT %s", curname);
			perror(msg);
			continue;
		}
		if(S_ISLNK(statbuf.st_mode)){
			lcounter++;
			if(!follow_links) continue;
		}
		size = statbuf.st_size;
		if(S_ISDIR(statbuf.st_mode)){
			dcounter++;
			proc_dir(curname, path);
		}else if(S_ISREG(statbuf.st_mode)){
			fcounter++;
			if(size > maxsize){
				maxsize = size;
				strncpy(maxname, curname, PATH_MAX-1);
			}
		}
	}
	if(oldWD) chdir(oldWD);
	closedir(dir);
}

// process directory path to make histogram (without err messages)
void proc_dir_hist(char *path, char *oldWD){
	DIR *dir;
	struct dirent *de;
	struct stat statbuf;
	char curname[PATH_MAX];
	printf("work... %s\r", indicator[indi++]); // simple indicator
	if(indi>3) indi = 0;
	fflush(stdout);
	if(chdir(path)) return;
	if(!(dir = opendir(path))) return;
	while((de = readdir(dir)) != NULL){
		if(!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
		if(Stat(de->d_name, &statbuf)) continue;
		if(!follow_links && S_ISLNK(statbuf.st_mode)) continue;
		if(S_ISDIR(statbuf.st_mode)){
			if(path[strlen(path)-1] == '/')
				snprintf(curname, PATH_MAX-1, "%s%s", path, de->d_name);
			else
				snprintf(curname, PATH_MAX-1, "%s/%s", path, de->d_name);
			proc_dir_hist(curname, path);
		}else if(S_ISREG(statbuf.st_mode)){
			histogram[Hist(statbuf.st_size)]++;
		}
	}
	if(oldWD) chdir(oldWD);
	closedir(dir);
}

double size_in_units(off_t sz, int *unit){
	double s = (double) sz;
	int i = 0;
	switch(FS){
		case 'G':
			s /= 1024.; // giga
			i++;
		case 'M':
			s /= 1024.; // mega
			i++;
		case 'K':
			s /= 1024.; // kilo
			i++;
		break;
		default:
			while(s > 1024. && ++i < 4) s /= 1024.;
	}
	if(i > 3) i = 3;
	if(unit) *unit = i;
	return s;
}

// Prints file size in human readable format (depends on FS key)
char* pretty_size(off_t sz){
	static char buf[256];
	char *units[4] = {"B", "KB", "MB", "GB"};
	int i;
	double s = size_in_units(sz, &i);
	snprintf(buf, 255, "%.3f%s", s, units[i]);
	return buf;
}

void print_stars(long long n, long long MaxNum){
	int i, N = (int)(n * StarAmount / MaxNum);
	for(i = 0; i < N; i++) printf("*");
	printf("\n");
}

void print_histo(){
	int i;
	double histunit = MAXsize / d_histosize;
	long long MaxNum = 0LL; // max number of files
	for(i = 0; i < histosize; i++)
		if(histogram[i] > MaxNum) MaxNum = histogram[i];
	if(Hist == linhist)
		for(i = 0; i < histosize; i++){
			printf("%12s%8lld  ", pretty_size((off_t)(histunit * i)), histogram[i]);
			print_stars(histogram[i], MaxNum);
		}
	else // log format
		for(i = 0; i < histosize; i++){
			double s = (exp((double)i / d_histosize) - 1.) * MAXsize / (M_E + 1.);
			printf("%12s%8lld  ", pretty_size((off_t)s), histogram[i]);
			print_stars(histogram[i], MaxNum);
		}
}

int main(int argc, char **argv){
	char short_options[] = "lhsGHKML";
	struct option long_options[] = {
		{"help",			0, 0, 'h'},
		{"follow-link",		0, 0, 'l'},
		{"show-histogram",	0, 0, 's'},
		{"human-readable",	0, 0, 'H'},
		{"kilobytes",		0, 0, 'K'},
		{"megabytes",		0, 0, 'M'},
		{"gigabytes",		0, 0, 'G'},
		{"log",				0, 0, 'L'},
		{ 0, 0, 0, 0 }
	};
	DIR *dir;
	char *path = NULL, *wd = NULL;
	double Time0 = dtime();
	Stat = lstat;
	Hist = linhist;
	while (1){
		int opt;
		if((opt = getopt_long(argc, argv, short_options,
			long_options, NULL)) == -1) break;
		switch(opt){
			case 'h':
				usage();
			break;
			case 'l':
				follow_links = 1;
				Stat = stat;
			break;
			case 's':
				show_histogram = 1;
			break;
			case 'H': break; // default
			case 'K': case 'M': case 'G':
				FS = opt;
			break;
			case 'L':
				Hist = loghist;
			break;
		default:
		usage();
		}
	}
	d_histosize = (double) histosize;
	argc -= optind;
	argv += optind;
	wd = get_current_dir_name();
	if(argc == 0) path = strdup(wd);
	else path = strdup(argv[0]);
	if(!(dir = opendir(path))){
		perror("Couldn't open dir, try to open CWD");
		if(!(dir = opendir(wd))){
			perror("Couldn't open CWD, die");
			exit(1);
		}else{
			free(path);
			path = strdup(wd);
		}
	}
	closedir(dir);
	proc_dir(path, wd);
	printf("Done!!!  \n");
	if(maxsize){
		printf("\nBiggest file: %s with size=%s\n", maxname, pretty_size(maxsize));
		printf("Processed: %lld files in %lld directories", fcounter, dcounter);
		if(!follow_links) printf("; %lld symlinks dropped\n\n", lcounter);
		else printf("\n\n");
	}else
		printf("\nDirectory %s doesn't contain non-empty files\n", path);
	if(show_histogram){
		histogram = calloc(histosize, sizeof(long long));
		if(!histogram){
			perror("Can't allocate memory for histogram");
			exit(1);
		}
		MAXsize = (double)maxsize;
		printf("Make histogram.\n");
		proc_dir_hist(path, wd);
		printf("Done!!!  \n");
		print_histo();
	}
	printf("Time of work: %.3fs\n", dtime() - Time0);
	free(wd); free(path);
	return 0;
}
