#include "CConnection.h"

#include "CNet.h"
#include "CDomain.h"
#include "Statistics.h"

//In STATISTIC traffic mode this builder is used only for the FIRST connection
//In STATIC traffic mode this builder is used for all connections
CConnection::CConnection(int id, CNet* net, double time, double duration, CNode* source, CNode* dest) {
  int random_1_2;
  
  this->active_cconnections++;
  
  this->Id=id;
  this->Net=net;
  this->GenerationTime=time;
  this->DurationTime=duration;
  this->SourceNode=source;
  this->DestinationNode=dest;

  this->SignalingTime=0;
  this->RestorationTime=0;
  this->DisruptionTime=0;

  this->RoutingFlag=0;
    
  int src_id=this->SourceNode->Id;
  int dst_id=this->DestinationNode->Id;
  
  if (this->SourceNode->ParentDomain->PathTable->DistanceMatrix[src_id][dst_id]==MAX) {
        if (Net->multi_domain_interconnection == CONTINUITY) {
            this->Type=INTER_DOMAIN_CONN_ALIEN;
            this->SuggestedBaseSlot_ALIEN=-1;
        } else {
            this->Type=INTER_DOMAIN_CONN;
        }
    } else {
        this->Type=INTRA_DOMAIN_CONN;
    }
    
  Status=REQUESTED;

  this->ProvisioningAttempt=1;
  this->RestorationAttempt=1;

  this->PCE_SuggestedLabel_ON=0;
  this->PCE_SuggestedLabel=-1;
    
  //For flexible connection
  random_1_2=randint(1,2,&net->RequiredSlotsSeed);
  
  switch (random_1_2) {
    case 1:
      //this->width_slots=3;
      this->width_slots=4;
      break;
    case 2:
      //this->width_slots=10;
      this->width_slots=6;
      break;
    default:
      ErrorMsg("CConnection: number of slices not supported");
  }
     
  #ifdef STDOUT
  cout << "Connection created conn id " << this->Id << " (s,d): " << this->SourceNode->Id << "->" << this->DestinationNode->Id << " conn id " << this->Id  << " GenerationTime: " << this->GenerationTime << " HoldingTime: " << this->DurationTime << " RequiredSlots: " << this->width_slots << endl;
  #endif
}

//This builder is used only connections after the first
CConnection::CConnection(int id, CNet* net) {
    int NodeId_s, NodeId_d, i, j, number, found, acc, random_1_2;
    
    this->active_cconnections++;

    this->Id=id;
    this->Net=net;
    
    this->ProvisioningAttempt=1;
    this->RestorationAttempt=1;
    this->ElapsedTime=0;
    this->RoutingFlag=0;

    /*--------------------- Genera tempo inizio e durata ---------------------*/
    this->GenerationTime = this->Net->Now + negexp(this->Net->MeanInterarrivalTime,&(this->Net->InterarrivalSeed));
    this->DurationTime   = negexp(this->Net->MeanDurationTime,&(this->Net->DurationSeed));

    //this->Net->InterArrivalTime->AddSample_Distribution(this->GenerationTime-this->Net->Now);
    //this->Net->HoldingTime->AddSample_Distribution(this->DurationTime);

    /*--------------------- Genera nodo sorgente e destinatario ---------------------*/
    number = randint(1, Net->Sum_TM, &(Net->SrcDstSeed));
    acc=0;
    found=0;
    for (i=0; i<Net->Node.size() && !found; i++)
        for (j=0; j<Net->Node.size() && !found; j++)
            if (i != j) {
                acc += Net->TM[i][j];
                if (acc>=number) {
                    NodeId_s=i;
                    NodeId_d=j;
                    found = 1;
                }
            }
    SourceNode=Net->GetNodePtrFromNodeID(NodeId_s);
    DestinationNode=Net->GetNodePtrFromNodeID(NodeId_d);
    /*---------------------------------------------------------------------------------*/

    int src_id=this->SourceNode->Id;
    int dst_id=this->DestinationNode->Id;
    
    vector<CNode*> pathnode;
    vector<CLink*> pathlink;
    
    net->PathTable->NoInfo_RandomPath(this->SourceNode,this->DestinationNode,&pathnode,&pathlink,this);
    
    if (this->SourceNode->ParentDomain->PathTable->DistanceMatrix[src_id][dst_id]==MAX) {
        if (Net->multi_domain_interconnection == CONTINUITY) {
            this->Type=INTER_DOMAIN_CONN_ALIEN;
            this->SuggestedBaseSlot_ALIEN=-1;
        } else {
            this->Type=INTER_DOMAIN_CONN;
        }
    } else {
        this->Type=INTRA_DOMAIN_CONN;
    }
    
    //For flexible connection
    random_1_2=randint(1,2,&net->RequiredSlotsSeed);
  
    switch (random_1_2) {
    case 1:
      //this->width_slots=3;
      if (pathnode.size() <= 8) this->width_slots=3;
      if (pathnode.size() > 8)  this->width_slots=4;
      break;
    case 2:
      //this->width_slots=10;
      if (pathnode.size() <= 4) this->width_slots=5;
      if ((pathnode.size() >4) && (pathnode.size()<=6)) this->width_slots=6;
      if (pathnode.size() > 6) this->width_slots=7;
      break;
     default:
      ErrorMsg("CConnection: number of slices not supported");
    }
    
    cout << "CARLO ALBERTO - conn id: " << this->Id << " pathsize: " << pathnode.size() << " slots: " << this->width_slots << endl;

    #ifdef STDOUT
    cout << "A NEW connection has been created: " << this->SourceNode->Id << "->" << this->DestinationNode->Id << " conn id " << this->Id  << " GenerationTime: " << this->GenerationTime << " HoldingTime: " << this->DurationTime << " RequiredSlots: " << this->width_slots << endl;
    #endif
    
    Status=REQUESTED;

    this->PCE_SuggestedLabel_ON=0;
    this->PCE_SuggestedLabel=0;
}

//This builder is used only to generate CONNECTION SEGMENTS in multi domain OPENFLOW
CConnection::CConnection(int id, CNet* net, CNode* source, CNode* dest) {
  this->Id=id;
  this->Net=net;
  this->SourceNode=source;
  this->DestinationNode=dest;
}

CConnection::~CConnection() {
  this->active_cconnections--;
  
  #ifdef MEMORY_DEBUG
  cout << "MEMORY DEBUG: CONNECTION " << this->active_cconnections << endl;
  #endif
}

void CConnection::print() {
    cout << "/**********************************************************************************************/" << endl;
    cout << "CONNECTION PRINTING conn id " << this->Id << " Status: " << this->Status << " Type: " << this->Type << " s-d: " << this->SourceNode->Id << "->" << this->DestinationNode->Id << ' ';
    cout << "GenTime: " << this->GenerationTime << " DurTime: " << this->DurationTime << endl << "NodePath: ";
    for (int i=0; i<this->NodePath.size(); i++) cout << this->NodePath[i]->Id << ' ';
    cout << endl << "LinkPath: ";
    for (int i=0; i<this->LinkPath.size(); i++) cout << this->LinkPath[i]->Id << ' ';
    cout << endl;
    cout << "/**********************************************************************************************/" << endl;
}

