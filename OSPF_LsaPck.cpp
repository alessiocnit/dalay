#include "OSPF_LsaPck.h"

OSPF_LsaPck::OSPF_LsaPck() {
    this->Type=OSPF_LSA;
    this->Comment="OSPF_LSA";
    this->ProcTime=PROC_OSPF_LSA;
}

OSPF_LsaPck::~OSPF_LsaPck() {
}

OSPF_LsaPck::OSPF_LsaPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, LinkState* ls):
        CPck::CPck(src,dst,dim,ttl,msgGenTime) {
    this->Type=OSPF_LSA;
    this->Comment="OSPF_LSA";
    this->ProcTime=PROC_OSPF_LSA;

    this->Advertising_Node=ls->AdvertisingNode;
    this->Link_Flooded=ls->Link;
    this->sequence_number=ls->SequenceNumber;
    this->free_lambdas=ls->FreeLambdas;
    this->LambdaStatus=ls->LambdaStatus;
}

OSPF_LsaPck::OSPF_LsaPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, OSPF_LsaPck* pk):
        CPck::CPck(src,dst,dim,ttl,msgGenTime) {
    this->Type=OSPF_LSA;
    this->Comment="OSPF_LSA";
    this->ProcTime=PROC_OSPF_LSA;

    this->Advertising_Node=pk->Advertising_Node;
    this->Link_Flooded=pk->Link_Flooded;
    this->sequence_number=pk->sequence_number;
    this->free_lambdas=pk->free_lambdas;
    this->LambdaStatus=pk->LambdaStatus;
}

OSPF_LsaPck::OSPF_LsaPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, CNode* AdvertisingNode, CLink* AdvertisedLink):
        CPck::CPck(src,dst,dim,ttl,msgGenTime) {
	  
    this->Type=OSPF_LSA;
    this->Comment="OSPF_LSA";
    this->ProcTime=PROC_OSPF_LSA;

    this->Link_Flooded=AdvertisedLink;
    this->Advertising_Node=AdvertisingNode;
    
    if (AdvertisingNode==AdvertisedLink->FromNode) 
      this->sequence_number=AdvertisedLink->LSA_LastSequenceNumber_FromNode+1;
    if (AdvertisingNode==AdvertisedLink->ToNode) 
      this->sequence_number=AdvertisedLink->LSA_LastSequenceNumber_ToNode+1;
}

void OSPF_LsaPck::LoadLsaFields (LinkState* ls) {
    this->Advertising_Node=ls->AdvertisingNode;
    this->Link_Flooded=ls->Link;
    this->sequence_number=ls->SequenceNumber;
    this->free_lambdas=ls->FreeLambdas;
    this->LambdaStatus=ls->LambdaStatus;
}

void OSPF_LsaPck::arrived(CNode* node) {
    node->ReceivedPck_OSPF_LSA(this);
}

