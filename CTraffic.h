#ifndef CTRAFFIC_H
#define CTRAFFIC_H

#include "defs.h"
#include "CNet.h"

using namespace std;

class CTraffic {
public:
    CNet*    Net;
    string   TrafficFileName;

    string   TrafficMatrixFileName;
    ifstream TrafficMatrixFile;

    CTraffic(CNet* parent_net);
    ~CTraffic();

    void FirstConnection_Generation();
    void FileConnections_Generation();
    void Load();
    void Print_TrafficMatrix();

    CConnection* CreateConnection(int id,double GenerationTime,double MeanDuration);
};
#endif
