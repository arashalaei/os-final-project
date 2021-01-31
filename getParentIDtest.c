#include "types.h"
#include "stat.h"
#include "user.h"
/*
** @author Arash Alaei <arashalaei22@gmail.com>.
** @since Saturday, January 30, 2021.
** @description this program tests getparentpid system call.
*/
int main(void){
  if(fork() > 0)
    sleep(10);
  if(fork() == 0){
    sleep(10);
    if(fork() > 0){
      sleep(10);
      printf(1,"This is process %d and the parent id is %d\n",getpid(),getparentpid()); 
      wait();
    }
  }
  printf(1,"This is process %d and the parent id is %d\n",getpid(),getparentpid());
  wait();
  wait();
  exit();
}