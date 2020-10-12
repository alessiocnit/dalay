#include "RSVP_ResvPck.h"
#include "RSVP_PathPck.h"

RSVP_ResvPck::RSVP_ResvPck(CNode* src,CNode* dst, RSVP_PathPck* Path_pk, int dim,int ttl,double msgGenTime,CConnection* conn,int RestFlag):
        CPck::CPck(src,dst,dim,ttl,msgGenTime) {
	  
    this->Type=RSVP_RESV;
    this->Comment="RSVP_RESV";

    //The ResvPck is created with the increased ProcTime for considering WA
    this->ProcTime=PROC_RSVP_RESV;
    //this->ProcTime=0.0;

    this->RestorationFlag=RestFlag;
    this->Conn=conn;
    this->ConnType=conn->Type;

    this->SuggestedWA=Path_pk->SuggestedWA;
    this->SuggestedLabel_ON=Path_pk->SuggestedLabel_ON;
    this->SuggestedLabel=Path_pk->SuggestedLabel;
    this->LabelSet_ON=Path_pk->LabelSet_ON;
    this->LabelSet=Path_pk->LabelSet;

    this->base_slot=-1;
    this->width_slots=0;
    
    this->NodePath=Path_pk->NodePath;
    this->LinkPath=Path_pk->LinkPath;

    this->ConnSourceNode=Path_pk->ConnSourceNode;
    this->ConnDestinationNode=Path_pk->ConnDestinationNode;
    //cout << "RSVP_RESV correctly created" << endl;
}

RSVP_ResvPck::~RSVP_ResvPck() {
    //cout << "Destroyed RSVP_RESV\n";
}

void RSVP_ResvPck::arrived(CNode *node) {
    node->ReceivedPck_RSVP_RESV(this);
}
