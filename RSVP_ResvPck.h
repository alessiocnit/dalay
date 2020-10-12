#ifndef RSVP_ResvPck_H
#define RSVP_ResvPck_H

#include "CPck.h"
#include "CNode.h"
#include "CConnection.h"

using namespace std;

class CPck;
class CNode;

class RSVP_ResvPck:public CPck {
public:
    int RestorationFlag;
    CConnection* Conn;

    CNode* ConnSourceNode;
    CNode* ConnDestinationNode;

    vector<CNode*> NodePath;
    vector<CLink*> LinkPath;

    TieBreak SuggestedWA;

    int SuggestedLabel_ON;
    int SuggestedLabel;

    int LabelSet_ON;
    vector<int> LabelSet;

    //For flexible
    int base_slot;
    int width_slots;
    
    ConnectionType ConnType;

    RSVP_ResvPck(CNode* src,CNode* dst, RSVP_PathPck* Path_pk,int dim,int ttl,double msgGenTime,CConnection* conn,int RestFlag);
    ~RSVP_ResvPck();

    virtual void arrived(CNode* node);
};

#endif
