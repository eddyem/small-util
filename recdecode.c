#define _FILE_OFFSET_BITS	64
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

#ifdef EBUG
	#define DBG(N, msg) fprintf(stderr, "%s: %s %d\n", __func__, msg, N)
#else
	#define DBG(N, msg) {}
#endif

#define LOCK(name, no) do{				\
		DBG(no, #name);					\
		pthread_mutex_lock(& name[no]);	\
		DBG(no, "locked");				\
	}while(0)

#define UNLOCK(name, no) do{				\
		DBG(no, #name);						\
		pthread_mutex_unlock(& name[no]);	\
		DBG(no, "unlocked");				\
	}while(0)

#ifndef BUF_SIZ
#define BUF_SIZ 1048576L
#endif
#ifndef NBUF
#define NBUF	8
#endif

pthread_mutex_t r_mutex[NBUF], w_mutex[NBUF];

int out;
struct sbuf{
	unsigned char *data;
	ssize_t len;
	int bufno;
}s_buf[NBUF];
char end = 0;

void *write_data(){
	int i = 0;
	while(s_buf[0].len == 0){
		DBG(s_buf[0].len, "buflen");
		#ifdef EBUG
		sleep(1);
		#endif
	};
	DBG(i, "begin");
	while(1){
		LOCK(w_mutex, i);
		if(end && s_buf[i].len == 0){
			UNLOCK(r_mutex, i);
			UNLOCK(w_mutex, i);
			return NULL;
		}
		write(out, s_buf[i].data, s_buf[i].len);
		s_buf[i].len = 0;
		UNLOCK(r_mutex, i);
		UNLOCK(w_mutex, i);
		i++;
		if(i == NBUF) i = 0;
	}
	return NULL;
}

void *xor_data(void *buffer){
	struct sbuf *buf = (struct sbuf*) buffer;
	int n = buf->bufno;
	unsigned char *ptr = buf->data;
	ssize_t i, len = buf->len;
	LOCK(r_mutex, n);
	for(i = 0; i < len; i++) *ptr++ ^= 0xaa;
	UNLOCK(w_mutex, n);
	return NULL;
}

int main(int argc, char **argv){
	int i, j, l, in;
	char not_first = 0;
	ssize_t rb;
	pthread_t w_thread, d_thread[NBUF];
	if(argc < 3){
		printf("\nUsage:\t%s <file.rec> <file.001> ... <file.avi>\n\t\t\"декодирует\" rec-файл в выходной avi-файл\n", argv[0]);
		exit(-1);
	}
	for(i = 0; i < NBUF; i++){
		s_buf[i].data = malloc(BUF_SIZ * sizeof(char));
		s_buf[i].len = 0;
		s_buf[i].bufno = i;
	}
	out = open(argv[argc - 1], O_RDWR|O_CREAT|O_TRUNC, 0666);
	l = argc - 1;
	i = 0;
	pthread_create(&w_thread, NULL, write_data, NULL);
	for(j = 1; j < l; j++){
		in = open(argv[j], O_RDONLY);
		do{
			if(not_first)
				if(pthread_join(d_thread[i], NULL)){
					perror("\n\nCan't join thread");
				}
			LOCK(r_mutex, i);
			LOCK(w_mutex, i);
			rb = read(in, s_buf[i].data, BUF_SIZ);
			s_buf[i].len = rb;
			UNLOCK(r_mutex, i);
			if(pthread_create(&d_thread[i], NULL, xor_data, &s_buf[i])){
				perror("\n\nCan't create thread");
				exit(-1);
			}
			DBG(i, "thread created");
			i++; if(i == NBUF){ i = 0; not_first = 1;}
		}while(rb);
		DBG(j, "EOF");
		close(in);
	}
	end = 1;
	DBG(1, "wait 4 xor");
	for(i = 0; i < NBUF; i++) pthread_join(d_thread[i], NULL);
	DBG(1, "xor ends");
	pthread_join(w_thread, NULL);
	close(out);
	for(i = 0; i < NBUF; i++) free(s_buf[i].data);
	return 0;
}
