#include "types.h"
#include "stat.h"
#include "user.h"


#define PRIORITY_DEFAULT 10
#define PRIORITY_PRIORITYSCHED 7
#define PRIORITY_REVERSEDPRIORITYSCHED 4
#define PRIORITY_RR 1


#define NUM_CHILD           40
#define NUM_STORE           6
#define PID                 0
#define CREATION_TIME       1
#define READY_TIME          2
#define RUNNUG_TIME         3
#define SLEEPING_TIME       4
#define TERMINATION_TIME    5

struct processTimes {
    int creationTime;
    int terminationTime;
    int sleepingTime;
    int readyTime;
    int runningTime;
} time[1000];

int main(void){
    int cp = changePolicy(3);
    if(cp){
    // Create 40 child
    for(int i = 1; i <= NUM_CHILD; i++){
        if(!fork()){
            if(i >= 1 && i <= 10){ // The first 10 children
                setPriority(PRIORITY_DEFAULT);
            }else if(i >= 11 && i <= 20){ // The second 10 children
                setPriority(PRIORITY_PRIORITYSCHED);
            }else if(i >= 21 && i <= 30){ // The third 10 children
                setPriority(PRIORITY_REVERSEDPRIORITYSCHED); 
            }else if(i >= 31 && i <= 40){ // The fourth 10 children
                setPriority(PRIORITY_RR);
            }
            for(int j = 0; j < 200; j++) {
               printf( 1, "%d : %d\n", getChildren(getpid()), i);
            }
            exit();
        }
    }

    printf(1, "\n");
    int store[NUM_CHILD][NUM_STORE];
    for(int i = 0; i < NUM_CHILD; i++){
        store[i][PID] = waiting(time); 
        store[i][CREATION_TIME] = time->creationTime;
        store[i][READY_TIME] = time->readyTime;
        store[i][RUNNUG_TIME] = time->runningTime; 
        store[i][SLEEPING_TIME] = time->sleepingTime;
        store[i][TERMINATION_TIME] = time->terminationTime;
    }
    
    int avgq_waitingTime[4];
    int avgq_turnaroundTime[4];
    int avgq_runningTime[4];

    int w_temp0 = 0, w_temp1 = 0, w_temp2 = 0, w_temp3 = 0;
    int r_temp0 = 0, r_temp1 = 0, r_temp2 = 0, r_temp3 = 0;
    for(int k = 0; k < NUM_CHILD; k++){
         int turnaroundT = store[k][TERMINATION_TIME] - store[k][CREATION_TIME];
        int waitingT = turnaroundT  - (store[k][RUNNUG_TIME] + store[k][SLEEPING_TIME]);
        if(k >= 1 && k <= 10){
            w_temp0 += waitingT;
            r_temp0 += store[k][RUNNUG_TIME];
        }
        else  if(k >= 11 && k <= 20){
            w_temp1 += waitingT;
            r_temp1 += store[k][RUNNUG_TIME];
        }
        else  if(k >= 21 && k <= 30){
            w_temp2 += waitingT;
            r_temp2 += store[k][RUNNUG_TIME];
        }
         if(k >= 31 && k <= 40){
            w_temp3 += waitingT;
            r_temp3 += store[k][RUNNUG_TIME];
        }
    }

    avgq_turnaroundTime[0] = ((w_temp0 + r_temp0)/NUM_CHILD);
    avgq_turnaroundTime[1] = ((w_temp1 + r_temp1)/NUM_CHILD);
    avgq_turnaroundTime[2] = ((w_temp2 + r_temp2)/NUM_CHILD);
    avgq_turnaroundTime[3] = ((w_temp3 + r_temp3)/NUM_CHILD);
    
    avgq_waitingTime[0] = w_temp0/NUM_CHILD;
    avgq_waitingTime[1] = w_temp1/NUM_CHILD;
    avgq_waitingTime[2] = w_temp2/NUM_CHILD;
    avgq_waitingTime[3] = w_temp2/NUM_CHILD;

    avgq_runningTime[0] = r_temp0/NUM_CHILD;
    avgq_runningTime[1] = r_temp1/NUM_CHILD;
    avgq_runningTime[2] = r_temp2/NUM_CHILD;
    avgq_runningTime[3] = r_temp3/NUM_CHILD;

    for (int j = 0; j < 4; j++) {
        printf(1, "The average turn around time of queue #%d is: %d\n", j, avgq_turnaroundTime[j]);
        printf(1, "The average wait time of queue #%d is: %d\n", j, avgq_waitingTime[j]);
        printf(1, "The average CBT of queue #%d is: %d\n", j, avgq_runningTime[j]);
    }

    uint CBT = 0; 
    uint turnAroundTime = 0;
    uint waitingTime = 0;
    for(int i = 0; i < NUM_CHILD; i++) {
        int tt = store[i][TERMINATION_TIME] - store[i][CREATION_TIME];
        int wt = tt  - (store[i][RUNNUG_TIME] + store[i][SLEEPING_TIME]);
        
        printf(1, "PID:%d => [TurnAroundTime => %d  CBT => %d  waiting time => %d]\n", store[i][PID], tt, store[i][RUNNUG_TIME], wt);
        turnAroundTime += tt;
        CBT += store[i][RUNNUG_TIME];
        waitingTime += wt;
    }
        printf(1, "Average turn around time => %d  average CBT => %d  average waiting time => %d\n", (turnAroundTime / NUM_CHILD), (CBT / NUM_CHILD), (waitingTime / NUM_CHILD) );

    exit();
} }