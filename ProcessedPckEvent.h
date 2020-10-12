#ifndef PROCESSED_PCK_EVENT_H
#define PROCESSED_PCK_EVENT_H

#include "CEvent.h"

using namespace std;

class CPck;
class CNode;

class ProcessedPckEvent: public CEvent {
public:
    CPck* Pck;
    CNode* AtNode;

    ProcessedPckEvent(CNode* n, CPck* pk, double time);
    ~ProcessedPckEvent();

    virtual void execute();
};

#endif

