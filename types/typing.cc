#include <stdio.h>
#include <typeinfo>

void parse_type(
template<typename T> void print_type(T *t)
{
  parse_type(typeid(*t).name());
}

int toto(void *x, int y)
{
  return 0;
}

int main()
{
  print_type(toto);

  return 0;
}
