#ifndef RSVP_TearDownPck_H
#define RSVP_TearDown_H

#include "CPck.h"
#include "CNode.h"
#include "CConnection.h"

using namespace std;

class CPck;
class CNode;

class RSVP_TearDownPck:public CPck {
public:
    int RestorationFlag;
    CConnection* Conn;

    CNode* ConnSourceNode;
    CNode* ConnDestinationNode;

    vector<CNode*> NodePath;
    vector<CLink*> LinkPath;

    ConnectionType ConnType;
    
   //For flexible
    int base_slot;
    int width_slots;

    RSVP_TearDownPck(CNode* src,CNode* dst,int dim,int ttl,double msgGenTime,CConnection* conn,int RestorationFlag);
    ~RSVP_TearDownPck();

    virtual void arrived(CNode* node);
};

#endif


