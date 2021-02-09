#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_CHILD           30
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
/*
** @author Arash Alaei <arashalaei22@gmail.com>.
** @since Tuesday, February 9, 2021
** @description this program tests prioroty scheduling.
*/
int main(void){
    // Change policy , (2) => prioroty scheduling.
    int cp = changePolicy(2);
    if(cp){
    // Create 30 child
    for(int i = 1; i <= NUM_CHILD; i++){
        if(!fork()){
            if(i >= 1 && i <= 5){ // The first five children
                setPriority(6);
            }else if(i >= 6 && i <= 10){ // The Five second children
                setPriority(5);
            }else if(i >= 11 && i <= 15){ // The Five third children
                setPriority(4); 
            }else if(i >= 16 && i <= 20){ // The Five fourth children
                setPriority(3);
            }else if(i >= 21 && i <= 25){// The Five fifth children
                setPriority(2);
            }else if(i >= 26 && i <= 30){ // The Five sixth children
                setPriority(1);
            }
            for(int j = 0; j < 250; j++) {
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