#ifndef GENERATECONNEVENT_H
#define GENERATECONNEVENT_H

#include "CEvent.h"

using namespace std;

class CNet;
class CNode;
class CConnection;

class GenerateConnEvent: public CEvent {
public:
    CNode* InNode;
    CConnection* Conn;

    GenerateConnEvent();

    ~GenerateConnEvent();
    GenerateConnEvent(int conn_id, CNet* net);
    GenerateConnEvent(CConnection* connection, double time);

    void execute();
    int ExperimentEnd(int Experiment_ID, double *TimeForRep);
};

#endif
