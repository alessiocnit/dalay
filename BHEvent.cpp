#include "BHEvent.h"

#include "defs.h"
//#define EVENTS

BHEvent::BHEvent() {
    c = NULL;
    left  = NULL;
    right = NULL;
    up    = NULL;
};
BHEvent::BHEvent(CEvent* eT) {
    c = eT;
    left  = NULL;
    right = NULL;
    up    = NULL;
};

BHEvent::~BHEvent() {
    delete c;
};

double BHEvent::getHotField() {
    return c->getHotField();
};

void BHEvent::execute() {
#ifdef EVENTS
    cout << "EVENT " << this->c->Comment << "\t" << this->c->eventType << "\tTIME " << this->c->eventTime << endl;
#endif

    c->execute();
};

CEvent* BHEvent::getEventContent() {
    return c;
};
