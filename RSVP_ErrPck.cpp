#include "RSVP_ErrPck.h"
#include "RSVP_ResvPck.h"

RSVP_ErrPck::RSVP_ErrPck() {
    this->Type=RSVP_RESV_ERR;
    this->Comment="RSVP_RESV_ERR";
    this->ProcTime=PROC_RSVP_RESVERR;
    //cout << "Created RSVP_ERR\n";
}

RSVP_ErrPck::~RSVP_ErrPck() {
    //cout << "Destroyed RSVP_ERR\n";
}

RSVP_ErrPck::RSVP_ErrPck(CNode* src,CNode* dst, RSVP_ResvPck* Resv_pk, int dim, int ttl,double msgGenTime,CConnection* conn,CNode* GenNode):
        CPck::CPck(src,dst,dim,ttl,msgGenTime) {
    this->Type=RSVP_RESV_ERR;
    this->Comment="RSVP_RESV_ERR";
    this->ProcTime=PROC_RSVP_RESVERR;

    this->Conn=conn;
    this->ConnType=conn->Type;
    this->GenerationNode=GenNode;

    this->NodePath=Resv_pk->NodePath;
    this->LinkPath=Resv_pk->LinkPath;
    
    this->base_slot=Resv_pk->base_slot;
    this->width_slots=Resv_pk->width_slots;

    this->ConnSourceNode=Resv_pk->ConnSourceNode;
    this->ConnDestinationNode=Resv_pk->ConnDestinationNode;

    this->ConnId=conn->Id;
    //cout << "Created RSVP_ERR\n";
}

void RSVP_ErrPck::arrived(CNode *node) {
    node->ReceivedPck_RSVP_RESV_ERR(this);
}

