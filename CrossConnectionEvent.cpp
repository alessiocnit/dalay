#include "CrossConnectionEvent.h"

#include "RSVP_ResvPck.h"
#include "OFP_FlowModPck.h"
#include "CNode.h"
#include "CDomain.h"
#include "CBuffer.h"
#include "DataPlaneBuffer.h"
#include "ReleaseConnEvent.h"
#include "PCEP_NotifyEstablishedPck.h"
#include "WritePckNodeEvent.h"
#include "FlowModeSession.h"

CrossConnectionEvent::CrossConnectionEvent(CNode* node, double time) {
    this->eventTime=time;
    this->eventType=CROSS_CONNECTION;
    this->Comment="CROSS_CONNECTION_EVENT";
    
    this->AtNode=node;
    this->buffer=node->DataBuffer;
}

CrossConnectionEvent::~CrossConnectionEvent() {
  
}

void CrossConnectionEvent::execute() {
  int i=0;
  CPck* packet;
  RSVP_ResvPck* resv_pk;
  OFP_FlowModPck* flow_add_pk;
  CNet* net=this->AtNode->ParentNet;
  CConnection* conn;
  
  if (this->buffer->PckSize()==0)
    ErrorMsg("CrossConnectionEvent in execution without packets in the buffer --- dalay_panic");
  
  packet=this->buffer->RemoveNextPck();
  
  //Here you can find RSVP_Resv packets or OPENFLOW FlowMod_ADD packets
  if ((strcmp(packet->Comment.c_str(),"RSVP_RESV")) && (strcmp(packet->Comment.c_str(),"FlowMod_ADD")))
    ErrorMsg("Packet type not supported");
  
  //This is a RSVP RESV packet
  if (!strcmp(packet->Comment.c_str(),"RSVP_RESV")) {
    resv_pk = static_cast <RSVP_ResvPck*> (packet);
    
    conn = resv_pk->Conn;
  
    #ifdef STDOUT
    cout << "NODE: " << this->AtNode->Id << " (" << this->AtNode->Name << "): conn id " << conn->Id << " CrossConnection command executed TIME: " << net->Now << endl;
    #endif
  
    //Computation of i index
    while ((this->AtNode!=resv_pk->NodePath[i]) && (i<resv_pk->NodePath.size())) i++;
    if (i==resv_pk->NodePath.size())
      ErrorMsg("RSVP_RESV: packet out of carried connection path");

    //-----------------------------------------------------------------------------------------------------------
    //Specific action at source node (ConnDestination)
    //-----------------------------------------------------------------------------------------------------------
    if (resv_pk->ConnDestinationNode==this->AtNode) {
      //Routing hop-by-hop of RSVP_RESV messages: re-write source and destination ip addresses not necessary at source node

      this->AtNode->SendPck(resv_pk);
    
      //Place the next CrossConnection Event
      if (this->buffer->PckSize()!=0) {
        CrossConnectionEvent* next_cx_eve=new CrossConnectionEvent(this->AtNode,net->Now + net->oxc_crossconnection_time);
        net->genEvent(next_cx_eve);
      }

      return;
    }
  
    //--------------------------------------------------------------------------------------------------------
    //Specific action at destination node (ConnSource)
    //--------------------------------------------------------------------------------------------------------
    if (resv_pk->ConnSourceNode==this->AtNode) {
      
      //If RSVP_RESV is for restoration
      if (resv_pk->RestorationFlag==1) {
        //Collecting statistics on restoration time
        resv_pk->Conn->RestorationTime=(net->Now)-(conn->DisruptionTime);
        if (resv_pk->Conn->RestorationTime<=0) 
	  ErrorMsg("Registered restoration time is negative --- dalay_panic");
	  
        #ifdef STDOUT
        cout << "Connection RESTORED conn id " << resv_pk->Conn->Id << " along lambda " << resv_pk->Conn->base_slot << " width " << resv_pk->Conn->width_slots << endl;
        cout << setprecision(9) << "Experiment: " << net->Experiment_ID << " connection restored conn id " << resv_pk->Conn->Id << " attempt: " << conn->RestorationAttempt << " recovery time: " << resv_pk->Conn->RestorationTime << endl;
        #endif
	
	int attempt=conn->RestorationAttempt-1;
      
        conn->Status=RESTORED;
        net->ConnRestored++; net->ConnRestoredREP++;
        net->ConnRestoredATT[attempt]++; net->ConnRestoredATT_REP[attempt]++;

        //The ReleaseConnEvent is placed at [pk->Conn->DurationTime] seconds after signaling conclusion.
        ReleaseConnEvent* eve=new ReleaseConnEvent(conn->SourceNode,conn,net->Now+(conn->DurationTime-conn->ElapsedTime),resv_pk->RestorationFlag);
        net->genEvent(eve);

        double ActualValue;
        ActualValue=net->TotRestTimeREP;
        ActualValue=(ActualValue*(net->ConnRestoredREP-1) + conn->RestorationTime)/(net->ConnRestoredREP);
        net->TotRestTimeREP=ActualValue;
	
	ActualValue=net->TotRestTimeATT_REP[attempt];
	ActualValue=(ActualValue*(net->ConnRestoredATT_REP[attempt]-1) + conn->RestorationTime)/(net->ConnRestoredATT_REP[attempt]);
	net->TotRestTimeATT_REP[attempt]=ActualValue;
	
	if (net->MaxRestTimeREP < conn->RestorationTime)
	  net->MaxRestTimeREP = conn->RestorationTime;
      }
      else {
        conn->Status=ESTABLISHED;
      
        net->ConnRequested++; net->ConnRequestedREP++;
        net->ConnEstablished++; net->ConnEstablishedREP++;

        net->ConnEstablishedATT[conn->ProvisioningAttempt-1]++; net->ConnEstablishedATT_REP[conn->ProvisioningAttempt-1]++;
        net->ConnRequestedATT[conn->ProvisioningAttempt-1]++; net->ConnRequestedATT_REP[conn->ProvisioningAttempt-1]++;
	    
        //The ReleaseConnEvent is placed at [pk->Conn->DurationTime] seconds after signaling conclusion.
        ReleaseConnEvent* eve=new ReleaseConnEvent(conn->SourceNode,conn,net->Now + conn->DurationTime,resv_pk->RestorationFlag);
        net->genEvent(eve);

        //Collecting statistics on signaling time during provisioning
        conn->SignalingTime=(net->Now)-(conn->GenerationTime);
        if (conn->SignalingTime<=0)
	  ErrorMsg("Registered signaling time is negative --- dalay_panic");

          double ActualValue;
          ActualValue=net->TotSignTimeREP;
          ActualValue=(ActualValue*(net->ConnEstablishedREP-1) + conn->SignalingTime)/(net->ConnEstablishedREP);
          net->TotSignTimeREP=ActualValue;

	  #ifdef STDOUT
          cout << "Connection established conn id " << conn->Id << " along lambda: " << conn->base_slot << endl;
          cout << "Connection established conn id " << conn->Id << " along route: ";
          for (int node_i=0; node_i<conn->NodePath.size(); node_i++)
             cout << conn->NodePath[node_i]->Id << " ";
          cout << endl;
          cout << setprecision(9) << "Connection established conn id " << conn->Id << " signaling time including RWA: " << conn->SignalingTime << " attempts: " << conn->ProvisioningAttempt << endl;
	  #endif
      }

      //If Notification message are active toward PCE
      if (net->PCEP_NotifyMessages) {
         //If there is a PCE_Proactive and Connection has been established along a different label
         if ((net->ProactivePCE) && (resv_pk->base_slot!=conn->PCE_SuggestedLabel)) {
	  #ifdef STDOUT
	  cout << "NODE: " << this->AtNode->Id << " (" << this->AtNode->Name << "): conn id " << conn->Id << " pk->SelectedLabel: " << resv_pk->base_slot << " pk->Conn->PCE_SuggestedLabel: " << conn->PCE_SuggestedLabel << endl;
	  #endif

	  PCEP_NotifyEstablishedPck* n_e_pk=new PCEP_NotifyEstablishedPck(this->AtNode,this->AtNode->ParentDomain->PCE_Node,128,128,net->Now,conn,resv_pk->RestorationFlag);
	  WritePckNodeEvent* n_e_eve=new WritePckNodeEvent(n_e_pk->SrcNode,n_e_pk,net->Now);
	  net->genEvent(n_e_eve);
	}

        //If there is a PCE_Reactive send a notification for the establishment
        if (!net->ProactivePCE) {
	  #ifdef STDOUT
	  cout << "NODE: " << this->AtNode->Id << " (" << this->AtNode->Name << "): conn id " << conn->Id << " PCEP_NotifyEstablishedPck on RESV sent: "  << net->Now << endl;
	  #endif

          PCEP_NotifyEstablishedPck* n_e_pk=new PCEP_NotifyEstablishedPck(this->AtNode,this->AtNode->ParentDomain->PCE_Node,128,128,net->Now,conn,resv_pk->RestorationFlag);
          WritePckNodeEvent* n_e_eve=new WritePckNodeEvent(n_e_pk->SrcNode,n_e_pk,net->Now);
          net->genEvent(n_e_eve);
	}
      }
      
      if (net->TypeHPCE == PROACTIVE_HPCE) {
	if ((resv_pk->Conn->Type == INTRA_DOMAIN_CONN) || (resv_pk->base_slot!=conn->PCE_SuggestedLabel)) {
	  PCEP_NotifyEstablishedPck* n_e_pk=new PCEP_NotifyEstablishedPck(this->AtNode,this->AtNode->ParentNet->HPCE_Node,128,128,net->Now,conn,resv_pk->RestorationFlag);
          WritePckNodeEvent* n_e_eve=new WritePckNodeEvent(n_e_pk->SrcNode,n_e_pk,net->Now);
          net->genEvent(n_e_eve);
	}
      }
    
      delete resv_pk;
    
      //Place the next CrossConnection Event
      if (this->buffer->PckSize()!=0) {
        CrossConnectionEvent* next_cx_eve=new CrossConnectionEvent(this->AtNode,net->Now + net->oxc_crossconnection_time);
        net->genEvent(next_cx_eve);
      }
    
      return;
    }
  
    //--------------------------------------------------------------------------------------------------------
    //Specific action at intermediate node
    //--------------------------------------------------------------------------------------------------------
    //Routing hop-by-hop of RSVP_RESV messages: re-write source and destination ip addresses
    resv_pk->SrcNode=resv_pk->NodePath[i];
    resv_pk->DstNode=resv_pk->NodePath[i-1];
  
    this->AtNode->SendPck(resv_pk);
    
    //Place the next CrossConnection Event
    if (this->buffer->PckSize()!=0) {
      CrossConnectionEvent* next_cx_eve=new CrossConnectionEvent(this->AtNode,net->Now + net->oxc_crossconnection_time);
      net->genEvent(next_cx_eve);
    }
  
    return;
  }

  //This is a OPENFLOW FlowMod_ADD packet
  if (!strcmp(packet->Comment.c_str(),"FlowMod_ADD")) {
    flow_add_pk = static_cast <OFP_FlowModPck*> (packet);
    FlowModeSession* session = flow_add_pk->Session;
    
    #ifdef STDOUT
    for (int i=0; i<flow_add_pk->FlowEntries.size();i++)
      cout << "NODE: " << this->AtNode->Id << " (" << this->AtNode->Name << "): conn id " << flow_add_pk->FlowEntries[i]->ConnId << " pk id " << flow_add_pk->Id << " CrossConnection command executed TIME: " << net->Now << endl;
    #endif
    
    //Reply with a OFP_FlowModPck_ACK packet
    OFP_FlowModPck* ack_pk=new OFP_FlowModPck(this->AtNode,flow_add_pk->SrcNode,128,128,net->Now,flow_add_pk->Session,FlowMod_ACK);
    WritePckNodeEvent* eve=new WritePckNodeEvent(this->AtNode,ack_pk,net->Now);
    net->genEvent(eve);
    
    //Place the next CrossConnection Event
    if (this->buffer->PckSize()!=0) {
      CrossConnectionEvent* next_cx_eve=new CrossConnectionEvent(this->AtNode,net->Now + net->oxc_crossconnection_time);
      net->genEvent(next_cx_eve);
    }
    
    delete flow_add_pk;
    return;
  }
  
  ErrorMsg("Function should return before - dalay_panic");
}

