#include "PCEP_PCInitPck.h"
#include "PCx_Node.h"
#include "CConnection.h"


PCEP_PCInitPck::PCEP_PCInitPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, CConnection* conn, InterDomainSession* session):
CPck::CPck(src,dst,dim,ttl,msgGenTime) {
    this->Type=PCEP_PCInit;
    this->Comment="PCEP_PCInitPck";
    this->ProcTime=PROC_PCEP_PCINIT;

    this->Conn=conn;
    this->ConnSource=conn->SourceNode;
    this->ConnDestination=conn->DestinationNode;
    
    this->ConnId=conn->Id;
    
    this->SetupSession=session;
}

PCEP_PCInitPck::~PCEP_PCInitPck() {
    //cout << "Destroyed PCEP_PCReq\n";
}

void PCEP_PCInitPck::arrived(CNode *node) {
    node->ReceivedPck_PCEP_PCInit(this);
}
