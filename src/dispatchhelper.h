#ifndef DISPATCHER_H_
#define DISPATCHER_H_

typedef struct Dispatcher Dispatcher;
typedef struct CPU CPU;

void CPUmake(char *alg);
void CPUfree();
void IOBurstUpdater();
void ContextS(int halfCS);
void CPUexec(PCB *pcb,int halfCS);
PCB* removeFromCPU();
void preempt(int halfCS);
void tick();
int getTime();
int getBurstTime();
void resetBurstTime();
struct Dispatcher {

};

struct CPU {
	PCB *pcb;
	char *alg;
};

extern CPU* cpu;

#endif
