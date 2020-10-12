#include "PathComputationSession.h"

#include "Statistics.h"
#include "CDomain.h"
#include "PCx_Node.h"
#include "CConnection.h"
#include "PCEP_PCReqPck.h"
#include "PCEP_PCRepPck.h"
#include "WritePckNodeEvent.h"

//#define DEBUG

PathComputationSession::PathComputationSession(PCEP_PCReqPck* pk, CConnection* conn, CNet* net)
{
  this->ReqPacket=pk;
  this->Conn=conn;
  this->ConnSource=conn->SourceNode;
  this->ConnDestination=conn->DestinationNode;
  this->ParentNet=net;
  this->ParentNode_HPCE=net->HPCE_Node;
  this->RestorationFlag=pk->RestorationFlag;
  
  this->SessionStatus=CLOSED;
}

PathComputationSession::~PathComputationSession() {
  for (int i=0; i<this->Requests.size(); i++)
    delete this->Requests[i];
  this->Requests.clear();
}

void PathComputationSession::start() {
  PathComputationRequest* ActualRequest;
  PathComputationRequest* StoredRequest;
  
  this->SessionStatus=OPEN;
  
  //Computation of the candidate paths
  this->ParentNet->HPCE_PathTable->HPCE_ComputeCandidatePaths(this->ConnSource,this->ConnDestination,this->table,this->Conn);

  //The list of requests is created with all the required attributes
  for (int i=0; i<this->table.size(); i++) {
    this->Create_Request(this->table[i]);
  }
  
  #ifdef DEBUG
  cout << "PathComputationSession conn id " << this->Conn->Id << " created requests: ";
  for (int i=0; i<this->Requests.size(); i++) {
    cout << "(" << this->Requests[i]->Src->Id << "->" << this->Requests[i]->Dst->Id << ") ";
  }
  cout << endl;
  #endif
 
  //For each request the LastReplyTimer is checked. If timer is over a PCEP packet is sent to child PCE
  for (int i=0; i<this->Requests.size(); i++) {
    ActualRequest=this->Requests[i];
    
    //Search inside the HPCE->PCEP_StoredRequests database if Src and Dst are edge nodes
    if ((ActualRequest->Src->Location==EDGE_DOMAIN) && (ActualRequest->Dst->Location==EDGE_DOMAIN)) {
      #ifdef DEBUG
      cout << "Looking for request in the HPCE database: (" << ActualRequest->Src->Id << "->" << ActualRequest->Dst->Id << ")" << endl;  
      #endif
      
      int j=0;
      while ((j<this->ParentNode_HPCE->PCEP_StoredRequests.size()) && (((this->ParentNode_HPCE->PCEP_StoredRequests[j]->Src!=ActualRequest->Src)) || ((this->ParentNode_HPCE->PCEP_StoredRequests[j]->Dst!=ActualRequest->Dst)))) {
	j++;
      }
      
      if (j==this->ParentNode_HPCE->PCEP_StoredRequests.size())
	ErrorMsg("PathComputationSession::start: request not found in the PCEP_StoredRequests databse");
      
      StoredRequest=this->ParentNode_HPCE->PCEP_StoredRequests[j];
      
      if ((StoredRequest->RequestStatus!=UNINITIALIZED) && (StoredRequest->NoPath==YES_PATH) && (this->ParentNet->Now - StoredRequest->LastReplyTime < this->ParentNet->HPCE_TimeThreshold)) {
	ActualRequest->nodes=StoredRequest->nodes;
	ActualRequest->links=StoredRequest->links;
	ActualRequest->NoPath=StoredRequest->NoPath;
	ActualRequest->FreeLambdas=StoredRequest->FreeLambdas;
	ActualRequest->LabelSet=StoredRequest->LabelSet;
	
	ActualRequest->RequestStatus=CLOSED;
      }
    }
    
    if (ActualRequest->RequestStatus==OPEN) {
      #ifdef DEBUG
      cout << "PCEP message for request: (" << ActualRequest->Src->Id << "->" << ActualRequest->Dst->Id << ")" << endl;  
      #endif
      
      PCx_Node* req_pk_dst = this->Requests[i]->Dst->ParentDomain->PCE_Node;

      PCEP_PCReqPck* req_pk = new PCEP_PCReqPck(this->ParentNode_HPCE,req_pk_dst,128,128,this->ParentNet->Now,this->Conn,0,Req_HPCE_PCE);
      req_pk->FromNode=this->Requests[i]->Src;
      req_pk->ToNode=this->Requests[i]->Dst;
      req_pk->ComputationSession=this;
    
      WritePckNodeEvent* eve=new WritePckNodeEvent(this->ParentNode_HPCE,req_pk,this->ParentNet->Now);
      this->ParentNet->genEvent(eve);
    }
  }
}

void PathComputationSession::Create_Request(Path* path) {
  CNode* s;
  CNode* d;
  
  for (int i=0; i<path->links.size(); i++) {
    s=path->nodes[i];
    d=path->nodes[i+1];
    
    if ((path->links[i]==NULL) && (Find_Request(s,d)==-1)) {
      PathComputationRequest* req = new PathComputationRequest;
      
      req->Src=s;
      req->Dst=d;
      req->RequestStatus=OPEN;
      
      this->Requests.push_back(req);
    }
  }
}

int PathComputationSession::Find_Request(CNode* s, CNode* d) {
  for (int i=0; i<this->Requests.size(); i++)
    if ((this->Requests[i]->Src==s) && this->Requests[i]->Dst==d)
      return i;
  return -1;
}

int PathComputationSession::LastReply() {
  for (int i=0; i<this->Requests.size(); i++)
    if (this->Requests[i]->RequestStatus==OPEN) return 0;
  return 1;
}

int PathComputationSession::close(vector<CNode*>* pathnode, vector <CLink*>* pathlink, int* base_slot) {
  int req_id, min_hops=MAX, max_weight=-1, path_available;
  vector<int> hops;
  vector<int> weights;
  vector<int> ll_base_slot;
  vector<int> lll_base_slot;
  vector<Path*> ltable;
  vector<Path*> lltable;
  vector<Path*> llltable;
  Path* ActualPath;
  Path* SelectedPath;
  CLink* ActualLink;
  PathComputationRequest* ActualRequest;
  PCx_Node* hpce=this->ParentNode_HPCE;
  int LinkBottleneck,PathBottleneck,TableEntry, LabelSetTest, rand_id;
  int lambdas=this->ParentNet->W;
  int required_slots=this->Conn->width_slots;
  int SelectedBaseSlot=-1;

  #ifdef STDOUT
  cout << "HPCE NODE: " << hpce->Id << " (" << hpce->Name << "): conn id " << this->Conn->Id;
  cout << " PathComputationSession CLOSED" << endl;
  #endif
  
  if (this->table.size()==0) {
    ErrorMsg("No paths in the this->table");
    return 0;// Potrebbe succedere se tutti i link inter-dominio sono pieni
  }
  
  #ifdef DEBUG
  //Printing the several edge sequences
  cout << "DEBUG --- this->table.size(): " << this->table.size() << endl;
  for (int i=0; i<this->table.size(); i++) {
    for (int j=0; j<this->table[i]->nodes.size(); j++) {
      cout << this->table[i]->nodes[j]->Id << " ";
    }
    cout << endl;
  }
  #endif
  
  //Build ltable excluding paths in this->table that includes a Request with NO_PATH
  for (int i=0; i<this->table.size(); i++) {
    ActualPath=this->table[i];
    path_available=YES_PATH;
    
    for (int j=0; j<ActualPath->links.size(); j++) {
      ActualLink=ActualPath->links[j];
      
      if (ActualLink==NULL) {
	req_id=this->Find_Request(ActualPath->nodes[j],ActualPath->nodes[j+1]);
	if (req_id==-1) ErrorMsg("Request not found");
	
	if (this->Requests[req_id]->NoPath==NO_PATH)
	  path_available=NO_PATH;
      }
    }
    
    if (path_available==YES_PATH)
      ltable.push_back(ActualPath);
  }
  
  //When all paths include a segment replied with NO_PATH
  if (ltable.size()==0) {
    return 0;
  }
  
  #ifdef DEBUG
  //Printing the several edge sequences including all segment that replied with YES_PATH
  cout << "DEBUG --- ltable.size(): " << ltable.size() << endl;
  for (int i=0; i<ltable.size(); i++) {
    for (int j=0; j<ltable[i]->nodes.size(); j++) {
      cout << ltable[i]->nodes[j]->Id << " ";
    }
    cout << endl;
  }
  #endif
  
  //Initialize the vector of number of hops
  for (int i=0; i<ltable.size(); i++) {
    hops.push_back(0);
  }
  
  //Computes number of hops of each edge node path in ltable
  for (int i=0; i<ltable.size(); i++) {
    ActualPath=ltable[i];
    
    for (int j=0; j<ActualPath->links.size(); j++) {
      ActualLink=ActualPath->links[j];
      
      if (ActualLink==NULL) { //If true, this is a intra-domain link
	req_id=this->Find_Request(ActualPath->nodes[j],ActualPath->nodes[j+1]);
	if (req_id==-1) ErrorMsg("Request not found");
	
	hops[i]+=this->Requests[req_id]->links.size(); //number of hops
      }
      else { //This is a inter-domain link
	hops[i]+=this->ParentNet->InterDomainLink_HopCount;
      }
    }
  }
  
  //Computes the minimum number of hops
  for (int i=0; i<ltable.size(); i++) {
    if (hops[i]<min_hops)
      min_hops=hops[i];
  }
  
  #ifdef DEBUG
  //Printing the several edge sequences including all segment that replied with YES_PATH
  cout << "DEBUG --- ltable.size(): " << ltable.size() << endl;
  for (int i=0; i<ltable.size(); i++) {
    for (int j=0; j<ltable[i]->nodes.size(); j++) {
      cout << ltable[i]->nodes[j]->Id << " ";
    }
    cout << "hop count: " << hops[i] << endl;
  }
  #endif
  
  //Build lltable inserting only path with min_hops or min_hops+net->RoutingHopThreshold
  for (int i=0; i<ltable.size(); i++) {
    if (this->ParentNet->ResponseMode_HPCE==NOINFO) {//Inserisce tutti i path
      lltable.push_back(ltable[i]);
    }
    if (this->ParentNet->ResponseMode_HPCE==HOPCOUNT) {//Inserisce solo i path entro RoutingHopThreshold_HPCE
       if (hops[i]<=min_hops+this->ParentNet->RoutingHopThreshold_HPCE)
	 lltable.push_back(ltable[i]);
    }
    if (this->ParentNet->ResponseMode_HPCE==BOTTLENECK) {//Inserisce tutti i path
      lltable.push_back(ltable[i]);
    }
    if (this->ParentNet->ResponseMode_HPCE==LABELSET) {//Inserisce tutti i path
      lltable.push_back(ltable[i]);
    }
  }
  
  if (lltable.size()==0)
    ErrorMsg("No paths in the lltable"); // Non deve mai succedere, ci sarà sempre un min_hops
    
  #ifdef DEBUG
  cout << "DEBUG --- conn id " << this->Conn->Id << " lltable.size(): " << lltable.size() << endl;
  cout << "DEBUG --- conn id " << this->Conn->Id << " hops: ";
  for (int i=0; i<ltable.size(); i++) {
    cout << hops[i] << " ";
  }
  cout << "min_hops: " << min_hops << endl;

  for (int i=0; i<lltable.size(); i++) {
    cout << "DEBUG --- conn id " << this->Conn->Id << " path " << i << ": ";
    for (int j=0; j<lltable[i]->nodes.size(); j++) {
      cout << lltable[i]->nodes[j]->Id << " ";
    }
    cout << endl;
  }
  #endif
  
  //Initialize the vector of weights
  for (int i=0; i<lltable.size(); i++) {
    weights.push_back(0);
  }
  
  //Routing with the proper scheme (complete the weights vector)
  //enum ResponseMode {NOINFO,HOPCOUNT,BOTTLENECK,LABELSET};
  switch (this->ParentNet->ResponseMode_HPCE) {
    case NOINFO: //Nothing to do all the path have the same weight
      break;
    case HOPCOUNT: //Assign higher weight to shorter paths
      for (int i=0; i<lltable.size(); i++) {
	weights[i]=10000-hops[i];
      }
      break;
    case BOTTLENECK: //Computes PathBottleneck for each path
      for (int i=0; i<lltable.size(); i++) {
	ActualPath=lltable[i];
	PathBottleneck=lambdas;
	LinkBottleneck=0;
	
	#ifdef _DEBUG
	for (int j=0; j<ActualPath->nodes.size(); j++) {
	  cout << ActualPath->nodes[j]->Id << " ";
	}
	cout << "hop_count: " << hops[i] << endl;
	#endif
	
	for (int j=0; j<ActualPath->links.size(); j++) {
	  ActualLink=ActualPath->links[j];
	  
	  if (ActualLink==NULL) { //Intra-domain path: look in the collected requests
	    req_id=this->Find_Request(ActualPath->nodes[j],ActualPath->nodes[j+1]);
	    if (req_id==-1) ErrorMsg("AggRouting HPCE: request not found");
	    
	    LinkBottleneck=this->Requests[req_id]->FreeLambdas;
	    //cout << j << " " << this->Requests[req_id]->Src->Id << "->" << this->Requests[req_id]->Dst->Id << ": virtual-link bottleneck " << this->Requests[req_id]->FreeLambdas << endl;
	    
	     //Updating Path Bottleneck
	    if (LinkBottleneck<PathBottleneck)
	      PathBottleneck=LinkBottleneck;
	  }
	  else { //Inter-domain link: look in the SNMP_HPCE_InterDomainLinks_dB
	    TableEntry=0;
	    while ((TableEntry<hpce->SNMP_HPCE_InterDomain_LinkState_dB.size()) && (hpce->SNMP_HPCE_InterDomain_LinkState_dB[TableEntry]->Link!=ActualLink))
	      TableEntry++;
	    
	    if (TableEntry==hpce->SNMP_HPCE_InterDomain_LinkState_dB.size())
	      ErrorMsg("PathComputationSession::close(AGG) inter-domain link state not found at the HPCE");
	    
	    LinkBottleneck=hpce->SNMP_HPCE_InterDomain_LinkState_dB[TableEntry]->FreeLambdas;
	    //cout << j << " " << ActualLink->FromNode->Id << "->" << ActualLink->ToNode->Id << ": link bottleneck " << hpce->SNMP_HPCE_InterDomain_LinkState_dB[TableEntry]->FreeLambdas << endl;
	    
	    //Updating Path Bottleneck
	    if (LinkBottleneck<PathBottleneck)
	      PathBottleneck=LinkBottleneck;
	  }
	}
	weights[i]=PathBottleneck;
      }
      break;
    case LABELSET: //Computes number of end-to-end available lambdas for each path
      for (int i=0; i<lltable.size(); i++) {
	ActualPath=lltable[i];
	
	ll_base_slot.push_back(-1);
	
	vector<int> VirtualLabelSet;
	for (int l=0;l<lambdas;l++) VirtualLabelSet.push_back(1);

	for (int j=0; j<ActualPath->links.size(); j++) {
	  ActualLink=ActualPath->links[j];
	  
	  if (ActualLink==NULL) { //Intra-domain path: look in the collected requests
	    req_id=this->Find_Request(ActualPath->nodes[j],ActualPath->nodes[j+1]);
	    if (req_id==-1) ErrorMsg("DetRouting HPCE: request not found");
	    
	    for (int l=0; l<lambdas; l++) {
	      if ((VirtualLabelSet[l]==1) && (this->Requests[req_id]->LabelSet[l]==0))
		VirtualLabelSet[l]=0;
	    }
	    
	    #ifdef DEBUG
	    cout << "DEBUG --- conn id " << this->Conn->Id << ": ";
	    for (int i=0; i<lambdas; i++)
	      cout << VirtualLabelSet[i] << " ";
	    cout << endl;
	    #endif
	  }
	  else { //Inter-domain link: look in the SNMP_HPCE_InterDomainLinks_dB
	    TableEntry=0;
	    while ((TableEntry<hpce->SNMP_HPCE_InterDomain_LinkState_dB.size()) && (hpce->SNMP_HPCE_InterDomain_LinkState_dB[TableEntry]->Link!=ActualLink))
	      TableEntry++;
	    
	    if (TableEntry==hpce->SNMP_HPCE_InterDomain_LinkState_dB.size())
	      ErrorMsg("PathComputationSession::close(DET) inter-domain link state not found at the HPCE");
	    
	    for (int l=0; l<lambdas; l++) {
	      if ((VirtualLabelSet[l]==1) && (hpce->SNMP_HPCE_InterDomain_LinkState_dB[TableEntry]->LambdaStatus[l]==1))
		VirtualLabelSet[l]=0;
	    }
	    
	    #ifdef DEBUG
	    cout << "DEBUG --- conn id " << this->Conn->Id << ": ";
	    for (int i=0; i<lambdas; i++)
	      cout << VirtualLabelSet[i] << " ";
	    cout << endl;
	    #endif
	  }
	}
	
	//Compute the weights considering the slots required by the connection
        for (int l=0; l<=lambdas-required_slots; l++) {
	  LabelSetTest=0;
	  for (int k=0;k<required_slots;k++) {
	    if (VirtualLabelSet[l+k]==1)
	      LabelSetTest++;
	  }
	  if (LabelSetTest==required_slots)
		weights[i]++;
	}
	
	//Associate a FF spectrum assignment to ActualPath (for each path in the lltable)
	if (this->ParentNet->HPCE_Node->hpce_wa_enabled) {
	  for (int l=0; l<=lambdas-required_slots; l++) {
	     LabelSetTest=0;
	     for (int k=0;k<required_slots;k++) {
		if (VirtualLabelSet[l+k]==1)
		  LabelSetTest++;
	     }
	     if (LabelSetTest==required_slots) {
		ll_base_slot[i]=l;
		break;
	     }
	  }
	}
	
      }
      break;
  }

  //Computes the maximum weight
  for (int i=0; i<lltable.size(); i++) {
    if (weights[i]>max_weight)
      max_weight=weights[i];
  }
  
  #ifdef DEBUG
  //Printing the path in lltable with related weights
  cout << "DEBUG --- conn id " << this->Conn->Id << " lltable.size(): " << lltable.size() << endl;
  for (int i=0; i<lltable.size(); i++) {
    cout << "DEBUG --- conn id " << this->Conn->Id << " path " << i << ": ";
    for (int j=0; j<lltable[i]->nodes.size(); j++) {
      cout << lltable[i]->nodes[j]->Id << " ";
    }
    cout << "hop_count: " << hops[i] << " weight: " << weights[i] << endl;
  }
  cout << "max_weight: " << max_weight << endl;
  #endif

  //If BOTTLENECK or LABELSET this means that there are not path with available resources
  //...in these cases NO_PATH is generated by the HPCE
  if ((max_weight==0) && ((this->ParentNet->ResponseMode_HPCE==BOTTLENECK) ||(this->ParentNet->ResponseMode_HPCE==LABELSET)))
    return 0;
  
  //Build llltable inserting only path with the maximum weight
  for (int i=0; i<lltable.size(); i++) {
    if (weights[i]==max_weight) {
      llltable.push_back(lltable[i]);
      lll_base_slot.push_back(ll_base_slot[i]);
    }
  }
  
  if (llltable.size()==0) {
    ErrorMsg("No paths in the llltable");
  }
  
  #ifdef DEBUG
  //Printing the path in llltable with related weights
  cout << "DEBUG --- llltable.size(): " << llltable.size() << endl;
  for (int i=0; i<llltable.size(); i++) {
    for (int j=0; j<llltable[i]->nodes.size(); j++) {
      cout << llltable[i]->nodes[j]->Id << " ";
    }
    cout << endl;
  }
  #endif
  
  //Randomly select one of the path in the llltable
  rand_id=randint(0,(llltable.size()-1),&this->ParentNet->PathSelectionSeed);
  SelectedPath=llltable[rand_id];
  SelectedBaseSlot=lll_base_slot[rand_id];
  
  //this->Conn->ActualPath.push_back(llltable[rand_id]);
  
  #ifdef DEBUG
  //Printing the selectde path
  cout << "Selected path: ";
  for (int i=0; i<SelectedPath->nodes.size(); i++) {
    cout << SelectedPath->nodes[i]->Id << " ";
  }
  cout << endl;
  #endif
  
  //Fills the pathnode and the linknode vector
  for (int i=0; i<SelectedPath->links.size(); i++) {
    ActualLink=SelectedPath->links[i];
    
    if (ActualLink==NULL) {
      //cout << "IntraDomain logic link " << SelectedPath->nodes[i]->Id << "->" << SelectedPath->nodes[i+1]->Id << endl;
      
      req_id=this->Find_Request(SelectedPath->nodes[i],SelectedPath->nodes[i+1]);
      if (req_id==-1) ErrorMsg("ExpandingNodeSequence HPCE: request not found");
      ActualRequest=this->Requests[req_id];
      
      //cout << "ActualRequest->links.size(): " << ActualRequest->links.size() << " req_id " << req_id << endl;
      for (int j=0; j<ActualRequest->links.size(); j++) {
	pathnode->push_back(ActualRequest->nodes[j]);
	pathlink->push_back(ActualRequest->links[j]);
      }
    }
    else {
      //cout << "InterDomain link " << SelectedPath->nodes[i]->Id << "->" << SelectedPath->nodes[i+1]->Id << endl;
      pathnode->push_back(SelectedPath->nodes[i]);
      pathlink->push_back(SelectedPath->links[i]);
    }
  }
  pathnode->push_back(SelectedPath->nodes[SelectedPath->nodes.size()-1]);
  
  return 1;
}

void PathComputationSession::ReceivedReply(PCEP_PCRepPck* rep_pk) {
  int req_id;
  PathComputationRequest* ActualRequest;
  PathComputationRequest* StoredRequest;
  
  #ifdef STDOUT
  cout << "HPCE NODE: " << this->ParentNode_HPCE->Id << " (" << this->ParentNode_HPCE->Name << "): conn id " << rep_pk->Conn->Id;
  cout << " PathComputationReply (" << rep_pk->FromNode->Id << "->" << rep_pk->ToNode->Id << ") FreeLambdas: " << rep_pk->FreeLambdas;
  cout << " NoPath: " << rep_pk->NoPath << " TIME: " << this->ParentNet->Now << endl;
  #endif
  
  req_id=this->Find_Request(rep_pk->FromNode,rep_pk->ToNode);
  if (req_id==-1)
    ErrorMsg("ReceivedReply: request not found");
  
  ActualRequest=this->Requests[req_id];
  if (ActualRequest->RequestStatus==CLOSED)
    ErrorMsg("PathComputationRequest must still be OPEN");
  
  if (rep_pk->NoPath==NO_PATH) {
	ActualRequest->NoPath=NO_PATH;
	ActualRequest->RequestStatus=CLOSED;
  }
  else {
	for (int i=0; i<rep_pk->NodePath.size(); i++) {
	  ActualRequest->nodes.push_back(rep_pk->NodePath[i]);
	}
	for (int i=0; i<rep_pk->LinkPath.size(); i++) {
	  ActualRequest->links.push_back(rep_pk->LinkPath[i]);
	}
	ActualRequest->NoPath=YES_PATH;
	ActualRequest->FreeLambdas=rep_pk->FreeLambdas;
	ActualRequest->LabelSet=rep_pk->LabelSet;
	ActualRequest->RequestStatus=CLOSED;
  }
  
  //Update of the database with the old requests HPCE->PCEP_StoredRequest
  if ((ActualRequest->Src->Location==EDGE_DOMAIN) && (ActualRequest->Dst->Location==EDGE_DOMAIN)) {
    
    int j=0;
    while ((j<this->ParentNode_HPCE->PCEP_StoredRequests.size()) && (((this->ParentNode_HPCE->PCEP_StoredRequests[j]->Src!=ActualRequest->Src)) || ((this->ParentNode_HPCE->PCEP_StoredRequests[j]->Dst!=ActualRequest->Dst)))) {
      j++;
    }
    
    if (j==this->ParentNode_HPCE->PCEP_StoredRequests.size())
      ErrorMsg("ReceivedReply: request not found in the PCEP_StoredRequests databse");
    
    StoredRequest=this->ParentNode_HPCE->PCEP_StoredRequests[j];
    
    StoredRequest->nodes=ActualRequest->nodes;
    StoredRequest->links=ActualRequest->links;
    StoredRequest->NoPath=ActualRequest->NoPath;
    StoredRequest->FreeLambdas=ActualRequest->FreeLambdas;
    StoredRequest->LabelSet=ActualRequest->LabelSet;
    StoredRequest->LastReplyTime=this->ParentNet->Now; //This field is used only for StoredRequest
    StoredRequest->RequestStatus=STORED;
  }
  
  return;
}




