#include "PCEP_PCReportPck.h"
#include "PCx_Node.h"
#include "CConnection.h"


PCEP_PCReportPck::PCEP_PCReportPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, CConnection* conn, InterDomainSession* session, int reply):
CPck::CPck(src,dst,dim,ttl,msgGenTime) {
    this->Type=PCEP_PCReport;
    this->Comment="PCEP_PCReportPck";
    this->ProcTime=PROC_PCEP_PCREPORT;

    this->Conn=conn;
    this->ConnSource=conn->SourceNode;
    this->ConnDestination=conn->DestinationNode;
    
    this->ConnId=conn->Id;
    
    this->SetupSession=session;
    
    this->ReplyType=reply;
}

PCEP_PCReportPck::~PCEP_PCReportPck() {
    //cout << "Destroyed PCEP_PCReq\n";
}

void PCEP_PCReportPck::arrived(CNode *node) {
    node->ReceivedPck_PCEP_PCReport(this);
}
