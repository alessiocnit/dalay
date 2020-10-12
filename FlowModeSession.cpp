#include "FlowModeSession.h"

#include "CNet.h"
#include "OFP_FlowModPck.h"
#include "OFP_LightpathPckOUT.h"
#include "PCEP_PCReportPck.h"
#include "PCEP_PCInitPck.h"
#include "WritePckNodeEvent.h"
#include "CConnection.h"
#include "CNode.h"
#include "PCx_Node.h"

FlowModeSession::FlowModeSession(CNet* net, CNode* controller, vector<CConnection*>& conns, FlowModeType type, CNode* reply_node)
{
  this->active_flowmodesessions++;
  this->Type=type;
  
  
  this->ParentNet=net;
  this->OpenFlowController=controller;
  this->NodeToReply=reply_node;
  this->SessionStartTime=net->Now;
  
  this->SessionStatus=OFP_CLOSED;
  this->RestorationFlag=0;
  
  for (int i=0; i<conns.size(); i++) {
    this->Conns.push_back(conns[i]);
  }
}

FlowModeSession::~FlowModeSession() {
  for (int i=0; i<this->Requests.size(); i++)
    delete this->Requests[i];
  this->Requests.clear();
  this->Conns.clear();

  this->active_flowmodesessions--;
  
  #ifdef MEMORY_DEBUG
  cout << "MEMORY DEBUG FlowModeSession: " << this->active_flowmodesessions << endl; 
  #endif
  
  return;
}

void FlowModeSession::start(vector<CConnection*>& conns) {
  CNode* ActualNode;
  FlowModRequest* ActualRequest;
  int visited[NMAX];
  vector<CNode*> n;
  
  this->SessionStatus=OFP_OPEN;
  
  //cout << "SESSION number of conns to be configured " << conns.size() << endl;
  
  for (int i=0; i<NMAX; i++)
    visited[i]=0;
  
  for (int i=0; i<conns.size(); i++) {
    for (int j=0; j<conns[i]->NodePath.size(); j++) {
      if (visited[conns[i]->NodePath[j]->Id]==0) {
	n.push_back(conns[i]->NodePath[j]);
	visited[conns[i]->NodePath[j]->Id]=1;
      }
    }
  }
  
  //The list of requests is created with all the required attributes
  for (int i=0; i<n.size(); i++) {
    ActualNode=n[i];
    this->Create_Request(ActualNode);
  }
  
  /*#ifdef DEBUG
  cout << "FlowModeSession conn id " << this->Conn->Id << " created requests to nodes: ";
  for (int i=0; i<this->Requests.size(); i++) {
    cout << this->Requests[i]->ConfiguredNode->Id << " ";
  }
  cout << endl;
  #endif
  */
  
  //Send an OFP_FlowModPck to each node in the path
  for (int i=0; i<this->Requests.size(); i++) {
    ActualNode = this->Requests[i]->ConfiguredNode;
    
    OFP_FlowModPck* config_pk=new OFP_FlowModPck(this->OpenFlowController,ActualNode,128,128,this->ParentNet->Now,conns,this,FlowMod_ADD);
    WritePckNodeEvent* eve=new WritePckNodeEvent(this->OpenFlowController,config_pk,this->ParentNet->Now);
    this->ParentNet->genEvent(eve);
  }
  
  return;
}

void FlowModeSession::Create_Request(CNode* n) {
  FlowModRequest* ActualRequest=new FlowModRequest;
  
  ActualRequest->RequestStatus=OFP_OPEN;
  ActualRequest->ConfiguredNode=n;
  
  this->Requests.push_back(ActualRequest);
  
  return;
}

void FlowModeSession::ReceivedReply(CNode* n) {
  FlowModRequest* ActualRequest;
  
  for(int i=0; i<this->Requests.size(); i++) {
    ActualRequest=this->Requests[i];
    
    if (ActualRequest->ConfiguredNode==n) {
      if (ActualRequest->RequestStatus==OFP_CLOSED)
        ErrorMsg("The request is still OFP_CLOSED");
      
      ActualRequest->RequestStatus=OFP_CLOSED;
    }
  }
}

int FlowModeSession::LastReply() {
  FlowModRequest* ActualRequest;
  
  for(int i=0; i<this->Requests.size(); i++) {
    ActualRequest=this->Requests[i];
    
    if (ActualRequest->RequestStatus==OFP_OPEN)
      return 0;
  }
  return 1;
}

int FlowModeSession::close() {
  CConnection* inter_conn;
  CNode* previous_controller;;
  
  //Send an OFP_LightpathPckOUT message to the connection source node
  for (int i=0; i<this->Conns.size(); i++) {
  
    if (this->Type == INTRA_DOMAIN_SESSION) { 
      OFP_LightpathPckOUT* out_pk = new OFP_LightpathPckOUT(this->OpenFlowController,this->Conns[i]->SourceNode,100,64,this->ParentNet->Now,this->Conns[i]);
      WritePckNodeEvent* out_eve = new WritePckNodeEvent(this->OpenFlowController,out_pk,this->ParentNet->Now);
      this->ParentNet->genEvent(out_eve);
    }
    if (this->Type == INTER_DOMAIN_SESSION) {
      inter_conn=this->ParentInterDomain_Session->Conn;

      int j=0;
      while (this->OpenFlowController->Id!=inter_conn->controllers_sequence[j]->Id) j++;
      
      if (j==0) previous_controller=NULL;
      else {
        previous_controller = inter_conn->controllers_sequence[j-1];
      }
      
      PCEP_PCReportPck* rep_pk = new PCEP_PCReportPck(this->OpenFlowController,this->NodeToReply,100,64,this->ParentNet->Now,this->Conns[i],this->ParentInterDomain_Session,YES_PATH);
      WritePckNodeEvent* rep_eve = new WritePckNodeEvent(this->OpenFlowController,rep_pk,this->ParentNet->Now);
      this->ParentNet->genEvent(rep_eve);
      
      if (this->ParentInterDomain_Session->SessionType == SEQUENTIAL_SESSION) {
        if (previous_controller!=NULL) {
        //cout << "conn id " << inter_conn->Id << " Forwarding to previous controller node: " << previous_controller->Id << endl;
	  
	  PCEP_PCInitPck* init_pk = new PCEP_PCInitPck(this->OpenFlowController,previous_controller,100,64,this->ParentNet->Now,inter_conn,this->ParentInterDomain_Session);
	  WritePckNodeEvent* init_eve = new WritePckNodeEvent(this->OpenFlowController,init_pk,this->ParentNet->Now);
	  this->ParentNet->genEvent(init_eve);
	}
      }
    }
  
  }
  
  return 1;
}
