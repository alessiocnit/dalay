#include "OFP_LightpathPckIN.h"

#include "CConnection.h"

OFP_LightpathPckIN::OFP_LightpathPckIN(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, vector<CConnection*>& conns):
CPck::CPck(src,dst,dim,ttl,msgGenTime) {
    
  this->Type=OFP_Lightpath_IN;
  this->Comment="OFP_Lightpath_IN";
  
  this->RestorationFlag=0;
  
  for (int i=0;i<conns.size();i++) {
    this->Conns.push_back(conns[i]);
    this->ConnIds.push_back(conns[i]->Id);
    this->ConnGenerationTimes.push_back(conns[i]->GenerationTime);
    this->ConnDurationTimes.push_back(conns[i]->DurationTime);
  }
}

OFP_LightpathPckIN::~OFP_LightpathPckIN() {
}

void OFP_LightpathPckIN::arrived(CNode *node) {
    node->ReceivedPck_OFP_LightpathPckIN(this);
}