#include "CNode.h"

#include "defs.h"

#include "Statistics.h"

#include "CDomain.h"
#include "CPathState_Table.h"
#include "NodeBuffer.h"
#include "LinkBuffer.h"
#include "DataPlaneBuffer.h"
#include "PCx_Node.h"

#include "WritePckNodeEvent.h"
#include "ReleaseConnEvent.h"

#include "RSVP_PathPck.h"
#include "RSVP_ResvPck.h"
#include "RSVP_ErrPck.h"
#include "RSVP_PathErrPck.h"
#include "RSVP_TearDownPck.h"

#include "OSPF_LinkFailurePck.h"
#include "OSPF_LinkRepairPck.h"
#include "OSPF_LsaPck.h"

#include "PCEP_PCReqPck.h"
#include "PCEP_NotifyBlockedPck.h"
#include "PCEP_NotifyEstablishedPck.h"
#include "PCEP_NotifyReleasedPck.h"
#include "PCEP_NotifyLinkStatePck.h"

#include "OFP_LightpathPckIN.h"

#include "DeliveredPckEvent.h"
#include "CrossConnectionEvent.h"
#include "CPU_Buffer.h"
#include "CPU_Event.h"

#include <sys/time.h>
#include <unistd.h>

CNode::CNode() {
    //cout << "\nCostruttore CNode()" << endl;
};

CNode::CNode(int id, string name,CNet* parent_net) {
    //cout << "\nCostruttore CNode(int, string, CNet*) id: " << id << endl;

    this->Id=id;
    this->Name=name;
    this->Location=IN_DOMAIN;
    this->ParentNet=parent_net;
    this->ParentDomain=NULL;

    this->PathState_dB=new CPathState_Table(this);

    //The buffer for incoming packets is 25601 Bytes
    this->InBuffer=new NodeBuffer(this);
    this->DataBuffer=new DataPlaneBuffer(this);
    this->NodeCPU_Buffer=new CPU_Buffer(this);
    
    //cout << "Building CNode: " << name << endl;
}

CNode::~CNode() {
    //cout << "Distruttore ~CNode()" << endl;

    //Già distrutta in EuropeNet
    //delete this->PathTable;

    delete this->PathState_dB;
    delete this->InBuffer;

    this->OutLink.clear();
    this->InLink.clear();
    this->LinkState_dB.clear(); //It is deleted by ~CTopology()

    if (this->OriginatedConns.size() || this->TerminatedConns.size())
        ErrorMsg("Connections are still in the node database when destroying the node");
}

void CNode::AddNewLink(int id, CNode* ToNode, double length, int maxbyte, double bitrate, int metric) {

    CLink* link=CreateNewLink(id,ToNode,length,maxbyte,bitrate,metric);
    this->OutLink.push_back(link);
}

CLink* CNode::CreateNewLink(int id, CNode* ToNode, double length, int maxbyte, double bitrate, int metric) {

    CLink* link=new CLink(id,length,this,ToNode,bitrate,metric);

    link->CreateBuffer(maxbyte);
    return link;
}

CLink* CNode::GetLinkToNode(CNode* n) {

    for (int i=0;i<OutLink.size();i++)
        if (OutLink[i]->ToNode==n)
            return OutLink[i];

    cout << "From " << this->Name << " to " << n->Name << endl;
    ErrorMsg("CNode->GetLinkToNode... no link between node specified pair");
    return NULL;
}

CLink* CNode::GetInterDomainLinkToNode(CNode* n) {

    for (int i=0;i<OutLink.size();i++)
        if ((OutLink[i]->ToNode==n) && (OutLink[i]->Type==INTER_DOMAIN))
            return OutLink[i];

    return NULL;
}

CLink* CNode::GetLinkFromNode(CNode* node) {
    for (int i=0;i<node->OutLink.size();i++)
        if (node->OutLink[i]->ToNode==this)
            return node->OutLink[i];
    return NULL;
}

void CNode::Fill_InLink() {
    CNode* NeighbourNode;
    CLink* OutgoingLink;
    CLink* IncomingLink;
    for (int i=0; i<this->OutLink.size(); i++) {
        NeighbourNode=this->OutLink[i]->ToNode;
        //cout << "\nNeighbourNode: " << NeighbourNode->Id << flush;
        IncomingLink=this->GetLinkFromNode(NeighbourNode);
        //cout << "IncomingLink: " << IncomingLink->Id << flush;
        this->InLink.push_back(IncomingLink);
    }
}

void CNode::RoutingTable_AddRecord(CNode* tonode,int nhop,CLink* thr) {
    dBS new_record;

    new_record.ToNode  = tonode;
    new_record.NHop    = nhop;
    new_record.Through_link = thr;

    this->dB.push_back(new_record);
}

void CNode::Destination_DomainNode_Table_AddRecord(CNode* to_node, CNode* dst_node) {
    DestinationDomainNode_entry new_record;

    new_record.ToNode = to_node;
    new_record.DstDomainNode = dst_node;

    this->Destination_DomainNode_dB.push_back(new_record);
}

//IMPORTANT: ReceivedPck_GENERIC is also utilized to deliver out all packet types from source node
//GENERIC packets delivery engine
void CNode::ReceivedPck_GENERIC(CPck* pk) {

    #ifdef STDOUT
    cout << "NODE: " << Id << " (" << Name << "): arrived GENERIC packet (id: " << pk->Id << " src: " << pk->SrcNode->Id << " dst: " << pk->DstNode->Id << ")" << endl;
    #endif

    //TTL test
    if (TTL_expired(pk)) return;

    //Specific action at destination node
    if (pk->DstNode==this) {
	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): GENERIC packet arrived at destination (id: " << pk->Id << " src: " << pk->SrcNode->Id << " dst: " << pk->DstNode->Id << ")" << endl;
	#endif

        ErrorMsg("GENERIC packet has not been identified at destination");

        delete pk;
        return;
    }
    //Specific action at intermediate node (simply delivery for GENERIC packet)
    SendPck(pk);
}

void CNode::ReceivedPck_OSPF_LinkFailure(OSPF_LinkFailurePck* pk) {
    vector<int> link;
    vector<int> path;
    vector<CConnection*> DisruptedConns;
    CConnection* DisruptedConn;
    CNet* net=this->ParentNet;

    //TTL test
    if (TTL_expired(pk)) return;

    //Specific action at destination node
    if (pk->DstNode==this) {
	
        #ifdef STDOUT
        cout << "NODE: " << this->Id << " (" << this->Name << "): arrived OSPF_LinkFailure link id " << pk->DisruptedLink->Id << "...destination reached TIME: " << ParentNet->Now << endl;
        #endif
        
        //Centralized Restoration
        if ((this->ParentNet->DomainArchitecture==OPENFLOW_CONTROLLER) && (this==this->ParentDomain->PCE_Node)) {
            PCx_Node* OpenFlowController= static_cast <PCx_Node*> (this);
	  
            //Write the number of disrupted connections from the Link occupation...
            link.push_back(pk->DisruptedLink->FromNode->Id);
            link.push_back(pk->DisruptedLink->ToNode->Id);
	  
            for (int i=0; i<this->ParentNet->ActiveConns.size(); i++) {
                path.clear();
	    
                for (int j=0; j<this->ParentNet->ActiveConns[i]->NodePath.size(); j++)
                    path.push_back(this->ParentNet->ActiveConns[i]->NodePath[j]->Id);
	    
                if (this->ParentNet->EqualLink(path,link)) {
                    DisruptedConn=this->ParentNet->ActiveConns[i];
	      
                    #ifdef STDOUT
                    cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << DisruptedConn->Id << " DISRUPTED TIME: " << this->ParentNet->Now << endl;
                    #endif
	      
            //For INTRA_DOMAIN_CONN and INTER_DOMAIN_CONN and INTER_DOMAIN_CONN_ALIEN originating in this domain
            if (DisruptedConn->SourceNode->ParentDomain == this->ParentDomain) {
                #ifdef STDOUT
                cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << DisruptedConn->Id << " RESTORING" << endl;
                #endif
                
                switch (DisruptedConn->Status) {
                    case ESTABLISHED:
                        DisruptedConn->Status=DISRUPTED;
                        DisruptedConn->DisruptionTime=pk->MsgGenTime;
		    
                        this->ParentNet->ConnDisrupted++;
                        this->ParentNet->ConnDisruptedREP++;
		  
                        DisruptedConns.push_back(DisruptedConn);
		  
                        //Next three lines for independent restoration
                        if (this->ParentNet->SDN_RestorationType==IND) {
                            OpenFlowController->StubRelease_OpenFlow(DisruptedConn);
		    
                            //Requires CPU so it it executed as a CPU Event sending a OFP_LightpathPckIN message with 
                            //OpenFlowController->ConnRestoration_OpenFlow(DisruptedConns);
                    
                            OFP_LightpathPckIN* in_pk = new OFP_LightpathPckIN(OpenFlowController,OpenFlowController,100,64,net->Now,DisruptedConns);
                            in_pk->RestorationFlag = 1;
                            WritePckNodeEvent* eve = new WritePckNodeEvent(OpenFlowController,in_pk,net->Now);
                            net->genEvent(eve);
                    
                            DisruptedConns.clear();
                        }
                        break;
                    case ADMITTED_PATH:
                        WarningMsg("Connection disrupted in ADMITTED_PATH status");
                        break;
                    case RESTORED:
                        WarningMsg("Connection disrupted in RESTORED status");
                        break;
                    case DISRUPTED:
                    case ADMITTED_PATH_RESTORATION:
                        cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << DisruptedConn->Id << " STATUS " << DisruptedConn->Status;
                        ErrorMsg("Connection disrupted in DISRUPTED/ADMITTED_PATH_RESTORATION status, not yet handled");
                        break;
                    default:
                        cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << DisruptedConn->Id << " STATUS " << DisruptedConn->Status;
                        ErrorMsg("Connection disrupted in NOT handled status");
                }
            }
	    }
	  }
	  
	  //Next two lines for bundle restoration
	  if (this->ParentNet->SDN_RestorationType==BUND) {
	    OpenFlowController->StubRelease_OpenFlow(DisruptedConn);
	    
	    OFP_LightpathPckIN* in_pk = new OFP_LightpathPckIN(OpenFlowController,OpenFlowController,100,64,net->Now,DisruptedConns);
	    in_pk->RestorationFlag = 1;
	    WritePckNodeEvent* eve = new WritePckNodeEvent(OpenFlowController,in_pk,net->Now);
	    net->genEvent(eve);
	    
	    //OpenFlowController->ConnRestoration_OpenFlow(DisruptedConns);
	    //DisruptedConns.clear();
	  }
	  
	  delete pk;
	  return;
	}

	//Distributed restoration (can use local or centralized path computation)
    //Update the OSPF link state database with the new status of the disrupted link
    if (pk->DisruptedLink->ParentDomain==this->ParentDomain) {
	  int LSA_id=this->SearchLinkState(pk->DisruptedLink);
	  this->LinkState_dB[LSA_id]->Status=DOWN;
	}
	
    //Disrupted links has to be identified and restored
    link.push_back(pk->DisruptedLink->FromNode->Id);
    link.push_back(pk->DisruptedLink->ToNode->Id);

    for (int i=0; i<this->OriginatedConns.size(); i++) {
        path.clear();
            
        for (int j=0; j<this->OriginatedConns[i]->NodePath.size(); j++)
            path.push_back(this->OriginatedConns[i]->NodePath[j]->Id);

        if (this->ParentNet->EqualLink(path,link)) {
            CConnection* DisruptedConn=this->OriginatedConns[i];

		#ifdef STDOUT
		cout << "NODE: " << Id << " (" << Name << "): conn id " << DisruptedConn->Id << " DISRUPTED by link failure TIME: " << ParentNet->Now << flush;
		cout << " path: ";
		for (int h=0; h<DisruptedConn->NodePath.size(); h++) {
		  cout << DisruptedConn->NodePath[h]->Id << " ";
		}
		cout << endl;
		#endif

        switch (DisruptedConn->Status) {
		  //This case triggers the restoration
                case ESTABLISHED: {
                    DisruptedConn->Status=DISRUPTED;
                    DisruptedConn->DisruptionTime=pk->MsgGenTime;

                    net->ConnDisrupted++; net->ConnDisruptedREP++;

                    this->ConnStubRelease(DisruptedConn);

                    DisruptedConn->ElapsedTime=(DisruptedConn->DisruptionTime)-(DisruptedConn->GenerationTime + DisruptedConn->SignalingTime);
                    DisruptedConn->OldLabel=DisruptedConn->base_slot;
                    DisruptedConn->NodePath.clear();
                    DisruptedConn->LinkPath.clear();

                    //If there is a PCE for restoration ask PCE for backup path
                    if (this->ParentNet->RestorationPCE) {
                        PCx_Node* PCC_Src=static_cast <PCx_Node*> (this);
                        PCC_Src->PCE_PathRequest_Restoration(DisruptedConn);
                    }
                    else {
                        this->ConnectionDistributedRestoration(DisruptedConn);
                    }
                    //--------------------------------------------------------------------

                    break;
                }
                //These cases are not common but correctly handled by the signaling
                case ADMITTED_PATH:
                    WarningMsg("Connection disrupted ADMITTED_PATH status");
                    break;
                case RELEASING:
                    WarningMsg("Connection disrupted in RELEASING status");
                    break;
                    //This case is very rare and not handled thus if happens the restoration is not triggered
                case RESTORED:
                    cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << DisruptedConn->Id << " STATUS RESTORED" << endl;
                    WarningMsg("Connection disrupted RESTORED status");
                    break;
                    //These cases are possible but are not correctly handled and therefore simulation is terminated
                case REQUESTED_PATH_RESTORATION:
                case ADMITTED_PATH_RESTORATION:
                case REFUSED_RESTORATION:
                case BLOCKED_PATH_RESTORATION:
                    cout << "NODE: " << Id << " (" << Name << "): conn id " << DisruptedConn->Id << " disrupted by link failure TIME: " << ParentNet->Now << endl;
                    cout << "ConnStatus: " << DisruptedConn->Status << endl;
                    ErrorMsg("Connection disrupted in a not correctly handled status -> dalay_panic");
                    break;
                    //These cases are not possible in this point of the code
                case REFUSED:
                case DISRUPTED:
                case REQUESTED:
                case BLOCKED_PATH:
                case RELEASED:
                    cout << "NODE: " << Id << " (" << Name << "): conn id " << DisruptedConn->Id << " disrupted by link failure TIME: " << ParentNet->Now << endl;
                    cout << "ConnStatus: " << DisruptedConn->Status << endl;
                    ErrorMsg("Connection disrupted in an impossible status -> dalay_panic");
                    break;
                    //Default
                default:
                    ErrorMsg("Connection disrupted in a not existing status -> dalay_panic");
                }
            }
        }

        delete pk;
        return;
    }
    
    //Specific action at intermediate and source node just forward the packet
    SendPck(pk);
}

void CNode::ReceivedPck_OSPF_LinkRepair(OSPF_LinkRepairPck* pk) {
    //TTL test
    if (TTL_expired(pk)) return;
    
    //Specific action at destination node
    if (pk->DstNode==this) {
	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): arrived OSPF_LinkRepair packet";
        cout << "...destination reached (time: " << ParentNet->Now << ") link " << pk->RepairedLink->Id << " local status updated -> UP " << endl;
	#endif
	
        //Update the OSPF link state database with the new status of the repaired link
        if (pk->RepairedLink->ParentDomain==this->ParentDomain) {
	  int LSA_id=this->SearchLinkState(pk->RepairedLink);
	  this->LinkState_dB[LSA_id]->Status=UP;
	}

        delete pk;
        return;
    }
    
    //Specific action at intermediate node packets flooding has to be implemented
    SendPck(pk);
}

void CNode::ReceivedPck_OSPF_LSA(OSPF_LsaPck* pk) {
    int LSA_id=0;
    while ((LSA_id<this->LinkState_dB.size()) && (pk->Link_Flooded!=this->LinkState_dB[LSA_id]->Link)) LSA_id++;

    if (LSA_id == this->LinkState_dB.size()) ErrorMsg("LSA of an unknown link has been received !!!");
    if (this->LinkState_dB[LSA_id]->SequenceNumber == 0) ErrorMsg("Forwarding LSA - SequenceNumber OVERFLOW !!!");
    if ((pk->Advertising_Node != this->LinkState_dB[LSA_id]->Link->ToNode) && (pk->Advertising_Node != this->LinkState_dB[LSA_id]->Link->FromNode))
      ErrorMsg("LSA received with wrong AdverstisingNode");

    LinkState* link_state=this->LinkState_dB[LSA_id];

    //Node generating the LSA: it fills the LSA pck fields and updates the proper LinkState entry
    if ((this==pk->SrcNode) && (this==pk->Advertising_Node)) {
	#ifdef STDOUT_FLOODING
        cout << "NODE: " << this->Name << " generated LSA link: " << pk->Link_Flooded->Id << " InstalledSeqNumber: " << link_state->SequenceNumber << " GeneratedSeqNumber: " << pk->sequence_number << " sent to node "<< pk->DstNode->Name << " TIME: " << this->ParentNet->Now << endl;
	#endif
	
        //pk->sequence_number must be equal to pk->Link_Flooded->LSA_LastSequenceNumber + 1
        if ((pk->Advertising_Node == link_state->Link->FromNode) && (pk->sequence_number != pk->Link_Flooded->LSA_LastSequenceNumber_FromNode+1))
            ErrorMsg("pk->sequence_number not consistent...dalay_panic");
	if ((pk->Advertising_Node == link_state->Link->ToNode) && (pk->sequence_number != pk->Link_Flooded->LSA_LastSequenceNumber_ToNode+1))
            ErrorMsg("pk->sequence_number not consistent...dalay_panic");

        LinkState* Installed_LinkState=this->Install_SelfOriginated_LinkState(pk);
        pk->LoadLsaFields(Installed_LinkState);

	if (pk->Advertising_Node == link_state->Link->FromNode) {
	  pk->Link_Flooded->LSA_LastGenerationTime_FromNode=pk->Link_Flooded->LSA_NextScheduledTime_FromNode;
	  pk->Link_Flooded->LSA_NextScheduledTime_FromNode=0;
	  pk->Link_Flooded->LSA_NextScheduled_FromNode=0;
	}
	if (pk->Advertising_Node == link_state->Link->ToNode) {
	  pk->Link_Flooded->LSA_LastGenerationTime_ToNode=pk->Link_Flooded->LSA_NextScheduledTime_ToNode;
	  pk->Link_Flooded->LSA_NextScheduledTime_ToNode=0;
	  pk->Link_Flooded->LSA_NextScheduled_ToNode=0;
	}

        //Generating LSA packets to be forwarded to nodes belonging to the same domain
        for (int j=0;j<this->OutLink.size();j++) {
            if (this->OutLink[j]->ToNode->ParentDomain==this->ParentDomain) {
                OSPF_LsaPck* o_pk=new OSPF_LsaPck(this,this->OutLink[j]->ToNode,128,128,ParentNet->Now,pk);
                o_pk->LoadLsaFields(Installed_LinkState);

		#ifdef STDOUT_FLOODING
                cout << "NODE: " << this->Name << " locally generated forwarded LSA link: ";
		cout << pk->Link_Flooded->FromNode->Id << "->" << pk->Link_Flooded->ToNode->Id;
		cout << " toward Node " << o_pk->DstNode->Name << " TIME: " << this->ParentNet->Now << endl;
		#endif

                ParentNet->ForwardedLSA++;
                ParentNet->ActiveLSA++;
                SendPck(o_pk);
            }
        }
        
        //PCE nodes notify to the HPCE the LSA of InterDomain links
	//This operation is not performed at the LSA source because the source of an LSA of an inter-domain link cannot be a PCE because PCE are never located in an edge node
        if (((this->Type==PCC_PCE) || (this->Type==PCC_PCE_HPCE)) && (this->ParentNet->TypeHPCE==STANDARD_HPCE) && (pk->Link_Flooded->Type==INTER_DOMAIN) && (pk->Link_Flooded->ParentDomain==this->ParentDomain)) {
	  #ifdef STDOUT_PCEP_NOTIFY
	  cout << "NODE: " << this->Id << " (" << this->Name << ") PCEP_NotifyLinkStatePck created for INTERDOMAIN link " << pk->Link_Flooded->FromNode->Id << "->" << pk->Link_Flooded->ToNode->Id << " TIME: " << this->ParentNet->Now << endl;
	  #endif
	  
	  PCEP_NotifyLinkStatePck* pcep_pk=new PCEP_NotifyLinkStatePck(this,this->ParentNet->HPCE_Node,128,128,this->ParentNet->Now,pk);
	  WritePckNodeEvent* pk_eve = new WritePckNodeEvent(this,pcep_pk,this->ParentNet->Now);
	  this->ParentNet->genEvent(pk_eve);
	}
	
	//PCE nodes notify to the HPCE the LSA of all links if BGPLS is considered
	//This operation is not performed at the LSA source because the source of an LSA of an inter-domain link cannot be a PCE because PCE are never located in an edge node
        if ( ((this->Type==PCC_PCE) || (this->Type==PCC_PCE_HPCE)) && (this->ParentNet->TypeHPCE == BGP_HPCE) && (pk->Link_Flooded->ParentDomain==this->ParentDomain)) {
	  #ifdef STDOUT_PCEP_NOTIFY
	  cout << "PCE NODE: " << this->Id << " (" << this->Name << ") BGP_NotifyLinkStatePck created for link " << pk->Link_Flooded->FromNode->Id << "->" << pk->Link_Flooded->ToNode->Id << " TIME: " << this->ParentNet->Now << endl;
	  #endif
	  
	  PCEP_NotifyLinkStatePck* pcep_pk=new PCEP_NotifyLinkStatePck(this,this->ParentNet->HPCE_Node,128,128,this->ParentNet->Now,pk);
	  WritePckNodeEvent* pk_eve = new WritePckNodeEvent(this,pcep_pk,this->ParentNet->Now);
	  this->ParentNet->genEvent(pk_eve);
	}

        delete pk;
        return;
    }

    //Node receiving an LSA
    if (this==pk->DstNode) {
	#ifdef STDOUT_FLOODING
        cout << "NODE: " << this->Name << " received LSA link: " << pk->Link_Flooded->Id << " sent by Node " << pk->SrcNode->Name << " InstalledSeqNumber: " << link_state->SequenceNumber << " ReceivedSeqNumber: " << pk->sequence_number;
	#endif

        if (this==pk->Advertising_Node) {
	    #ifdef STDOUT_FLOODING
            cout << " dropped self-originated LSA - TIME: " << this->ParentNet->Now << endl;
	    #endif

            //This event is not allowed in normal conditions (see RFC 2328 Sec. 13.4)
            if (link_state->SequenceNumber < pk->sequence_number)
                ErrorMsg("Self-originated LSA with a newer SequenceNumber");

            ParentNet->SelfOriginatedLSA++;
            ParentNet->DroppedLSA++;
            ParentNet->ActiveLSA--;
            delete pk;
            return;
        }

        if (link_state->SequenceNumber == pk->sequence_number) {
	     #ifdef STDOUT_FLOODING
            cout<< " dropped duplicate LSA - TIME: " << this->ParentNet->Now << endl;
	    #endif

            ParentNet->DuplicateLSA++;
            ParentNet->DroppedLSA++;
            ParentNet->ActiveLSA--;
            delete pk;
            return;
        }

        if (link_state->SequenceNumber > pk->sequence_number) {
	    #ifdef STDOUT_FLOODING
            cout<< " dropped outdated LSA - TIME: " << this->ParentNet->Now << endl;
	    #endif

            ParentNet->OutOfDateLSA++;
            ParentNet->DroppedLSA++;
            ParentNet->ActiveLSA--;
            delete pk;
            return;
        }

        //Here we know that pk->sequence_number > this->LinkState_dB[LSA_id]->SequenceNumber

        if (this->ParentNet->Now - link_state->InstallationTime < this->ParentNet->OSPF_MinLSArrival) {
	    #ifdef STDOUT_FLOODING
            cout<< " dropped too frequent LSA - TIME: " << ParentNet->Now << endl;
	    #endif

            ParentNet->TooFrequentLSA++;
            ParentNet->DroppedLSA++;
            ParentNet->ActiveLSA--;
            delete pk;
            return;
        }

	#ifdef STDOUT_FLOODING
        cout << " valid LSA - TIME: " << this->ParentNet->Now << endl;
	#endif

        LinkState* Installed_LinkState=this->Install_Received_LinkState(pk);

        //Generating LSA packets to be forwarded (excluded nodes: src and nodes of other domains)
        for (int j=0;j<this->OutLink.size();j++) {
            if (this->OutLink[j]->ToNode->ParentDomain==this->ParentDomain) {
                if (this->OutLink[j]->ToNode->Id!=pk->SrcNode->Id) {
                    OSPF_LsaPck* o_pk=new OSPF_LsaPck(pk->DstNode,this->OutLink[j]->ToNode,128,128,ParentNet->Now,pk);
                    o_pk->LoadLsaFields(Installed_LinkState);

		    #ifdef STDOUT_FLOODING
                    cout << "NODE: " << this->Id << "(" << this->Name << ") forwarded LSA link: " << pk->Link_Flooded->Id << " toward Node " << pk->DstNode->Name << " TIME: " << this->ParentNet->Now << endl;
		    #endif

                    ParentNet->ForwardedLSA++;
                    ParentNet->ActiveLSA++;
                    SendPck(o_pk);
                }
            }
        }
        
        //PCE nodes notify to the HPCE the LSA of InterDomain links
	//This operation is not performed at the LSA source because the source of an LSA of an inter-domain link cannot be a PCE because PCE are never located in an edge node
        if (((this->Type==PCC_PCE) || (this->Type==PCC_PCE_HPCE)) && (this->ParentNet->TypeHPCE==STANDARD_HPCE) && (pk->Link_Flooded->Type==INTER_DOMAIN) && (pk->Link_Flooded->ParentDomain==this->ParentDomain)) {
	  #ifdef STDOUT_PCEP_NOTIFY
	  cout << "NODE: " << this->Id << " (" << this->Name << ") PCEP_NotifyLinkStatePck created for INTERDOMAIN link " << pk->Link_Flooded->FromNode->Id << "->" << pk->Link_Flooded->ToNode->Id << " TIME: " << this->ParentNet->Now << endl;
	  #endif
	  
	  PCEP_NotifyLinkStatePck* pcep_pk=new PCEP_NotifyLinkStatePck(this,this->ParentNet->HPCE_Node,128,128,this->ParentNet->Now,pk);
	  WritePckNodeEvent* pk_eve = new WritePckNodeEvent(this,pcep_pk,this->ParentNet->Now);
	  this->ParentNet->genEvent(pk_eve);
	}
	
	//PCE nodes notify to the HPCE the LSA of all links if BGPLS is considered
	//This operation is not performed at the LSA source because the source of an LSA of an inter-domain link cannot be a PCE because PCE are never located in an edge node
        if ( ((this->Type==PCC_PCE) || (this->Type==PCC_PCE_HPCE)) && (this->ParentNet->TypeHPCE == BGP_HPCE) && (pk->Link_Flooded->ParentDomain==this->ParentDomain)) {
	  #ifdef STDOUT_PCEP_NOTIFY
	  cout << "PCE NODE: " << this->Id << " (" << this->Name << ") BGP_NotifyLinkStatePck created for link " << pk->Link_Flooded->FromNode->Id << "->" << pk->Link_Flooded->ToNode->Id << " TIME: " << this->ParentNet->Now << endl;
	  #endif
	  
	  PCEP_NotifyLinkStatePck* pcep_pk=new PCEP_NotifyLinkStatePck(this,this->ParentNet->HPCE_Node,128,128,this->ParentNet->Now,pk);
	  WritePckNodeEvent* pk_eve = new WritePckNodeEvent(this,pcep_pk,this->ParentNet->Now);
	  this->ParentNet->genEvent(pk_eve);
	}

        ParentNet->ReceivedLSA++;
        ParentNet->ActiveLSA--;
        delete pk;
    }
}

//RSVP_PATH packets delivery engine: routing end-to-end | direction: OriginatedConnsource -> ConnDestination
void CNode::ReceivedPck_RSVP_PATH(RSVP_PathPck* pk) {
    int i=0, j, SendingSuccess=0;
    CLink *IncomingLink=NULL, *OutgoingLink=NULL, *UpstreamOutgoingLink=NULL, *UpstreamIncomingLink=NULL;
    int lambdas=this->ParentNet->W, SelectedUpstreamLabel, SelectedId;
    int ConnectionAdmitted;

    //IncomingLink and OutgoingLink initialization
    while ((this!=pk->NodePath[i]) && (i<pk->NodePath.size())) i++;
    if (i==pk->NodePath.size()) {
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << endl;
        ErrorMsg("RSVP_PATH: packet out of carried connection path");
    }
    if (i > 0) {
        IncomingLink=this->GetLinkFromNode(pk->NodePath[i-1]);
        UpstreamOutgoingLink=this->GetLinkToNode(pk->NodePath[i-1]);
    }
    if (i < pk->NodePath.size()-1) {
        OutgoingLink=this->GetLinkToNode(pk->NodePath[i+1]);
        UpstreamIncomingLink=this->GetLinkFromNode(pk->NodePath[i+1]);
    }

    //TTL test
    if (TTL_expired(pk)) ErrorMsg("TTL expired in a RSVP_PATH message");

    //The ProcTime is set to the default value (routing is not necessary at next node)
    pk->ProcTime=PROC_RSVP_PATH;

    //--------------------------------------------------------------------------------------------------------
    //Specific action at source node
    //--------------------------------------------------------------------------------------------------------
    if (pk->ConnSourceNode==this) {
	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " created RSVP_PATH packet TIME: " << this->ParentNet->Now << endl;
	#endif

        /*The PathPck builder has set to 1 all the members of the LabelSet object.
        Here the LabelSet is updated removing label locally seen as busy*/
        j=this->SearchLocalLink(OutgoingLink,this->OutgoingLink_dB);
        this->LabelSet_Update(pk,this->OutgoingLink_dB[j]);

        /*---------------------------------------------------------------------------------------------
        If all lambdas are busy in the OutgoingLink a PathPck_Err is generated and locally managed --*/
        if (!this->LabelSet_Test(pk->LabelSet)) {
	    #ifdef STDOUT
            cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " LabelSet object is empty RSVP_PATH_ERR (EMPTY_LABELSET) message...sent to localhost" << endl;
	    #endif
	    
            //Generate RSVP_PATH_ERR packet to localhost (GenerationNode is initialized as a NULL pointer)
            RSVP_PathErrPck* p_e_pk=new RSVP_PathErrPck(this,this,pk,128,128,ParentNet->Now,pk->Conn,NULL,EMPTY_LABELSET,pk->RestorationFlag);
            WritePckNodeEvent* p_e_eve=new WritePckNodeEvent(p_e_pk->SrcNode,p_e_pk,ParentNet->Now);
            ParentNet->genEvent(p_e_eve);
	    
            delete pk;
            return;
        }

        /*---------------------------------------------------------------------------------------------
        Setting preferred label for restoration attempt --- smart mapping --------------------------- */
        /*if ((pk->RestorationFlag==1) && (this->ParentNet->SmartRestorationMapping==1)) {
	  pk->SuggestedLabel_ON=1;
	  pk->SuggestedLabel = lambdas - pk->Conn->OldLabel - pk->Conn->width_slots + 1;
	  
	  #ifdef STDOUT
	  cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " SuggestedLabel set for restoration " << pk->SuggestedLabel << endl;
	  #endif
        }*/

        /*---------------------------------------------------------------------------------------------
        Insertion of the PathState in the PathState_dB ----------------------------------------------*/
        if ( this->ParentNet->SimulationMode==PROV)
            this->PathState_dB->PathStateInsert(pk,OutgoingLink);
        if ((this->ParentNet->SimulationMode==REST) && (pk->RestorationFlag==1))
            this->PathState_dB->PathStateInsert(pk,OutgoingLink);
        /*---------------------------------------------------------------------------------------------*/

        //If next hop is not the destination and it is in an other domain a routing will be needed
        if (!this->ParentNet->Provisioning_DomainPCE)
            if ((pk->NodePath[i+1]!=pk->ConnDestinationNode) && (this->ParentDomain!=pk->NodePath[i+1]->ParentDomain))
                pk->ProcTime=PROC_RSVP_PATH;

        SendingSuccess=SendPck(pk,pk->NodePath[1]); //SendPck with ExplicitRoute
        return;
    }
    //--------------------------------------------------------------------------------------------------------
    //Specific action in both destination and intermediate nodes
    //--------------------------------------------------------------------------------------------------------
    #ifdef STDOUT
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " arrived RSVP_PATH packet TIME: " << this->ParentNet->Now << endl;
    #endif
    //--------------------------------------------------------------------------------------------------------
    //Specific action at destination node: sends back to source a RSVP_RESV packet
    //--------------------------------------------------------------------------------------------------------
    if (pk->ConnDestinationNode==this) {
	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " destination reached -> performing Wavelength Assignment TIME: " << this->ParentNet->Now << endl;
	#endif

        //The RSVP_RESV packet is here created but WA is performed while processing RESV at ConnDestinationNode
        RSVP_ResvPck* r_pk=new RSVP_ResvPck(this,pk->NodePath[i-1],pk,128,128,ParentNet->Now,pk->Conn,pk->RestorationFlag);
        WritePckNodeEvent* eve=new WritePckNodeEvent(r_pk->SrcNode,r_pk,ParentNet->Now);
        ParentNet->genEvent(eve);
	
        delete pk;
        return;
    }
    //--------------------------------------------------------------------------------------------------------
    //Specific action at intermediate domain edge node
    //--------------------------------------------------------------------------------------------------------
    if (i==pk->NodePath.size()-1) {
        if (!this->ParentNet->Provisioning_DomainPCE) {
	    #ifdef STDOUT
            cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " RSVP_PATH packet reached a domain edge node -> per-domain routing" << endl;
            cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " received LabelSet object: ";
            for (int ii=0; ii < lambdas; ii++) cout << pk->LabelSet[ii] << " ";
            cout << endl;
	    #endif

            ConnectionAdmitted=this->ConnectionAdmissionControl_IntermediateDomain(pk->Conn,pk);
            if (ConnectionAdmitted==1) {
                OutgoingLink=this->GetLinkToNode(pk->NodePath[i+1]);
		
                j=this->SearchLocalLink(OutgoingLink,this->OutgoingLink_dB);
		this->LabelSet_Update(pk, this->OutgoingLink_dB[j]);

		#ifdef STDOUT
		cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " ADMITTED for first signaling attempt on Domain: " << this->ParentDomain->Name << endl;
                cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " computed LabelSet object: ";
                for (int ii=0; ii < lambdas; ii++) cout << pk->LabelSet[ii] << " ";
                cout << endl;
		#endif
            }
            else {
		#ifdef STDOUT
                cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " REFUSED before first signaling attempt on Domain: " << this->ParentDomain->Name << endl;
		#endif

                RSVP_PathErrPck* e_pk=new RSVP_PathErrPck(this,pk->NodePath[i-1],pk,128,128,ParentNet->Now,pk->Conn,this,REMOTE_ROUTING,pk->RestorationFlag);
                WritePckNodeEvent* eve=new WritePckNodeEvent(e_pk->SrcNode,e_pk,ParentNet->Now);
                ParentNet->genEvent(eve);

                delete pk;
                return;
            }
        }
        else {
	    #ifdef STDOUT
            cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " RSVP_PATH packet reached a domain edge node -> PCE routing " << endl;
	    #endif

            if (this->ParentDomain->PCE_Node==NULL)
                ErrorMsg("The pointer to the PCE of this domain is NULL");

            this->Bufferize_PathPck(pk);

            PCx_Node* PCC=static_cast <PCx_Node*> (this);

            PCC->PCC_PCE_PathRequest(this,pk->Conn);

            return;
        }
    }

    //--------------------------------------------------------------------------------------------------------
    //Specific action at intermediate node (LabelSet managing and packet delivering)
    //--------------------------------------------------------------------------------------------------------

    //TODO Attenzione questo for introduce il ricontrollo del LabelSet in ricezione - riduce le collisioni di un 40% con FF FF
    //New management of LabelSet: updating the LabelSet removing label of IncomingLink busy
    /*j=this->SearchLocalLink(IncomingLink->Id,this->IncomingLink_dB);
    this->LabelSet_Update(pk, this->IncomingLink_dB[j]);*/

    //New management of LabelSet: updating the LabelSet removing label locally seen as busy
    j=this->SearchLocalLink(OutgoingLink,this->OutgoingLink_dB);
    this->LabelSet_Update(pk,this->OutgoingLink_dB[j]);

    //If the LabelSet object is empty a PathErr pck is sent back
    if (!this->LabelSet_Test(pk->LabelSet)) {
	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " LabelSet object is empty RSVP_PATH_ERR (EMPTY_LABELSET) message...sent back" << endl;
	#endif

        RSVP_PathErrPck* e_pk=new RSVP_PathErrPck(this,pk->NodePath[i-1],pk,128,128,ParentNet->Now,pk->Conn,this,EMPTY_LABELSET,pk->RestorationFlag);
        WritePckNodeEvent* eve=new WritePckNodeEvent(e_pk->SrcNode,e_pk,ParentNet->Now);
        ParentNet->genEvent(eve);

        delete pk;
        return;
    }

    if ( this->ParentNet->SimulationMode==PROV)
      this->PathState_dB->PathStateInsert(pk,OutgoingLink);
    if ((this->ParentNet->SimulationMode==REST) && (pk->RestorationFlag==1))
      this->PathState_dB->PathStateInsert(pk,OutgoingLink);
    //---------------------------------------------------------------------------------------------//

    //If next hop is not the destination and it is in an other domain a routing will be needed
    if (!this->ParentNet->Provisioning_DomainPCE)
        if ((pk->NodePath[i+1]!=pk->ConnDestinationNode) && (this->ParentDomain!=pk->NodePath[i+1]->ParentDomain))
            pk->ProcTime=PROC_RSVP_PATH;

    SendingSuccess=SendPck(pk,pk->NodePath[i+1]); //SendPck with ExplicitRoute
    return;
}

//RSVP_RESV packets delivery engine: routing hop-by-hop | direction: ConnDestination -> OriginatedConnsource
void CNode::ReceivedPck_RSVP_RESV(RSVP_ResvPck* pk) {
    int i=0, j;
    CLink *IncomingLink=NULL, *OutgoingLink=NULL, *UpstreamIncomingLink=NULL, *UpstreamOutgoingLink=NULL;

    //IncomingLink and OutgoingLink initialization
    while ((this!=pk->NodePath[i]) && (i<pk->NodePath.size())) i++;
    if (i==pk->NodePath.size()) ErrorMsg("RSVP_RESV: packet out of carried connection path");
    if (i > 0) {
        IncomingLink=this->GetLinkFromNode(pk->NodePath[i-1]);
        UpstreamOutgoingLink=this->GetLinkToNode(pk->NodePath[i-1]);
    }
    if (i < pk->NodePath.size()-1) {
        OutgoingLink=this->GetLinkToNode(pk->NodePath[i+1]);
        UpstreamIncomingLink=this->GetLinkFromNode(pk->NodePath[i+1]);
    }

    //TTL test
    if (TTL_expired(pk)) return;
    //-----------------------------------------------------------------------------------------------------------
    //Specific action at source node: the source node of the RSVP_RESV is the destination node of the connection
    //-----------------------------------------------------------------------------------------------------------
    if (pk->ConnDestinationNode==this) {
	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " <<  pk->Conn->Id << " created RSVP_RESV packet TIME: " << this->ParentNet->Now << endl;
	#endif

        if (IncomingLink->Status==DOWN) {
	    #ifdef STDOUT
            cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " [LINK DOWN] RSVP_ERR and RSVP_PATH_ERR messages...sent to localhost and backward" << endl;
	    #endif
	    
            //The RSVP_ERR packet is not generated because we are at the ConnDestinationNode it is not necessary
            //Generate RSVP_PATH_ERR packet
            RSVP_PathErrPck* p_e_pk=new RSVP_PathErrPck(this,pk->NodePath[i-1],pk,128,128,ParentNet->Now,pk->Conn,this,LINK_DOWN,pk->RestorationFlag);
            WritePckNodeEvent* p_eve=new WritePckNodeEvent(p_e_pk->SrcNode,p_e_pk,ParentNet->Now);
            ParentNet->genEvent(p_eve);

            delete pk;
            return;
        }

        //-------- Wavelength Assignement --------
        int SelectedBaseSlot;

        //WA considering information stored in LabelSet - CollisionDetection - SuggestedLabel - SuggestedVector
        //SimulationMode PROV
        if (this->ParentNet->SimulationMode==PROV) {
            switch (pk->SuggestedWA) {
	    case FF:
		SelectedBaseSlot=this->Flexible_Distributed_WavelengthAssignmentFF(pk);
		break;
            default:
                ErrorMsg("Provisioning phase: SuggestedWA flag not supported value");
            }
        }
        //In SimulationMode REST: Provisioning_TieBreakPolicy is always applied during provisioning
        if (this->ParentNet->SimulationMode==REST) {
            if (pk->RestorationFlag==0) {
                switch (this->ParentNet->Provisioning_DefaultPolicy) {
                case FF:
                    SelectedBaseSlot=this->Flexible_Distributed_WavelengthAssignmentFF(pk);
                    break;
                default:
                    ErrorMsg("Provisioning phase: TieBreakPolicy flag not supported value");
                }
            }
            else {
                switch (pk->SuggestedWA) {
                case FF:
                    SelectedBaseSlot=this->Flexible_Distributed_WavelengthAssignmentFF(pk);
                    break;
                default:
                    ErrorMsg("Restoration phase: SuggestedWA flag not supported value");
                }
                
		#ifdef STDOUT
                cout << "Restoration wavelength assignment conn id " << pk->Conn->Id << " AssignedBaseSlot: " << SelectedBaseSlot << endl;
		#endif
            }
        }

        if (SelectedBaseSlot < 0) {
	    #ifdef STDOUT
            cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " Wavelength Assignment failed - RSVP_PATH_ERR (EMPTY_LABELSET) message...sent back" << endl;
	    #endif

            //This is counted as Backward Blocking but EMPTY_LABEL_SET because next attempt must be on a different path
            RSVP_PathErrPck* e_pk=new RSVP_PathErrPck(this,pk->NodePath[i-1],pk,128,128,ParentNet->Now,pk->Conn,this,EMPTY_LABELSET,pk->RestorationFlag);
            WritePckNodeEvent* eve=new WritePckNodeEvent(e_pk->SrcNode,e_pk,ParentNet->Now);
            ParentNet->genEvent(eve);

            delete pk;
            return;
        }
        else {
	  pk->base_slot=SelectedBaseSlot;
	  pk->Conn->base_slot=SelectedBaseSlot;
	  pk->width_slots=pk->Conn->width_slots;
        }
        //-------- End of Wavelength Assignement --------

        if (!this->check_available_link_spectrum(pk->base_slot,pk->width_slots,IncomingLink)) {
	    #ifdef STDOUT
            cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " signaling collision RSVP_ERR and RSVP_PATH_ERR messages...sent to localhost and forward" << endl;
	    #endif

            WarningMsg("Collision at destination node...");

            //The RSVP_ERR packet is not generated because we are at the ConnDestinationNode
            //Generate RSVP_PATH_ERR packet
            RSVP_PathErrPck* p_e_pk=new RSVP_PathErrPck(this,pk->NodePath[i-1],pk,128,128,ParentNet->Now,pk->Conn,this,COLLISION,pk->RestorationFlag);
            WritePckNodeEvent* p_eve=new WritePckNodeEvent(p_e_pk->SrcNode,p_e_pk,ParentNet->Now);
            ParentNet->genEvent(p_eve);

            delete pk;
            return;
        }
        //Reserve the spectrum in the incoming link
	this->reserve_link_spectrum(pk->base_slot,pk->width_slots,IncomingLink);
	
	//Updating the IncomingLink_dB
	this->reserve_incoming_link_spectrum_db(pk->base_slot,pk->width_slots,IncomingLink);

	#ifdef FLOODING
	//Generation of OSPF_LSA by the ConnDestination node when RSVP_RESV reserves lambda
        this->Generate_LsaPck(IncomingLink);
	#endif
	  
	//Before forwarding packet is inserted in data plane buffer to perform cross connection
	#ifdef STDOUT
	cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " CrossConnection command inserted in data plane buffer TIME: " << this->ParentNet->Now << endl;
	#endif
	
	this->DataBuffer->Add(pk);
	
	if (this->DataBuffer->PckSize()==1) {
	  //Piazzare il CrossConnection Event
	  CrossConnectionEvent* cx_eve=new CrossConnectionEvent(this,ParentNet->Now + ParentNet->oxc_crossconnection_time);
          ParentNet->genEvent(cx_eve);
	}
	else {
	  //Niente perché vuol dire che c'è già schedulato un CrossConnectionEvent, che quando arriverà si occuperà di piazzare il prossimo evento
	}
	
        return;
    }
    //--------------------------------------------------------------------------------------------------------
    //Specific action in both ConnSource and Intermediate nodes
    //--------------------------------------------------------------------------------------------------------
    #ifdef STDOUT
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " arrived RSVP_RESV packet TIME: " << this->ParentNet->Now << endl;
    #endif

    //Remove the path state from the path state Database
    if ( this->ParentNet->SimulationMode==PROV) this->PathState_dB->PathStateRemove(pk);
    if ((this->ParentNet->SimulationMode==REST) && (pk->RestorationFlag==1)) this->PathState_dB->PathStateRemove(pk);
    
    //--------------------------------------------------------------------------------------------------------
    //Specific action at destination node (ConnSource)
    //Set the connection state to ESTABLISHED and introduce release connection event
    //--------------------------------------------------------------------------------------------------------
    if (pk->ConnSourceNode==this) {
	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " destination reached TIME: " << this->ParentNet->Now << endl;
	#endif

        //Checks the status of the utilized links in the local database: if one link is LINK_DOWN
	//An RSVP_ERR is generated to release reserved resources up to ConnDestination
	//An RSVP_PATH_ERR is generated (locally addressed) to correctly managed this connection as blocked
        if (this->ParentNet->SimulationMode==REST) {
	  for (int i=0; i<pk->Conn->LinkPath.size(); i++) {

	    //int j=this->SearchLinkState(pk->Conn->LinkPath[i]);
            if (pk->Conn->LinkPath[i]->Status==DOWN) {
              WarningMsg("Successfull signaling along a in status [LINK DOWN]");
	      #ifdef STDOUT
              cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " [LINK DOWN] RSVP_ERR and RSVP_PATH_ERR messages...sent forward and to localhost" << endl;
	      #endif

              //Generate RSVP_ERR packet toward conn destination
              RSVP_ErrPck* e_pk=new RSVP_ErrPck(this,pk->SrcNode,pk,128,128,ParentNet->Now,pk->Conn,this);
              WritePckNodeEvent* eve=new WritePckNodeEvent(e_pk->SrcNode,e_pk,ParentNet->Now);
              ParentNet->genEvent(eve);

              //Generate RSVP_PATHERR packet toward conn source
              RSVP_PathErrPck* p_e_pk=new RSVP_PathErrPck(this,this,pk,128,128,ParentNet->Now,pk->Conn,NULL,LINK_DOWN,pk->RestorationFlag);
              WritePckNodeEvent* p_eve=new WritePckNodeEvent(p_e_pk->SrcNode,p_e_pk,ParentNet->Now);
              ParentNet->genEvent(p_eve);

              delete pk;
              return;
	    }
	  }
        }
        //Updating the OutgoingLink_dB
        this->reserve_outgoing_link_spectrum_db(pk->base_slot,pk->width_slots,OutgoingLink);

	#ifdef FLOODING
	//Generation of OSPF_LSA by the ConnSource node when RSVP_RESV reserves lambda on the outgoing link
        if (OutgoingLink->Type==INTER_DOMAIN) {
            this->Generate_LsaPck(OutgoingLink);
	}
	#endif
	
	//Before forwarding packet is inserted in data plane buffer to perform cross connection
	#ifdef STDOUT
	cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " CrossConnection command inserted in data plane buffer TIME: " << this->ParentNet->Now << endl;
	#endif
	
	this->DataBuffer->Add(pk);
	
	if (this->DataBuffer->PckSize()==1) {
	//Piazzare il CrossConnection Event
	  CrossConnectionEvent* cx_eve=new CrossConnectionEvent(this,ParentNet->Now + ParentNet->oxc_crossconnection_time);
	  ParentNet->genEvent(cx_eve);
	}
	else {
	//Niente perché vuol dire che c'è già schedulato un CrossConnectionEvent, che quando arriverà si occuperà di piazzare il prossimo evento
	}
	
        return;
    }
    //--------------------------------------------------------------------------------------------------------
    //Specific action at intermediate node: control availability in outgoing reverse link, if yes reserve it, if not generate RSVP_ERR toward conn dest and RSVP_PATH_ERR toward conn source
    //--------------------------------------------------------------------------------------------------------
    if (IncomingLink->Status==DOWN) {
	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " [LINK DOWN] RSVP_ERR and RSVP_PATH_ERR messages...sent back and forward" << endl;
	#endif
	
        //Generate RSVP_PATH_ERR packet toward conn source
        RSVP_PathErrPck* p_e_pk=new RSVP_PathErrPck(this,pk->NodePath[i-1],pk,128,128,ParentNet->Now,pk->Conn,this,LINK_DOWN,pk->RestorationFlag);
        WritePckNodeEvent* p_eve=new WritePckNodeEvent(p_e_pk->SrcNode,p_e_pk,ParentNet->Now);
        ParentNet->genEvent(p_eve);

        //Generate RSVP_ERR packet toward conn destination
        RSVP_ErrPck* e_pk=new RSVP_ErrPck(this,pk->SrcNode,pk,128,128,ParentNet->Now,pk->Conn,this);
        WritePckNodeEvent* eve=new WritePckNodeEvent(e_pk->SrcNode,e_pk,ParentNet->Now);
        ParentNet->genEvent(eve);

        delete pk;
        return;
    }
    //Check resource availability in both directions
    if (!this->check_available_link_spectrum(pk->base_slot,pk->width_slots,IncomingLink)) {
	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " signaling collision RSVP_ERR and RSVP_PATH_ERR messages...sent back and forward" << endl;
	#endif

        //Generate RSVP_PATH_ERR packet toward conn source
        RSVP_PathErrPck* p_e_pk=new RSVP_PathErrPck(this,pk->NodePath[i-1],pk,128,128,ParentNet->Now,pk->Conn,this,COLLISION,pk->RestorationFlag);
        WritePckNodeEvent* p_eve=new WritePckNodeEvent(p_e_pk->SrcNode,p_e_pk,ParentNet->Now);
        ParentNet->genEvent(p_eve);

        //Generate RSVP_ERR packet toward conn destination
        RSVP_ErrPck* e_pk=new RSVP_ErrPck(this,pk->SrcNode,pk,128,128,ParentNet->Now,pk->Conn,this);
        WritePckNodeEvent* eve=new WritePckNodeEvent(e_pk->SrcNode,e_pk,ParentNet->Now);
        ParentNet->genEvent(eve);

        delete pk;
        return;
    }
    //Reserve the spectrum in the incoming link
    this->reserve_link_spectrum(pk->base_slot,pk->width_slots,IncomingLink);

    //Updating the IncomingLink_dB and the OutgoingLink_dB
    this->reserve_incoming_link_spectrum_db(pk->base_slot,pk->width_slots,IncomingLink);
    this->reserve_outgoing_link_spectrum_db(pk->base_slot,pk->width_slots,OutgoingLink);

    #ifdef FLOODING
    //Generation of OSPF_LSA by intermediate nodes when RSVP_RESV reserves lambda on the incoming link
    //An OSPF_LSA is generated for outgoing link if this is an inter-domain link
    this->Generate_LsaPck(IncomingLink);
    
    if (OutgoingLink->Type==INTER_DOMAIN) {
      this->Generate_LsaPck(OutgoingLink);
    }
    #endif
    
    //Before forwarding packet is inserted in data plane buffer to perform cross connection
    #ifdef STDOUT
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " CrossConnection command inserted in data plane buffer TIME: " << this->ParentNet->Now << endl;
    #endif
    
    this->DataBuffer->Add(pk);
	
    if (this->DataBuffer->PckSize()==1) {
      //Piazzare il CrossConnection Event
      CrossConnectionEvent* cx_eve=new CrossConnectionEvent(this,ParentNet->Now + ParentNet->oxc_crossconnection_time);
      ParentNet->genEvent(cx_eve);
    }
    else {
      //Niente perché vuol dire che c'è già schedulato un CrossConnectionEvent, che quando arriverà si occuperà di piazzare il prossimo evento
    }

    return;
}

//RSVP_PATH_ERR packets delivery engine: routing hop-by-hop | direction: ConnDestination -> OriginatedConnsource
void CNode::ReceivedPck_RSVP_PATH_ERR(RSVP_PathErrPck* pk) {
    int i=0, j, SendingSuccess=0;
    CLink *IncomingLink=NULL, *OutgoingLink=NULL, *UpstreamIncomingLink=NULL, *UpstreamOutgoingLink=NULL;

    //IncomingLink and OutgoingLink initialization
    while ((this!=pk->NodePath[i]) && (i<pk->NodePath.size())) i++;
    if (i==pk->NodePath.size()) ErrorMsg("RSVP_PATH_ERR: packet out of carried connection path");
    if (i > 0) {
        IncomingLink=this->GetLinkFromNode(pk->NodePath[i-1]);
        UpstreamOutgoingLink=this->GetLinkToNode(pk->NodePath[i-1]);
    }
    if (i < pk->NodePath.size()-1) {
        OutgoingLink=this->GetLinkToNode(pk->NodePath[i+1]);
        UpstreamIncomingLink=this->GetLinkFromNode(pk->NodePath[i+1]);
    }

    //TTL test
    if (TTL_expired(pk)) return;

    /*----------------------------------------------------------------------------------------------------------------------------
    GenerationNode==NULL therefore RSVP_PATH_ERR has been generated at ConnSourceNode ------------------------------------------------------
    It is not necessary to remove the specific PathState from the PathStateDatabase ----------------------------------------------
    Indeed if GenerationNode==NULL it is not inserted (GeneratioMode==EMPTY_LABELSET) or already removed (GenerationMode==LINK_DOWN) --*/
    if (pk->GenerationNode==NULL) {
        //All in this function simply to clear the code
        if (pk->RestorationFlag) PathErrPckManage_ConnSourceNode_Restoration(pk);
        else PathErrPckManage_ConnSourceNode_Provisioning(pk);

	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " RSVP_PATH_ERR packet managed correctly" << endl;
        WarningMsg("RSVP_PATH_ERR packet with NULL pointer as GenerationNode detected");
	#endif
	
        return;
    }
    
    //--------------------------------------------------------------------------------------------------------
    //Specific action at RSVP_PATH_ERR packet originating node...simply forward
    //--------------------------------------------------------------------------------------------------------
    if (pk->GenerationNode==this) {
	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " created RSVP_PATH_ERR packet TIME: " << this->ParentNet->Now << endl;
	#endif

        SendingSuccess=SendPck(pk);
        return;
    }
    
    //--------------------------------------------------------------------------------------------------------
    //Specific action in both destination (connection's source node) and intermediate nodes
    //--------------------------------------------------------------------------------------------------------
    #ifdef STDOUT
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " arrived RSVP_PATH_ERR packet TIME: " << this->ParentNet->Now << endl;
    #endif

    //Remove the path state from the path state Database
    //In case of multiple attempts a new path state will be added upon arrival of the new PATH message
    if ( this->ParentNet->SimulationMode==PROV)
      this->PathState_dB->PathStateRemove(pk);
    
    if ((this->ParentNet->SimulationMode==REST) && (pk->RestorationFlag==1))
      this->PathState_dB->PathStateRemove(pk);
    
    //--------------------------------------------------------------------------------------------------------
    //Specific action at destination node (connection's source node)
    //--------------------------------------------------------------------------------------------------------
    if (pk->ConnSourceNode==this) {
	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " arrived RSVP_PATH_ERR packet TIME: " << this->ParentNet->Now << "...destination reached -> connection blocked" << endl;
	pk->Conn->print();
	#endif

        //If there is a PCE send a notification for the blocked attempt
        if ((this->ParentNet->ProactivePCE) && (this->ParentNet->PCEP_NotifyMessages)) {
	    #ifdef STDOUT
            cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " PCEP_NotifyBlockedPck on PATH_ERR sent TIME: "  << this->ParentNet->Now << endl;
	    #endif

            PCEP_NotifyBlockedPck* n_b_pk=new PCEP_NotifyBlockedPck(this,this->ParentDomain->PCE_Node,128,128,ParentNet->Now,pk->Conn,pk,pk->GenerationDirection,pk->RestorationFlag);
            WritePckNodeEvent* n_b_eve=new WritePckNodeEvent(n_b_pk->SrcNode,n_b_pk,ParentNet->Now);
            ParentNet->genEvent(n_b_eve);
        }
        
        //All in this function simply to clear the code
        if (pk->RestorationFlag) PathErrPckManage_ConnSourceNode_Restoration(pk);
        else PathErrPckManage_ConnSourceNode_Provisioning(pk);
	
        return;
    }
    //--------------------------------------------------------------------------------------------------------
    //Specific action at intermediate node: deliver packet to next node
    //--------------------------------------------------------------------------------------------------------

    //Routing hop-by-hop of RSVP_PATH_ERR messages: re-write source and destination ip addresses
    pk->SrcNode=pk->NodePath[i];
    pk->DstNode=pk->NodePath[i-1];

    SendingSuccess=SendPck(pk);
    return;
}

//RSVP_ERR packets delivery engine: routing hop-by-hop | direction: OriginatedConnsource -> ConnDestination
void CNode::ReceivedPck_RSVP_RESV_ERR(RSVP_ErrPck* pk) {
    int i=0, j, SendingSuccess=0;
    CLink *IncomingLink=NULL, *OutgoingLink=NULL, *UpstreamIncomingLink=NULL, *UpstreamOutgoingLink=NULL;

    //IncomingLink and OutgoingLink initialization
    while ((this!=pk->NodePath[i]) && (i<pk->NodePath.size())) i++;
    if (i==pk->NodePath.size()) ErrorMsg("RSVP_RESV_ERR: packet out of carried connection path");
    if (i > 0) {
        IncomingLink=this->GetLinkFromNode(pk->NodePath[i-1]);
        UpstreamOutgoingLink=this->GetLinkToNode(pk->NodePath[i-1]);
    }
    if (i < pk->NodePath.size()-1) {
        OutgoingLink=this->GetLinkToNode(pk->NodePath[i+1]);
        UpstreamIncomingLink=this->GetLinkFromNode(pk->NodePath[i+1]);
    }

    //TTL test
    if (TTL_expired(pk)) return;
    //--------------------------------------------------------------------------------------------------------
    //Specific action at RSVP_ERR packet originating node...simply forward
    //--------------------------------------------------------------------------------------------------------
    if (pk->GenerationNode==this) {
	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " created RSVP_RESV_ERR packet TIME: " << this->ParentNet->Now << endl;
	#endif

        SendingSuccess=SendPck(pk);
        return;
    }
    //--------------------------------------------------------------------------------------------------------
    //Specific action at destination node
    //--------------------------------------------------------------------------------------------------------
    if (pk->ConnDestinationNode==this) {
	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " arrived RSVP_RESV_ERR packet TIME: " << this->ParentNet->Now;
        cout << "...destination reached -> resources correctly released" << endl;
	#endif

        //Updating the IncomingLink status
	if (this->check_busy_link_spectrum(pk->base_slot,pk->width_slots,IncomingLink)) {
	  this->release_link_spectrum(pk->base_slot,pk->width_slots,IncomingLink);
	}
	else ErrorMsg("RSVP_ERR: expected busy resource is available");
	  
        //Updating the IncomingLink_dB
	this->release_incoming_link_spectrum_db(pk->base_slot,pk->width_slots,IncomingLink);

	#ifdef FLOODING
        this->Generate_LsaPck(IncomingLink);
	#endif

        delete pk;
        return;
    }
    //--------------------------------------------------------------------------------------------------------
    //Specific action at intermediate node: deliver packet to next node
    //--------------------------------------------------------------------------------------------------------
    #ifdef STDOUT
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " arrived RSVP_RESV_ERR packet TIME: " << this->ParentNet->Now << endl;
    #endif

    //Release resources on IncomingLink
    if (this->check_busy_link_spectrum(pk->base_slot,pk->width_slots,IncomingLink)) {
      this->release_link_spectrum(pk->base_slot,pk->width_slots,IncomingLink);
    }
    else ErrorMsg("RSVP_RESV_ERR: expected busy resource is available");

    //Updating the IncomingLink_dB
    this->release_incoming_link_spectrum_db(pk->base_slot,pk->width_slots,IncomingLink);
    
    //Updating the OutgoingLink_dB
    this->release_outgoing_link_spectrum_db(pk->base_slot,pk->width_slots,OutgoingLink);

    #ifdef FLOODING
    this->Generate_LsaPck(IncomingLink);

    if (OutgoingLink->Type==INTER_DOMAIN) {
        this->Generate_LsaPck(OutgoingLink);
    }
    #endif

    //Routing hop-by-hop of RSVP_RESV messages: re-write source and destination ip addresses
    pk->SrcNode=pk->NodePath[i];
    pk->DstNode=pk->NodePath[i+1];

    SendingSuccess=SendPck(pk);
    return;
}

//RSVP_TEARDOWN packets delivery engine: routing end-to-end | direction: OriginatedConnsource -> ConnDestination
void CNode::ReceivedPck_RSVP_TEARDOWN(RSVP_TearDownPck* pk) {
    int i=0, j, SendingSuccess=0;
    CLink *IncomingLink=NULL, *OutgoingLink=NULL, *UpstreamIncomingLink=NULL, *UpstreamOutgoingLink=NULL;

    //IncomingLink and OutgoingLink initialization
    while ((this!=pk->NodePath[i]) && (i<pk->NodePath.size())) i++;
    if (i==pk->NodePath.size()) ErrorMsg("RSVP_TEARDOWN: packet out of carried connection path");
    if (i > 0) {
        IncomingLink=this->GetLinkFromNode(pk->NodePath[i-1]);
        UpstreamOutgoingLink=this->GetLinkToNode(pk->NodePath[i-1]);
    }
    if (i < pk->NodePath.size()-1) {
        OutgoingLink=this->GetLinkToNode(pk->NodePath[i+1]);
        UpstreamIncomingLink=this->GetLinkFromNode(pk->NodePath[i+1]);
    }

    //TTL test
    if (TTL_expired(pk)) return;

    //--------------------------------------------------------------------------------------------------------
    //Specific action at source node
    //--------------------------------------------------------------------------------------------------------
    if (pk->ConnSourceNode==this) {
	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " <<  pk->Conn->Id << " created RSVP_TEARDOWN packet TIME: "  << this->ParentNet->Now << " base_slot " << pk->base_slot << " width_slots " << pk->width_slots << endl;
	#endif

        //Updating the OutgoingLink_dB
        this->release_outgoing_link_spectrum_db(pk->base_slot,pk->width_slots,OutgoingLink);

	#ifdef FLOODING
        if (OutgoingLink->Type==INTER_DOMAIN) {
            this->Generate_LsaPck(OutgoingLink);
	}
	#endif

        //The PCEP_NotifyReleasedPck to the PCE is generated in ConnStubRelease and ConneRelease functions togheter with the first TEAR_DOWN

        //Forwarding packet
        SendingSuccess=SendPck(pk,pk->NodePath[1]);
        return;
    }

    //--------------------------------------------------------------------------------------------------------
    //Specific action at destination node
    //--------------------------------------------------------------------------------------------------------
    if (pk->ConnDestinationNode==this) {
	#ifdef STDOUT
        if (pk->RestorationFlag==0) cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " arrived RSVP_TEARDOWN packet base_slot " << pk->base_slot << " width_slots " << pk->width_slots << "...destination reached -> conn released" << endl;
        else cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " arrived RSVP_TEARDOWN packet base_slot " << pk->base_slot << " width_slots " << pk->width_slots << "...destination reached -> stub released" << endl;

	if (IncomingLink->Status==DOWN)
	  cout << "A lambda channel has been released on a DOWN link" << endl;
	#endif
	
	//Updating the IncomigLink status
	if (this->check_busy_link_spectrum(pk->base_slot,pk->width_slots,IncomingLink)) {
	  this->release_link_spectrum(pk->base_slot,pk->width_slots,IncomingLink);
	}
	else ErrorMsg("RSVP_TEARDOWN: expected busy resource is available");
	
        //Updating the IncomingLink_dB
        this->release_incoming_link_spectrum_db(pk->base_slot,pk->width_slots,IncomingLink);

	#ifdef FLOODING
        this->Generate_LsaPck(IncomingLink);
	#endif

        if (pk->RestorationFlag==0) {
	  //If there is a PCE send a notification for the released connection, during restoration the Notification is generated
	  if (this->ParentNet->TypeHPCE == PROACTIVE_HPCE) {
	    PCEP_NotifyReleasedPck* n_r_pk=new PCEP_NotifyReleasedPck(this,this->ParentNet->HPCE_Node,128,128,ParentNet->Now,pk);
	    WritePckNodeEvent* n_r_eve=new WritePckNodeEvent(n_r_pk->SrcNode,n_r_pk,ParentNet->Now);
	    ParentNet->genEvent(n_r_eve);
	  }
	  
	  if (this->ParentNet->PCEP_NotifyMessages) {
	    PCEP_NotifyReleasedPck* n_r_pk=new PCEP_NotifyReleasedPck(this,this->ParentDomain->PCE_Node,128,128,ParentNet->Now,pk);
	    WritePckNodeEvent* n_r_eve=new WritePckNodeEvent(n_r_pk->SrcNode,n_r_pk,ParentNet->Now);
	    ParentNet->genEvent(n_r_eve);
	  }
	  
	  //Connection Released
          pk->Conn->Status=RELEASED;
          pk->Conn->Net->ConnReleased++;

          delete pk->Conn; //TODO Questa istruzione andrebbe eseguita al nodo sorgente quando riceve L'Ack DEL TearDown
          delete pk;
          return;
        }
        else {
	  //If there is a PCE send a notification for the released connection, during restoration the Notification is generated
	  if (this->ParentNet->TypeHPCE == PROACTIVE_HPCE) {
	    PCEP_NotifyReleasedPck* n_r_pk=new PCEP_NotifyReleasedPck(this,this->ParentNet->HPCE_Node,128,128,ParentNet->Now,pk);
	    WritePckNodeEvent* n_r_eve=new WritePckNodeEvent(n_r_pk->SrcNode,n_r_pk,ParentNet->Now);
	    ParentNet->genEvent(n_r_eve);
	  }
	  
	  if (this->ParentNet->PCEP_NotifyMessages) {
	    PCEP_NotifyReleasedPck* n_r_pk=new PCEP_NotifyReleasedPck(this,this->ParentDomain->PCE_Node,128,128,ParentNet->Now,pk);
	    WritePckNodeEvent* n_r_eve=new WritePckNodeEvent(n_r_pk->SrcNode,n_r_pk,ParentNet->Now);
	    ParentNet->genEvent(n_r_eve);
	  }
	  
	  //Connection Stub Released

          delete pk;
          return;
        }
    }

    //--------------------------------------------------------------------------------------------------------
    //Specific action at intermediate node: deliver packet to next node
    //--------------------------------------------------------------------------------------------------------
    #ifdef STDOUT
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " arrived RSVP_TEARDOWN packet TIME: "  << this->ParentNet->Now << " base_slot " << pk->base_slot << " width_slots " << pk->width_slots << endl;
    
    if (IncomingLink->Status==DOWN)
      cout << "A lambda channel has been released on a DOWN link" << endl;
    #endif

    //Updating IncomingLink status
    if (this->check_busy_link_spectrum(pk->base_slot,pk->width_slots,IncomingLink)) {
      this->release_link_spectrum(pk->base_slot,pk->width_slots,IncomingLink);
    }
    else ErrorMsg("RSVP_TEARDOWN: expected busy resource is available");

    //Updating the IncomingLink_dB
    this->release_incoming_link_spectrum_db(pk->base_slot,pk->width_slots,IncomingLink);

    //Updating the OutgoingLink_dB
    this->release_outgoing_link_spectrum_db(pk->base_slot,pk->width_slots,OutgoingLink);

    #ifdef FLOODING
    this->Generate_LsaPck(IncomingLink);

    if (OutgoingLink->Type==INTER_DOMAIN) {
        this->Generate_LsaPck(OutgoingLink);
    }
    #endif

    SendingSuccess=SendPck(pk,pk->NodePath[i+1]);
    return;
}

int CNode::TTL_expired(CPck* pk) {

    if (pk->TTL==0) {
#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): TTL EXPIRED -> packet dropped\n";
#endif

        ErrorMsg("TTL EXPIRED... dalay_panic");

        ParentNet->PckBlocked++;
        delete pk;
        return 1;
    }
    return 0;
}

//Return 1 if the packet is succefully sent return 0 if the packet is dropped
//It uses the dB and the pk->DstNode for selecting the outgoing link
int CNode::SendPck(CPck* pk) {
    double Now=this->ParentNet->Now;
    int i=0;

    //Searching rigth output
    while ((dB[i].ToNode!=pk->DstNode) && (i<dB.size())) i++;
    if (i==dB.size()) {
        ParentNet->LogFile << "NO WAY TO DESTINATION -> packet dropped" << endl;
        cout << "NO WAY TO DESTINATION -> packet " << pk->Comment << " dropped at node: " << this->Id << " (" << this->Name << ") " << endl;
        cout << "DESTINATION IS: " << pk->DstNode->Id << endl;
	ErrorMsg("SendPck(CPck* pk):: NO WAY TO DESTINATION -> packet dropped");
    }
    pk->TTL--;

    CLink* link=dB[i].Through_link;
    LinkBuffer* buffer=link->ControlPlane_Buffer;

    buffer->Add(pk);

    if (buffer->PckSize()==1) {
        DeliveredPckEvent* d_eve=new DeliveredPckEvent(this,buffer, Now + pk->Dim*8/link->Trx_Bitrate);
        this->ParentNet->genEvent(d_eve);
    }
    else {
      //next event will be managed inside DeliveredPckEvent
    }

    return 1;
}

//Return 1 if the packet is succefully sent return 0 if the packet is dropped
//It uses the dB and the TowardNode for selecting the outgoing link
int CNode::SendPck(CPck* pk, CNode* TowardNode) {
    double Now=this->ParentNet->Now;
    int i=0;

    //Searching rigth output
    while ((dB[i].ToNode!=TowardNode) && (i<dB.size())) i++;
    if (i==dB.size()) {
        ParentNet->LogFile << "NO WAY TO DESTINATION -> packet dropped" << endl;
        ErrorMsg("SendPck(CPck* pk, CNode* TowardNode):: NO WAY TO DESTINATION -> packet dropped");
    }
    pk->TTL--;

    CLink* link=dB[i].Through_link;
    LinkBuffer* buffer=link->ControlPlane_Buffer;

    buffer->Add(pk);

    if (buffer->PckSize()==1) {
        DeliveredPckEvent* d_eve=new DeliveredPckEvent(this, buffer, Now + pk->Dim*8/link->Trx_Bitrate);
        this->ParentNet->genEvent(d_eve);
    }

    return 1;
}

//StartTime is the time in which the first Path message is generated (e.g., routing is already performed)
void CNode::ConnEstablish(CConnection* conn, double StartTime) {
    int RestorationFlag=0;
    RSVP_PathPck* path_pk;
    
    //If conn has been disrupted the restoration flag field in the PATH packet is set to 1
    if (conn->Status==ADMITTED_PATH_RESTORATION) {
        RestorationFlag=1;
	path_pk=new RSVP_PathPck(conn->SourceNode,conn->DestinationNode,128,128,StartTime,conn,RestorationFlag);
	
	#ifdef STDOUT
        cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << conn->Id << " ADMITTED_PATH_RESTORATION TIME: " << this->ParentNet->Now<< endl;
	#endif
	
	if (conn->RoutingFlag==1) { //Re-routing has been performed locally, conn is queued at the CPU
	   this->Add_CPU_EVENT_SRC_RSA(path_pk);
	}
	else { //Routing has been not performed locally (e.g., routed at the PCE, not re-routed due to collision), path is generated
	  WritePckNodeEvent* eve=new WritePckNodeEvent(path_pk->SrcNode,path_pk,StartTime);
	  this->ParentNet->genEvent(eve);
	}

	return;
    }

    if (conn->ProvisioningAttempt==1) {
        //Se siamo al primo tentativo inserisce la connessione nel vector di connessioni attive al nodo sorgente e in tutta la rete
	#ifdef STDOUT
        cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << conn->Id << " CONNECTION MEMORY (writing) size: " << this->ParentNet->ActiveConns.size() << " TIME: " << this->ParentNet->Now<< endl;
	#endif

        this->OriginatedConns.push_back(conn);
        this->ParentNet->ActiveConns.push_back(conn);
    }
    
    path_pk=new RSVP_PathPck(conn->SourceNode,conn->DestinationNode,128,128, StartTime,conn,RestorationFlag);
    
    if (conn->RoutingFlag == 1) { //Routing has been performed locally, conn is queued at the CPU
      conn->RoutingFlag=0;
      this->Add_CPU_EVENT_SRC_RSA(path_pk);
    }
    else { //Routing has been not performed locally (e.g., routed at the PCE), path is generated
      WritePckNodeEvent* eve=new WritePckNodeEvent(path_pk->SrcNode,path_pk,StartTime);
      this->ParentNet->genEvent(eve);
    }

    /***** Stampa del vettore attuale delle connessioni attive con sorgente questo nodo DEBUG *****
    cout << "\nTime:" << this->ParentNet->Now << " CONNECTION LIST AT NODE: " << this->Id << endl;
    for (int i=0; i<(this->OriginatedConns.size()); i++)
    this->OriginatedConns[i]->print();
    cout << endl;
    /*********************************************************************************************/
    
    if (conn->Status != ADMITTED_PATH) {
      cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << conn->Id << " status " << conn->Status << endl;
      ErrorMsg("Connection state !ADMITTED_PATH not consistent");
    }
    
    return;
}

void CNode::GeneratePathErr_EdgeNode(CConnection* conn, double StartTime) {
    #ifdef STDOUT
    cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << conn->Id << " GeneratePathErr_EdgeNode: RSVP_PATH packet retrieved" << endl;
    #endif

    RSVP_PathPck* path_pk=this->DeBufferize_PathPck(conn); //Retrieve the pointer to the path packet of conn

    CNode* PreviousHop=path_pk->NodePath[path_pk->NodePath.size()-2];

    RSVP_PathErrPck* err_pk=new RSVP_PathErrPck(this,PreviousHop,path_pk,128,128,StartTime,conn,this,REMOTE_ROUTING, 0);
    WritePckNodeEvent* err_eve=new WritePckNodeEvent(err_pk->SrcNode,err_pk,this->ParentNet->Now);
    ParentNet->genEvent(err_eve);

    delete path_pk;
    return;
}

//StartTime is the time in which the first Path message in this domani is generated (e.g., routing is already performed)
void CNode::ConnEstablish_IntermediateDomain(CConnection* conn, double StartTime) {

#ifdef STDOUT
    cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << conn->Id << " ConnEstablish_IntermediateDomain RSVP_PATH packet retrieved" << endl;
#endif

    RSVP_PathPck* path_pk=this->DeBufferize_PathPck(conn); //Retrieve the pointer to the path packet of conn

    //The paths of the pk are first cleared and then copied from conn
    path_pk->NodePath.clear();
    path_pk->LinkPath.clear();

    for (int i=0; i<conn->NodePath.size(); i++)
        path_pk->NodePath.push_back(conn->NodePath[i]);
    for (int i=0; i<conn->LinkPath.size(); i++)
        path_pk->LinkPath.push_back(conn->LinkPath[i]);

    WritePckNodeEvent* eve=new WritePckNodeEvent(this,path_pk,StartTime);
    this->ParentNet->genEvent(eve);

    if (conn->Status!=ADMITTED_PATH) 
      ErrorMsg("Connection state not consistent");
    
    return;
}

//PROBLEMI NEL CANCELLARE CONN ID 0

void CNode::ConnRelease(CConnection* conn) {
  int RestorationFlag=0;
  
  #ifdef STDOUT
  cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << conn->Id << " CONNECTION MEMORY (erasing) size: " << this->ParentNet->ActiveConns.size() << " TIME: " << this->ParentNet->Now<< endl;
  #endif
  
  this->connection_memory_erasing(conn); 
  
  /*Avvia la segnalazione di release generando il primo pacchetto TearDown*/
  RSVP_TearDownPck* pk=new RSVP_TearDownPck(conn->SourceNode,conn->DestinationNode,128,128,this->ParentNet->Now,conn,RestorationFlag);
  WritePckNodeEvent* eve=new WritePckNodeEvent(pk->SrcNode,pk,this->ParentNet->Now);
  this->ParentNet->genEvent(eve);

  //Notification to PCE is nowsent when the Teardown is received at destination.
  
  //If there is a PCE send a notification for the released connection, during restoration the Notification is generated
  /*if (this->ParentNet->PCEP_NotifyMessages) {
      PCEP_NotifyReleasedPck* n_r_pk=new PCEP_NotifyReleasedPck(this,this->ParentDomain->PCE_Node,128,128,ParentNet->Now,pk->Conn,pk->RestorationFlag);
      WritePckNodeEvent* n_r_eve=new WritePckNodeEvent(n_r_pk->SrcNode,n_r_pk,ParentNet->Now);
      ParentNet->genEvent(n_r_eve);
  }*/
}

void CNode::ConnStubRelease(CConnection* conn) {
    int RestorationFlag=1;

    /*Avvia la segnalazione di release generando il primo pacchetto TearDown*/
    RSVP_TearDownPck* pk=new RSVP_TearDownPck(conn->SourceNode,conn->DestinationNode,128,128,this->ParentNet->Now,conn,RestorationFlag);
    WritePckNodeEvent* eve=new WritePckNodeEvent(pk->SrcNode,pk,this->ParentNet->Now);
    this->ParentNet->genEvent(eve);

    //Notification to PCE is nowsent when the Teardown is received at destination.
    
    //If there is a PCE send a notification for the released connection, during restoration the Notification is generated
    /*if (this->ParentNet->PCEP_NotifyMessages) {
        PCEP_NotifyReleasedPck* n_r_pk=new PCEP_NotifyReleasedPck(this,this->ParentDomain->PCE_Node,128,128,ParentNet->Now,pk->Conn,pk->RestorationFlag);
        WritePckNodeEvent* n_r_eve=new WritePckNodeEvent(n_r_pk->SrcNode,n_r_pk,ParentNet->Now);
        ParentNet->genEvent(n_r_eve);
    }*/
    
    return;
}

void CNode::FailureDetected(CLink* link) {
    /*Spedisce a tutti i nodi della rete compreso se stesso un Pacchetto OSPF_LinkFailure*/
    double FailureDetectionDuration=0; //Period of time necessary to detect the failure of the adjacent link

    for (int i=0; i<this->ParentNet->Node.size(); i++) {
        OSPF_LinkFailurePck* pk=new OSPF_LinkFailurePck(this,this->ParentNet->Node[i],128,128,this->ParentNet->Now+FailureDetectionDuration,link);
        WritePckNodeEvent* eve=new WritePckNodeEvent(this,pk,this->ParentNet->Now);
        this->ParentNet->genEvent(eve);
    }
}

void CNode::FailureDetected_OpenFlow(CLink* link) {
    /*Spedisce al controllore locale un Pacchetto OSPF_LinkFailure*/
    double FailureDetectionDuration=0; //Period of time necessary to detect the failure of the adjacent link

    OSPF_LinkFailurePck* pk=new OSPF_LinkFailurePck(this,this->ParentDomain->PCE_Node,128,128,this->ParentNet->Now+FailureDetectionDuration,link);
    WritePckNodeEvent* eve=new WritePckNodeEvent(this,pk,this->ParentNet->Now);
    this->ParentNet->genEvent(eve);
}

void CNode::RepairDetected(CLink* link) {
    /*Spedisce a tutti i nodi della rete compreso se stesso un Pacchetto OSPF_LinkFailure*/
    double RepairDetectionDuration=0; //Period of time necessary to detect the failure of the adjacent link

    for (int i=0; i<this->ParentNet->Node.size(); i++) {
        OSPF_LinkRepairPck* pk=new OSPF_LinkRepairPck(this,this->ParentNet->Node[i],128,128,this->ParentNet->Now+RepairDetectionDuration,link);
        WritePckNodeEvent* eve=new WritePckNodeEvent(this,pk,this->ParentNet->Now);
        this->ParentNet->genEvent(eve);
    }
}

void CNode::RepairDetected_OpenFlow(CLink* link) {
    /*Spedisce a tutti i nodi della rete compreso se stesso un Pacchetto OSPF_LinkFailure*/
    double RepairDetectionDuration=0; //Period of time necessary to detect the failure of the adjacent link

    OSPF_LinkRepairPck* pk=new OSPF_LinkRepairPck(this,this->ParentDomain->PCE_Node,128,128,this->ParentNet->Now+RepairDetectionDuration,link);
    WritePckNodeEvent* eve=new WritePckNodeEvent(this,pk,this->ParentNet->Now);
    this->ParentNet->genEvent(eve);
}


void CNode::CreateLinkDatabase() {
    CLink* ActualLink;
    link_dBS* LinkRecord;

    for (int i=0; i<this->ParentNet->Link.size(); i++) {
        ActualLink=this->ParentNet->Link[i];
	
	
        if (this==ActualLink->ToNode) {
	    LinkRecord = new link_dBS;
            LinkRecord->LocalPointer=ActualLink;
            LinkRecord->LocalStatus=UP;
            for (int j=0; j<this->ParentNet->W; j++)
                LinkRecord->LocalLambdaStatus.push_back(0);
            this->IncomingLink_dB.push_back(LinkRecord);
        }
        if (this==ActualLink->FromNode) {
	    LinkRecord = new link_dBS;
            LinkRecord->LocalPointer=ActualLink;
            LinkRecord->LocalStatus=UP;
            for (int j=0; j<this->ParentNet->W; j++)
                LinkRecord->LocalLambdaStatus.push_back(0);
            this->OutgoingLink_dB.push_back(LinkRecord);
        }
    }
}

//Returns the index of the link in the local link_dB with the LinkId passed as parameter
int CNode::SearchLocalLink(CLink* link, vector<link_dBS*> link_dB) {
    for (int i=0; i<link_dB.size(); i++)
        if (link_dB[i]->LocalPointer==link) return i;
        
    cout << "NODE: " << this->Id << " (" << this->Name << ") searching for link " << link->FromNode->Id << "-" << link->ToNode->Id << endl;

    ErrorMsg("SearchLocalLink: Link Id not found");
    return -1;
}

//Returns the index of the link in the LinkStateDatabase of this node
int CNode::SearchLinkState(CLink* link) {
    for (int i=0; i<this->LinkState_dB.size(); i++)
        if (this->LinkState_dB[i]->Link==link) return i;
	
    cout << "NODE: " << this->Id << " (" << this->Name << ") searching for link state " << link->FromNode->Id << "-" << link->ToNode->Id << endl;
        
    ErrorMsg("SearchLinkState: Link Id not found");
    return -1;
}

//TODO This function will return the number of differences between local database and real network status
int CNode::ScanLinkDatabase() {} //not yet implemented.

int CNode::Distributed_WavelengthAssignmentFF(RSVP_ResvPck* pk) {
    int Lambdas, SelectedLambda;
    CLink* IncomingLink;

    IncomingLink=this->GetLinkFromNode(pk->NodePath[pk->NodePath.size()-2]);
    Lambdas=this->ParentNet->W;

    #ifdef STDOUT
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " Distributed_WavelengthAssignmentFF" << endl;
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " LabelSet object:           ";
    for (int i=0; i < Lambdas; i++)
        cout << pk->LabelSet[i] << " ";
    cout << endl;
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " LambdaStatus IncomingLink: ";
    for (int i=0; i < Lambdas; i++)
        cout << IncomingLink->LambdaStatus[i] << " ";
    cout << endl;
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " FreeLambdas IncomingLink: " << IncomingLink->FreeLambdas << endl;
    #endif

    //If the object SuggestedLabel is present and if the suggested lambda is free on the incoming link the node selects it
    if ((pk->SuggestedLabel_ON) && (IncomingLink->LambdaStatus[pk->SuggestedLabel]==0) && (pk->LabelSet[pk->SuggestedLabel]==1)) {
        SelectedLambda=pk->SuggestedLabel;
	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id;
        cout << " Selected lambda (SuggestedLabel): " << SelectedLambda << endl;
	#endif
        return SelectedLambda;
    }
    
    //If no SuggestedLabel or suggested lambda is busy the node utilize the FF WA considering just lambda in the LabelSet Object
    #ifdef STDOUT
    if (!pk->SuggestedLabel_ON) cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " SuggestedLabel OFF" << endl;
    if ((pk->SuggestedLabel_ON) && ((IncomingLink->LambdaStatus[pk->SuggestedLabel]==1) || (pk->LabelSet[pk->SuggestedLabel]==0)))
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " SuggestedLabel BUSY" << endl;
    #endif
    
    if (pk->LabelSet_ON==0) ErrorMsg("Distributed_WavelengthAssignmentFF - LabelSet object not present");
    if (pk->LabelSet_ON==1) {
	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " Scanning lambda: " << flush;
	#endif
	
        for (int i=0; i<Lambdas; i++) {
	    #ifdef STDOUT
            if (pk->LabelSet[i]==1) cout << i << " " << flush;
	    #endif
	    
            if ((pk->LabelSet[i]==1) && (IncomingLink->LambdaStatus[i]==0)) {
		#ifdef STDOUT
                cout << endl << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " Selected lambda: " << i << endl;
		#endif
		
                return i;
            }
        }
    }
    //It returns -1 if all lambdas in the LabelSet are busy (it is possible because the upstream node has a delayed vision of the link)
    return -1;
}

int CNode::Distributed_WavelengthAssignmentLF(RSVP_ResvPck* pk) {
    int Lambdas, SelectedLambda;
    CLink* IncomingLink;

    IncomingLink=this->GetLinkFromNode(pk->NodePath[pk->NodePath.size()-2]);
    Lambdas=this->ParentNet->W;

#ifdef STDOUT
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " Distributed_WavelengthAssignmentLF" << endl;
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " LabelSet object:           ";
    for (int i=0; i < Lambdas; i++)
        cout << pk->LabelSet[i] << " ";
    cout << endl;
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " LambdaStatus IncomingLink: ";
    for (int i=0; i < Lambdas; i++)
        cout << IncomingLink->LambdaStatus[i] << " ";
    cout << endl;
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " FreeLambdas IncomingLink: " << IncomingLink->FreeLambdas << endl;
#endif

    //If the object SuggestedLabel is present and if the suggested lambda is free on the incoming link the node selects it
    if ((pk->SuggestedLabel_ON) && (IncomingLink->LambdaStatus[pk->SuggestedLabel]==0) && (pk->LabelSet[pk->SuggestedLabel]==1)) {
        SelectedLambda=pk->SuggestedLabel;
#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id;
        cout << " Selected lambda (SuggestedLabel): " << SelectedLambda << endl;
#endif
        return SelectedLambda;
    }
    //If no SuggestedLabel or suggested lambda is busy the node utilize the LF WA considering just lambda in the LabelSet Object
#ifdef STDOUT
    if (!pk->SuggestedLabel_ON) cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " SuggestedLabel is OFF" << endl;
    if (IncomingLink->LambdaStatus[pk->SuggestedLabel]==1) cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " SuggestedLabel BUSY" << endl;
#endif
    if (pk->LabelSet_ON==0) ErrorMsg("Distributed_WavelengthAssignmentLF - LabelSet object not present");
    if (pk->LabelSet_ON==1) {
#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " scanning lambda: " << flush;
#endif
        for (int i=Lambdas-1; i>=0; i--) {
#ifdef STDOUT
            cout << i << " " << flush;
#endif
            if ((pk->LabelSet[i]==1) && (IncomingLink->LambdaStatus[i]==0)) {
#ifdef STDOUT
                cout << " Selected lambda: " << i << endl;
#endif
                return i;
            }
        }
    }
    //It returns -1 if all lambdas in the LabelSet are busy (it is possible because the upstream node has a delayed vision of the link)
    return -1;
}

int CNode::Distributed_WavelengthAssignmentRANDOM(RSVP_ResvPck* pk) {
    int Lambdas, SelectedLambda;
    CLink* IncomingLink;

    IncomingLink=this->GetLinkFromNode(pk->NodePath[pk->NodePath.size()-2]);
    Lambdas=this->ParentNet->W;

    #ifdef STDOUT
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " Distributed_WavelengthAssignmentRANDOM" << endl;
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " LabelSet object:           ";
    for (int i=0; i < Lambdas; i++)
        cout << pk->LabelSet[i] << " ";
    cout << endl;
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " LambdaStatus IncomingLink: ";
    for (int i=0; i < Lambdas; i++)
        cout << IncomingLink->LambdaStatus[i] << " ";
    cout << endl;
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " FreeLambdas IncomingLink: " << IncomingLink->FreeLambdas << endl;
    #endif

    //If the object SuggestedLabel is present and if the suggested lambda is free on the incoming link the node selects it
    if ((pk->SuggestedLabel_ON) && (IncomingLink->LambdaStatus[pk->SuggestedLabel]==0) && (pk->LabelSet[pk->SuggestedLabel]==1)) {
        SelectedLambda=pk->SuggestedLabel;
	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id;
        cout << " Selected lambda (SuggestedLabel): " << SelectedLambda << endl;
	#endif
        return SelectedLambda;
    }
    //If no SuggestedLabel or suggested lambda is busy the node utilizes the RANDOM WA considering just lambda in the LabelSet Object
    #ifdef STDOUT
    if (!pk->SuggestedLabel_ON) cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " SuggestedLabel OFF" << endl;
    if ((pk->SuggestedLabel_ON) && (IncomingLink->LambdaStatus[pk->SuggestedLabel]==1)) cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " SuggestedLabel BUSY" << endl;
    #endif

    if (pk->LabelSet_ON==0) ErrorMsg("Distributed_WavelengthAssignmentRANDOM - LabelSet object not present");
    if (pk->LabelSet_ON==1) {
        int LabelSet_Test=0, SelectedId, j=0;
	
        for (int i=0; i<Lambdas; i++)
            if (IncomingLink->LambdaStatus[i]==0) LabelSet_Test+=pk->LabelSet[i];
	    
        if (LabelSet_Test==0) return -1;

        //this->ParentNet->LabelSetSize->AddSample(LabelSet_Test);
        //this->ParentNet->LabelSetSize->AddSample_Distribution(LabelSet_Test);

        SelectedId=randint(1,LabelSet_Test,&this->ParentNet->WavelengthAssignmentSeed);
        for (int i=0; i<Lambdas; i++) {
            if (pk->LabelSet[i]==1 && (IncomingLink->LambdaStatus[i]==0)) j++;
            if (j==SelectedId) {
		#ifdef STDOUT
                cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id;
                cout << " LabelSet_Test: " << LabelSet_Test << " SelectedId: " << SelectedId;
                cout << " Selected lambda: " << i << endl;
		#endif
                return i;
            }
        }
    }
    return -1;
}

int CNode::Flexible_Distributed_WavelengthAssignmentFF(RSVP_ResvPck* pk) {
  int SelectedSlotBase;
  vector<int> CandidatesSlotBase;
  CLink* IncomingLink;
  int slots;
  int required_slots;
  int spectrum_test;
  vector<int> updated_label_set;
  
  IncomingLink=this->GetLinkFromNode(pk->NodePath[pk->NodePath.size()-2]);
  slots=this->ParentNet->W;
  required_slots=pk->Conn->width_slots;

  #ifdef STDOUT
  cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " Flexible Distributed_WavelengthAssignmentFF" << endl;
  cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " LabelSet object:           ";
  for (int i=0; i < slots; i++)
        cout << pk->LabelSet[i] << " ";
  cout << endl;
  cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " LambdaStatus IncomingLink: ";
  for (int i=0; i < slots; i++)
    cout << IncomingLink->LambdaStatus[i] << " ";
  cout << endl;
  cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " FreeLambdas IncomingLink: " << IncomingLink->FreeLambdas << endl;
  #endif
  
  //Update the LabelSet status with the current status of the incoming link
  for (int i=0; i<slots; i++) {
    updated_label_set.push_back(pk->LabelSet[i]);
    if ((updated_label_set[i]==1) && (IncomingLink->LambdaStatus[i]==1)) {
      updated_label_set[i]=0;
    }
  }
  
  //Fill the candidate slot base
  for (int i=0; i<=slots-required_slots; i++) {
    spectrum_test=0;
    
    for (int j=0; j<required_slots; j++) 
      spectrum_test+=updated_label_set[i+j];
    
    if (spectrum_test==required_slots)
      CandidatesSlotBase.push_back(i);
  }
  
  if (CandidatesSlotBase.size()!=0) {
    #ifdef STDOUT
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id;
    //cout << " Normalized selected base slot: " << CandidatesSlotBase[0]/pk->Conn->width_slots << endl;
    cout << " Selected base slot: " << CandidatesSlotBase[0] << endl;
    //cout << " RequiredSlots: " << pk->Conn->width_slots << endl;
    #endif
    
    //Select the suggested label if it is available 
    if (pk->SuggestedLabel_ON==1) {
      for (int i=0; i<CandidatesSlotBase.size(); i++) {
	if (pk->SuggestedLabel==CandidatesSlotBase[i]) {
	  #ifdef STDOUT
	  cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " SuggestetLabel SELECTED" << endl;
	  #endif
    
	  return CandidatesSlotBase[i];
	}
      }
    }
    //Use first-fit if SuggestedLabel is not used or it is not available
    #ifdef STDOUT
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " SuggestetLabel NOT SELECTED" << endl;
    #endif
	  
    return CandidatesSlotBase[0];
  }
  else {
    return -1;
  }
}

//Performed by the source node the pk->LambdaPath is then written by the source node
int CNode::LinkStateBased_WavelengthAssignmentRANDOM(RSVP_PathPck* pk) {
    int lambdas=this->ParentNet->W;
    int VirtualLabelSet[lambdas];
    int VirtualLabelSet_Test=0, SelectedId;
    int j;

    //If the flooding is OFF or AGGRREGATED a RANDOM choice on the LabelSet is performed
    if ((this->ParentNet->RoutingMode_PCC==OFF) || (this->ParentNet->RoutingMode_PCC==AGGREGATED)) {
        //VirtualLabelSet is copied from the pk->LabelSet
        for (int i=0; i<lambdas; i++) VirtualLabelSet[i]=pk->LabelSet[i];
        VirtualLabelSet_Test=this->LabelSet_Test(pk->LabelSet);

#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " first link LabelSet_Test: " << VirtualLabelSet_Test << " LabelSet object: ";
        for (int i=0; i < lambdas; i++) cout << VirtualLabelSet[i] << " ";
        cout << endl;
#endif

        if (VirtualLabelSet_Test==0) ErrorMsg("Source RWA-RANDOM dalay_panic 0");

        SelectedId=randint(1,VirtualLabelSet_Test,&this->ParentNet->WavelengthAssignmentSeed);
        int j=0;
        for (int i=0; i<lambdas; i++) {
            if (VirtualLabelSet[i]==1) j++;
            if (j==SelectedId) {
#ifdef STDOUT
                cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " LinkStateBased_WavelengthAssignmentRANDOM (OFF/AGGREGATED)" << endl;
                cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " SelectedId: " << SelectedId << " Selected lambda: " << i << endl;
#endif
                return i;
            }
        }
        return -1;
    }

    if (this->ParentNet->RoutingMode_PCC==DETAILED) {
        //VirtualLabelSet is copied from the pk->LabelSet
        for (int i=0; i<lambdas; i++) VirtualLabelSet[i]=pk->LabelSet[i];

#ifdef STDOUT
        j=0;
        while (pk->LinkPath[0]!=this->LinkState_dB[j]->Link) j++;

        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " LinkStatus[0]     : ";
        for (int ii=0; ii < lambdas; ii++) cout << this->LinkState_dB[j]->LambdaStatus[ii] << " ";
        cout << endl;

        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " VirtualLabelSet[0]: ";
        for (int ii=0; ii < lambdas; ii++) cout << VirtualLabelSet[ii] << " ";
        cout << endl;
#endif

        //Virtually propagating the VirtualLabelSet along the LinkPath
        for (int i=1; i<pk->LinkPath.size(); i++) {
            j=0;
            while (pk->LinkPath[i]!=this->LinkState_dB[j]->Link) j++;

#ifdef STDOUT
            cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " LinkStatus[" << i << "]     : ";
            for (int ii=0; ii<lambdas; ii++) cout << this->LinkState_dB[j]->LambdaStatus[ii] << " ";
            cout << endl;
#endif

            for (int k=0; k<lambdas; k++)
                if ((VirtualLabelSet[k]==1) && (this->LinkState_dB[j]->LambdaStatus[k]==1))
                    VirtualLabelSet[k]=0;

#ifdef STDOUT
            cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " VirtualLabelSet[" << i << "]: ";
            for (int ii=0; ii < lambdas; ii++) cout << VirtualLabelSet[ii] << " ";
            cout << endl;
#endif
        }
        VirtualLabelSet_Test=0;
        for (int i=0; i<lambdas; i++) VirtualLabelSet_Test+=VirtualLabelSet[i];

        //Qui abbiam messo ErrorMsg perché se il VirtualLabelSet è nullo se ne doveva rendere conto anche il Routing che quindi non doveva scegliere questo percorso
        if (VirtualLabelSet_Test==0) ErrorMsg("Source RWA-RANDOM dalay_panic 1");

#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " VirtualLabelSet_Test: " << VirtualLabelSet_Test << endl;
#endif

        SelectedId=randint(1,VirtualLabelSet_Test,&this->ParentNet->WavelengthAssignmentSeed);
        j=0;
        for (int i=0; i<lambdas; i++) {
            if (VirtualLabelSet[i]==1) j++;
            if (j==SelectedId) {
#ifdef STDOUT
                cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " LinkStateBased_WavelengthAssignmentRANDOM (DETAILED)" << endl;
                cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " SelectedId: " << SelectedId << " Selected lambda: " << i << endl;
#endif
                return i;
            }
        }
        ErrorMsg("Source RWA-RANDOM dalay_panic 2");
        return -1;
    }
}

int CNode::LinkStateBased_WavelengthAssignmentFF(RSVP_PathPck* pk) {
    int lambdas=this->ParentNet->W;
    vector<int> VirtualLabelSet;
    int VirtualLabelSet_Test=0, SelectedId;
    int j;

    //If the flooding is OFF or AGGRREGATED a FF on the LabelSet is performed
    if ((this->ParentNet->RoutingMode_PCC==OFF) || (this->ParentNet->RoutingMode_PCC==AGGREGATED)) {
        for (int i=0; i<pk->LabelSet.size(); i++) {
            if (pk->LabelSet[i]==1) {
		#ifdef STDOUT
                cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " LinkStateBased_WavelengthAssignmentFF (OFF/AGGREGATED)" << endl;
                cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " LabelSet_Test: " << this->LabelSet_Test(pk->LabelSet) << " Selected lambda: " << i << endl;
		#endif

                return i;
            }
        }
        return -1;
    }

    if (this->ParentNet->RoutingMode_PCC==DETAILED) {
        //VirtualLabelSet is copied from the pk->LabelSet
        for (int i=0; i<lambdas; i++) VirtualLabelSet.push_back(pk->LabelSet[i]);

	#ifdef STDOUT
        j=0;
        while (pk->LinkPath[0]!=this->LinkState_dB[j]->Link) j++;

        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " LinkStatus[0]     : ";
        for (int ii=0; ii < lambdas; ii++) cout << this->LinkState_dB[j]->LambdaStatus[ii] << " ";
        cout << endl;

        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " VirtualLabelSet[0]: ";
        for (int ii=0; ii < lambdas; ii++) cout << VirtualLabelSet[ii] << " ";
        cout << endl;
	#endif

        //Virtually propagating the VirtualLabelSet along the LinkPath
        for (int i=1; i<pk->LinkPath.size(); i++) {
            j=0;
            while (pk->LinkPath[i]!=this->LinkState_dB[j]->Link) j++;

#ifdef STDOUT
            cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " LinkStatus[" << i << "]     : ";
            for (int ii=0; ii < lambdas; ii++) cout << this->LinkState_dB[j]->LambdaStatus[ii] << " ";
            cout << endl;
#endif

            for (int k=0; k<lambdas; k++)
                if ((VirtualLabelSet[k]==1) && (this->LinkState_dB[j]->LambdaStatus[k]==1))
                    VirtualLabelSet[k]=0;

#ifdef STDOUT
            cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " VirtualLabelSet[" << i << "]: ";
            for (int ii=0; ii < lambdas; ii++) cout << VirtualLabelSet[ii] << " ";
            cout << endl;
#endif
        }
        for (int i=0; i<lambdas; i++)
            VirtualLabelSet_Test+=VirtualLabelSet[i];

#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " VirtualLabelSet_Test: " << VirtualLabelSet_Test << endl;
#endif

        //Qui abbiam messo ErrorMsg perché se il VirtualLabelSet è nullo se ne doveva rendere conto anche il Routing che quindi non doveva scegliere questo percorso
        if (VirtualLabelSet_Test==0) ErrorMsg("Source RWA-FF dalay_panic 1");

        for (int i=0; i<VirtualLabelSet.size(); i++) {
            if (VirtualLabelSet[i]==1) {
#ifdef STDOUT
                cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " LinkStateBased_WavelengthAssignmentFF (DETAILED)" << endl;
                cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " VirtualLabelSet_Test: " << VirtualLabelSet_Test << " Selected lambda: " << i << endl;
#endif
                return i;
            }
        }
        ErrorMsg("Source RWA-FF dalay_panic 2");
        return -1;
    }
}

void CNode::LabelSet_Update(RSVP_PathPck* pk, link_dBS* link) {
    int lambdas=this->ParentNet->W;

    for (int k=0; k<lambdas; k++)
        if ((pk->LabelSet[k]==1) && (link->LocalLambdaStatus[k]==1)) pk->LabelSet[k]=0;
}

//It returns the number of labels available in the LabelSet
int CNode::LabelSet_Test(vector<int> LabelSet) {
    int test=0, lambdas=this->ParentNet->W;

    for (int k=0; k<lambdas; k++)
        test+=LabelSet[k];

    return test;
}

void CNode::PathErrPckManage_ConnSourceNode_Provisioning(RSVP_PathErrPck* pk) {
  int attempt;
  CNet* net=this->ParentNet;
  
  attempt=pk->Conn->ProvisioningAttempt-1;

    //The connection status is changed in BLOCKED_PATH
    pk->Conn->Status=BLOCKED_PATH;

    //Insert in the list of Blocked path only the path inside this domain
    for (int path_id=0; path_id<pk->Conn->ActualPath.size(); path_id++) {
        Path* CurrentPath=pk->Conn->ActualPath[path_id];
        if (CurrentPath->ParentDomain == this->ParentDomain)
            pk->Conn->PathsBlocked.push_back(CurrentPath);
    }

    //The blocking reason is checked and the proper counters are increased.
    net->ConnBlocked++; net->ConnBlockedREP++;
    net->ConnBlockedATT[attempt]++; net->ConnBlockedATT_REP[attempt]++;
    net->ConnRequestedATT[attempt]++; net->ConnRequestedATT_REP[attempt]++;

    //Switch of Block_Type (FORWARD, BACKWARD)
    switch (pk->GenerationDirection) {
    case FORWARD:
        //Insert in the list of Blocked_Forward path only the path inside this domain
        for (int path_id=0; path_id<pk->Conn->ActualPath.size(); path_id++) {
            Path* CurrentPath=pk->Conn->ActualPath[path_id];
            if (CurrentPath->ParentDomain == this->ParentDomain)
                pk->Conn->PathsBlocked_Forward.push_back(CurrentPath);
        }
        net->ConnBlockedForward++; net->ConnBlockedForwardREP++;
        net->ConnBlockedForwardATT[attempt]++; this->ParentNet->ConnBlockedForwardATT_REP[attempt]++;
        break;

    case BACKWARD:
        //Insert in the list of Blocked_Backward path only the path inside this domain
        for (int path_id=0; path_id<pk->Conn->ActualPath.size(); path_id++) {
            Path* CurrentPath=pk->Conn->ActualPath[path_id];
            if (CurrentPath->ParentDomain == this->ParentDomain)
                pk->Conn->PathsBlocked_Backward.push_back(CurrentPath);
        }
        net->ConnBlockedBackward++; net->ConnBlockedBackwardREP++;
        net->ConnBlockedBackwardATT[attempt]++; net->ConnBlockedBackwardATT_REP[attempt]++;
        break;

    default:
        ErrorMsg("Packet GenerationDirection not known (allowed values: FORWARD, BACKWARD)");
    }
    
    //Switch of Block_SubType (EMPTY_LABELSET (FORWARD), COLLISION (BACKWARD), LINK_DOWN (BACKWARD))
    switch (pk->GenerationMode) {
    case COLLISION: //BACKWARD BLOCKING of SubType COLLISION
        //Insert in the list of Blocked_Collision path only the path inside this domain
        for (int path_id=0; path_id<pk->Conn->ActualPath.size(); path_id++) {
            Path* CurrentPath=pk->Conn->ActualPath[path_id];
            if (CurrentPath->ParentDomain == this->ParentDomain)
                pk->Conn->PathsBlocked_Collision.push_back(CurrentPath);
        }

        this->ParentNet->ConnBlocked_Collision++;
        break;

    case EMPTY_LABELSET: //FORWARD BLOCKING of SubType EMPTY_LABELSET
        //Insert in the list of Blocked_Forward path only the path inside this domain
        for (int path_id=0; path_id<pk->Conn->ActualPath.size(); path_id++) {
            Path* CurrentPath=pk->Conn->ActualPath[path_id];
            if (CurrentPath->ParentDomain == this->ParentDomain)
                pk->Conn->PathsBlocked_EmptyLabelSet.push_back(CurrentPath);
        }

        this->ParentNet->ConnBlocked_EmptyLabelSet++;
        break;

    case LINK_DOWN: //BACKWARD BLOCKING of SubType LINK_DOWN
        //Insert in the list of Blocked_Forward path only the path inside this domain
        for (int path_id=0; path_id<pk->Conn->ActualPath.size(); path_id++) {
            Path* CurrentPath=pk->Conn->ActualPath[path_id];
            if (CurrentPath->ParentDomain == this->ParentDomain)
                pk->Conn->PathsBlocked_LinkDown.push_back(CurrentPath);
        }

        this->ParentNet->ConnBlocked_LinkDown++;
        break;

    default:
      //UNACCEPTABLE_LABEL and REMOTE_ROUTING not supported in this version
      ErrorMsg("Packet GenerationMode not known (allowed values: EMPTY_LABELSET, COLLISION, LINK_DOWN)");
    }

    #ifdef STDOUT
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " connection BLOCKED_PATH (0-COLLISION, 1-EMPTY_LABELSET, 2-LINK_DOWN): " << pk->GenerationMode << endl;
    #endif
    
    //Increase the establishment attempt counter
    pk->Conn->ProvisioningAttempt++;
    attempt=pk->Conn->ProvisioningAttempt-1;

    if (pk->Conn->ProvisioningAttempt <= this->ParentNet->ProvisioningAttempt_MAX) {
      if (this->ParentNet->Provisioning_DomainPCE)
	ErrorMsg("Using PCE only one provisioning attempt is supported");
      
      //Depending on the PathErr GenerationMode it is necessary to compute a new path (ConnectionAdmissionControl) or not
        int ConnectionAdmitted, RoutingPerformed;

        switch (pk->GenerationMode) {
        case COLLISION:
	  ConnectionAdmitted=1;
	  RoutingPerformed=0;
	  break;

        case EMPTY_LABELSET:
	  ConnectionAdmitted=this->ConnectionAdmissionControl(pk->Conn);
	  RoutingPerformed=1;
	  break;

        case LINK_DOWN:
	  if (this->ParentNet->SimulationMode==PROV)
	    ErrorMsg("LINK_DOWN during Provisioning");
	  
	  ConnectionAdmitted=this->ConnectionAdmissionControl(pk->Conn);
	  RoutingPerformed=1;
	  break;
	  
        default:
            ErrorMsg("Packet GenerationMode not known (allowed values: EMPTY_LABELSET, COLLISION, LINK_DOWN)");
        }

        if (ConnectionAdmitted==1) {
	    #ifdef STDOUT
            cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << pk->Conn->Id << " type: " << pk->Conn->Type << " ADMITTED for signaling attempt: " << pk->Conn->ProvisioningAttempt << " along the path:";
            for (int i=0; i < pk->NodePath.size(); i++)
                cout << " " << pk->NodePath[i]->Id;
            cout << endl;
	    #endif

            pk->Conn->Status=ADMITTED_PATH;
	    
	    net->ConnAdmittedPath++; net->ConnAdmittedPathREP++;
	    net->ConnAdmittedPathATT[attempt]++; net->ConnAdmittedPathATT_REP[attempt]++;

	    //RoutingPerformed is 0 in case of collision because the same path is used
	    //RoutingPerformed is 1 in case of empty_labelset and link_down because a new path is computed
	    pk->Conn->RoutingFlag=RoutingPerformed;

            this->ConnEstablish(pk->Conn,this->ParentNet->Now);
        }
        else {
	    #ifdef STDOUT
            cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " REFUSED after " << pk->Conn->ProvisioningAttempt-1 << " signaling attempts (NO MORE PATHS)" << endl;
	    #endif
	    
	    this->connection_memory_erasing(pk->Conn);

            pk->Conn->Status=REFUSED;
	    
	    net->ConnRequestedATT[attempt]++; net->ConnRequestedATT_REP[attempt]++;
	    net->ConnBlockedATT[attempt]++; net->ConnBlockedATT_REP[attempt]++; 
	    net->ConnBlockedRoutingATT[attempt]++; net->ConnBlockedRoutingATT_REP[attempt]++;

            delete pk->Conn;
        }
    }
    //Establishment attempt limit has been reached
    else {
	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " REFUSED after " << pk->Conn->ProvisioningAttempt-1 << " signaling attempts (MAX ATTEMPTS)" << endl;
        cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << pk->Conn->Id << " CONNECTION MEMORY (erasing) size: " << this->ParentNet->ActiveConns.size() << " TIME: " << this->ParentNet->Now<< endl;
	#endif
	
	if ((this->ParentNet->TypeHPCE == PROACTIVE_HPCE) && (pk->ConnType==INTER_DOMAIN_CONN)) {
	      PCEP_NotifyBlockedPck* n_b_pk=new PCEP_NotifyBlockedPck(this,this->ParentNet->HPCE_Node,128,128,ParentNet->Now,pk->Conn,pk,pk->GenerationDirection,pk->RestorationFlag);
	      WritePckNodeEvent* n_b_eve=new WritePckNodeEvent(n_b_pk->SrcNode,n_b_pk,ParentNet->Now);
	      ParentNet->genEvent(n_b_eve);
	    }
	
        this->connection_memory_erasing(pk->Conn);

        pk->Conn->Status=REFUSED;

	net->ConnRequested++; net->ConnRequestedREP++;
        net->ConnRefused++; net->ConnRefusedREP++;

        delete pk->Conn;
    }

    delete pk;
    return;
}

void CNode::PathErrPckManage_ConnSourceNode_Restoration(RSVP_PathErrPck* pk) {
  int attempt;
  CNet* net=this->ParentNet;
  
  #ifdef STDOUT
  cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " PathErrPckManage_ConnSourceNode_Restoration" << endl;
  #endif

  attempt=pk->Conn->RestorationAttempt-1;
  
  //The connection status is changed in BLOCKED_PATH
  pk->Conn->Status=BLOCKED_PATH_RESTORATION;

  //Insert in the list of Blocked path only the path inside this domain
  for (int path_id=0; path_id<pk->Conn->ActualPath.size(); path_id++) {
    Path* CurrentPath=pk->Conn->ActualPath[path_id];
    if (CurrentPath->ParentDomain == this->ParentDomain)
      pk->Conn->PathsBlocked_Restoration.push_back(CurrentPath);
  }

  net->ConnBlocked_Restoration++; 
  net->ConnBlocked_RestorationREP++;
  net->ConnBlocked_RestorationATT[attempt]++; 
  net->ConnBlocked_RestorationATT_REP[attempt]++;

  switch (pk->GenerationMode) {
    case COLLISION:
        //Insert in the list of Blocked path only the path inside this domain
        for (int path_id=0; path_id<pk->Conn->ActualPath.size(); path_id++) {
            Path* CurrentPath=pk->Conn->ActualPath[path_id];
            if (CurrentPath->ParentDomain == this->ParentDomain) {
                pk->Conn->PathsBlocked_Backward_Restoration.push_back(CurrentPath);
                pk->Conn->PathsBlocked_Collision_Restoration.push_back(CurrentPath);
            }
        }
        net->ConnBlockedBackward_Restoration++; net->ConnBlockedBackward_RestorationREP++;
        net->ConnBlockedBackward_RestorationATT[attempt]++; net->ConnBlockedBackward_RestorationATT_REP[attempt]++;
	
        break;
	
    case EMPTY_LABELSET:
        //Insert in the list of Blocked path only the path inside this domain
        for (int path_id=0; path_id<pk->Conn->ActualPath.size(); path_id++) {
            Path* CurrentPath=pk->Conn->ActualPath[path_id];
            if (CurrentPath->ParentDomain == this->ParentDomain) {
                pk->Conn->PathsBlocked_Forward_Restoration.push_back(CurrentPath);
                pk->Conn->PathsBlocked_EmptyLabelSet_Restoration.push_back(CurrentPath);
            }
        }
        net->ConnBlockedForward_Restoration++; net->ConnBlockedForward_RestorationREP++;
        net->ConnBlockedForward_RestorationATT[attempt]++; net->ConnBlockedForward_RestorationATT_REP[attempt]++;
	
        break;
	
    case LINK_DOWN:
        ErrorMsg("Connection blocked LINK_DOWN during restoration (1)");
        break;
    default:
        ErrorMsg("PathErrPckManage_ConnSourceNode_Restoration (1): pk->GenerationMode NOT KNOWN");
    }

    //Increase the restoration attempt counter
    pk->Conn->RestorationAttempt++;
    attempt=pk->Conn->RestorationAttempt-1;

    if ((pk->Conn->RestorationAttempt <= this->ParentNet->RestorationAttempt_MAX) && (pk->GenerationMode==COLLISION)) {
        int ConnectionAdmitted, RoutingPerformed;

        switch (pk->GenerationMode) {
        case COLLISION:
            ConnectionAdmitted=1;
            RoutingPerformed=0;
            break;
        case EMPTY_LABELSET:
            ConnectionAdmitted=this->ConnectionAdmissionControl(pk->Conn);
            RoutingPerformed=1;
            break;
        case LINK_DOWN:
            ErrorMsg("Connection blocked LINK_DOWN during restoration (2)");
            break;
        default:
            ErrorMsg("PathErrPckManage_ConnSourceNode_Restoration (2): pk->GenerationMode NOT KNOWN");
        }

        if (ConnectionAdmitted==1) {
	    #ifdef STDOUT
            cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " ADMITTED for restoration attempt: " << pk->Conn->RestorationAttempt << " along the path:";
            for (int i=0; i < pk->NodePath.size(); i++)
                cout << " " << pk->NodePath[i]->Id;
            cout << endl;
	    #endif

            pk->Conn->Status=ADMITTED_PATH_RESTORATION;
            net->ConnAdmittedPath_Restoration++; net->ConnAdmittedPath_RestorationREP++;

	    //RoutingPerformed is 0 in case of collision because the same path is used
	    //RoutingPerformed is 1 in case of empty_labelset and link_down because a new path is computed
	    pk->Conn->RoutingFlag=RoutingPerformed;
	    
            this->ConnEstablish(pk->Conn,this->ParentNet->Now);
        }
        else {
	    #ifdef STDOUT
            cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " REFUSED_RESTORATION after " << pk->Conn->ProvisioningAttempt << " restoration attempts (NO MORE PATHS)" << endl;
	    #endif
	    
	    this->connection_memory_erasing(pk->Conn);

            pk->Conn->Status=REFUSED_RESTORATION;

            net->ConnRefused_Restoration++; net->ConnRefused_RestorationREP++;
	    
            net->ConnRefusedRouting_Restoration++; net->ConnRefusedRouting_RestorationREP++;
            net->ConnRefusedRouting_RestorationATT[attempt]++; net->ConnRefusedRouting_RestorationATT_REP[attempt]++;

            //Non cancella la connessione perché verrà fatto dal release event
        }
        
        delete pk;
	return;
    }
    else {
	#ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " REFUSED_RESTORATION after " << pk->Conn->RestorationAttempt-1 << " restoration attempts (MAX ATTEMPTS)" << endl;
        cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << pk->Conn->Id << " CONNECTION MEMORY (erasing) size: " << this->ParentNet->ActiveConns.size() << " TIME: " << this->ParentNet->Now<< endl;
	#endif
	
	if ((this->ParentNet->TypeHPCE == PROACTIVE_HPCE) && (pk->ConnType==INTER_DOMAIN_CONN)) {
	      PCEP_NotifyBlockedPck* n_b_pk=new PCEP_NotifyBlockedPck(this,this->ParentNet->HPCE_Node,128,128,ParentNet->Now,pk->Conn,pk,pk->GenerationDirection,pk->RestorationFlag);
	      WritePckNodeEvent* n_b_eve=new WritePckNodeEvent(n_b_pk->SrcNode,n_b_pk,ParentNet->Now);
	      ParentNet->genEvent(n_b_eve);
	    }
	
	this->connection_memory_erasing(pk->Conn);

        pk->Conn->Status=REFUSED_RESTORATION;
	
	net->ConnRefused_Restoration++; net->ConnRefused_RestorationREP++;
	net->ConnRefusedMaxAtt_Restoration++; net->ConnRefusedMaxAtt_RestorationREP++;

        delete pk;
        return;
    }
}

//TODO Ci sono due timer su ciacun link il ghe implica la duplicazione di alcuni pezzi di codice tra cui la seguente funzione
//TODO Mettere il timer al nodo per ciascun link risolverebbe questa duplicazione
//If the LSA pck is scheduled NOW the function returns 1 otherwise it returns 0
int CNode::Generate_LsaPck(CLink* AdvertisedLink) {
    CNet* net=this->ParentNet;
    
    //If NO_INFO: do nothing
    if (net->FloodingMode==OFF)
	return 0;
    
    if (this==AdvertisedLink->ToNode) {
      //If an LSA is already scheduled for this link: do nothing
      if (AdvertisedLink->LSA_NextScheduled_ToNode==1) {
	  #ifdef STDOUT_FLOODING
	  cout << "\nPOSTPONED LSAs generation already SCHEDULED at node: " << this->Id << " for link: " << AdvertisedLink->FromNode->Id << "->" << AdvertisedLink->ToNode->Id << " TIME: " << this->ParentNet->Now << " postponed at TIME: " << AdvertisedLink->LSA_LastGenerationTime_ToNode + this->ParentNet->OSPF_MinLSInterval << endl;
	  #endif
	  
	  return 0;
      }
    
      //If an LSA is not scheduled but the timer from last LSA is not expired: schedule next LSA at timer expiration
      if ((net->Now < AdvertisedLink->LSA_LastGenerationTime_ToNode + net->OSPF_MinLSInterval) && (AdvertisedLink->LSA_LastGenerationTime_ToNode > 0)) {
	  #ifdef STDOUT_FLOODING
	  cout << "\nPOSTPONED LSAs generation at node: " << this->Id << " for link: " << AdvertisedLink->FromNode->Id << "->" << AdvertisedLink->ToNode->Id << " TIME: " << this->ParentNet->Now << " postponed at TIME: " << AdvertisedLink->LSA_LastGenerationTime_ToNode + this->ParentNet->OSPF_MinLSInterval << endl;
	  #endif
	  
	  ParentNet->GeneratedLSA++;
	  ParentNet->PostponedLSA++;

	  AdvertisedLink->LSA_NextScheduled_ToNode=1;
	  AdvertisedLink->LSA_NextScheduledTime_ToNode = AdvertisedLink->LSA_LastGenerationTime_ToNode + net->OSPF_MinLSInterval;

	  OSPF_LsaPck* o_pk=new OSPF_LsaPck(this,this,128,128,AdvertisedLink->LSA_NextScheduledTime_ToNode,this,AdvertisedLink);
	  WritePckNodeEvent* w_eve=new WritePckNodeEvent(this,o_pk,AdvertisedLink->LSA_NextScheduledTime_ToNode);
	  this->ParentNet->genEvent(w_eve);

	  return 0;
      }
    
      //If LSA is not scheduled and the timer is expired: immediate generation of LSA and start the timer
      if ((net->Now >= AdvertisedLink->LSA_LastGenerationTime_ToNode + net->OSPF_MinLSInterval) || (AdvertisedLink->LSA_LastGenerationTime_ToNode == 0))  {
	  #ifdef STDOUT_FLOODING
	  cout << "\nIMMEDIATE LSAs generation at node: " << this->Id << " for link: " << AdvertisedLink->FromNode->Id << "->" << AdvertisedLink->ToNode->Id << " TIME: " << ParentNet->Now << endl;
	  #endif
	  
	  ParentNet->GeneratedLSA++;
	  ParentNet->ImmediateLSA++;

	  AdvertisedLink->LSA_NextScheduled_ToNode=1;
	  AdvertisedLink->LSA_NextScheduledTime_ToNode = ParentNet->Now;

	  OSPF_LsaPck* o_pk=new OSPF_LsaPck(this,this,128,128,ParentNet->Now,this,AdvertisedLink);
	  WritePckNodeEvent* w_eve=new WritePckNodeEvent(this,o_pk,ParentNet->Now);
	  this->ParentNet->genEvent(w_eve);

	  return 1;
      }
    }
    if (this==AdvertisedLink->FromNode) {
      //If an LSA is already scheduled for this link: do nothing
      if (AdvertisedLink->LSA_NextScheduled_FromNode==1) {
      	#ifdef STDOUT_FLOODING
        cout << "\nPOSTPONED LSAs generation already SCHEDULED at node: " << this->Name << " for link: " << AdvertisedLink->FromNode->Id << "->" << AdvertisedLink->ToNode->Id << " TIME: " << this->ParentNet->Now << " postponed at TIME: " << AdvertisedLink->LSA_LastGenerationTime_FromNode + this->ParentNet->OSPF_MinLSInterval << endl;
	#endif
	
        return 0;
    }
    
    //If an LSA is not scheduled but the timer from last LSA is not expired: schedule next LSA at timer expiration
    if ((net->Now < AdvertisedLink->LSA_LastGenerationTime_FromNode + net->OSPF_MinLSInterval) && (AdvertisedLink->LSA_LastGenerationTime_FromNode > 0)) {
	#ifdef STDOUT_FLOODING
        cout << "\nPOSTPONED LSAs generation at node: " << this->Id << " for link: " << AdvertisedLink->FromNode->Id << "->" << AdvertisedLink->ToNode->Id << " TIME: " << this->ParentNet->Now << " postponed at TIME: " << AdvertisedLink->LSA_LastGenerationTime_FromNode + this->ParentNet->OSPF_MinLSInterval << endl;
	#endif
	
        ParentNet->GeneratedLSA++;
        ParentNet->PostponedLSA++;

        AdvertisedLink->LSA_NextScheduled_FromNode=1;
        AdvertisedLink->LSA_NextScheduledTime_FromNode = AdvertisedLink->LSA_LastGenerationTime_FromNode + net->OSPF_MinLSInterval;

        OSPF_LsaPck* o_pk=new OSPF_LsaPck(this,this,128,128,AdvertisedLink->LSA_NextScheduledTime_FromNode,this,AdvertisedLink);
        WritePckNodeEvent* w_eve=new WritePckNodeEvent(this,o_pk,AdvertisedLink->LSA_NextScheduledTime_FromNode);
        this->ParentNet->genEvent(w_eve);

        return 0;
    }
    
    //If LSA is not scheduled and the timer is expired: immediate generation of LSA and start the timer
    if ((net->Now >= AdvertisedLink->LSA_LastGenerationTime_FromNode + net->OSPF_MinLSInterval) || (AdvertisedLink->LSA_LastGenerationTime_FromNode == 0))  {
	#ifdef STDOUT_FLOODING
        cout << "\nIMMEDIATE LSAs generation at node: " << this->Id << " for link: " << AdvertisedLink->FromNode->Id << "->" << AdvertisedLink->ToNode->Id << " TIME: " << ParentNet->Now << endl;
	#endif
	
        ParentNet->GeneratedLSA++;
        ParentNet->ImmediateLSA++;

        AdvertisedLink->LSA_NextScheduled_FromNode=1;
        AdvertisedLink->LSA_NextScheduledTime_FromNode = ParentNet->Now;

        OSPF_LsaPck* o_pk=new OSPF_LsaPck(this,this,128,128,ParentNet->Now,this,AdvertisedLink);
        WritePckNodeEvent* w_eve=new WritePckNodeEvent(this,o_pk,ParentNet->Now);
        this->ParentNet->genEvent(w_eve);

        return 1;
    }
    }
}

LinkState* CNode::Install_SelfOriginated_LinkState(OSPF_LsaPck* pk) {
    int LSA_id=0,BusyLambdas=0,FreeLambdas=0;
    CLink* link=pk->Link_Flooded;
    LinkState* link_state;
    int local_link_id;
    link_dBS* local_link;

    while (link!=this->LinkState_dB[LSA_id]->Link) LSA_id++;
    if (this->LinkState_dB[LSA_id]->SequenceNumber == 0) ErrorMsg("Generating LSA - SequenceNumber OVERFLOW !!!");

    link_state=this->LinkState_dB[LSA_id];
    
    //The link is Outgoing
    if (pk->Advertising_Node==link->FromNode) {
      link->LSA_LastSequenceNumber_FromNode=pk->sequence_number;
      
      local_link_id=this->SearchLocalLink(link,this->OutgoingLink_dB);
      local_link=this->OutgoingLink_dB[local_link_id];
    }
    
    //The link is Incoming
    if (pk->Advertising_Node==link->ToNode) {
      link->LSA_LastSequenceNumber_ToNode=pk->sequence_number;
      
      local_link_id=this->SearchLocalLink(link,this->IncomingLink_dB);
      local_link=this->IncomingLink_dB[local_link_id];
    }

    //The information must be loaded from IncomingLink_dB and OutgoingLink_dB (not directly from CLink)
    for (int j=0; j<local_link->LocalLambdaStatus.size(); j++)
        BusyLambdas+=local_link->LocalLambdaStatus[j];
    FreeLambdas=this->ParentNet->W-BusyLambdas;

    link_state->AdvertisingNode=this;
    link_state->SequenceNumber=pk->sequence_number;
    link_state->FreeLambdas=FreeLambdas;
    link_state->LambdaStatus.clear();
    for (int j=0; j<local_link->LocalLambdaStatus.size(); j++)
        link_state->LambdaStatus.push_back(local_link->LocalLambdaStatus[j]);

    link_state->InstallationTime=this->ParentNet->Now;
    return link_state;
}

LinkState* CNode::Install_Received_LinkState(OSPF_LsaPck* pk) {
    int LSA_id=0,BusyLambdas=0,FreeLambdas=0;

    CLink* link=pk->Link_Flooded;

    while (link!=this->LinkState_dB[LSA_id]->Link) LSA_id++;
    if (this->LinkState_dB[LSA_id]->SequenceNumber == 0) ErrorMsg("Generating LSA - SequenceNumber OVERFLOW !!!");

    LinkState* link_state=this->LinkState_dB[LSA_id];

    //Setting the fields of the LinkState_dB entry
    link_state->AdvertisingNode=pk->Advertising_Node;
    link_state->SequenceNumber=pk->sequence_number;
    link_state->FreeLambdas=pk->free_lambdas;
    link_state->LambdaStatus.clear();
    for (int j=0; j<pk->LambdaStatus.size(); j++)
        link_state->LambdaStatus.push_back(pk->LambdaStatus[j]);

    link_state->InstallationTime=this->ParentNet->Now;
    return link_state;
}


int CNode::ConnectionAdmissionControl(CConnection* conn) {
    int i,peso,SelectedLambda,FreeLambdas;
    vector<CNode*> nodes;
    vector<CLink*> links;
    vector<CNode*> ReverseNodes;
    vector<CLink*> ReverseLinks;

    #ifdef STDOUT
    cout << "NODE: " << Id << " (" << Name << "): conn id " << conn->Id << " src: " << conn->SourceNode->Id << " dst: " << conn->DestinationNode->Id;
    cout << " performing Connection Admission Control for signaling attempt: " << conn->ProvisioningAttempt <<  " in Source Domain: " << this->ParentDomain->Name << endl;
    #endif

    //The paths of the connection are cleared
    conn->NodePath.clear();
    conn->LinkPath.clear();

    /*struct timeval tv;
    struct timezone tz;
    int micro_1,micro_2;
    int sec_1,sec_2;
    int required_time;

    gettimeofday(&tv,&tz);
    sec_1=tv.tv_sec;
    micro_1=tv.tv_usec;*/

    //Distributed random choice of the path
    switch (this->ParentNet->RoutingMode_PCC) {
    case OFF:
        peso=this->PathTable->NoInfo_RandomPath(this,conn->DestinationNode, &nodes, &links, conn);
        break;
    case AGGREGATED:
        peso=this->PathTable->AggInfo_LeastCongestedPath(this,conn->DestinationNode, &nodes, &links, conn);
        break;
    case DETAILED:
        peso=this->PathTable->Flexible_DetInfo_LeastCongestedPath(this,conn->DestinationNode, &nodes, &links, conn);
        break;
    }

    /*gettimeofday(&tv,&tz);
    sec_2=tv.tv_sec;
    micro_2=tv.tv_usec;

    required_time = (sec_2-sec_1)*1e6+(micro_2-micro_1);
    this->ParentNet->RoutingTime.push_back(required_time);
    if (this->ParentNet->RoutingTime.size()==10000) {
    	double mu=0,sigma2=0,CI=0;
    	this->ParentNet->ConfidenceInterval(90,110,10,90,this->ParentNet->RoutingTime,&mu,&sigma2,&CI);
    	cout << "mu: " << mu << " ci: " << CI << endl;
    }

    cout << "sec_1: " << sec_1 << " sec_2: " << sec_2 << " delta micro time: " << micro_2-micro_1 << " corretto: " << (sec_2-sec_1)*1e6+(micro_2-micro_1) << endl;
    if ( micro_2-micro_1 < 0) WarningMsg("negative time");*/

    if (peso==0) {
	#ifdef STDOUT
        cout << "\nConnection REFUSED during routing: conn id " << conn->Id << endl;
        conn->print();
	#endif

        return 0;
    }

    /*Connection Admitted along the selected path - Writing NodePath and LinkPath in the CConnection*/
    conn->NodePath=nodes;
    conn->LinkPath=links;

    #ifdef STDOUT
    cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << conn->Id << " type: " << conn->Type << " ADMITTED for signaling attempt: " << conn->ProvisioningAttempt << " along the path:";
    for (int i=0; i < conn->NodePath.size(); i++)
        cout << " " << conn->NodePath[i]->Id;
    cout << endl;
    //conn->print();
    #endif

    return 1;
}

int CNode::ConnectionAdmissionControl_IntermediateDomain(CConnection* conn, RSVP_PathPck* pk) {
    int i,peso,SelectedLambda,FreeLambdas;
    vector<CNode*> nodes = pk->NodePath;
    vector<CLink*> links = pk->LinkPath;

    #ifdef STDOUT
    cout << "NODE: " << Id << " (" << Name << "): conn id " << conn->Id << " src: " << conn->SourceNode->Id << " dst: " << conn->DestinationNode->Id;
    cout << " performing Connection Admission Control in Intemediate Domain: " << this->ParentDomain->Name << endl;
    #endif

    //Last hop in ERO is removed beacuse it will be included as source of the computed path in this domain
    nodes.pop_back();

    //Distributed random choice of the path
    switch (this->ParentNet->RoutingMode_PCC) {
    case OFF:
        peso=this->PathTable->NoInfo_RandomPath(this,conn->DestinationNode, &nodes, &links, conn);
        break;
    case AGGREGATED:
        peso=this->PathTable->AggInfo_LeastCongestedPath(this,conn->DestinationNode, &nodes, &links, conn);
        break;
    case DETAILED:
        peso=this->PathTable->Flexible_DetInfo_LeastCongestedPath(this,conn->DestinationNode, &nodes, &links, conn);
        break;
    }

    if (peso==0) {
#ifdef STDOUT
        cout << "\nConnection REFUSED during routing: conn id " << conn->Id << endl;
        conn->print();
#endif

        return 0;
    }

    //The paths of the connection are cleared
    conn->NodePath.clear();
    conn->LinkPath.clear();

    //The EROs are cleared
    pk->NodePath.clear();
    pk->LinkPath.clear();

    /*Connection Admitted along the selected path - Writing NodePath and LinkPath in the CConnection*/
    i=0;
    while (i<nodes.size()) {
        conn->NodePath.push_back(nodes[i]);
        pk->NodePath.push_back(nodes[i]);
        i++;
    }
    i=0;
    while (i<links.size()) {
        conn->LinkPath.push_back(links[i]);
        pk->LinkPath.push_back(links[i]);
        i++;
    }

#ifdef STDOUT
    cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << conn->Id << " type: " << conn->Type << " ADMITTED for signaling attempt: " << conn->ProvisioningAttempt << " along the path:";
    for (int i=0; i < nodes.size(); i++)
        cout << " " << nodes[i]->Id;
    cout << endl;
    //conn->print();
#endif

    return 1;
}

int CNode::ConnectionDistributedRestoration(CConnection* conn) {
    int peso, SelectedLambda;
    vector<CNode*> nodes;
    vector<CLink*> links;

    #ifdef STDOUT
    cout << "\nPerforming DISTRIBUTED restoration... conn id " << conn->Id << " " << conn->Status << " " << conn->SourceNode->Id << "->" << conn->DestinationNode->Id << endl;
    #endif
    
    //Re-routing is always performed locally in distributed restoration
    conn->RoutingFlag=1;

    //Dynamically choose a new path for connection conn
    switch (this->ParentNet->RoutingMode_PCC) {
    case OFF:
        peso=this->PathTable->NoInfo_RandomPath(this,conn->DestinationNode, &nodes, &links, conn);
        break;
    case AGGREGATED:
        peso=this->PathTable->AggInfo_LeastCongestedPath(this,conn->DestinationNode, &nodes, &links, conn);
        break;
    case DETAILED:
        peso=this->PathTable->Flexible_DetInfo_LeastCongestedPath(this,conn->DestinationNode, &nodes, &links, conn);
        break;
    }

    if (peso==0) {
	#ifdef STDOUT
	cout << "NODE: " << Id << " (" << Name << "): conn id " << conn->Id << " REFUSED_RESTORATION after " << conn->RestorationAttempt-1 << " routing fails in DistributedRestoration" << endl;
        cout << "\nConnection cannot be restored (routing fails) conn id " << conn->Id << endl;
        conn->print();
	#endif

        /*Rimuove il puntatore alla connessione nel vector di connessioni attive al nodo sorgente*/
        vector<CConnection*>::iterator it_node=this->OriginatedConns.begin();
        while (conn!=*it_node) it_node++;
        this->OriginatedConns.erase(it_node);
        /*Rimuove il puntatore alla connessione nel vector di connessioni attive in tutta la rete*/
        vector<CConnection*>::iterator it_net=this->ParentNet->ActiveConns.begin();
        while (conn!=*it_net) it_net++;
        this->ParentNet->ActiveConns.erase(it_net);

        conn->Status=REFUSED_RESTORATION;

        this->ParentNet->ConnRefused_Restoration++;
        this->ParentNet->ConnRefused_RestorationREP++;
        return 0;
    }
    //If the execution is here selected lambda along computed path is actually available
    
    #ifdef STDOUT
    cout << "NODE: " << conn->SourceNode->Id << " (" << conn->SourceNode->Name << "): conn id " << conn->Id << " ADMITTED for restoration attempt: " << conn->ProvisioningAttempt << " along the path:";
    for (int i=0; i < nodes.size(); i++)
        cout << " " << nodes[i]->Id;
    cout << endl;
    #endif
    
    int i=0;
    while (i<nodes.size()) {
        conn->NodePath.push_back(nodes[i]);
        i++;
    }
    i=0;
    while (i<links.size()) {
        conn->LinkPath.push_back(links[i]);
        i++;
    }

    conn->Status=ADMITTED_PATH_RESTORATION;

    //Connection Admitted along selected path -> START SIGNALING
    this->ConnEstablish(conn,this->ParentNet->Now);

    return 1;
}

void CNode::Bufferize_PathPck(RSVP_PathPck* pk) {
    this->Bufferized_PathPck.push_back(pk);
}

RSVP_PathPck* CNode::DeBufferize_PathPck(CConnection* conn) {
    int i=0;
    RSVP_PathPck* pointer;

    //Find the path packet pointer
    while (conn!=this->Bufferized_PathPck[i]->Conn) i++;
    pointer=this->Bufferized_PathPck[i];

    //Erase the path packet pointer from the vector
    vector<RSVP_PathPck*>::iterator begin=this->Bufferized_PathPck.begin();
    this->Bufferized_PathPck.erase(begin+i);

    return pointer;
}

void CNode::print() {
    cout << "%%%%%%%%%%%%%%%%%%%%%%" << endl;
    cout << "% Node: " << this->Id << " (" << this->Name << ") %" << endl;
    cout << "%%%%%%%%%%%%%%%%%%%%%%" << endl;

    for (int i=0; i<this->Destination_DomainNode_dB.size(); i++)
        cout << "ToNode: " << this->Destination_DomainNode_dB[i].ToNode->Id << " DomainDstNode: " << this->Destination_DomainNode_dB[i].DstDomainNode->Id << endl;
}

int CNode::check_available_link_spectrum(int base, int width, CLink* link) {
  int spectrum_test=0;
  
  for (int i=base; i<base+width; i++) {
    spectrum_test+=link->LambdaStatus[i];
  }
  
  if (spectrum_test==0) return 1;
  else return 0;
}

int CNode::check_busy_link_spectrum(int base, int width, CLink* link) {
  int spectrum_test=0;
  
  for (int i=base; i<base+width; i++) {
    spectrum_test+=link->LambdaStatus[i];
  }
  
  if (spectrum_test==width) return 1;
  else return 0;
}

void CNode::reserve_link_spectrum(int base, int width, CLink* link) {
  //Reserve resources in the link
  for (int i=base; i<base+width; i++) {
    link->LambdaStatus[i]=1;
  }
  link->FreeLambdas=link->FreeLambdas-width;
  
  return;
}

void CNode::release_link_spectrum(int base, int width, CLink* link) {
  //Reserve resources in the link
  for (int i=base; i<base+width; i++) {
    link->LambdaStatus[i]=0;
  }
  link->FreeLambdas=link->FreeLambdas+width;
  
  return;
}

void CNode::reserve_incoming_link_spectrum_db(int base, int width, CLink* link) {
  int j;
  
  j=this->SearchLocalLink(link,this->IncomingLink_dB);
  
  for (int i=base; i<base+width; i++) {
    if (this->IncomingLink_dB[j]->LocalLambdaStatus[i]==1)
      ErrorMsg("RSVP reserve_incoming local_dB dalay_panic");
    
    this->IncomingLink_dB[j]->LocalLambdaStatus[i]=1;
  }
  
  return;
}

void CNode::reserve_outgoing_link_spectrum_db(int base, int width, CLink* link) {
  int j;
  
  j=this->SearchLocalLink(link,this->OutgoingLink_dB);
  
  //cout << "reserve_outgoing_link_spectrum_db link: " << j << " spectrum: ";
  for (int i=base; i<base+width; i++) {
    if (this->OutgoingLink_dB[j]->LocalLambdaStatus[i]==1)
      ErrorMsg("RSVP reserve_outgoing local_dB dalay_panic");
    
    this->OutgoingLink_dB[j]->LocalLambdaStatus[i]=1;
    //cout << i << " ";
  }
  //cout << endl;
  
  return;
}

void CNode::release_incoming_link_spectrum_db(int base, int width, CLink* link) {
  int j;
  
  j=this->SearchLocalLink(link,this->IncomingLink_dB);
  
  for (int i=base; i<base+width; i++) {
    if (this->IncomingLink_dB[j]->LocalLambdaStatus[i]==0)
      ErrorMsg("RSVP release_incoming local_dB dalay_panic");
    
    this->IncomingLink_dB[j]->LocalLambdaStatus[i]=0;
  }
  
  return;
}

void CNode::release_outgoing_link_spectrum_db(int base, int width, CLink* link) {
  int j;
  
  j=this->SearchLocalLink(link,this->OutgoingLink_dB);
  
  //cout << "release_outgoing_link_spectrum_db link: " << j << " spectrum: ";
  for (int i=base; i<base+width; i++) {
    if (this->OutgoingLink_dB[j]->LocalLambdaStatus[i]==0)
      ErrorMsg("RSVP release_outgoing local_dB dalay_panic");
    
    this->OutgoingLink_dB[j]->LocalLambdaStatus[i]=0;
    //cout << i << " ";
  }
  //cout << endl;
  
  return;
}

void CNode::connection_memory_erasing(CConnection* conn) {
  int i, local_conn_found=0, global_conn_found=0;
  
    for (i=0; i<this->OriginatedConns.size(); i++) {
      if (this->OriginatedConns[i] == conn)
	local_conn_found=1;
    }
    if (local_conn_found==0)
      ErrorMsg("connection_memory_erasing --- Connection not found ---");
  
    /*Rimuove il puntatore alla connessione nel vector di connessioni attive al nodo sorgente*/
    vector<CConnection*>::iterator it_node=this->OriginatedConns.begin();
    while (conn!=*it_node) it_node++;
    this->OriginatedConns.erase(it_node);
    
   for (i=0; i<this->ParentNet->ActiveConns.size(); i++) {
      if (this->ParentNet->ActiveConns[i] == conn)
	global_conn_found=1;
    }
    if (global_conn_found==0)
      ErrorMsg("connection_memory_erasing --- Connection not found ---");
    
    /*Rimuove il puntatore alla connessione nel vector di connessioni attive in tutta la rete*/
    vector<CConnection*>::iterator it_net=this->ParentNet->ActiveConns.begin();
    while (conn!=*it_net) it_net++;
    this->ParentNet->ActiveConns.erase(it_net);
    
    return;
}

void CNode::Add_CPU_EVENT_SRC_RSA(RSVP_PathPck* pk) {
  
  this->NodeCPU_Buffer->Add(pk);
  
  #ifdef STDOUT
  cout << "CPU NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " queued source routing, TIME:" << this->ParentNet->Now << endl;
  #endif
      
  if (this->NodeCPU_Buffer->PckSize()==1) {
      
    //Place the next CPU Event
    CPU_Event* pc_eve = new CPU_Event(this, ParentNet->Now + ParentNet->distr_routing_time, CPU_EVENT_SRC_RSA);
    ParentNet->genEvent(pc_eve);
  }
  else {
      
    //Niente perché vuol dire che c'è già schedulato un CrossConnectionEvent, che quando arriverà si occuperà di piazzare il prossimo evento
  }
  return;
}
