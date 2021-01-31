#include "types.h"
#include "stat.h"
#include "user.h"
/*
** @author Arash Alaei <arashalaei22@gmail.com>.
** @since Sunday, January 31, 2021.
** @description this program tests getSyscallCounter system call.
*/
int main(int argc,char const *argv[]){
  int n = atoi(argv[1]);
  // The number of times to call system call number 11(getpid) is: 6
  getpid();
  getpid();
  getpid();
  getpid();
  getpid();
  getpid();
  // The number of times to call system call number 22(getparentpid) is: 5
  getparentpid();
  getparentpid();
  getparentpid();
  getparentpid();
  getparentpid();

  printf(1,"The number of times to call system call number %d is: %d\n",n,getSyscallCounter(n));
  exit();
}