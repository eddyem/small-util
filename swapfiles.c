#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
//#define __USE_MISC
#include <sys/shm.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, char** argv){
	int w1, w2, r1, r2, bytes1, bytes2, bytes;
	int memsize, id1, id2, Mb = 1024*1024;
	key_t key1, key2;
	unsigned char *data1, *data2;
	if(argc < 3){
		printf("\nUsage: %s <file1> <file2> [mem]\n", argv[0]);
		printf("\tswaps data in files <file1> & <file2>\n");
		printf("mem - shared memory segments size in Mb (by default: 100)\n\n");
		exit(1);
	}
	w1=open(argv[1], O_WRONLY);
	r1=open(argv[1], O_RDONLY);
	if(w1 < 1 || r1 < 1){
		perror("Cant open file1");
		exit(1);
	}
	w2=open(argv[2], O_WRONLY);
	r2=open(argv[2], O_RDONLY);
	if(w2 < 1 || r2 < 1){
		perror("Cant open file2");
		exit(1);
	}
	if(argc == 4) memsize = atoi(argv[3]);
	else memsize = 100;
	memsize *= Mb;
	key1 = ftok(argv[1], 22);
	key2 = ftok(argv[2], 22);
	id1 = shmget(key1, memsize, IPC_CREAT|0644|SHM_DEST);
	id2 = shmget(key2, memsize, IPC_CREAT|0644|SHM_DEST);
	if(id1 < 0 || id2 < 0){
		perror("Can't get shared memory");
		goto ex;
	}
	data1 = (unsigned char*)shmat(id1, NULL, 0);
	data2 = (unsigned char*)shmat(id2, NULL, 0);
	if(data1 == ((void*)-1) || data2 == ((void*)-1)){
		perror("Can't attach shared memory");
		exit(1);
	}
	do{
		bytes1 = read(r1, data1, memsize);
		bytes2 = read(r2, data2, memsize);
		if(bytes1 < 0 || bytes2 < 0){
			perror("Can't read data from files");
			goto ex;
		}
		bytes = (bytes1 > bytes2) ? bytes2:bytes1;
		bytes1 = write(w1, data2, bytes);
		bytes2 = write(w2, data1, bytes);
		if(bytes1 != bytes || bytes2 != bytes){
			perror("Can't write data to files");
			goto ex;
		}
	}while(bytes == memsize);
ex:
	shmdt(data2);
	shmctl(id2, IPC_RMID, NULL);
	shmdt(data1);
	shmctl(id1, IPC_RMID, NULL);
	return 0;
}
