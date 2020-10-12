#ifndef CTOPOLOGY_H
#define CTOPOLOGY_H

#include "defs.h"
#include "CNet.h"

using namespace std;

class CTopology {
private:
    CNet* GlobalNet;
    string FileName;

public:
    CTopology(CNet* net);
    ~CTopology();

    void load();
};

#endif


