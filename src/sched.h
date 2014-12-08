#ifndef SCHEDULER_H_
#define SCHEDULER_H_
#define MAX_BURSTS 64

typedef struct PCB PCB;
typedef struct Queue Queue;

struct PCB{
	int pid;
	enum Status{
		idle,
		ready,
		running,
		preempted,
		waiting
	} status;
	int arrivalT;
	int CPUBurst[MAX_BURSTS];
	int IOburst[MAX_BURSTS];
	int *CPUBurstcurrent;
	int *IOBurstcurrent;
	int BursetRemcurrent;
	int tBurstTime;
	int TAT; //turn around time
	int wt; //wait time
	int IOWait; //IO
	int RT; // response time
	PCB *next;
    PCB *prev;
};

struct Queue{
	PCB *head;
	PCB *tail;
};

extern Queue* queue;
extern Queue* waitQ;

void enQ(Queue *queue, PCB *item);
PCB* deQ(Queue *queue);
PCB* QRemove(Queue *queue, PCB *pcb);
int getQuantum();

#endif
