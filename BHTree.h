/* BHTree creates a Binary Heap of BHEvents.
A BHEvent is composed by 4 pointers:
- 3 pointers for the connection to others BHevents in the BHTree
- 1 pointer to the related CEvent object
A CEvent is the user defined event, users can change every thing inside of CEvent unless:
- The class name (MUST REMAIN CEvent)
- The name of the function getHotField()

Users can change the variable that BHTree uses to order the events (the HotField) simply
changing it in the function CEvent::getHotField() but the type of variable can't change:
it MUST be a long! */

#ifndef BHTREE_H
#define BHTREE_H

#include "BHEvent.h"

using namespace std;

class CNet;

class BHTree {
public:
    CNet* ParentNet;

    BHEvent *Head;
    BHEvent *LastLeaf;
    BHEvent *globtemp;

    BHTree(CNet* net);
    ~BHTree();

    BHEvent* NewLeaf(CEvent* value);//creating a new event object in the first available leaf place
    BHEvent* DeleteLastLeaf();
    BHEvent* GainPositions(BHEvent *eve);
    BHEvent* Add(CEvent* val);
    BHEvent* NextEvent();
    void     execute();
    void     ReleaseEvent(BHEvent *eve);
    void     XChange(BHEvent *event1, BHEvent *event2);
    void     GoUp(BHEvent *event);
};

#endif
