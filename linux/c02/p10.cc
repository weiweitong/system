#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

int main() {
  /**
   * int select (int n,                     // maximum fd + 1
   *             fd_set *readfds,           // read non-blockable
   *             fd_set *writefds,          // write non-blockable
   *             fd_set *exceptfds,         // execute non-blockable
   *             struct timeval *timeout);  // go back when tv_sec and tv_usec(mini second)
   * FD_CLR(int fd, fd_set *set);
   * FD_ISSET(int fd, fd_set *set);
   * FD_SET(int fd, fd_set *set);
   * FD_ZERO(fd_set *set);
   * 
   * struct timeval {
   *   long tv_sec;
   *   long tv_usec;
   * };
   */

  
}

