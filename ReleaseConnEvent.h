#ifndef RELEASECONNEVENT_H
#define RELEASECONNEVENT_H

#include "CEvent.h"
#include "CNode.h"
#include "CConnection.h"

using namespace std;

//All of different events inherited from this class
class ReleaseConnEvent: public CEvent {
public:
    CNode* InNode;
    CConnection* Conn;
    int RestorationFlag;

    ~ReleaseConnEvent();
    ReleaseConnEvent(CNode* node, CConnection* connection, double time, int RestorationFlag);

    void execute();
};

#endif
