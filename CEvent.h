#ifndef CEVENT_H
#define CEVENT_H

#include "defs.h"

using namespace std;
enum EventType {
    START_EXPERIMENT,

    WRITE_PCK_NODE,
    PROCESSED_PCK,
    DELIVERED_PCK,

    GENERATE_CONN,
    RELEASE_CONN,

    LINK_FAILURE,
    LINK_REPAIR,
    
    CROSS_CONNECTION,
    CPU_EVENT_SRC_RSA,
    CPU_EVENT_PCE_RSA,
    CPU_EVENT_DST_SA
};

//All of different events inherited from this class
class CEvent {
public:
    double eventTime;

    EventType eventType;
    string Comment;

    CEvent();
    virtual ~CEvent()=0;

    double getHotField();

    virtual void execute()=0;// execute the event
};

#endif
