#include <unistd.h>
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
	int type;
	if (argc == 2){
		type = 1;	
	}else if (argc == 1){
		type = 2;	
	}else{
		fprintf(stderr,"Invalid Arguments\n"); 
		exit(EXIT_FAILURE);
	}

    	int i,item,shmid;
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
		exit(EXIT_FAILURE);
	}

	if ( (shared_memory = shmat(shmid,(void *)0,0) ) == (void *)-1) {
		fprintf(stderr,"shmat failed\n");
		exit(EXIT_FAILURE);
	}
	shared_stuff = (struct shared_use_st *)shared_memory;
	
	sem_p(rdcntmutex);
	if (shared_stuff->reader_count == 0)
	{
		sem_p(mutex);
	}
	shared_stuff->reader_count = shared_stuff->reader_count + 1;
	sem_v(rdcntmutex);

	if (type == 1){
		i = atoi(argv[1]);
		if (i < 0 || i > 19){
			fprintf(stderr,"out of range\n");
			exit(EXIT_FAILURE);
		}
		item = shared_stuff->buffer[i];
		sleep(1);
		printf("Data %d: %d\n",i, item);
	}else if (type == 2){
		for(i=0;i<BUFFER_SIZE;i++)
		{
			item = shared_stuff->buffer[i];
			sleep(1);
			printf("Data %d: %d\n",i, item);

		}
	}else{
		fprintf(stderr,"invalid arguments\n");
		exit(EXIT_FAILURE);
	}
	
	sem_p(rdcntmutex);
	shared_stuff->reader_count = shared_stuff->reader_count - 1;
	if (shared_stuff->reader_count == 0)
	{
		sem_v(mutex);
	}
	sem_v(rdcntmutex);

    	if (shmdt(shared_memory) == -1) {
       		fprintf(stderr, "shmdt failed\n"); 
		exit(EXIT_FAILURE);
	}
	printf("Finish!\n");
	getchar();
  	exit(EXIT_SUCCESS);
}
