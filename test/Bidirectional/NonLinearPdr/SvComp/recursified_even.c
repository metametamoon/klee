// RUN: rm -rf test-suite
// RUN: %kleef --nonlinear-pdr --backwards-full-logs --bidirectional --output-dir=%t.klee-out --property-file=%S/unreach-call.prp --max-memory=15000000000 --max-cputime-soft=90 --32 %s &>%t.log
// RUN: FileCheck %s -input-file=%t.log
// CHECK: [FALSE POSITIVE]

extern void abort(void);
extern void __assert_fail(const char *, const char *, unsigned int, const char *) __attribute__((__nothrow__, __leaf__)) __attribute__((__noreturn__));
void reach_error()
{
  __assert_fail("0", "even.c", 3, "reach_error");
}

extern int __VERIFIER_nondet_int();
void __VERIFIER_assert(int cond)
{
  if (!cond)
  {
    ERROR:
    {
      reach_error();
      abort();
    }

  }
  else
  {
  }
  return;
}

void func_to_recursive_line_13_to_14_0(unsigned int *x)
{
  if (__VERIFIER_nondet_int())
  {
    {
      *x += 2;
    }
    func_to_recursive_line_13_to_14_0(x);
  }
  else
  {
  }
}

int main(void)
{
  unsigned int x = 0;
  func_to_recursive_line_13_to_14_0(&x);
  __VERIFIER_assert(!(x % 2));
  return 0;
}

