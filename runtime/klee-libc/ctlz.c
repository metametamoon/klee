/*===-- ctlz.c ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===--------------------------------------------------------------------===*/

unsigned ctlz(unsigned val) {
    unsigned total_bits = sizeof(val) * 8;

    unsigned res = 0;
    unsigned mask = 1 << (total_bits - 1);
    while (!(val & mask)) {
        mask = (mask >> 1);
        res++;
    }

    return res;
}

unsigned long ctlzl(unsigned long val) {
    unsigned long total_bits = sizeof(val) * 8;

    unsigned long res = 0;
    unsigned long mask = 1 << (total_bits - 1);
    while (!(val & mask)) {
        mask = (mask >> 1);
        res++;
    }

    return res;
}

unsigned char ctlzc(unsigned char val) {
    unsigned char total_bits = sizeof(val) * 8;

    unsigned char res = 0;
    unsigned char mask = 1 << (total_bits - 1);
    while (!(val & mask)) {
        mask = (mask >> 1);
        res++;
    }

    return res;
}
