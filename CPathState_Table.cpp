#include "CPathState_Table.h"
#include "RSVP_PathPck.h"
#include "RSVP_ResvPck.h"
#include "RSVP_PathErrPck.h"
#include "Statistics.h"

//#define STDOUT_PATHSTATE

CPathState_Table::CPathState_Table() {
}

CPathState_Table::CPathState_Table(CNode* node) {
    this->ParentNode=node;
    //cout << "CPathState_Table created" << endl;
}

CPathState_Table::~CPathState_Table() {
    if (this->PathState_Database.size())
        ErrorMsg("PathState_dB is not empty while destroying");
}

void CPathState_Table::PathStateInsert(RSVP_PathPck* pk, CLink* link) {

    PathState* ps=new PathState;

    ps->Conn = pk->Conn;
    ps->OutgoingLink = link;
    ps->DestinationNode = pk->Conn->DestinationNode;
    ps->SuggestedLabel = pk->SuggestedLabel;
    ps->LabelSet = pk->LabelSet;
    ps->SuggestedWA = pk->SuggestedWA;

    this->PathState_Database.push_back(ps);

#ifdef STDOUT_PATHSTATE
    cout << "\nPathState - NODE: " << this->ParentNode->Id << " conn id " << pk->Conn->Id << " PathState inserted TIME: " << this->ParentNode->ParentNet->Now << endl;
#endif
};

void CPathState_Table::PathStateRemove(RSVP_PathErrPck* pk) {
    int i=0;
    while (pk->Conn!=this->PathState_Database[i]->Conn) i++;
    delete this->PathState_Database[i];
    vector<PathState*>::iterator it_ps=this->PathState_Database.begin();
    it_ps+=i;
    this->PathState_Database.erase(it_ps);

#ifdef STDOUT_PATHSTATE
    cout << "\nPathState - NODE: " << this->ParentNode->Id << " conn id " << pk->Conn->Id << " PathState removed RSVP_PATH_ERR TIME: " << this->ParentNode->ParentNet->Now << endl;
#endif
};

void CPathState_Table::PathStateRemove(RSVP_ResvPck* pk) {
    int i=0;
    while (pk->Conn!=this->PathState_Database[i]->Conn) i++;
    delete this->PathState_Database[i];

    vector<PathState*>::iterator it_ps=this->PathState_Database.begin();
    it_ps+=i;
    this->PathState_Database.erase(it_ps);

#ifdef STDOUT_PATHSTATE
    cout << "\nPathState - NODE: " << this->ParentNode->Id << " conn id " << pk->Conn->Id << " PathState removed RSVP_RESV TIME: " << this->ParentNode->ParentNet->Now << endl;
#endif
};

