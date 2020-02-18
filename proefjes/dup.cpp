#include <final/final.h>
#include <iostream>
#include <ostream>
int main()
{
  std::cout << "In stdout" << std::endl;

  int stdoutBack = dup(1);
  close(1);

  int output = open("buffer.txt", O_RDWR|O_CREAT|O_APPEND, 0777);
  dup2(output, 1);

  std::cout << "In buffer" << std::endl;
dup2(stdoutBack, 1);
  close(output);

  std::cout << "In stdout" << std::endl;
}
