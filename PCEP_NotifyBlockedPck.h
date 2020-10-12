#ifndef PCEP_NotifyBlockedPck_H
#define PCEP_NotifyBlockedPck_H

#include "CPck.h"
#include "RSVP_PathErrPck.h"

using namespace std;

class CNode;
class CLink;
class CConnection;

class PCEP_NotifyBlockedPck:public CPck {
public:
    int RestorationFlag; //This is one if the request is for restoration

    int ConnId;
    int ConnUsedLambda;
    
    int conn_base_slot;
    int conn_width_slots;
    int conn_pce_SuggestedLabel;
    
    CConnection* Conn;
    vector<CLink*> ConnLinkPath;
    CNode* ConnSource;
    CNode* ConnDestination;

    PCEP_NotifyBlockedPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, CConnection* conn, RSVP_PathErrPck* PathErr_pk, Block_Type block_direction,int rest_flag);
    ~PCEP_NotifyBlockedPck();

    virtual void arrived(CNode* node);
};

#endif





