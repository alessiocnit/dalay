#include "RSVP_PathErrPck.h"
#include "RSVP_PathPck.h"
#include "RSVP_ResvPck.h"

RSVP_PathErrPck::RSVP_PathErrPck() {
    this->Type=RSVP_PATH_ERR;
    this->Comment="RSVP_PATH_ERR";
    this->ProcTime=PROC_RSVP_PATHERR;
    //cout << "Created RSVP_PATH_ERR\n";
}

RSVP_PathErrPck::~RSVP_PathErrPck() {
    //cout << "Destroyed RSVP_PATH_ERR\n";
}

RSVP_PathErrPck::RSVP_PathErrPck(CNode* src, CNode* dst, RSVP_PathPck* Path_pk, int dim, int ttl, double msgGenTime, CConnection* conn, CNode* GenNode, Block_SubType generationmode, int RestFlag):
        CPck::CPck(src,dst,dim,ttl,msgGenTime) {
    this->Type=RSVP_PATH_ERR;
    this->Comment="RSVP_PATH_ERR";
    this->ProcTime=PROC_RSVP_PATHERR;

    this->GenerationDirection=FORWARD;

    this->RestorationFlag=RestFlag;
    this->Conn=conn;
    this->GenerationNode=GenNode;
    this->GenerationMode=generationmode;
    this->ConnType=conn->Type;

    this->NodePath=Path_pk->NodePath;
    this->LinkPath=Path_pk->LinkPath;
    
    this->width_slots=Path_pk->width_slots;

    this->ConnSourceNode=Path_pk->ConnSourceNode;
    this->ConnDestinationNode=Path_pk->ConnDestinationNode;
    //cout << "Created RSVP_PATH_ERR\n";
}

RSVP_PathErrPck::RSVP_PathErrPck(CNode* src, CNode* dst, RSVP_ResvPck* Resv_pk, int dim, int ttl, double msgGenTime, CConnection* conn, CNode* GenNode, Block_SubType generationmode, int RestFlag):
        CPck::CPck(src,dst,dim,ttl,msgGenTime) {
    this->Type=RSVP_PATH_ERR;
    this->Comment="RSVP_PATH_ERR";
    this->ProcTime=PROC_RSVP_PATHERR;

    this->GenerationDirection=BACKWARD;

    this->RestorationFlag=RestFlag;
    this->Conn=conn;
    this->GenerationNode=GenNode;
    this->GenerationMode=generationmode;
    this->ConnType=conn->Type;

    this->NodePath=Resv_pk->NodePath;
    this->LinkPath=Resv_pk->LinkPath;
    
    this->base_slot=Resv_pk->base_slot;
    this->width_slots=Resv_pk->width_slots;

    this->ConnSourceNode=Resv_pk->ConnSourceNode;
    this->ConnDestinationNode=Resv_pk->ConnDestinationNode;
    //cout << "Created RSVP_PATH_ERR\n";
}

void RSVP_PathErrPck::arrived(CNode *node) {
    node->ReceivedPck_RSVP_PATH_ERR(this);
}

