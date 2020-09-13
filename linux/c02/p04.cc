#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

int main() {
  // int fsync(int fd);
  // int fdatasync(int fd);
  int fd = open("./c02/p04.txt", O_CREAT | O_WRONLY);
  if (fd == -1) {
    perror("open");
    return -1;
  }

  // off_t ret;
  // SEEK_END
  // ret = lseek(fd, (off_t) 1825, SEEK_SET);
  // if (ret == (off_t)-1) {
  //   perror("lseek error");
  //   return -1;
  // }
  // int pos = lseek(fd, 0, SEEK_CUR);
  // std::cout << pos << std::endl;

  /**
   * fd's pos --> count bytes --> buf
   * size_t pread(int fd, void *buf, size_t count, off_t pos);
   * buf --> count bytes --> fd's pos
   * size_t pwrite(int fd, void *buf, isze_t count, off_t pos);
   * 
   * int ftruncate(int fd, off_t len);
   * 
   * int truncate(const char *path, off_t len);
   * 
   */

  int nr = ftruncate(fd, 45);
  if (nr == -1) {
    perror("ftruncate");
    return -1;
  }
  return 0;
}