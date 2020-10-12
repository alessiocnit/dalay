#ifndef RSVP_PathErrPck_H
#define RSVP_PathErrPck_H

#include "CPck.h"
#include "RSVP_PathPck.h"
#include "RSVP_ResvPck.h"
#include "CNode.h"
#include "CConnection.h"

using namespace std;

class CPck;
class CNode;

/*----------------------------------------------------------------------------------------------------------------------------------
A PathErr message can be generated for several reasons
1. BACKWARD direction: when the RESV message propagation fails
	- COLLISION: resource contention in reserving an already busy lambda channel
	- LINK_DOWN: error in reserving a lambda channel along a disrupted data link
2. FORWARD direction: when the PATH message propagation fails
	- EMPTY_LABELSET: the LabelSet object is empty in an intermediate node
-----------------------------------------------------------------------------------------------------------------------------------*/

class RSVP_PathErrPck:public CPck {
public:
    int RestorationFlag;
    CConnection* Conn;

    Block_Type GenerationDirection;
    Block_SubType GenerationMode;

    CNode* GenerationNode;
    CNode* ConnSourceNode;
    CNode* ConnDestinationNode;

    vector<CNode*> NodePath;
    vector<CLink*> LinkPath;

    //For flexible
    int base_slot;
    int width_slots;

    ConnectionType ConnType;

    RSVP_PathErrPck();

    //It is used when the PathErr is generated during FORWARD signaling
    RSVP_PathErrPck(CNode* src,CNode* dst, RSVP_PathPck* Path_pk, int dim,int ttl, double msgGenTime, CConnection* conn, CNode* GenNode, Block_SubType generationmode, int RestFlag);
    //It is used when the PathErr is generated during BACKWARD signaling
    RSVP_PathErrPck(CNode* src,CNode* dst, RSVP_ResvPck* Resv_pk, int dim,int ttl, double msgGenTime, CConnection* conn, CNode* GenNode, Block_SubType generationmode, int RestFlag);

    ~RSVP_PathErrPck();

    virtual void arrived(CNode* node);
};

#endif
