#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

/*
** @author Arash Alaei <arashalaei22@gmail.com>
** @since Saturday, January 30, 2021
** @description Implement a system call that returns parnet process ID.
*/
int
sys_getparentpid(void){
  return myproc()->parent->pid;
}

/*
** @author Arash Alaei <arashalaei22@gmail.com>
** @since Suday, January 31, 2021
** @description Implement a system call that returns the numbert of system calls.
*/
int
sys_getSyscallCounter(void){
  int n;
  argint(0,&n);
  return myproc()->sysCallCounter[n];
}

/*
** @author Arash Alaei <arashalaei22@gmail.com>
** @since Monday, February 1, 2021
** @description Implement a system call that returns the children of process.
*/
int
sys_getChildren(void){
  int PID;
  argint(0,&PID);
  return get_children(PID);
}

/*
** @author Arash Alaei <arashalaei22@gmail.com>
** @since Monday, February 7, 2021
** @description Implement a system call that changes the priority of processes.
*/
int
sys_setPriority(void){
  int priority;
  argint(0,&priority);
  // If the priority is outside the range of 1 to 6, it should be known as 5.
  if(priority < 1 || priority > 6) 
    myproc()->priority = 5;
  else
    myproc()->priority = priority;

  return 1;
}

/*
** @author Arash Alaei <arashalaei22@gmail.com>
** @since Monday, February 8, 2021
** @description Implement a system call that changes the policy of scheduling algorithm.
*/
int
sys_changePolicy(void){
  int policy;
  argint(0,&policy);
  return change_policy(policy);
}

/*
** @author Arash Alaei <arashalaei22@gmail.com>
** @since Monday, February 8, 2021
** @description Implement a system call that waits for all child of current process.
*/
int
sys_waiting(void){
	struct processTimes *PTV;
  argptr(0, (void*)&PTV ,sizeof(*PTV));
  return wait_for_child(PTV);
}

int
sys_changeQueue(void){
    int res = changeQueue();
    return res;
}