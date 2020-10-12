#ifndef CPATHSTATE_TABLE_H
#define CPATHSTATE_TABLE_H

#include "CNet.h"
#include "CNode.h"

using namespace std;

class RSVP_PathPck;
class RSVP_ResvPck;
class RSVP_PathErrPck;

struct PathState {
    CConnection* Conn;
    CLink* OutgoingLink;
    CNode* DestinationNode;
    int CollisionDetected;
    TieBreak SuggestedWA;
    int SuggestedLabel;
    vector<int> LabelSet;
};

class CPathState_Table {
public:
    //PathState Database
    CNode* ParentNode;
    vector<PathState*> PathState_Database;

    CPathState_Table();
    CPathState_Table(CNode* node);
    ~CPathState_Table();

    void PathStateInsert(RSVP_PathPck* pk,CLink* link);
    void PathStateRemove(RSVP_ResvPck* pk);
    void PathStateRemove(RSVP_PathErrPck* pk);
};

#endif
