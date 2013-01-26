#! /usr/bin/env

import math

p_bin = lambda x: int(pow(2, x))

tablebits = 8
tableres = 8
tablesize = p_bin(tablebits) 

h = open("sinetable.h", "w")
c = open("sinetable.c", "w")

h.write("""
#ifndef __sinetable_h_
#define __sinetable_h_

#include <avr/pgmspace.h>
#include <stdint.h>

#define SINETABLE_SIZE %d
#define SINETABLE_BITS %d
#define SINETABLE_RES %d
#define SINETABLE_MASK %d

extern const PROGMEM uint%d_t sinetable[%d];

#endif // __sinetable_h_

""" % (tablesize, tablebits, tableres, tablesize-1, tableres, tablesize))
h.close()


c.write("""
#include "sinetable.h"

const PROGMEM uint%d_t sinetable[%d] = {
    """ % (tableres, tablesize))


maxval = p_bin(tableres) - 1
meanval = int((maxval + 1) / 2) 

for i in xrange(tablesize):
    r = 2 * math.pi * float(i/float(tablesize))
    v = math.sin(r) * (meanval - 1) + meanval;

    if i < (tablesize - 1):
        c.write("%d,\n" % int(v))
    else:
        c.write("%d\n" % int(v))

c.write("};\n\n")
c.close()
