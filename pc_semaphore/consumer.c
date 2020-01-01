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
		sem_p(full);
		sem_p(mutex);
		//item = remove_item();
		item = shared_stuff->buffer[shared_stuff->lo];
		(shared_stuff->buffer)[(shared_stuff->lo)]=0;
		(shared_stuff->lo) = ((shared_stuff->lo)+1) % BUFFER_SIZE;
		printf("Removing item %d\n",item);
		//display_buffer();
		sem_v(mutex);
		sem_v(empty);
		//consume_item(item);
		printf("Consuming item %d\n",item);
		sleep(2);
	}

    	if (shmdt(shared_memory) == -1) {
       		fprintf(stderr, "shmdt failed\n"); 
		exit(EXIT_FAILURE);
	}
	printf("Finish!\n");
	getchar();
  	exit(EXIT_SUCCESS);
}
