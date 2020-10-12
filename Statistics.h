#ifndef STATISTIC_H
#define STATISTIC_H

#include "defs.h"

#define MODULE  2147483647
#define A       16807
#define LASTXN  127773
#define UPTOMOD -2836
#define RATIO   0.46566128e-9   /* 1/MODULE */
#define ALPHA   1.9

using namespace std;

int randint(int min,int max,long *seed);
long rnd32(long seed);

double uniform(double a, double b,long *lseed);
double negexp(double mean, long *seed);
double selfsimilar(double lambda, long *seed);
double uni_rv();
double exp_rv(double lambda);
#endif

