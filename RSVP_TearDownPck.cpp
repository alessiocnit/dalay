#include "RSVP_TearDownPck.h"

RSVP_TearDownPck::RSVP_TearDownPck(CNode* src,CNode* dst,int dim, int ttl,double msgGenTime,CConnection* conn, int RestorationFlag):
        CPck::CPck(src,dst,dim,ttl,msgGenTime) {
    this->Type=RSVP_TEARDOWN;
    this->Comment="RSVP_TEARDOWN";
    this->ProcTime=PROC_RSVP_PATHTEAR;

    this->RestorationFlag=RestorationFlag;
    this->Conn=conn;
    this->ConnType=conn->Type;

    this->NodePath=conn->NodePath;
    this->LinkPath=conn->LinkPath;
    
    this->base_slot=conn->base_slot;
    this->width_slots=conn->width_slots;

    this->ConnSourceNode=conn->SourceNode;
    this->ConnDestinationNode=conn->DestinationNode;
    //cout << "Created RSVP_ERR\n";
}

RSVP_TearDownPck::~RSVP_TearDownPck() {
    //cout << "Destroyed RSVP_ERR\n";
}

void RSVP_TearDownPck::arrived(CNode *node) {
    node->ReceivedPck_RSVP_TEARDOWN(this);
}

