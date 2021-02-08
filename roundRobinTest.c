#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_CHILD           10
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
** @since Monday, February 8, 2021
** @description this program tests Round Robin algorithm.
*/

int main(void){
    int cp = changePolicy(1);
    if(cp){
        // Create 10 process with fork
        int i , j;
        for (i = NUM_CHILD; i > 0; i--){
            if (!fork()){
                for(j = 1000; j > 0; j--) {
                printf(1, "%d : %d\n", getpid(), j);
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
    }

    exit();
}