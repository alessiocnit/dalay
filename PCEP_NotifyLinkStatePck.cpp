#include "PCEP_NotifyLinkStatePck.h"

#include "CNode.h"
#include "OSPF_LsaPck.h"

PCEP_NotifyLinkStatePck::PCEP_NotifyLinkStatePck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, OSPF_LsaPck* lsa_pk):
  CPck::CPck(src,dst,dim,ttl,msgGenTime) {
    this->Type=PCEP_NotifyLinkState;
    this->Comment="PCEP_NotifyLinkStatePck";
    
  this->Link_Flooded=lsa_pk->Link_Flooded;
  this->Advertising_Node=lsa_pk->Advertising_Node;
  this->sequence_number=lsa_pk->sequence_number;
  this->free_lambdas=lsa_pk->free_lambdas;
  this->LambdaStatus=lsa_pk->LambdaStatus;
}

PCEP_NotifyLinkStatePck::~PCEP_NotifyLinkStatePck() {
}

void PCEP_NotifyLinkStatePck::arrived(CNode* node) {
  node->ReceivedPck_PCEP_NotifyLinkState(this);
}


