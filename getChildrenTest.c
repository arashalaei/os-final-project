#include "types.h"
#include "stat.h"
#include "user.h"
/*
** @author Arash Alaei <arashalaei22@gmail.com>.
** @since Monday, February 1, 2021
** @description this program tests getChildrenTest system call.
*/
int main(void){
  if(fork() > 0)
    sleep(10);
  if(fork() == 0)
    sleep(10);

  printf(1,"This is process %d and children are %d\n",getpid(),getChildren(getpid()));
  wait();
  wait();
  exit();
}