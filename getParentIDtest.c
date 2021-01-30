#include "types.h"
#include "stat.h"
#include "user.h"
/*
** @author Arash Alaei <arashalaei22@gmail.com>.
** @since Saturday, January 30, 2021.
** @description this program tests getparentpid system call.
*/
int main(void){
  printf(1,"This is process %d and the parent id is %d\n",getpid(),getparentpid());
  fork();
  printf(1,"This is process %d and the parent id is %d\n",getpid(),getparentpid());
  if(fork() == 0){
    printf(1,"This is process %d and the parent id is %d\n",getpid(),getparentpid());
  }else{
    printf(1,"This is process %d and the parent id is %d\n",getpid(),getparentpid());
  }
  printf(1,"This is process %d and the parent id is %d\n",getpid(),getparentpid());
  exit();
}