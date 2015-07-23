#define _FILE_OFFSET_BITS	64
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define CACHE_FILE	"/tmp/.parsesquid_cache"
#define LOG_FILE	"/var/log/squid/access.log"
#define TIME_INTERVAL 25000

#define NEW		0
#define UPDATE	1

struct idx{
	int time;
	off_t offset;
} s_cache;

void help(const char* name){
	printf("\nИспользование:\n%s [Ключи]\nВыдает информацию о трафике, полученном извне и из кеша прокси\n", name);
	printf("\t-h\t--help\t\t\tЭто сообщение\n");
	printf("\t-f\t--from ДАТА\t\tУказать дату, с которой необходимо начать подсчет\n");
	printf("\t-t\t--to ДАТА\t\tУказать дату, на которой необходимо закончить подсчет\n");
	printf("\t-a\t--address addr\t\tУказать часть имени сайта, для которого производить подсчет\n");
	printf("\t-u\t--update\t\tОбновить индексный файл\n");
	printf("\t-n\t--new\t\tЗаново создать индексный файл\n");
	printf("\nФормат даты: Д/М/Г-ч:м, Д/М/Г, ч:м, Д/М, М\n\n");
	exit(0);
}

int get_date(char *line){
	time_t date;
	struct tm time_, time_now;
	time(&date);
	time_now = *localtime(&date);
	time_.tm_sec = 0;
	if(sscanf(line, "%d/%d/%d-%d:%d", &time_.tm_mday, &time_.tm_mon, &time_.tm_year, 
		&time_.tm_hour, &time_.tm_min) == 5){time_.tm_mon -= 1;}
	else if(!strchr(line, ':') && sscanf(line, "%d/%d/%d", &time_.tm_mday, &time_.tm_mon, &time_.tm_year) == 3){
		date = -1; time_.tm_mon -= 1;}
	else if(!strchr(line, ':') && sscanf(line, "%d/%d", &time_.tm_mday, &time_.tm_mon) == 2){
		date = -1; time_.tm_mon -= 1; time_.tm_year = time_now.tm_year;}
	else if(sscanf(line, "%d:%d", &time_.tm_hour, &time_.tm_min) == 2){
		time_.tm_year = time_now.tm_year; time_.tm_mon = time_now.tm_mon;
		time_.tm_mday = time_now.tm_mday;}
	else if(!strchr(line, ':') && !strchr(line, '/') && !strchr(line, '.') && !strchr(line, '-')
			&& sscanf(line, "%d", &time_.tm_mon) == 1){
		date = -1; time_.tm_mon -= 1; time_.tm_year = time_now.tm_year;
		time_.tm_mday = 1;}
	else{
		printf("\nНеверный формат времени!\n");
		printf("Форматы: D/M/Y-hh:mm, D/M/Y, hh:mm, D/M, M\n");
		exit(1);
	}
	if(date == -1){
		time_.tm_hour = 0;
		time_.tm_min = 0;		
	}
	if(time_.tm_mon > 11 || time_.tm_mon < 0){
		printf("\nМесяц вне диапазона 1..12\n");
		exit(2);
	}
	if(time_.tm_mday > 31 || time_.tm_mday < 1){
		printf("\nЧисло месяца вне диапазона 1..31, %d\n", time_.tm_mday);
		exit(3);
	}
	if(time_.tm_year > 1900) time_.tm_year -= 1900;
	else if(time_.tm_year > -1 && time_.tm_year < 100) time_.tm_year += 100;
	else if(time_.tm_year < 0 || time_.tm_year > 200){
		printf("\nНеверный формат года %d\n", time_.tm_year);
		exit(4);
	}
	if(time_.tm_hour > 23 || time_.tm_hour < 0){
		printf("\nВремя вне диапазона 0..23 часа\n");
		exit(5);
	}
	if(time_.tm_min > 59 || time_.tm_min < 0){
		printf("\nВремя вне диапазона 0..59 минут\n");
		exit(6);
	}
	date = mktime(&time_);
#ifdef DEBUG
	printf("date: %d\n", date);
#endif
	return (int)date;
}

void makecache(unsigned char flag){
	int f_log, cache = open(CACHE_FILE, O_CREAT|O_RDWR, 00644);
	off_t offset = 0, tmp = 0;
	if(cache < 0){
		printf("\nНе могу создать индекс-файл\n");
		exit(7);
	}
	if(flag == UPDATE){
		if((tmp = lseek(cache, -((off_t)(sizeof(struct idx))), SEEK_END)) > 0){
			if(read(cache, &s_cache, sizeof(struct idx)) > 0){
				offset = s_cache.offset;
			}
		}
#ifdef EBUG
			printf("off=%lld\n", offset);
#endif
	}
	off_t len = 0;
	char *string = NULL;
	struct stat filestat;
	FILE *log = fopen(LOG_FILE, "r");
	if(!log){
		printf("\nНе могу открыть " LOG_FILE " \n");
		exit(8);
	}
	f_log = fileno(log);
	if(flag == NEW) printf("\nСоздаю индексный файл\n");
	else printf("\nОбновляю индексный файл\n");
	if(stat(LOG_FILE, &filestat) != 0){
		printf("\nОшибка, " LOG_FILE ": не могу сделать stat\n");
		exit(10);
	}
	if(lseek(f_log, offset, SEEK_SET) != 0){
		printf("\nВнимание: " LOG_FILE " устарел, обновляю индексный файл полностью\n");
		offset = 0;
	}
	s_cache.offset = offset;
	if(getline(&string, (size_t*)&len, log) < 1){
		printf("\nОшибка: " LOG_FILE "пуст\n");
		exit(9);
	}
	off_t dataportion = filestat.st_size / 100;
	int indpos = 1;
	int frac = 0;
	if(offset > 0) frac = atoi(string) / TIME_INTERVAL;
	do{
		
		if( ( tmp = ((s_cache.time = atoi(string)) / TIME_INTERVAL)) != frac ){
			write(cache, &s_cache, sizeof(struct idx));
#ifdef DEBUG
			printf("очередная строка, время: %d, смещение: %lld, sizeof(s_cache)=%d\n", s_cache.time, s_cache.offset, sizeof(struct idx));
#endif
			frac = tmp;
		}
		s_cache.offset = lseek(f_log, 0, SEEK_CUR);
		if( (tmp = s_cache.offset / dataportion) > indpos ){
			if( (tmp % 10) == 0) printf(" %lld%% ", (long long)tmp);
			else printf(".");
			indpos = tmp;
			fflush(stdout);
		}
	} while(getline(&string, (size_t*)&len, log) > 0);
	printf("\nГотово!\n");
	free(string);
	close(cache);
	fclose(log);
}

int count_bytes(off_t from_offset, off_t to_offset, int from_date, int to_date, char* addr){
	off_t dataportion = (to_offset - from_offset) / 100, tmp, indpos = 0;
	long long bytes_from_outside = 0, bytes_from_cache = 0;
	char* string = NULL;
	off_t len = 0, curpos;
	int time, bytes;
	FILE *log = fopen(LOG_FILE, "r");
	int f_log = fileno(log);
	lseek(f_log, from_offset, SEEK_SET);
	while(getline(&string, (size_t*)&len, log) > 0){
		time = atoi(string);
		curpos = lseek(f_log, 0, SEEK_CUR) - from_offset;
		if( (tmp = curpos / dataportion) > indpos){
			if( (tmp % 10) == 0) printf(" %lld%% ", (long long)tmp);
			else printf(".");
			indpos = tmp;
			fflush(stdout);		
		}
		if(time < from_date) continue;
		else if(time > to_date) break;
		sscanf(string, "%*s %*s %*s %*s %d",  &bytes);
		if(addr)
			if(!strcasestr(string, addr)) continue;
		if(strstr(string, "NONE")) bytes_from_cache += bytes;
		else bytes_from_outside += bytes;
	}
	if(addr) printf("\nПоиск по подстроке URI:  \"%s\"", addr);
	printf("\nПолучено информации\n\t\tиз мира: %lld байт (%.2f МБ);\n\t\tиз кэша: %lld байт (%.2f МБ)\n",
		bytes_from_outside, (double)bytes_from_outside/1024./1024., bytes_from_cache,
			(double)bytes_from_cache/1024./1024.);
	free(string);
	fclose(log);
	return(time);
}

int main(int argc, char** argv){
	int from_date = 0, to_date = INT_MAX, last_cache_time, last_log_time;
	struct stat filestat;
	char* const short_options = "hf:t:una:";
	char *addr = NULL;
	struct option long_options[] = {
		{ "help",	0,	NULL,	'h'},
		{ "from",	1,	NULL,	'f'},
		{ "to",		1,	NULL,	't'},
		{ "update",	0,	NULL,	'u'},
		{ "new",	0,	NULL,	'n'},
		{ "address",	1,	NULL,	'a'},
		{ NULL,		0,	NULL,	0  }
	};
	int next_option;
	do{
		next_option = getopt_long(argc, argv, short_options, long_options, NULL);
		switch(next_option){
			case 'h': help(argv[0]);
			break;
			case 'f': from_date = get_date((char*)optarg);
			break;
			case 't': to_date = get_date((char*)optarg);
			break;
			case 'u': makecache(UPDATE); exit(0);
			break;
			case 'n': makecache(NEW); exit(0);
			break;
			case 'a': addr = strdup(optarg);
			break;
		}
	}while(next_option != -1);
	if(stat(CACHE_FILE, &filestat) != 0) makecache(NEW);
	else if(filestat.st_size == 0) makecache(NEW);
	int cache = open(CACHE_FILE, O_RDONLY);
	off_t bytes_read;
	unsigned char find_from = 1;
	off_t from_offset = 0, to_offset = 0;
	bytes_read = read(cache, &s_cache, sizeof(s_cache));
#ifdef DEBUG
	printf("\nfirst, time=%d, from=%lld\n", s_cache.time, s_cache.offset);
#endif
	while(bytes_read == sizeof(struct idx)){
#ifdef DEBUG
		printf("\ntime=%d, from=%lld, idxpos=%lld\n", s_cache.time, s_cache.offset, lseek(cache, 0, SEEK_CUR));
#endif
		if( (last_cache_time = s_cache.time) >= from_date && find_from){
			find_from = 0;
			from_offset = s_cache.offset;
		}
		else if(s_cache.time >= to_date)
			to_offset = s_cache.offset;
		bytes_read = read(cache, &s_cache, sizeof(struct idx));
	}
	close(cache);
	if(find_from){
		printf("\nНачальная дата поиска старше последней записи логов\n");
		exit(11);
	}
	if(to_offset == 0){
		stat(LOG_FILE, &filestat);
		to_offset = filestat.st_size;
	}
	if(to_offset <= from_offset){
		printf("\nНеверно выбраны начало и конец диапазона дат,\n"
			"\tлибо сведения за указанный период отсутствуют\n");
			exit(12);
	}
	printf("\nПровожу подсчет (от %lld до %lld)\n", (long long)from_offset, (long long)to_offset);
	printf("\n====================================================================\n");
	last_log_time = count_bytes(from_offset, to_offset, from_date, to_date, addr);
	printf("\n====================================================================\n");
	if(last_log_time - last_cache_time > TIME_INTERVAL){
		printf("\nЗаписи файла кеша устарели. Обновляю...\n");
		makecache(UPDATE);
//		count_bytes(from_offset, to_offset, from_date, to_date);
	}
	exit(0);
}
