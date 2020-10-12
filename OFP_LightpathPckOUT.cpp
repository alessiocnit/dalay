#include "OFP_LightpathPckOUT.h"

#include "CConnection.h"

OFP_LightpathPckOUT::OFP_LightpathPckOUT(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, CConnection* conn):
  CPck::CPck(src,dst,dim,ttl,msgGenTime) {
  
  this->Type=OFP_Lightpath_IN;
  this->Comment="OFP_Lightpath_IN";
  
  this->Conn=conn;
  this->ConnId=conn->Id;
  this->ConnGenerationTime=conn->GenerationTime;
  this->ConnDurationTime=conn->DurationTime;
  
  this->ProcTime=PROC_OFP_LIGHTPATH_OUT;
}

OFP_LightpathPckOUT::~OFP_LightpathPckOUT(){

}

void OFP_LightpathPckOUT::arrived(CNode *node) {
    node->ReceivedPck_OFP_LightpathPckOUT(this);
}