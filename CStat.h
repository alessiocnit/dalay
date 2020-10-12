#ifndef STDCSTAT_H
#define STDCSTAT_H

#include "defs.h"

using namespace std;


class CStat {
public:

    double min;
    double max;
    int bins;

    double delta;

    int distribution_samples;
    int out_ditribution_samples;

    string OutFileName;

    vector<int> data_distribution;
    vector<double> data;

    CStat(double min, double max, int bins, string file_name);

    ~CStat();

    void AddSample_Distribution(double sample);
    void AddSample(double sample);

    double ConfidenceInterval(int cl);

    void print_data();

};

#endif
