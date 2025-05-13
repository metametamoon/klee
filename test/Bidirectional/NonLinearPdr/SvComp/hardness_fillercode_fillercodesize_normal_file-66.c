// RUN: rm -rf test-suite
// RUN: %kleef --nonlinear-pdr --backwards-full-logs --bidirectional --output-dir=%t.klee-out --property-file=%S/unreach-call.prp --max-memory=15000000000 --max-cputime-soft=90 --32 %s &>%t.log
// RUN: FileCheck %s -input-file=%t.log
// CHECK: [FALSE POSITIVE]

extern long __VERIFIER_nondet_long();
extern unsigned char __VERIFIER_nondet_uchar();
extern char __VERIFIER_nondet_char();
extern unsigned short __VERIFIER_nondet_ushort();
extern short __VERIFIER_nondet_short();
extern float __VERIFIER_nondet_float();
extern double __VERIFIER_nondet_double();

extern void abort(void);
extern void __assert_fail(const char *, const char *, unsigned int, const char *) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
void reach_error() { __assert_fail("0", "Req1_Prop1_Batch66normal.c", 13, "reach_error"); }
void __VERIFIER_assert(int cond) { if(!(cond)) { ERROR: {reach_error();abort();} } return; }
void assume_abort_if_not(int cond) { if(!cond) { abort(); } }



#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define abs(a) (((a) < 0 ) ? -(a) : (a))





// Function prototypes


// Internal control logic variables
unsigned char isInitial = 0;

// Signal variables
signed char var_1_1 = 50;
signed char var_1_3 = 8;
signed char var_1_4 = 10;
double var_1_5 = 16.5;
signed char var_1_6 = 64;
signed char var_1_7 = 2;
unsigned short int var_1_8 = 256;
unsigned short int var_1_10 = 63583;
unsigned short int var_1_11 = 33789;
unsigned char var_1_12 = 1;
unsigned char var_1_14 = 0;
signed long int var_1_15 = 8;
signed short int var_1_16 = -16;
signed short int var_1_18 = 16;
signed short int var_1_19 = 25;
signed short int var_1_20 = 25;
signed short int var_1_21 = 50;

// Calibration values

// Last'ed variables
unsigned char last_1_var_1_12 = 1;

// Additional functions


void initially(void) {
}



void step(void) {
	// From: Req3Batch66normal
	signed char stepLocal_1 = var_1_6;
	unsigned char stepLocal_0 = last_1_var_1_12;
	if (-25 == stepLocal_1) {
		if (last_1_var_1_12 && stepLocal_0) {
			var_1_8 = (max (((var_1_10 - 10) - 16) , (var_1_11 - var_1_7)));
		}
	} else {
		var_1_8 = (max (var_1_7 , var_1_11));
	}


	// From: Req4Batch66normal
	signed char stepLocal_2 = var_1_7;
	if (var_1_8 >= stepLocal_2) {
		var_1_12 = (! var_1_14);
	}


	// From: Req1Batch66normal
	if (var_1_12) {
		var_1_1 = (var_1_3 + var_1_4);
	}


	// From: Req2Batch66normal
	if (var_1_4 >= (var_1_6 - var_1_7)) {
		if (var_1_12) {
			var_1_5 = 8.25;
		}
	}


	// From: Req7Batch66normal
	signed char stepLocal_4 = var_1_6;
	if (var_1_1 < stepLocal_4) {
		var_1_18 = (var_1_6 - var_1_7);
	} else {
		var_1_18 = (min ((var_1_6 + var_1_4) , var_1_1));
	}


	// From: Req8Batch66normal
	if (var_1_12) {
		var_1_19 = ((var_1_7 - (min (var_1_20 , var_1_21))) + var_1_6);
	}


	// From: Req5Batch66normal
	if (-64 > (var_1_6 + var_1_19)) {
		var_1_15 = (var_1_11 - var_1_7);
	} else {
		if (! var_1_12) {
			if (var_1_12) {
				var_1_15 = 4;
			}
		}
	}


	// From: Req6Batch66normal
	signed long int stepLocal_3 = (max (var_1_11 , var_1_18)) + (var_1_15 / var_1_10);
	if (var_1_19 > stepLocal_3) {
		var_1_16 = var_1_4;
	}
}



void updateVariables() {
	var_1_3 = __VERIFIER_nondet_char();
	assume_abort_if_not(var_1_3 >= -63);
	assume_abort_if_not(var_1_3 <= 63);
	var_1_4 = __VERIFIER_nondet_char();
	assume_abort_if_not(var_1_4 >= -63);
	assume_abort_if_not(var_1_4 <= 63);
	var_1_6 = __VERIFIER_nondet_char();
	assume_abort_if_not(var_1_6 >= -1);
	assume_abort_if_not(var_1_6 <= 127);
	var_1_7 = __VERIFIER_nondet_char();
	assume_abort_if_not(var_1_7 >= 0);
	assume_abort_if_not(var_1_7 <= 127);
	var_1_10 = __VERIFIER_nondet_ushort();
	assume_abort_if_not(var_1_10 >= 49150);
	assume_abort_if_not(var_1_10 <= 65534);
	var_1_11 = __VERIFIER_nondet_ushort();
	assume_abort_if_not(var_1_11 >= 32767);
	assume_abort_if_not(var_1_11 <= 65534);
	var_1_14 = __VERIFIER_nondet_uchar();
	assume_abort_if_not(var_1_14 >= 0);
	assume_abort_if_not(var_1_14 <= 0);
	var_1_20 = __VERIFIER_nondet_short();
	assume_abort_if_not(var_1_20 >= 0);
	assume_abort_if_not(var_1_20 <= 16383);
	var_1_21 = __VERIFIER_nondet_short();
	assume_abort_if_not(var_1_21 >= 0);
	assume_abort_if_not(var_1_21 <= 16383);
}



void updateLastVariables() {
	last_1_var_1_12 = var_1_12;
}

int property() {
	return (((((((var_1_12 ? (var_1_1 == ((signed char) (var_1_3 + var_1_4))) : 1) && ((var_1_4 >= (var_1_6 - var_1_7)) ? (var_1_12 ? (var_1_5 == ((double) 8.25)) : 1) : 1)) && ((-25 == var_1_6) ? ((last_1_var_1_12 && last_1_var_1_12) ? (var_1_8 == ((unsigned short int) (max (((var_1_10 - 10) - 16) , (var_1_11 - var_1_7))))) : 1) : (var_1_8 == ((unsigned short int) (max (var_1_7 , var_1_11)))))) && ((var_1_8 >= var_1_7) ? (var_1_12 == ((unsigned char) (! var_1_14))) : 1)) && ((-64 > (var_1_6 + var_1_19)) ? (var_1_15 == ((signed long int) (var_1_11 - var_1_7))) : ((! var_1_12) ? (var_1_12 ? (var_1_15 == ((signed long int) 4)) : 1) : 1))) && ((var_1_19 > ((max (var_1_11 , var_1_18)) + (var_1_15 / var_1_10))) ? (var_1_16 == ((signed short int) var_1_4)) : 1)) && ((var_1_1 < var_1_6) ? (var_1_18 == ((signed short int) (var_1_6 - var_1_7))) : (var_1_18 == ((signed short int) (min ((var_1_6 + var_1_4) , var_1_1)))))) && (var_1_12 ? (var_1_19 == ((signed short int) ((var_1_7 - (min (var_1_20 , var_1_21))) + var_1_6))) : 1)
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
