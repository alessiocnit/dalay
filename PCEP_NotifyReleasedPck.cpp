#include "PCEP_NotifyReleasedPck.h"
#include "PCx_Node.h"
#include "CConnection.h"
#include "RSVP_TearDownPck.h"

PCEP_NotifyReleasedPck::~PCEP_NotifyReleasedPck() {
    //cout << "Destroyed PCEP_PCReq\n";
}

PCEP_NotifyReleasedPck::PCEP_NotifyReleasedPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, RSVP_TearDownPck* pk):
        CPck::CPck(src,dst,dim,ttl,msgGenTime) {
    this->Type=PCEP_NotifyReleased;
    this->Comment="PCEP_NotifyReleasedPck";
    this->RestorationFlag=pk->RestorationFlag;

    this->ConnId=pk->Conn->Id;

    for (int i=0; i<pk->LinkPath.size(); i++) {
      this->ConnLinkPath.push_back(pk->LinkPath[i]);
    }
    
    this->ConnSource=pk->ConnSourceNode;
    this->ConnDestination=pk->ConnDestinationNode;
    
    this->conn_base_slot=pk->base_slot;
    this->conn_width_slots=pk->width_slots;

    this->ProcTime=PROC_PCEP_NOTIFY;
}

void PCEP_NotifyReleasedPck::arrived(CNode *node) {
    node->ReceivedPck_PCEP_NotifyReleased(this);
}
