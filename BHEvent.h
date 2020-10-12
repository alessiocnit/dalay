#ifndef BHEVENT_H
#define BHEVENT_H

#include "CEvent.h"

using namespace std;

class BHEvent {
public:
    CEvent  *c;
    BHEvent *left;
    BHEvent *right;
    BHEvent *up;

public:

    BHEvent();
    BHEvent(CEvent* eT);

    ~BHEvent();

    double  getHotField();
    void    execute();
    CEvent* getEventContent();
};
#endif
