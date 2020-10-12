#include "OSPF_LinkFailurePck.h"

OSPF_LinkFailurePck::OSPF_LinkFailurePck() {
    this->Type=OSPF_LinkFailure;
    //cout << "Created RSVP_ERR\n";
}

OSPF_LinkFailurePck::OSPF_LinkFailurePck(CNode* src,CNode* dst,int dim, int ttl,double msgGenTime,CLink* link):
        CPck::CPck(src,dst,dim,ttl,msgGenTime) {
    this->Type=OSPF_LinkFailure;
    this->Comment="OSPF_LinkFailure";
    this->ProcTime=PROC_OSPF_LINKFAILURE;

    this->DisruptedLink=link;
    this->FailureDetectionTime=msgGenTime;
}

OSPF_LinkFailurePck::~OSPF_LinkFailurePck() {
    //cout << "Destroyed RSVP_ERR\n";
}

void OSPF_LinkFailurePck::arrived(CNode *node) {
    node->ReceivedPck_OSPF_LinkFailure(this);
}

