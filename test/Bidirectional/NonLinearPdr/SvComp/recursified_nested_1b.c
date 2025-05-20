// RUN: rm -rf test-suite
// RUN: %kleef --nonlinear-pdr --backwards-full-logs --bidirectional --output-dir=%t.klee-out --property-file=%S/unreach-call.prp --max-memory=15000000000 --max-cputime-soft=90 --32 %s &>%t.log
// RUN: FileCheck %s -input-file=%t.log
// CHECK: [TRUE POSITIVE]

extern void __assert_fail(const char *, const char *, unsigned int, const char *) __attribute__((__nothrow__, __leaf__)) __attribute__((__noreturn__));
void reach_error()
{
  __assert_fail("0", "nested_1b.c", 13, "reach_error");
}

void func_to_recursive_line_19_to_19_0(int *a)
{
  if ((*a) < 6)
  {
    {
      {
      }
      ++(*a);
    }
    func_to_recursive_line_19_to_19_0(a);
  }
  else
  {
  }
}

int main()
{
  int a = 6;
  {
    a = 0;
    func_to_recursive_line_19_to_19_0(&a);
  }
  if (a == 6)
  {
    reach_error();
  }
  else
  {
  }
  return 1;
}

