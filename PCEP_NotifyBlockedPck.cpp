#include "PCEP_NotifyBlockedPck.h"
#include "PCx_Node.h"
#include "CConnection.h"

PCEP_NotifyBlockedPck::~PCEP_NotifyBlockedPck() {
    //cout << "Destroyed PCEP_PCReq\n";
}

PCEP_NotifyBlockedPck::PCEP_NotifyBlockedPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, CConnection* conn, RSVP_PathErrPck* PathErr_pk, Block_Type block_direction, int rest_flag):
        CPck::CPck(src,dst,dim,ttl,msgGenTime) {
    this->Type=PCEP_NotifyBlocked;
    this->Comment="PCEP_NotifyBlocked";
    this->RestorationFlag=rest_flag;
    this->Conn=conn;

    //In case of FORWARD blocking the WA has not been yet performed
    if (block_direction==FORWARD)
        this->ConnUsedLambda=-1;
    if (block_direction==BACKWARD)
        this->ConnUsedLambda=PathErr_pk->base_slot;

    this->ConnId=conn->Id;
    this->ConnLinkPath=conn->LinkPath;
    this->ConnSource=conn->SourceNode;
    this->ConnDestination=conn->DestinationNode;
    
    this->conn_base_slot=conn->base_slot;
    this->conn_width_slots=conn->width_slots;
    this->conn_pce_SuggestedLabel=conn->PCE_SuggestedLabel;

    this->ProcTime=PROC_PCEP_NOTIFY;
}

void PCEP_NotifyBlockedPck::arrived(CNode *node) {
    node->ReceivedPck_PCEP_NotifyBlocked(this);
}
