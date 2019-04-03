/*					TASK.H					*/

#include<time.h>
#include <pthread.h>
#include <stdio.h>

#define NT	7 // numero di task

struct task_par {
    int     arg;
    long    wcet;
    int     period;
    int     deadline;
    int     priority;
    int     dmiss;
    struct  timespec at;
    struct  timespec dl;
};


struct timespec     start, finish;      //Non utilizzate
time_t              tstart,tend;

struct sched_param	mypar;
struct task_par		tp[NT];
pthread_attr_t		att[NT];
pthread_t		tid[NT];

void time_copy(struct timespec *td, struct timespec ts);

void time_add_ms(struct timespec *t, int ms);

int time_cmp(struct timespec t1, struct timespec t2);


int get_task_index(void* arg);
void set_activation(int i);
int deadline_miss(int i);
void wait_for_activation(int i);

int task_create(
	void*(*task)(void *),
	int i,
	int period,
	int drel,
	int prio
	);

/******************************************* END OF FILE ****************************************************/
