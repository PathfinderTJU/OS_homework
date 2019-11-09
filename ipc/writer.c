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
	int type;
	if (argc == 3){
		type = 1;	
	}else if (argc == 1){
		type = 2;	
	}else{
		fprintf(stderr,"Invalid Arguments\n"); 
		exit(EXIT_FAILURE);
	}

    	int i,item,shmid;
	semaphore mutex;
    	union semun sem_union;
	void *shared_memory = (void *)0;
	struct shared_use_st *shared_stuff;

	if ( (mutex=semget((key_t)KEY_MUTEX,1,0666|IPC_CREAT)) == -1 ) {
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

	sem_p(mutex);
	if (type == 1){
		item = atoi(argv[2]);
		i = atoi(argv[1]);
		if(i < 0 || i > 19){
			fprintf(stderr,"out of range\n");
			exit(EXIT_FAILURE);
		}
		shared_stuff->buffer[i] = item;
		printf("Set %d: %d\n", i, item);
		sleep(1);
	}else if (type == 2){
		for(i=0;i<BUFFER_SIZE;i++)
		{
			item = i*3+i;
			shared_stuff->buffer[i] = item;
			printf("Set %d: %d\n", i, item);
			sleep(1);
		}
	}else{
		fprintf(stderr,"Invalid Arguments"); 
		exit(EXIT_FAILURE);	
	}
	sem_v(mutex);

    	if (shmdt(shared_memory) == -1) {
       		fprintf(stderr, "shmdt failed\n"); 
		exit(EXIT_FAILURE);
	}
	printf("Finish!\n");
	getchar();
  	exit(EXIT_SUCCESS);
}
