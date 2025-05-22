// REQUIRES: geq-llvm-12.0

// RUN: rm -rf %t.klee-out || echo "ok"
// RUN: %kleef --bidirectional --output-dir=%t.klee-out --property-file=%S/unreach-call.prp --max-memory=15000000000 --max-cputime-soft=900 --32 %s &>%t.log
// RUN: FileCheck %s -input-file=%t.log
// CHECK: [TRUE POSITIVE]

#define N 10
#define fun rangesum

extern void abort(void);
#include <assert.h>
void reach_error() { assert(0); }
extern int __VERIFIER_nondet_int();

void init_nondet(int x[N]) {
  int i;
  for (i = 0; i < N; i++) {
    x[i] = __VERIFIER_nondet_int();
  }
}

int rangesum (int x[N])
{
  int i;
  long long ret;
  ret = 0;
  int cnt = 0;
  for (i = 0; i < N; i++) {
    if( i > N/2){
      ret = ret + x[i];
      cnt = cnt + 1;
    }
  }
  if ( cnt !=0)
    return ret / cnt;
  else
    return 0;
}

int main ()
{
  int x[N];
  init_nondet(x);
  int temp;
  int ret;
  int ret2;
  int ret5;

  ret = fun(x);

  temp=x[0];x[0] = x[1]; x[1] = temp;
  ret2 = fun(x);
  temp=x[0];
  for(int i =0 ; i<N-1; i++){
    x[i] = x[i+1];
  }
  x[N-1] = temp;
  ret5 = fun(x);

  if(ret != ret2 || ret !=ret5){
    {reach_error();}
  }
  return 1;
}
