#include <stdio.h>
#include <fstream>
#include <iostream>

int main() {
  std::ifstream input("./c01/p01.txt", std::ios_base::in);
  if (!input.is_open()) {
    perror("open p01.txt error");
  }
  std::string line;
  while (std::getline(input, line)) {
    std::cout << line << std::endl;
  }
  input.close();
}