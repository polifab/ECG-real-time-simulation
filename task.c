#ifndef TASK_H
#define TASK_H
#include "task.h"

struct timespec     start, finish;      //Non utilizzate
time_t              tstart,tend;




void time_copy(struct timespec *td, struct timespec ts) {
    td->tv_sec = ts.tv_sec;
    td->tv_nsec = ts.tv_nsec;
}

void time_add_ms(struct timespec *t, int ms) {
    t->tv_sec += ms/1000;
    t->tv_nsec += (ms%1000)*1000000;
    if (t->tv_nsec >  1000000000) {
        t->tv_nsec -= 1000000000;
        t->tv_sec += 1;
    }
}

int time_cmp(struct timespec t1, struct timespec t2) {
    if (t1.tv_sec > t2.tv_sec)      return 1;
    if (t1.tv_sec < t2.tv_sec)      return -1;
    if (t1.tv_nsec > t2.tv_nsec)    return 1;
    if (t1.tv_nsec < t2.tv_nsec)    return -1;
    return 0;
}



void set_period(struct task_par *tp) {
    struct timespec     t;

    clock_gettime(CLOCK_MONOTONIC, &t);
    time_copy(&(tp->at), t);
    time_copy(&(tp->dl), t);
    time_add_ms(&(tp->at), tp->period);
    time_add_ms(&(tp->dl), tp->deadline);
}

void  wait_for_period(struct task_par *tp){
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &(tp->at), NULL);

    time_add_ms(&(tp->at), tp->period);
    time_add_ms(&(tp->dl), tp->period);
}

int deadline_miss_stefano(struct task_par *tp) {
    struct timespec     now;

    clock_gettime(CLOCK_MONOTONIC, &now);
    if (time_cmp(now, tp->dl) > 0) {
        tp->dmiss++;
        return 1;
    }
    return 0;
}


void set_activation(int i)
{

struct timespec t;

	clock_gettime(CLOCK_MONOTONIC, &t);
	time_copy(&(tp[i].at), t);
	time_copy(&(tp[i].dl), t);
	time_add_ms(&(tp[i].at), tp[i].period);
	time_add_ms(&(tp[i].dl), tp[i].deadline);
}

int get_task_index(void* arg)
{
struct task_par *tp;
tp = (struct task_par *)arg;
return tp->arg;
}


int deadline_miss(int i)
{
struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	if (time_cmp(now, tp[i].dl) > 0) {
		tp[i].dmiss++;
		return 1;
	}
	return 0;
}

void wait_for_activation(int i)
{
	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &(tp[i].at), NULL);
 
	time_add_ms(&(tp[i].at), tp[i].period);
	time_add_ms(&(tp[i].dl), tp[i].period);
}

int task_create(void*(*task)(void *), int	i, int	period, int	drel, int	prio)
{

pthread_attr_t	myatt;
struct	sched_param mypar;

int	tret;

	tp[i].arg = i;
	tp[i].period = period;
	tp[i].deadline = drel;
	tp[i].priority = prio;
	tp[i].dmiss = 0;
	int err;
	err = pthread_attr_init(&myatt);
	if(err != 0){ printf("ERROR INIT = %d", err);}
	err = pthread_attr_setinheritsched(&myatt, PTHREAD_EXPLICIT_SCHED);
	if(err != 0){ printf("ERROR SCHED = %d", err);}
	err = pthread_attr_setschedpolicy(&myatt, SCHED_FIFO);
	if(err != 0){ printf("ERROR POLICY = %d", err);}
	mypar.sched_priority = tp[i].priority;
	//printf("priority = %d\n", tp[i].priority);
	err = pthread_attr_setschedparam(&myatt, &mypar);
	if(err != 0){ printf("ERROR PARAM = %d", err);}
	printf("Creazione thread\n");
	if(tret = pthread_create(&tid[i], &myatt, task, (void*)(&tp[i]))){
	
		fprintf(stderr, "Error creating thread, cod = %d\n",tret);
		return 1;

	}
	return tret;
}

/*
void startTimer() {
    clock_gettime(CLOCK_REALTIME, &start); 
}

long getTimer() {
    clock_gettime(CLOCK_REALTIME, &finish); 

    long seconds = finish.tv_sec - start.tv_sec; 
    long ns = finish.tv_nsec - start.tv_nsec; 

    if (start.tv_nsec > finish.tv_nsec) {
        --seconds; 
        ns += 1000000000; 
    } 
//    printf("\t\tseconds without ns: %ld\n", seconds); 
//    printf("\t\tnanoseconds: %ld\n", ns);
    return ns;
}


void _startTimer() {
    tstart = time(NULL);
}

void _getTimer() {
    tend=time(NULL);
//    printf("\nHo finito, ho impiegato %f secondi ", difftime(tend,tstart));
}
*/
#endif
