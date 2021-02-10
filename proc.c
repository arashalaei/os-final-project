#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#define PRIORITY_DEFAULT 10
#define PRIORITY_PRIORITYSCHED 7
#define PRIORITY_REVERSEDPRIORITYSCHED 4
#define PRIORITY_RR 1


struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;
// *** Our implementation ***
int SCHEDULING_POLICY = DEFAULT_SCHEDULING;
// --------------------------
int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

// *** Our implementation ***
  p->priority = 3; // Default process priority

  p->creationTime = ticks;
	p->terminationTime = 0;
	p->sleepingTime = 0;
	p->readyTime = 0;
	p->runningTime = 0;

  struct proc *pro; // Process counter variable 
	int minPriority;  // To update process module
  int flag = 0;     // Helper variable to indicate 

  for(pro = ptable.proc; pro < &ptable.proc[NPROC]; pro++){
    if((pro->state == SLEEPING) || pro->state == RUNNABLE || pro->state == RUNNING){
      if(pro->priorityModule > 0 && pro->priorityModule < MIN_MODULO){ //  0 < priorityModule < MIN_MODULO
				minPriority = pro->priorityModule;
				flag = 1;
      }
    }
  }

	if(flag == 0){
		p->priorityModule = 0;
  }else{ 
		p->priorityModule = minPriority;
	}
  //---------------------------

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  // *** Our implementation ***
  // Initialize the number of each system calls
  for(int i = 0 ; i < NO_SYSCALL ; i++)
    p->sysCallCounter[i] = 0;
  //---------------------------

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  // *** Our implementation ***
  // Set process termination time equal cpu ticks after exit.
  myproc()->terminationTime = ticks;
  // -------------------------------
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

 // *** Our implementation ***
int
changeQueue(){
     struct proc *p = p;
     if(p){
        if (p->priority == PRIORITY_DEFAULT){
            p->priority = PRIORITY_PRIORITYSCHED;
            return 0;
        } else if (p->priority == PRIORITY_PRIORITYSCHED) {
            p->priority = PRIORITY_REVERSEDPRIORITYSCHED;
            return 0;
        }    
            else if (p->priority == PRIORITY_REVERSEDPRIORITYSCHED) {
            p->priority = PRIORITY_RR;
            return 0;
        } else if (p->priority == PRIORITY_RR){
            return -1;
        }
}
    return -1;
}



int 
mlq(void)
{
   struct proc *p;
    int exec_proc = -1;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state == RUNNABLE && p->priority == PRIORITY_DEFAULT){
            exec_proc = p->pid;
        }
    }
    if (exec_proc == -1){
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
            if(p->state == RUNNABLE && p->priority == PRIORITY_PRIORITYSCHED){
            struct proc *HPP = p; // High Priority Process.
            // Higher priority must be selected.
              for(struct proc *pro = ptable.proc; pro < &ptable.proc[NPROC]; pro++){
              if(pro->state != RUNNABLE)
              continue;
              if( HPP->priorityModule > pro->priorityModule  ) 
              HPP = pro;
        }
        p = HPP;
        // After each quantum, the algorithm updates the current priorityModule 
        // as priorityModule += priority just for the running process
        p->priorityModule += p->priority; 
        exec_proc = p -> pid;
            }   
        }
    }
    if (exec_proc == -1){
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
            if(p->state == RUNNABLE && p->priority == PRIORITY_REVERSEDPRIORITYSCHED){
                 struct proc *HPP = p; // High Priority Process.
                for(struct proc *pro = ptable.proc; pro < &ptable.proc[NPROC]; pro++){
              if(pro->state != RUNNABLE)
                continue;
              if( HPP->priorityModule < pro->priorityModule  ) 
                  HPP = pro;
                }
                p = HPP;
                // After each quantum, the algorithm updates the current priorityModule 
                // as priorityModule += priority just for the running process
                p->priorityModule += p->priority; 
                    exec_proc = p->pid;
            }   
        }
    }
    if (exec_proc == -1){
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            if (p->state == RUNNABLE && p->priority == PRIORITY_RR){
                exec_proc = p->pid;
            }
        }
    }
    return exec_proc;
}


 // ---------------------------


//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  int my_pid = -1;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    if(SCHEDULING_POLICY == PRIORITY_SCHEDULING){
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state != RUNNABLE)
          continue;

        // *** Our implementation ***
        struct proc *HPP = p; // High Priority Process.
        // Higher priority must be selected.
        for(struct proc *pro = ptable.proc; pro < &ptable.proc[NPROC]; pro++){
          if(pro->state != RUNNABLE)
            continue;
          if( HPP->priorityModule > pro->priorityModule  ) 
            HPP = pro;
        }
        p = HPP;
        // After each quantum, the algorithm updates the current priorityModule 
        // as priorityModule += priority just for the running process
        p->priorityModule += p->priority; 
        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;
        swtch(&(c->scheduler), p->context);
        switchkvm();
        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
    }
  }else if(SCHEDULING_POLICY == MLQ_SCHEDULING){
          my_pid = mlq();
          for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
            if(p->pid != my_pid)
              continue;
            
             c->proc = p;
        switchuvm(p);
        p->state = RUNNING;
        swtch(&(c->scheduler), p->context);
        switchkvm();

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
        }
  }
  
  
  else{// policy == default
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
          if(p->state != RUNNABLE)
            continue;
          // Switch to chosen process.  It is the process's job	      
          // to release ptable.lock and then reacquire it	      
          // before jumping back to us.	      
          c->proc = p;	      
          // *** Our implementation ***
          p->ticksPassed = 0;
          // ---------------------------
          switchuvm(p);	      
          p->state = RUNNING;	     
          swtch(&(c->scheduler), p->context);
          switchkvm();
          // Process is done running for now.
          // It should have changed its p->state before coming back.
          c->proc = 0;
    }
  }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  // *** Our implementation ***
  myproc()->ticksPassed++;
  // if ticksPassed larger than time slot then context-switch is needed
  if(SCHEDULING_POLICY == DEFAULT_SCHEDULING || ((myproc()->ticksPassed > QUANTUM) && (SCHEDULING_POLICY == MUTATE_DEFAULT_SCHEDULING))){
    myproc()->ticksPassed = 0; // Set to zero after context-switch
  //---------------------------
    acquire(&ptable.lock);  //DOC: yieldlock
    myproc()->state = RUNNABLE;
    sched();
    release(&ptable.lock);
  }
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
/*
** @author Arash Alaei <arashalaei22@gmail.com>
** @since Monday, February 1, 2021
** @description Implement a function that returns the children of process as a digits number.
*/
int
get_children(int PID){
  int children = 0;
  acquire(&ptable.lock);
  for(struct proc *p = ptable.proc ; p < &ptable.proc[NPROC] ; p++){
    if(p->parent->pid == PID){
      if(p->pid >= 10)
        children *= 100;
      else
        children *= 10;

      children = children + p->pid;
    }
  }
  release(&ptable.lock);
  return children;
}

/*
** @author Arash Alaei <arashalaei22@gmail.com>
** @since Monday, February 8, 2021
** @description Implement a function that changes the policy of scheduling algorithm..
*/
int
change_policy(int policy){
	if(policy > 2 || policy < 0) // If ploicy out of range [0,2] then set dafault policy.
    SCHEDULING_POLICY = DEFAULT_SCHEDULING;
  else
    SCHEDULING_POLICY = policy;
  // Just so that no error is displayed  
	return 1;
}

/*
** @author Arash Alaei <arashalaei22@gmail.com>
** @since Monday, February 8, 2021
** @description Implement a function that updates the times(ready, running , ...) of each process.
*/
void
updateProcessTimes(){
  acquire(&ptable.lock);
  for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if (p->state == RUNNABLE) p->readyTime++;
    else if(p->state == RUNNING) p->runningTime++;
    else if (p->state == SLEEPING) p->sleepingTime++;
  }
  release(&ptable.lock);
}

// Helper functions
void
updateTimeProperty(struct proc *p,struct processTimes *t){
  t->creationTime = p->creationTime;
  t->readyTime = p->readyTime;
  t->runningTime = p->runningTime;
  t->sleepingTime = p->sleepingTime;
  t->terminationTime = p->terminationTime; 
}

void
setZeroProcessProperty(struct proc *p){
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->killed = 0;
  p->state = UNUSED;
}

/*
** @author Arash Alaei <arashalaei22@gmail.com>
** @since Monday, February 8, 2021
** @description Implement a function that that waits for all child of current process.
*/
int
wait_for_child(struct processTimes *t){
  int flag, PID;
  struct proc *currentProcess = myproc();
  acquire(&ptable.lock);
  while(1){
    flag = 0; // this flag determines the existence of a child
    for(struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      // The current process has no children 
      if(p->parent != currentProcess)
        continue;
      flag = 1; // Found one child.
      if(p->state == ZOMBIE){
        updateTimeProperty(p,t);

        PID = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        setZeroProcessProperty(p);
        release(&ptable.lock);
        return PID;
      }
    }

    // We will not wait if there is no child or the process is killed.
    if(currentProcess->killed || !flag){
      release(&ptable.lock);
      return -10;
    }
    sleep(currentProcess, &ptable.lock);  
  }
}