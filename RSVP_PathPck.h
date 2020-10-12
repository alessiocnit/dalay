#ifndef RSVP_PathPck_H
#define RSVP_PathPck_H

#include "CPck.h"
#include "CNode.h"
#include "CConnection.h"

using namespace std;

class CPck;
class CNode;

class RSVP_PathPck:public CPck {
public:
    int RestorationFlag;
    CConnection* Conn;

    CNode* ConnSourceNode;
    CNode* ConnDestinationNode;

    vector<CDomain*> DomainPath;
    vector<CNode*> EdgeNodePath;

    vector<CNode*> NodePath;
    vector<CLink*> LinkPath;

    //int CollisionDetection_ON;
    //int CollisionDetected;
    TieBreak SuggestedWA;

    int LabelSet_ON;
    vector<int> LabelSet;

    int SuggestedLabel_ON;
    int SuggestedLabel;

    int TraversedDomains;
    
    //For flexible
    //int base_slot; to be selected at destination
    int width_slots;

    ConnectionType ConnType;

    RSVP_PathPck(CNode* src,CNode* dst,int dim,int ttl,double msgGenTime,CConnection* conn,int RestFlag);
    ~RSVP_PathPck();

    virtual void arrived(CNode* node);
};

#endif

