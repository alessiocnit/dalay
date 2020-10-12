#include "OSPF_LinkRepairPck.h"

OSPF_LinkRepairPck::OSPF_LinkRepairPck() {
    this->Type=OSPF_LinkRepair;
    //cout << "Created RSVP_ERR\n";
}

OSPF_LinkRepairPck::OSPF_LinkRepairPck(CNode* src,CNode* dst,int dim, int ttl,double msgGenTime,CLink* link):
        CPck::CPck(src,dst,dim,ttl,msgGenTime) {
    this->Type=OSPF_LinkRepair;
    this->Comment="OSPF_LinkRepair";
    this->ProcTime=PROC_OSPF_LINKREPAIR;

    this->RepairedLink=link;
}

OSPF_LinkRepairPck::~OSPF_LinkRepairPck() {
    //cout << "Destroyed RSVP_ERR\n";
}

void OSPF_LinkRepairPck::arrived(CNode *node) {
    node->ReceivedPck_OSPF_LinkRepair(this);
}

