#include "OFP_FlowModPck.h"
#include "CNode.h"
#include "CConnection.h"

OFP_FlowModPck::OFP_FlowModPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime,vector<CConnection*>& conns, FlowModeSession* session, OFP_FlowModType type):
CPck::CPck(src,dst,dim,ttl,msgGenTime) {
    #ifdef MEMORY_DEBUG
    this->active_pk++;
    #endif
  
    this->Type=OFP_FlowMod;
    
    this->Session=session;
    this->OFP_PckType=type;
    
    if (this->OFP_PckType==FlowMod_DELETE) 
      ErrorMsg("Wrong builder for FlowMod_DELETE");
    
    if (this->OFP_PckType==FlowMod_ACK) 
      ErrorMsg("Wrong builder for FlowMod_DELETE");
    
    if (this->OFP_PckType==FlowMod_ADD) {
      this->ProcTime=PROC_OFP_FLOWMOD;
      this->Comment="FlowMod_ADD";
    }
    
    if (this->OFP_PckType==FlowMod_CLOSE) { 
      this->ProcTime=0.0;
      this->Comment="FlowMod_CLOSE";
    }
    
    for (int i=0; i<conns.size();i++) {
      flow_entry* flow = new flow_entry;
      
      flow->Conn=conns[i];
      flow->ConnId=conns[i]->Id;
      flow->base_slot=conns[i]->base_slot;
      flow->width_slots=conns[i]->width_slots;
      flow->ConnNodePath=conns[i]->NodePath;
      //flow->ConnLinkPath=conns[i]->LinkPath;
      
      this->FlowEntries.push_back(flow);
    }    
}

//Used for DEL packets without the pointer to the Session
OFP_FlowModPck::OFP_FlowModPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime,vector<CConnection*>& conns,OFP_FlowModType type):
CPck::CPck(src,dst,dim,ttl,msgGenTime) {
  #ifdef MEMORY_DEBUG
  this->active_pk++;
  #endif
    
  this->Type=OFP_FlowMod;
    
  this->OFP_PckType=type;
  //this->Session=session;
    
  if (this->OFP_PckType==FlowMod_ADD) 
    ErrorMsg("Wrong builder for FlowMod_ADD");
    
  if (this->OFP_PckType==FlowMod_ACK) 
    ErrorMsg("Wrong builder for FlowMod_ACK");
    
  if (this->OFP_PckType==FlowMod_CLOSE)
    ErrorMsg("Wrong builder for FlowMod_CLOSE");
    
  if (this->OFP_PckType==FlowMod_DELETE) {
    this->ProcTime=PROC_OFP_FLOWMOD;
    this->Comment="FlowMod_DELETE";
  }
    
  for (int i=0; i<conns.size();i++) {
    flow_entry* flow = new flow_entry;
      
    flow->Conn=conns[i];
    flow->ConnId=conns[i]->Id;
    flow->base_slot=conns[i]->base_slot;
    flow->width_slots=conns[i]->width_slots;
    flow->ConnNodePath=conns[i]->NodePath;
    //flow->ConnLinkPath=conns[i]->LinkPath;
      
    this->FlowEntries.push_back(flow);
  }    
}

//Used for ACK packets without connections
OFP_FlowModPck::OFP_FlowModPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, FlowModeSession* session, OFP_FlowModType type):
CPck::CPck(src,dst,dim,ttl,msgGenTime) {
    
  #ifdef MEMORY_DEBUG
  this->active_pk++;
  #endif
    
  this->Type=OFP_FlowMod;

  this->OFP_PckType=type;
  
  if (this->OFP_PckType!=FlowMod_ACK) {
    ErrorMsg("Wrong builder");
  }
  
  this->Session=session;
  
  this->ProcTime=PROC_OFP_FLOWMOD;
  this->Comment="FlowMod_ACK";
}

OFP_FlowModPck::~OFP_FlowModPck() {
  for (int i=0; i<this->FlowEntries.size(); i++)
    delete this->FlowEntries[i];
  this->FlowEntries.clear();
  
  
  
  #ifdef MEMORY_DEBUG
  this->active_pk--;
  cout << "MEMORY DEBUG: OFP_FlowModPck " << this->active_pk << endl;
  #endif
}

void OFP_FlowModPck::arrived(CNode *node) {
    node->ReceivedPck_OFP_FlowModPck(this);
}