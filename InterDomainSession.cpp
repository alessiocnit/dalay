#include "InterDomainSession.h"

#include "CNet.h"
#include "OFP_FlowModPck.h"
#include "OFP_LightpathPckOUT.h"
#include "PCEP_PCInitPck.h"
#include "WritePckNodeEvent.h"
#include "CConnection.h"
#include "CDomain.h"
#include "PCx_Node.h"

InterDomainSession::InterDomainSession(CNet* net, PCx_Node* controller, CConnection* conn)
{
  this->ParentNet=net;
  this->OpenFlowController=controller;
  this->SessionStartTime=net->Now;
  
  this->SessionStatus=INTERDOMAIN_PCEP_CLOSED;
  
  this->Conn=conn;
}

InterDomainSession::~InterDomainSession() {
  for (int i=0; i<this->Requests.size(); i++)
    delete this->Requests[i];
  
  this->Requests.clear();
  
  return;
}

void InterDomainSession::start_sequential_session(CConnection* conn) {
  CNode *ActualNode, *NextNode;
  PCx_Node *LastController;
  CDomain *ActualDomain;
  ControllerRequest* ActualRequest;

  this->SessionType=SEQUENTIAL_SESSION;
  this->SessionStatus=INTERDOMAIN_PCEP_OPEN;
  
  ActualDomain=conn->NodePath[0]->ParentDomain;
  this->controllers_sequence.push_back(ActualDomain->PCE_Node);
  
  for (int i=1; i<conn->NodePath.size()-1; i++) {
    ActualNode=conn->NodePath[i];
    NextNode=conn->NodePath[i+1];
    
    if ((ActualNode->ParentDomain == NextNode->ParentDomain) && (ActualNode->ParentDomain != ActualDomain) ) {
      ActualDomain=ActualNode->ParentDomain;
      this->controllers_sequence.push_back(ActualDomain->PCE_Node);
    }
  }
  
  conn->controllers_sequence = this->controllers_sequence;
  
  //cout << "puppamelo " << this->controllers_sequence.size() << " conn id " << conn->Id << endl;
  
  //The list of requests is created with all the required attributes
  for (int i=0; i<this->controllers_sequence.size(); i++) {
    this->Create_Request(this->controllers_sequence[i]);
  }
  
  //Invia solo all'ultimo controllore, che poi risalirà il percorso
  LastController=this->controllers_sequence[this->controllers_sequence.size()-1];
  
  PCEP_PCInitPck* init_pk=new PCEP_PCInitPck(this->OpenFlowController,LastController,128,128,this->ParentNet->Now,conn,this);
  WritePckNodeEvent* eve=new WritePckNodeEvent(this->OpenFlowController,init_pk,this->ParentNet->Now);
  this->ParentNet->genEvent(eve);
  
  //ANDARE AI CONTROLLORI FARE SETUP E RISALIRE IL PERCORSO
  
  return;
}

void InterDomainSession::start_parallel_session(CConnection* conn) {
  CNode *ActualNode, *NextNode;
  PCx_Node *ActualController;
  CDomain *ActualDomain;
  ControllerRequest* ActualRequest;

  this->SessionType=PARALLEL_SESSION;
  this->SessionStatus=INTERDOMAIN_PCEP_OPEN;
  
  //Costruisce la sequenza dei controller
  ActualDomain=conn->NodePath[0]->ParentDomain;
  this->controllers_sequence.push_back(ActualDomain->PCE_Node);
  
  for (int i=1; i<conn->NodePath.size()-1; i++) {
    ActualNode=conn->NodePath[i];
    NextNode=conn->NodePath[i+1];
    
    if ((ActualNode->ParentDomain == NextNode->ParentDomain) && (ActualNode->ParentDomain != ActualDomain) ) {
      ActualDomain=ActualNode->ParentDomain;
      this->controllers_sequence.push_back(ActualDomain->PCE_Node);
    }
  }
  
  conn->controllers_sequence = this->controllers_sequence;
  
  #ifdef STDOUT
  cout << "NODE: " << this->OpenFlowController->Id << " (" << this->OpenFlowController->Name << "): conn id " << conn->Id << " path: ";
  for (int i=0; i<conn->NodePath.size(); i++) {
    cout << conn->NodePath[i]->Id << " ";
  }
  cout << "controllers sequence: ";
  for (int i=0; i<this->controllers_sequence.size(); i++) {
    cout << this->controllers_sequence[i]->Id << " ";
  }
  cout << endl;
  #endif

  //The list of requests is created with all the required attributes
  for (int i=0; i<this->controllers_sequence.size(); i++) {
    this->Create_Request(this->controllers_sequence[i]);
  }
  
  //Send an PCEP_PCInitPck to each controller in the list in the path
  for (int i=0; i<this->Requests.size(); i++) {
    ActualController = this->Requests[i]->ConfiguringController;
    
    PCEP_PCInitPck* init_pk=new PCEP_PCInitPck(this->OpenFlowController,ActualController,128,128,this->ParentNet->Now,conn,this);
    WritePckNodeEvent* eve=new WritePckNodeEvent(this->OpenFlowController,init_pk,this->ParentNet->Now);
    this->ParentNet->genEvent(eve);
  }
  
  return;
}

void InterDomainSession::Create_Request(PCx_Node* n) {
  ControllerRequest* ActualRequest=new ControllerRequest;
  
  ActualRequest->RequestStatus=INTERDOMAIN_PCEP_OPEN;
  ActualRequest->ConfiguringController=n;
  
  this->Requests.push_back(ActualRequest);
  
  return;
}

void InterDomainSession::ReceivedReply(CNode* n, CConnection* conn, int reply) {
  ControllerRequest* ActualRequest;
  
  for(int i=0; i<this->Requests.size(); i++) {
    ActualRequest=this->Requests[i];
    
    if (ActualRequest->ConfiguringController==n) {
        
      if (ActualRequest->RequestStatus==INTERDOMAIN_PCEP_CLOSED)
        ErrorMsg("The request is still INTERDOMAIN_PCEP_CLOSED");
      
      ActualRequest->conn_segment=conn;
      
      ActualRequest->RequestStatus=INTERDOMAIN_PCEP_CLOSED;
      
      ActualRequest->ReplyType=reply;
      
      #ifdef STDOUT
      cout << "NODE: " << this->OpenFlowController->Id << " (" << this->OpenFlowController->Name << "): conn id " << this->Conn->Id << " received segment from: " << n->Id << endl;
      #endif
    }
  }
}

int InterDomainSession::LastReply() {
  ControllerRequest* ActualRequest;
  
  for(int i=0; i<this->Requests.size(); i++) {
    ActualRequest=this->Requests[i];
    
    if (ActualRequest->RequestStatus==INTERDOMAIN_PCEP_OPEN)
      return 0;
  }
  return 1;
}

int InterDomainSession::close() {
  ControllerRequest* ActualRequest;
  CConnection* ActualConnSegment;
  CNet* net=this->ParentNet;
  
  //Check if all replies are YES_PATH
  for (int i=0; i<this->Requests.size(); i++) {
    ActualRequest=this->Requests[i];
    
    //Se una è negativa si devono rilasciare tutte le risorse
    if (ActualRequest->ReplyType==NO_PATH) {
      	this->Conn->Status=REFUSED;
    }
  }
  
  if (this->Conn->Status==REFUSED) {
      
    #ifdef STDOUT
    cout << "NODE: " << this->OpenFlowController->Id << " (" << this->OpenFlowController->Name << "): conn id " << this->Conn->Id << " INTERDOMAIN CONNECTION REFUSED - SIGNALING" << endl;
    cout << "NODE: " << this->OpenFlowController->Id << " (" << this->OpenFlowController->Name << "): conn id " << this->Conn->Id << " CONNECTION MEMORY (erasing) size: " << this->ParentNet->ActiveConns.size() << " TIME: " << this->ParentNet->Now<< endl;
    #endif
    
    net->ConnRequested++; net->ConnRequestedREP++;
    net->ConnRequestedATT[0]++; net->ConnRequestedATT_REP[0]++;
    
    net->ConnRefused++; net->ConnRefusedREP++;
    net->ConnBlocked++; net->ConnBlockedREP++;
    //net->ConnBlockedRouting++; net->ConnBlockedRoutingREP++;
    
    //Only one attempt is supported with OPENFLOW
    net->ConnBlockedATT[0]++; net->ConnBlockedATT_REP[0]++;
    //net->ConnBlockedRoutingATT[0]++; net->ConnBlockedRoutingATT_REP[0]++;
    
    /*Rimuove il puntatore alla connessione nel vector di connessioni attive in tutta la rete*/
    vector<CConnection*>::iterator it_net=this->ParentNet->ActiveConns.begin();
    while (this->Conn!=*it_net) it_net++;
    this->ParentNet->ActiveConns.erase(it_net);
    
    delete this->Conn;
    
    #ifdef STDOUT
    for (int i=0; i<this->Requests.size(); i++) {
      ActualRequest=this->Requests[i];
      ActualConnSegment=ActualRequest->conn_segment;
      
      cout << "NODE: " << this->OpenFlowController->Id << " (" << this->OpenFlowController->Name << "): conn id " << this->Requests[i]->conn_segment->Id << " conn segment: ";
      for (int j=0; j<ActualConnSegment->NodePath.size(); j++) {
        cout << ActualConnSegment->NodePath[j]->Id << " ";
      }
      cout << " base_slot: " << ActualConnSegment->base_slot;
      
      if (ActualRequest->ReplyType==YES_PATH)
        cout << " YES_PATH " << endl;
      if (ActualRequest->ReplyType==NO_PATH)
        cout << " NO_PATH " << endl;
    }
    #endif
    
    for (int i=0; i<this->Requests.size(); i++) {
      ActualRequest=this->Requests[i];
      
      //Rilasciare le risorse in quei domini dove è stata correttamente stabilita
      if (ActualRequest->ReplyType==YES_PATH) {
        ActualRequest->ConfiguringController->ConnRelease_OpenFlow_segment(ActualRequest->conn_segment);
      }
      else {
        delete ActualRequest->conn_segment;
      }
    }
    
    return 2;
  }
  
  //CONNECTION CORRECTLY ESTABLISHED IN ALL DOMAINS
  #ifdef STDOUT
  cout << "NODE: " << this->OpenFlowController->Id << " (" << this->OpenFlowController->Name << "): conn id " << this->Conn->Id << " INTERDOMAIN CONNECTION ACCEPTED " << endl;
  #endif
  
  net->ConnRequested++; net->ConnRequestedREP++;
  net->ConnRequestedATT[0]++; net->ConnRequestedATT_REP[0]++;
  
  net->ConnEstablished++; net->ConnEstablishedREP++;
  net->ConnEstablishedATT[0]++; net->ConnEstablishedATT_REP[0]++;
  
  //Riempie il base_slot_path usando RSA fatti nei vari segmenti
  for (int i=0; i<this->Requests.size(); i++) {
    ActualRequest=this->Requests[i];
    ActualConnSegment=ActualRequest->conn_segment;
    
    #ifdef STDOUT
    cout << "NODE: " << this->OpenFlowController->Id << " (" << this->OpenFlowController->Name << "): conn id " << this->Requests[i]->conn_segment->Id << " conn segment: ";
    for (int j=0; j<ActualConnSegment->NodePath.size(); j++) {
      cout << ActualConnSegment->NodePath[j]->Id << " ";
    }
    cout << " base_slot: " << ActualConnSegment->base_slot;
    if (ActualRequest->ReplyType==YES_PATH)
	cout << " YES_PATH " << endl;
      if (ActualRequest->ReplyType==NO_PATH)
	cout << " NO_PATH " << endl;
    #endif
    
    for (int j=0; j<ActualConnSegment->LinkPath.size(); j++) {
      //this->Conn->LinkPath.push_back(ActualConnSegment->LinkPath[j]);
      this->Conn->base_slot_path.push_back(ActualConnSegment->base_slot);
    }
    
    delete ActualConnSegment;
  }
  
  #ifdef STDOUT
  cout << "NODE: " << this->OpenFlowController->Id << " (" << this->OpenFlowController->Name << "): conn id " << this->Conn->Id << " overall_conn_info NodePath: ";
  for (int j=0; j<this->Conn->NodePath.size(); j++) {
    cout << this->Conn->NodePath[j]->Id << " ";
  }
  cout << endl;
  cout << "NODE: " << this->OpenFlowController->Id << " (" << this->OpenFlowController->Name << "): conn id " << this->Conn->Id << " overall_conn_info LinkPath: ";
  for (int j=0; j<this->Conn->LinkPath.size(); j++) {
    cout << this->Conn->LinkPath[j]->Id << " ";
  }
  cout << endl;
  cout << "NODE: " << this->OpenFlowController->Id << " (" << this->OpenFlowController->Name << "): conn id " << this->Conn->Id << " overall_conn_info BaseSlotPath: ";
  for (int j=0; j<this->Conn->LinkPath.size(); j++) {
    cout << this->Conn->base_slot_path[j] << " ";
  }
  cout << endl;
  #endif
  
  //Send an OFP_LightpathPckOUT message to the connection source node
  OFP_LightpathPckOUT* out_pk = new OFP_LightpathPckOUT(this->OpenFlowController,this->Conn->SourceNode,100,64,this->ParentNet->Now,this->Conn);
  WritePckNodeEvent* out_eve = new WritePckNodeEvent(this->OpenFlowController,out_pk,this->ParentNet->Now);
  this->ParentNet->genEvent(out_eve);
  
  return 1;
}
