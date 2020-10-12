#ifndef LINKREPAIREVENT_H
#define LINKREPAIREVENT_H

#include "CEvent.h"
#include "CNode.h"

using namespace std;

//This class repairs a bidirectional link: both fibers between a node pair
class LinkRepairEvent: public CEvent {
public:

    CNode *Node_1, *Node_2;
    CLink *Link_1, *Link_2;
    CNet* ParentNet;
    CDomain* ParentDomain;

    LinkRepairEvent(CLink* l, double time);
    ~LinkRepairEvent();

    void execute();
};

#endif
