// RUN: rm -rf %t.klee-out || echo "ok"
// RUN: %kleef --bidirectional --output-dir=%t.klee-out --property-file=%S/unreach-call.prp --max-memory=15000000000 --max-cputime-soft=900 --32 %s &>%t.log
// RUN: FileCheck %s -input-file=%t.log
// CHECK: [FALSE POSITIVE] FOUND FALSE POSITIVE AT


// This file is part of the SV-Benchmarks collection of verification tasks:
// https://gitlab.com/sosy-lab/benchmarking/sv-benchmarks
//
// SPDX-FileCopyrightText: 2022 Jana (Philipp) Berger
//
// SPDX-License-Identifier: GPL-3.0-or-later

extern unsigned long __VERIFIER_nondet_ulong();
extern long __VERIFIER_nondet_long();
extern unsigned char __VERIFIER_nondet_uchar();
extern char __VERIFIER_nondet_char();
extern unsigned short __VERIFIER_nondet_ushort();
extern short __VERIFIER_nondet_short();
extern float __VERIFIER_nondet_float();
extern double __VERIFIER_nondet_double();
extern void abort(void);
extern void __assert_fail(const char *, const char *, unsigned int, const char *) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
void reach_error() { __assert_fail("0", "Req1_Prop1_Batch20dependencies.c", 13, "reach_error"); }
void __VERIFIER_assert(int cond) { if(!(cond)) { ERROR: {reach_error();abort();} } return; }
void assume_abort_if_not(int cond) { if(!cond) { abort(); } }
unsigned char isInitial = 0;
unsigned char var_1_1 = 5;
unsigned char var_1_2 = 0;
unsigned char var_1_3 = 128;
unsigned char var_1_4 = 2;
unsigned char var_1_5 = 2;
unsigned char var_1_6 = 32;
unsigned short int var_1_8 = 100;
float var_1_9 = 255.75;
signed char var_1_10 = 64;
signed char var_1_11 = 32;
signed char var_1_12 = 4;
float var_1_13 = 0.0;
float var_1_14 = 7.25;
float var_1_15 = 8.125;
float var_1_16 = 24.5;
signed short int var_1_17 = -256;
double var_1_18 = 31.5;
unsigned char last_1_var_1_6 = 32;
unsigned short int last_1_var_1_8 = 100;
void initially(void) {
}
void step(void) {
 if ((64 + (var_1_5 / var_1_3)) != (last_1_var_1_8 - (var_1_4 + last_1_var_1_6))) {
  var_1_6 = var_1_4;
 } else {
  var_1_6 = 128;
 }
 if (var_1_2) {
  var_1_1 = (var_1_3 - var_1_4);
 } else {
  var_1_1 = (var_1_4 + var_1_5);
 }
 if (((var_1_10 - var_1_11) - (16 + var_1_12)) != var_1_5) {
  var_1_9 = (64.8f - (var_1_13 - var_1_14));
 } else {
  var_1_9 = var_1_13;
 }
 var_1_15 = (var_1_14 + var_1_16);
 if (var_1_2) {
  var_1_17 = (var_1_3 + var_1_12);
 } else {
  if (var_1_14 == (((((99.8f) < (var_1_16)) ? (99.8f) : (var_1_16))) / ((((255.6f) > (var_1_13)) ? (255.6f) : (var_1_13))))) {
   var_1_17 = var_1_4;
  } else {
   var_1_17 = (var_1_5 - var_1_3);
  }
 }
 if ((var_1_2 || (var_1_16 > var_1_13)) && (var_1_3 < var_1_1)) {
  if ((var_1_11 << var_1_12) > var_1_5) {
   var_1_18 = var_1_13;
  }
 } else {
  var_1_18 = var_1_16;
 }
 if ((var_1_17 + var_1_6) >= (var_1_3 - var_1_5)) {
  var_1_8 = ((((var_1_6) > (var_1_3)) ? (var_1_6) : (var_1_3)));
 } else {
  var_1_8 = var_1_6;
 }
}
void updateVariables() {
 var_1_2 = __VERIFIER_nondet_uchar();
 assume_abort_if_not(var_1_2 >= 0);
 assume_abort_if_not(var_1_2 <= 1);
 var_1_3 = __VERIFIER_nondet_uchar();
 assume_abort_if_not(var_1_3 >= 127);
 assume_abort_if_not(var_1_3 <= 254);
 var_1_4 = __VERIFIER_nondet_uchar();
 assume_abort_if_not(var_1_4 >= 0);
 assume_abort_if_not(var_1_4 <= 127);
 var_1_5 = __VERIFIER_nondet_uchar();
 assume_abort_if_not(var_1_5 >= 0);
 assume_abort_if_not(var_1_5 <= 127);
 var_1_10 = __VERIFIER_nondet_char();
 assume_abort_if_not(var_1_10 >= 63);
 assume_abort_if_not(var_1_10 <= 127);
 var_1_11 = __VERIFIER_nondet_char();
 assume_abort_if_not(var_1_11 >= 0);
 assume_abort_if_not(var_1_11 <= 64);
 var_1_12 = __VERIFIER_nondet_char();
 assume_abort_if_not(var_1_12 >= 0);
 assume_abort_if_not(var_1_12 <= 63);
 var_1_13 = __VERIFIER_nondet_float();
 assume_abort_if_not((var_1_13 >= 4611686.018427382800e+12F && var_1_13 <= -1.0e-20F) || (var_1_13 <= 9223372.036854765600e+12F && var_1_13 >= 1.0e-20F ));
 var_1_14 = __VERIFIER_nondet_float();
 assume_abort_if_not((var_1_14 >= 0.0F && var_1_14 <= -1.0e-20F) || (var_1_14 <= 4611686.018427382800e+12F && var_1_14 >= 1.0e-20F ));
 var_1_16 = __VERIFIER_nondet_float();
 assume_abort_if_not((var_1_16 >= -461168.6018427382800e+13F && var_1_16 <= -1.0e-20F) || (var_1_16 <= 4611686.018427382800e+12F && var_1_16 >= 1.0e-20F ));
}
void updateLastVariables() {
 last_1_var_1_6 = var_1_6;
 last_1_var_1_8 = var_1_8;
}
int property() {
 return ((((((var_1_2 ? (var_1_1 == ((unsigned char) (var_1_3 - var_1_4))) : (var_1_1 == ((unsigned char) (var_1_4 + var_1_5)))) && (((64 + (var_1_5 / var_1_3)) != (last_1_var_1_8 - (var_1_4 + last_1_var_1_6))) ? (var_1_6 == ((unsigned char) var_1_4)) : (var_1_6 == ((unsigned char) 128)))) && (((var_1_17 + var_1_6) >= (var_1_3 - var_1_5)) ? (var_1_8 == ((unsigned short int) ((((var_1_6) > (var_1_3)) ? (var_1_6) : (var_1_3))))) : (var_1_8 == ((unsigned short int) var_1_6)))) && ((((var_1_10 - var_1_11) - (16 + var_1_12)) != var_1_5) ? (var_1_9 == ((float) (64.8f - (var_1_13 - var_1_14)))) : (var_1_9 == ((float) var_1_13)))) && (var_1_15 == ((float) (var_1_14 + var_1_16)))) && (var_1_2 ? (var_1_17 == ((signed short int) (var_1_3 + var_1_12))) : ((var_1_14 == (((((99.8f) < (var_1_16)) ? (99.8f) : (var_1_16))) / ((((255.6f) > (var_1_13)) ? (255.6f) : (var_1_13))))) ? (var_1_17 == ((signed short int) var_1_4)) : (var_1_17 == ((signed short int) (var_1_5 - var_1_3)))))) && (((var_1_2 || (var_1_16 > var_1_13)) && (var_1_3 < var_1_1)) ? (((var_1_11 << var_1_12) > var_1_5) ? (var_1_18 == ((double) var_1_13)) : 1) : (var_1_18 == ((double) var_1_16)))
;
}
int main(void) {
 isInitial = 1;
 initially();
 while (1) {
  updateLastVariables();
  updateVariables();
  step();
  __VERIFIER_assert(property());
  isInitial = 0;
 }
 return 0;
}