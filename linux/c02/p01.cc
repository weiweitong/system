#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

int main() {
  int fd;
  fd = open("./c02/p01.txt", O_RDONLY | O_NONBLOCK);
  // fd = creat("./c02/p01.txt", 0644);
  if (fd == -1) {
    perror("can not open p01 txt");
  }
  // ssize_t read (int fd, void *buf, size_t len);
  ssize_t ret;
  int len = 20;
  char *buf = new char[20];
  char *x = buf;
  while (len != 0 && (ret = read(fd, buf, len)) != 0) {
    if (ret == -1) {
      if (errno == EINTR) {
        continue;
      }
      if (errno == EAGAIN) {
        return -1;
      }
      perror("read");
      break;
    }
    len -= ret;
    buf += ret;
  }
  std::cout << "the pid of c02 p01 txt is " << fd << std::endl;
  std::cout << x << std::endl;
  std::cout << len << std::endl;

  close(fd);

  int fd2 = creat("./c02/p02.txt", 0644);
  unsigned long word = 1720;
  size_t count;
  ssize_t nr;
  count = sizeof(word);
  nr = write(fd2, &word, count);
  if (nr == -1) {
    perror("cannot write");
    
  }
}