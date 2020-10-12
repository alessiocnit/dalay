#include "CPU_Event.h"
#include "CNode.h"
#include "PCx_Node.h"
#include "CDomain.h"
#include "CNet.h"
#include "CPck.h"
#include "PCEP_PCRepPck.h"
#include "RSVP_PathPck.h"
#include "CBuffer.h"
#include "CPU_Buffer.h"
#include "WritePckNodeEvent.h"
#include "CConnection.h"
#include "OFP_LightpathPckIN.h"

CPU_Event::CPU_Event(CNode* node, double time, EventType type) {
    this->eventTime=time;
    this->eventType=type;
    
    switch (type) {
      case CPU_EVENT_SRC_RSA: 
	this->Comment = "CPU_Event RSA at source node";
	break;
      case CPU_EVENT_PCE_RSA:
	this->Comment = "CPU_Event RSA at PCE";
	break;
      case CPU_EVENT_DST_SA:
	this->Comment = "CPU_Event SA at destination node";
	break;
      default: ErrorMsg("Builder of CPU_Event, type not supported");
    }
    
    this->AtNode=node;
    this->buffer=node->NodeCPU_Buffer;
    
    /*if ((type == CPU_EVENT_PCE_RSA) && (this->AtNode != this->AtNode->ParentDomain->PCE_Node)) {
      ErrorMsg("Builder of CPU_Event, CPU_EVENT_PCE_RSA outside the PCE");
    }*/
}

CPU_Event::~CPU_Event() {
  
}

void CPU_Event::execute() {
  CNet* net=this->AtNode->ParentNet;
  CPck* packet;
  CPck* next_packet;
  PCEP_PCRepPck* pcep_rep_pk;
  RSVP_PathPck* rsvp_path_pk;
  OFP_LightpathPckIN* ofp_in_pk;
  PCEP_PCInitPck* pcep_init_pk;
  CPU_Event* next_cpu_eve;
  double next_cpe_eve_time;
  
  if (this->buffer->PckSize()==0)
    ErrorMsg("CrossConnectionEvent in execution without packets in the buffer --- dalay_panic");
  
  packet=this->buffer->RemoveNextPck();
  
  //Here you can find PCEP_PCRep, RSVP_Path, RSVP_Resv
  if ((strcmp(packet->Comment.c_str(),"PCEP_PCRepPck")) && (strcmp(packet->Comment.c_str(),"RSVP_PATH")) && (strcmp(packet->Comment.c_str(),"OFP_Lightpath_IN"))) {
    cout << packet->Comment << endl;
    ErrorMsg("CPU_Event: Packet type not supported");
  }
  
  //This is a PCEP_PCRep packet already assembled that has to be simply inserted in the transmission queue
  if (!strcmp(packet->Comment.c_str(),"PCEP_PCRepPck")) {
    
    WritePckNodeEvent* P_eve=new WritePckNodeEvent(this->AtNode,packet,net->Now);
    net->genEvent(P_eve);
   
    #ifdef STDOUT
    pcep_rep_pk = static_cast <PCEP_PCRepPck*> (packet); 
    cout << "PCE CPU NODE: " << this->AtNode->Id << " (" << this->AtNode->Name << "): conn id " << pcep_rep_pk->Conn->Id << " CPU executed path computation TIME: " << net->Now << endl;
    #endif
   
  }
  
  if (!strcmp(packet->Comment.c_str(),"OFP_Lightpath_IN")) {
    ofp_in_pk = static_cast <OFP_LightpathPckIN*> (packet); 
    
    if (ofp_in_pk->RestorationFlag==0) {
      this->AtNode->ParentDomain->PCE_Node->ConnEstablish_OpenFlow(ofp_in_pk->Conns);

      #ifdef STDOUT
      for (int i=0; i<ofp_in_pk->Conns.size(); i++)
          cout << "PCE CPU NODE: " << this->AtNode->Id << " (" << this->AtNode->Name << "): conn id " << ofp_in_pk->ConnIds[i] << " CPU executed path computation (provisioning) TIME: " << net->Now << endl;
      #endif
    }
    
    if (ofp_in_pk->RestorationFlag==1) {
      this->AtNode->ParentDomain->PCE_Node->ConnRestoration_OpenFlow(ofp_in_pk->Conns);
      
      #ifdef STDOUT
      for (int i=0; i<ofp_in_pk->Conns.size(); i++)
        cout << "PCE CPU NODE: " << this->AtNode->Id << " (" << this->AtNode->Name << "): conn id " << ofp_in_pk->ConnIds[i] << " CPU executed path computation (restoration) TIME: " << net->Now << endl;
      #endif
    }
    
    delete ofp_in_pk;
  }
  
  if (!strcmp(packet->Comment.c_str(),"RSVP_PATH")) {
    WritePckNodeEvent* P_eve=new WritePckNodeEvent(this->AtNode,packet,net->Now);
    net->genEvent(P_eve);
    
    #ifdef STDOUT
    rsvp_path_pk = static_cast <RSVP_PathPck*> (packet); 
    cout << "CPU NODE: " << this->AtNode->Id << " (" << this->AtNode->Name << "): conn id " << rsvp_path_pk->Conn->Id << " CPU executed path computation TIME: " << net->Now << endl;
    #endif
  }
    
  //Place the next CPU event a switch is required, because the time is different depending on the type of packet
  if (this->buffer->PckSize()!=0) {
    next_packet=this->buffer->NextPck();
    
    switch (next_packet->Type) {
      case PCEP_PCRep:
        next_cpu_eve=new CPU_Event(this->AtNode, net->Now + net->pce_routing_time, CPU_EVENT_PCE_RSA);
        net->genEvent(next_cpu_eve);
        break;
      case RSVP_PATH:
        next_cpu_eve=new CPU_Event(this->AtNode, net->Now + net->distr_routing_time, CPU_EVENT_SRC_RSA);
        net->genEvent(next_cpu_eve);
	break;
      case OFP_Lightpath_IN:
        ofp_in_pk = static_cast <OFP_LightpathPckIN*> (next_packet);
        next_cpe_eve_time = net->pce_routing_time * ofp_in_pk->Conns.size();
        next_cpu_eve=new CPU_Event(this->AtNode,net->Now + next_cpe_eve_time, CPU_EVENT_PCE_RSA);
        net->genEvent(next_cpu_eve);
        break;
      default: ErrorMsg("Execution of CPU_Event, next packet type not supported");
    }
  }
  return;
} 











