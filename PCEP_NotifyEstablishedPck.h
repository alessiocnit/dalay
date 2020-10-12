#ifndef PCEP_NotifyEstablishedPck_H
#define PCEP_NotifyEstablishedPck_H

#include "CPck.h"

using namespace std;

class CNode;
class PCx_Node;
class CLink;
class CConnection;

enum NotifyEstablishedType {NOTIFY_ESTABLISHED,NOTIFY_ROUTED};

class PCEP_NotifyEstablishedPck:public CPck {
public:
    int RestorationFlag; //This is one if the request is for restoration
    
    NotifyEstablishedType SubType;

    int ConnId;
    int conn_base_slot;
    int conn_width_slots;
    int conn_pce_SuggestedLabel;

    CConnection* Conn;

    vector<CLink*> ConnLinkPath;
    CNode* ConnSource;
    CNode* ConnDestination;

    PCEP_NotifyEstablishedPck(CNode* src, PCx_Node* dst, int dim, int ttl, double msgGenTime, CConnection* conn, int rest_flag);
    ~PCEP_NotifyEstablishedPck();

    virtual void arrived(CNode* node);
};

#endif





