#include "Statistics.h"

// Attenzione  : Il valore di '*seed' e' cambiato
// It generates a random integer x with [min <= x <= max]
int randint(int min,int max,long *seed)
{
    double temp;
    *seed = rnd32(*seed);
    temp = (double)(*seed) - 1.0;
    temp = temp*(max-min+1.0)/(MODULE-1);
    return ((int) temp + min);
}


double uniform(double a, double b,long *lseed) {
    double u;

    *lseed = rnd32 (*lseed);
    /*  *seed = rand();*/
    u = (*lseed) * RATIO;
    u = a + u * (b-a);
    return (u);
};

long rnd32(long seed) {
    long times, rest, prod1, prod2;

    times = seed / LASTXN;
    rest  = seed - times * LASTXN;

    prod1 = times * UPTOMOD;
    prod2 = rest * A;
    seed  = prod1 + prod2;

    if (seed < 0) seed = seed + MODULE;

    return (seed);
};

double negexp(double mean, long *seed) {
    double u;
    *seed=rnd32(*seed);
    u=(*seed)*RATIO;
    return (-mean*log(u));
};

double selfsimilar(double lambda, long *seed) {
    /* Expression for pareto distribution txTime = ((ALPHA-1)/(lambda*ALPHA))/pow(p,1.0/ALPHA)
    lambda - required mean, ALPHA - 1,2(Generally it is between 1 and 2).
    p - Uniform R.V. (0,1] For selfsimilar distribution ALPHA = 1.2*/
    return ((0.9)/(lambda*ALPHA))/pow(uniform(0.0,1.0,seed),1.0/ALPHA);
};
