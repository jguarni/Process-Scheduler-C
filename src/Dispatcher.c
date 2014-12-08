#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sched.h"
#include "dispatchhelper.h"

int time = 0, burstTime;
CPU * cpu;

void CPUmake(char *alg){
	cpu = malloc(sizeof(CPU));
	cpu->alg = alg;
}

void CPUfree(){
	free(cpu);
}
//Statuses
char* getStatus(enum Status status){
	switch(status)
	{
		case idle: return "idle";
		case ready: return "ready";
		case running: return "running";
		case preempted: return "preempt";
		case waiting: return "waiting";
	}
}

// Wait Q traversal
void IOBurstUpdater() {
	PCB* temp = waitQ->head;
	PCB* temp1 = temp;
	while(temp != NULL){
		temp1 = temp;
		*temp->IOBurstcurrent = *temp->IOBurstcurrent-1;
		if (*temp->IOBurstcurrent == 0) { //IO burst is done
			if (temp->BursetRemcurrent != 1) //not last IOburst
				temp->IOBurstcurrent++;
			temp = QRemove(waitQ,temp);
			enQ(queue,temp1);
		}
		else
			temp = temp->next;
	}
}

void ContextS(int halfCS) {
	int i;
	for (i = 0; i < halfCS; i++)
		tick();
}

void CPUexec(PCB *pcb, int halfCS){
	if(cpu->pcb == NULL){
		if (pcb->status == ready)
			ContextS(halfCS);
		else if (pcb->status == idle)
			pcb->RT = getTime() - pcb->arrivalT;
		printf("PROCESS \"P%d\" IS RUNNING AT TIME %d.\n",pcb->pid, getTime());
		cpu->pcb = pcb;
		pcb->status = running;
	}
}

PCB* removeFromCPU(){
	PCB *rem = cpu->pcb;
	cpu->pcb = NULL;
	return rem;
}

void preempt(int halfCS){
	burstTime = 0;
	PCB *rem = removeFromCPU();
	rem->status = preempted;
	enQ(queue,rem);
	ContextS(halfCS);
}

void tick(){
	time++;
	if (!strcmp(cpu->alg, "FCFSP") && cpu->pcb != NULL)
		burstTime++;
	IOBurstUpdater();
}

int getTime(){
	return time;
}

int getBurstTime(){
	return burstTime;
}

void resetBurstTime(){
	burstTime = 0;
}
