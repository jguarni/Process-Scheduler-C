#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "sched.h"
#include "dispatchhelper.h"

static char *usage = "Usage: sched <input file> <scheduling algorithm> <scheduling parameters>\n\t Supported schedulers:\n\t\t 1) FCFSP (preemptive) - Parameters: Quantum, Cost of half C.S.\n\t\t 2) FCFSNP (non-preemptive) - Parameters: Cost of half C.S.";
static FILE *inputFile;
static char *alg;
static int quantum = -1;
static int halfCS;
int startTime = 0;
Queue *queue;
Queue *waitQ;

// Variable Declarations
float tCPUtime = 0;
float tTAT = 0;
float twait = 0;
float tresponse = 0;
float tIO = 0;
int maxTAT = 0;
int maxWAIT = 0;
int maxRESPONSE = 0;
int maxIO = 0;


//Parse Input file (text)
void parser(int* args, char* line){
	char *token = NULL;
	char delims[] = {" \n"};
	int i = 0;
	token = strtok(line, delims);
	while(token != '\0'){
		args[i++] = atoi(token);
		token = strtok(NULL, delims);
	}
}

//Create PCB
PCB* createPCB(int pid, int *args){
	int i, j = 0;
	PCB *pcb;
	pcb = malloc(sizeof(PCB));

	pcb->pid = pid;
	pcb->status = idle;
	pcb->arrivalT = args[0];
	pcb->BursetRemcurrent = args[1];
	for (i = 2; i < (2*(pcb->BursetRemcurrent))+1; i++){
		if (i%2 == 0){ //CPU bursts
			pcb->CPUBurst[j] = args[i];
			pcb->tBurstTime = pcb->tBurstTime + args[i];
			tCPUtime = tCPUtime + args[i];
		}
		else { //IO bursts
			pcb->IOburst[j] = args[i];
			pcb->IOWait = pcb->IOWait + args[i];
			tIO = tIO + args[i];
			if (tIO > maxIO)
				maxIO = tIO;
			j++;
		}
	}
	pcb->CPUBurstcurrent = &(pcb->CPUBurst[0]);
	pcb->IOBurstcurrent = &(pcb->IOburst[0]);
	pcb->next = NULL;
	pcb->prev = NULL;

	return pcb;
}

void enQ(Queue *queue
		, PCB *item) {
	if (item->status == idle) // On Queue for first time?
		printf("PROCESS \"P%d\" HAS ARRIVED AT TIME %d.\n", item->pid, getTime());
	else if (item->status == running){
		printf("PROCESS \"P%d\" IS WAITING AT TIME %d.\n", item->pid, getTime());
		item->status = waiting;
	}
	else if (item->status == preempted){
		printf("Process \"P%d\" was preempted, and returned to ready state at time %d.\n",item->pid, getTime());
		item->status = ready;
	}
	else { //wait over
		printf("Process \"P%d\" has completed IO and returned to ready state at time %d.\n", item->pid, getTime());
		item->status = ready;
	}
	if (queue->head == NULL)
		queue->head = queue->tail = item;
	else {
		queue->head->prev = item;
		item->next = queue->head;
		queue->head = item;
		item->prev = NULL;
	}
}

PCB* deQ(Queue *queue){
	PCB *obj = queue->tail;
	if (queue->head == NULL)
		return NULL;
	else if (queue->head == queue->tail) {
		queue->head = queue->tail = NULL;
		return obj;
	}
	else {
		queue->tail->prev->next = NULL;
		queue->tail = queue->tail->prev;
		obj->prev = obj->next = NULL;
		return obj;
	}
}

PCB* QRemove(Queue *queue, PCB *pcb) /*Removes pid specified process from queue */ {
	PCB *ret = pcb;
	if(queue->head == NULL || pcb == NULL)
		ret = NULL;
	else if(queue->head->pid == pcb->pid && queue->tail->pid == pcb->pid){
		queue->head = queue->tail = NULL;
		ret = NULL;
	}
	else if(queue->head->pid == pcb->pid){
		queue->head = pcb->next;
		queue->head->prev = NULL;
		ret = NULL;
	}
	else if(queue->tail->pid == pcb->pid) {
		queue->tail = pcb->prev;
		queue->tail->next = NULL;
		ret = pcb->prev;
	}
	else{
		pcb->next->prev = pcb->prev;
		pcb->prev->next = pcb->next;
		ret = pcb->prev;
	}
	pcb->next = NULL;
	pcb->prev = NULL;
	return ret;
}

void NPrempt(){
	PCB* pcb = cpu->pcb;
	if (pcb == NULL)
		CPUexec(deQ(queue),halfCS);
	else {
		*pcb->CPUBurstcurrent = *pcb->CPUBurstcurrent-1;
		if (*pcb->CPUBurstcurrent == 0) { //burst complete
			pcb->BursetRemcurrent = pcb->BursetRemcurrent-1;
			if (pcb->BursetRemcurrent > 0) {
				pcb->CPUBurstcurrent++;
				enQ(waitQ,removeFromCPU());
				ContextS(halfCS);
			}
			else { //finished process
				pcb->TAT = getTime() - pcb->arrivalT;
				pcb->wt = pcb->TAT - pcb->tBurstTime;
				tTAT = tTAT + pcb->TAT;
				twait = twait + pcb->wt;
				tresponse = tresponse + pcb->RT;
				if (pcb->TAT > maxTAT)
					maxTAT = pcb->TAT;
				if (pcb->wt > maxWAIT)
					maxWAIT = pcb->wt;
				if (pcb->RT > maxRESPONSE)
					maxRESPONSE = pcb->RT;
				printf("Process \"P%d\" terminated at time %d: TAT=%d, Wait Time=%d, I/O Wait=%d, Response Time=%d\n",pcb->pid,getTime(),pcb->TAT,pcb->wt,pcb->IOWait,pcb->RT);
				free(removeFromCPU());
				ContextS(halfCS);
			}
			if (queue->head != NULL)
				CPUexec(deQ(queue),halfCS);
		}
	}
}

void Preempt(){
	PCB* pcb = cpu->pcb;
	if (pcb == NULL)
		CPUexec(deQ(queue),halfCS);
	else {
		*pcb->CPUBurstcurrent = *pcb->CPUBurstcurrent-1;
		if (*pcb->CPUBurstcurrent == 0) {
			pcb->BursetRemcurrent = pcb->BursetRemcurrent-1;
			if (pcb->BursetRemcurrent > 0) {
				pcb->CPUBurstcurrent++;
				enQ(waitQ,removeFromCPU());
				ContextS(halfCS);
				resetBurstTime();
			}
			else {
				pcb->TAT = getTime()- pcb->arrivalT;
				pcb->wt = pcb->TAT - pcb->tBurstTime;
				tTAT = tTAT + pcb->TAT;
				twait = twait + pcb->wt;
				tresponse = tresponse + pcb->RT;
				if (pcb->TAT > maxTAT)
					maxTAT = pcb->TAT;
				if (pcb->wt > maxWAIT)
					maxWAIT = pcb->wt;
				if (pcb->RT > maxRESPONSE)
					maxRESPONSE = pcb->RT;
				printf("Process \"P%d\" terminated at time %d: TAT=%d, Wait Time=%d, I/O Wait=%d, Response Time=%d\n",pcb->pid,getTime(),pcb->TAT,pcb->wt,pcb->IOWait,pcb->RT);
				free(removeFromCPU());
				ContextS(halfCS);
			}
			if (queue->head != NULL)
				CPUexec(deQ(queue),halfCS);
		}
		if (quantum <= getBurstTime()){ //check quantom+preempt
			preempt(halfCS);
			CPUexec(deQ(queue),halfCS);
		}
	}
}

void AlgorithmExec(){
	if(!strcmp(alg, "FCFSP"))
		Preempt();
	else
		NPrempt();
}

int main(int argc, char* argv[]){
	char line[1024];
	int args[2*MAX_BURSTS+1];
	memset(&args[0], 0, sizeof(args));
	int pid = 1;
	PCB *pcb;
	queue = malloc(sizeof(Queue));
	queue->head = NULL;
	queue->tail = NULL;

	waitQ = malloc(sizeof(Queue));
	waitQ->head = NULL;
	waitQ->tail = NULL;

	if (argc < 4){
		printf("%s\n",usage);
		return 0;
	}

	if ((inputFile = fopen(argv[1], "r")) == NULL) {
		if (errno == ENOENT)
			printf("EXCEPTION: File not found or Directory does not exist\n");
		else
			printf("EXCEPTION: File read error\n");
		return 0;
	}

	alg = argv[2];

	if (strcmp(alg,"FCFSP") != 0 && strcmp(alg,"FCFSNP") != 0) {
		printf("%s\n",usage);
		return 0;
	}

	else if (strcmp(alg,"FCFSP") == 0) {
		if ((argc != 5) || (atoi(argv[3]) == 0) || (atoi(argv[4]) == 0)) {
			printf("%s\n",usage);
			return 0;
		}
		else {
			quantum = atoi(argv[3]);
			halfCS = atoi(argv[4]);
		}
	}

	else if (strcmp(alg,"FCFSNP") == 0) {
		if ((argc != 4) || (atoi(argv[3]) == 0)) {
			printf("%s\n",usage);
			return 0;
		}
		else
			halfCS = atoi(argv[3]);
	}
	CPUmake(alg);

	while(fgets(line, 1024, inputFile) != NULL){
		parser(args,line);
		pcb = createPCB(pid++,args);

		while (getTime() < pcb->arrivalT){
			if (queue->head != NULL && queue->tail != NULL){
				AlgorithmExec();
			}
			tick();
		}

		enQ(queue,pcb);
		memset(&args[0], 0, sizeof(args));
	}
	fclose(inputFile);
	while((queue->head != NULL) || (cpu->pcb != NULL)){
		AlgorithmExec();
		tick();
	}

	float util = tCPUtime/getTime()*100;
	float avgTAT = tTAT/pid;
	float avgWait = twait/pid;
	float avgResponse = tresponse/pid;
	float avgIO = tIO/pid;

	printf("\nALL PROCESSES FINSHED\n\nCPU UTILIZATION=%f%%\nAVERAGE TAT=%f\t\t\tMAX TAT=%d\nAVERAGE WAIT=%f\t\t\tMAX WAIT=%d\nAVERAGE RESPONSE=%f\t\t\tMAX RESPONSE=%d\nAVERAGE I/O=%f\t\t\t\tMAX I/O=%d\n",util,avgTAT,maxTAT,avgWait,maxWAIT,avgResponse,maxRESPONSE,avgIO,maxIO);

	free(waitQ);
	free(queue);
	CPUfree();

	return 0;
}
