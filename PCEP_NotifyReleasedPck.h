#ifndef PCEP_NotifyReleasedPck_H
#define PCEP_NotifyReleasedPck_H

#include "CPck.h"

using namespace std;

class CNode;
class CLink;
class CConnection;
class RSVP_TearDownPck;

class PCEP_NotifyReleasedPck:public CPck {
public:
    int RestorationFlag; //This is one during stub release

    int ConnId;
    int conn_base_slot;
    int conn_width_slots;
    vector<CLink*> ConnLinkPath;
    CNode* ConnSource;
    CNode* ConnDestination;

    PCEP_NotifyReleasedPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, RSVP_TearDownPck* pk);
    ~PCEP_NotifyReleasedPck();

    virtual void arrived(CNode* node);
};

#endif





