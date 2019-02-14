#include<time.h>
#include <pthread.h>
#include <stdio.h>

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
struct task_par		tp[5];
pthread_attr_t		att[5];
pthread_t		tid[5];

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


void set_period(struct task_par *tp);

void wait_for_period(struct task_par *tp);

int deadline_miss_stefano(struct task_par *tp);


