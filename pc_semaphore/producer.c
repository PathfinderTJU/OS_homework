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
    	int i,item,shmid;
	semaphore mutex,empty,full;
    	union semun sem_union;
	void *shared_memory = (void *)0;
	struct shared_use_st *shared_stuff;

	if ( (mutex=semget((key_t)KEY_MUTEX,1,0666|IPC_CREAT)) == -1 ) {
		fprintf(stderr,"Failed to create semaphore!"); 
		exit(EXIT_FAILURE);
	}
	if ( (empty = semget((key_t)KEY_EMPTY,1,0666|IPC_CREAT)) == -1 ) {
		fprintf(stderr,"Failed to create semaphore!"); 
		exit(EXIT_FAILURE);
	}
	if ( (full = semget((key_t)KEY_FULL,1,0666|IPC_CREAT)) == -1 ) {
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
	for(i=0;i<30;i++)
	{
		//item = produce_item(shared_stuff);
		item = ++(shared_stuff->cur);
		sleep(1);
		printf("Producing item %d\n",item);
		sem_p(empty);
		sem_p(mutex);
		//insert_item(shared_stuff,item);
		(shared_stuff->buffer)[(shared_stuff->hi)]=item;
		(shared_stuff->hi) = ((shared_stuff->hi)+1) % BUFFER_SIZE;
		printf("Inserting item %d\n",item);
		//display_buffer();
		sem_v(mutex);
		sem_v(full);
	}

    	if (shmdt(shared_memory) == -1) {
       		fprintf(stderr, "shmdt failed\n"); 
		exit(EXIT_FAILURE);
	}
	printf("Finish!\n");
	getchar();
  	exit(EXIT_SUCCESS);
}
