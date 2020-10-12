#ifndef DELIVERED_PCK_EVENT_H
#define DELIVERED_PCK_EVENT_H

#include "CEvent.h"

using namespace std;

class LinkBuffer;
class CNode;

class DeliveredPckEvent:public CEvent {
public:
    LinkBuffer* FromBuffer;
    CNode* AtNode;

    DeliveredPckEvent(CNode* AtNode, LinkBuffer* fb, double time);

    virtual ~DeliveredPckEvent();
    virtual void execute();
};

#endif

