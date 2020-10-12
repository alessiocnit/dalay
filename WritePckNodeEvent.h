#ifndef WRITE_PCK_NODE_EVENT_H
#define WRITE_PCK_NODE_EVENT_H

#include "CEvent.h"

using namespace std;

class CPck;
class CNode;
class NodeBuffer;

class WritePckNodeEvent:public CEvent {
public:
    CPck* Pck;
    CNode* AtNode;
    NodeBuffer* InBuffer;

    WritePckNodeEvent(CNode* node, CPck* pk, double time);
    ~WritePckNodeEvent();

    void execute();
};

#endif

