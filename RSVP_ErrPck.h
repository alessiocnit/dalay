#ifndef RSVP_ErrPck_H
#define RSVP_ErrPck_H

#include "CPck.h"
#include "CNode.h"
#include "CConnection.h"

using namespace std;

class CPck;
class CNode;

class RSVP_ErrPck:public CPck {
public:
    CConnection* Conn;
    CNode* GenerationNode;

    CNode* ConnSourceNode;
    CNode* ConnDestinationNode;
    int ConnId;

    vector<CNode*> NodePath;
    vector<CLink*> LinkPath;

    //For flexible
    int base_slot;
    int width_slots;
    
    ConnectionType ConnType;

    RSVP_ErrPck();
    RSVP_ErrPck(CNode* src,CNode* dst, RSVP_ResvPck* Path_pk,int dim,int ttl,double msgGenTime,CConnection* conn,CNode* GenNode);
    ~RSVP_ErrPck();

    virtual void arrived(CNode* node);
};

#endif


