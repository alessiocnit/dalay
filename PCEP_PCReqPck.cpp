#include "PCEP_PCReqPck.h"
#include "PCx_Node.h"
#include "CConnection.h"

PCEP_PCReqPck::~PCEP_PCReqPck() {
    //cout << "Destroyed PCEP_PCReq\n";
}

PCEP_PCReqPck::PCEP_PCReqPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, CConnection* conn, int rest_flag, PCEP_ReqType type):
        CPck::CPck(src,dst,dim,ttl,msgGenTime) {
	  
    this->Type=PCEP_PCReq;
    this->Comment="PCEP_PCReqPck";
    this->ProcTime=PROC_PCEP_PCREQ;
    this->ReqType=type;

    this->Conn=conn;
    this->ConnSource=conn->SourceNode;
    this->ConnDestination=conn->DestinationNode;
    this->Conn_width_slots=conn->width_slots;
    this->RestorationFlag=rest_flag;

    this->RestorationPath=NULL;

    PCx_Node* PCC_Src=static_cast <PCx_Node*> (this->SrcNode);
}

void PCEP_PCReqPck::arrived(CNode *node) {
    node->ReceivedPck_PCEP_PCReq(this);
}
