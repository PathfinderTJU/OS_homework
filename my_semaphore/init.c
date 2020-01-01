#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include "myipc.h"

int main(int argc, char *argv[])
{
    	int i,reader_pid,writer_pid,item,shmid;
	semaphore mutex,rdcntmutex;
    	union semun sem_union;
	void *shared_memory = (void *)0;
	struct shared_use_st *shared_stuff;

	if ( (mutex=semget((key_t)KEY_MUTEX,1,0666|IPC_CREAT)) == -1 ) {
		fprintf(stderr,"Failed to create semaphore!"); 
		exit(EXIT_FAILURE);
	}
	if ( (rdcntmutex = semget((key_t)KEY_RDCNTMUTEX,1,0666|IPC_CREAT)) == -1 ) {
		fprintf(stderr,"Failed to create semaphore!"); 
		exit(EXIT_FAILURE);
	}
	if ( (shmid = shmget((key_t)KEY_SHM,sizeof(struct shared_use_st),0666|IPC_CREAT)) == -1 ) {
		fprintf(stderr,"Failed to create shared memory!"); 
		perror("error");
		exit(EXIT_FAILURE);
	}

    	sem_union.val = 1;
    	if (semctl(mutex, 0, SETVAL, sem_union) == -1) {
		fprintf(stderr,"Failed to set semaphore!"); 
		exit(EXIT_FAILURE);
	}

    	sem_union.val = 1;
    	if (semctl(rdcntmutex, 0, SETVAL, sem_union) == -1) {
		fprintf(stderr,"Failed to set semaphore!"); 
		exit(EXIT_FAILURE);
	}

	if ( (shared_memory = shmat(shmid,(void *)0,0) ) == (void *)-1) {
		fprintf(stderr,"shmat failed\n");
		exit(EXIT_FAILURE);
	}
	shared_stuff = (struct shared_use_st *)shared_memory;

	for(i=0;i<BUFFER_SIZE;i++)
	{
		shared_stuff->buffer[i] = i;
	}
	shared_stuff->reader_count = 0;
	
  	exit(EXIT_SUCCESS);
}
