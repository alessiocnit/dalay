#ifndef LINKFAILUREEVENT_H
#define LINKFAILUREEVENT_H

#include "CEvent.h"
#include "CNode.h"

using namespace std;

//This class provides the failure of a bidirectional link: both fibers between a node pair
//All of different events inherited from this class
class LinkFailureEvent: public CEvent {
public:
    int G_l[NMAX][NMAX];

    CNode *Node_1, *Node_2;
    CLink *Link_1, *Link_2;
    CNet* ParentNet;
    CDomain* ParentDomain;

    LinkFailureEvent(CLink* l, double time);
    ~LinkFailureEvent();

    void execute();
};

#endif
