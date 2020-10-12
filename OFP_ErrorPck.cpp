#include "OFP_ErrorPck.h"
#include "CConnection.h"

OFP_ErrorPck::OFP_ErrorPck(CNode* src,CNode* dst,int dim, int ttl,double msgGenTime,CConnection* conn, FlowModeSession* Session):
  CPck::CPck(src,dst,dim,ttl,msgGenTime) {
    this->active_pk++;
    
    this->Type=OFP_Error;
    
    this->Conn=conn;
    this->ConnId=conn->Id;
    this->Session=Session;
}

OFP_ErrorPck::~OFP_ErrorPck() {
  this->active_pk--;
  
  #ifdef MEMORY_DEBUG
  cout << "MEMORY DEBUG: OFP_ErrorPck " << this->active_pk << endl;
  #endif
}

