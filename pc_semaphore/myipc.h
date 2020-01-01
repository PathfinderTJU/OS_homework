#define KEY_MUTEX 100
#define KEY_EMPTY 101
#define KEY_FULL 102
#define KEY_SHM	200
#define BUFFER_SIZE 10
typedef int semaphore;
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	/*struct seminfo *__buf;*/
};

/*struct sembuf {
	short sem_num;
	short sem_op;
	short sem_flg;
};*/

/*struct shmid_ds {
	uid_t shm_perm.uid;
	uid_t shm_perm.gid;
	mode_t shm_perm.mode;
};*/

struct shared_use_st {
	int buffer[BUFFER_SIZE];
	//int count;
	int lo;
	int hi;
	int cur;
};

extern int sem_p(semaphore sem_id);
extern int sem_v(semaphore sem_id);
