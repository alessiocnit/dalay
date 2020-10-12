#include "PCEP_NotifyEstablishedPck.h"
#include "PCx_Node.h"
#include "CConnection.h"

PCEP_NotifyEstablishedPck::~PCEP_NotifyEstablishedPck() {
    //cout << "Destroyed PCEP_PCReq\n";
}

PCEP_NotifyEstablishedPck::PCEP_NotifyEstablishedPck(CNode* src, PCx_Node* dst, int dim, int ttl, double msgGenTime, CConnection* conn, int rest_flag):
        CPck::CPck(src,dst,dim,ttl,msgGenTime) {
    this->Type=PCEP_NotifyEstablished;
    this->SubType=NOTIFY_ESTABLISHED;
    this->Comment="PCEP_NotifyEstablishedPck";
    this->RestorationFlag=rest_flag;

    this->Conn=conn;
    this->ConnId=conn->Id;
    this->ConnLinkPath=conn->LinkPath;
    this->ConnSource=conn->SourceNode;
    this->ConnDestination=conn->DestinationNode;
    
    this->conn_base_slot=conn->base_slot;
    this->conn_width_slots=conn->width_slots;
    this->conn_pce_SuggestedLabel=conn->PCE_SuggestedLabel;

    this->ProcTime=PROC_PCEP_NOTIFY;
}

void PCEP_NotifyEstablishedPck::arrived(CNode *node) {
    node->ReceivedPck_PCEP_NotifyEstablished(this);
}
