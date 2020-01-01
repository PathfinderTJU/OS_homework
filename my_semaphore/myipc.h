#define KEY_MUTEX 100
#define KEY_RDCNTMUTEX 101
#define KEY_SHM	200
#define BUFFER_SIZE 20

typedef int semaphore;
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	/*struct seminfo *__buf;*/
};

struct shared_use_st {
	int reader_count;
	int buffer[BUFFER_SIZE];
};

extern int sem_p(semaphore sem_id);
extern int sem_v(semaphore sem_id);
