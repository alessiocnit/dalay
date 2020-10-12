#include "PCx_Node.h"

#include "CNet.h"
#include "CDomain.h"
#include "CConnection.h"
#include "WritePckNodeEvent.h"
#include "CPathTable.h"
#include "DataPlaneBuffer.h"
#include "CrossConnectionEvent.h"

#include "PCEP_PCRepPck.h"
#include "PCEP_PCReqPck.h"
#include "PCEP_NotifyBlockedPck.h"
#include "PCEP_NotifyEstablishedPck.h"
#include "PCEP_NotifyReleasedPck.h"
#include "PCEP_NotifyLinkStatePck.h"
#include "PCEP_PCInitPck.h"
#include "PCEP_PCReportPck.h"

#include "OFP_FlowModPck.h"
#include "OFP_ErrorPck.h"
#include "OFP_LightpathPckIN.h"
#include "OFP_LightpathPckOUT.h"

#include "PathComputationSession.h"
#include "ReleaseConnEvent.h"

#include "FlowModeSession.h"
#include "InterDomainSession.h"
#include "CBuffer.h"
#include "CPU_Buffer.h"
#include "CPU_Event.h"

using namespace std;

PCx_Node::PCx_Node(int id, string name,CNet* parent_net):
        CNode::CNode(id,name,parent_net) {
    //cout << "Costruttore PCx_Node(int, string, CNet*) id: " << id << endl;

    this->Type=PCC;
}

PCx_Node::~PCx_Node() {
    //cout << "Distruttore PCx_Node" << endl;
}

//Used with ReActive PCE, and with ProActive PCE if suggested label is not used 
void PCx_Node::ReceivedPck_PCEP_NotifyEstablished(PCEP_NotifyEstablishedPck* pk) {
    int SendingSuccess;

    //TTL test
    if (TTL_expired(pk)) return;

    if (this==pk->DstNode) {
      #ifdef STDOUT_PCEP_NOTIFY
      cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " PCEP_NotifyEstablished packet... arrived at destination TIME: " << this->ParentNet->Now << endl;
      cout << "Path: ";
      for (int h=0;h<pk->ConnLinkPath.size();h++)
          cout << pk->ConnLinkPath[h]->FromNode->Id << " ";
      cout << pk->ConnLinkPath[pk->ConnLinkPath.size()-1]->ToNode->Id << endl;
      #endif
      
      if ((this->Type!=PCC_PCE) && (this->Type!=PCC_PCE_HPCE))
	ErrorMsg("NotifyEstablished PCEP_PCReq_PCC_PCE addressed to a not PCC node");

      if (this->ParentNet->TypeHPCE == PROACTIVE_HPCE) {
	//Release not used slots and reserve the used slots in case of inter-domain
	if (this->ParentNet->ProactivePCE==1) {
	  if (pk->SubType==NOTIFY_ESTABLISHED) {
	    #ifdef STDOUT_PCEP_NOTIFY
	    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " received PCEP-NotEst BGP database (different spectrum)" << endl;
	    #endif
	  
	    this->SNMP_release_path_resources(this->BGPLS_HPCE_LinkState_dB,pk->ConnLinkPath,pk->conn_pce_SuggestedLabel,pk->conn_width_slots,pk->ConnId);
	    this->SNMP_reserve_path_resources(this->BGPLS_HPCE_LinkState_dB,pk->ConnLinkPath,pk->conn_base_slot,pk->conn_width_slots,pk->ConnId);
	  }
	  //Reserve the used slots in case of intra-domain
	  if (pk->SubType==NOTIFY_ROUTED) {
	    #ifdef STDOUT_PCEP_NOTIFY
	    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " received PCEP-NotEst BGP database (routed at cPCE)" << endl;
	    #endif
	  
	    this->SNMP_reserve_path_resources(this->BGPLS_HPCE_LinkState_dB,pk->ConnLinkPath,pk->conn_base_slot,pk->conn_width_slots,pk->ConnId);
	  }
	}
	if (this->ParentNet->ProactivePCE==0) {
	  if (pk->Conn->Type==INTER_DOMAIN_CONN) {
	    #ifdef STDOUT_PCEP_NOTIFY
	    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " received PCEP-NotEst BGP database updating INTER-DOMAIN conn" << endl;
	    #endif
	  
	    this->SNMP_release_path_resources(this->BGPLS_HPCE_LinkState_dB,pk->ConnLinkPath,pk->conn_pce_SuggestedLabel,pk->conn_width_slots,pk->ConnId);
	    this->SNMP_reserve_path_resources(this->BGPLS_HPCE_LinkState_dB,pk->ConnLinkPath,pk->conn_base_slot,pk->conn_width_slots,pk->ConnId);
	  }
	  if (pk->Conn->Type==INTRA_DOMAIN_CONN) {
	    #ifdef STDOUT_PCEP_NOTIFY
	    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " received PCEP-NotEst BGP database updating INTRA-DOMAIN conn" << endl;
	    #endif
	  
	    this->SNMP_reserve_path_resources(this->BGPLS_HPCE_LinkState_dB,pk->ConnLinkPath,pk->conn_base_slot,pk->conn_width_slots,pk->ConnId);
	  }
	}
      }
      else {
	#ifdef STDOUT_PCEP_NOTIFY
	cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " SNMP database updating" << endl;
	#endif
	
	//If this is a ProActive PCE the NotifyEstablished is received if a different base slot is used
	//Therefore the first action is to cancel the proactive reservation 
	if (this->ParentNet->ProactivePCE)
	  this->SNMP_release_path_resources(this->SNMP_LinkState_dB,pk->ConnLinkPath,pk->conn_pce_SuggestedLabel,pk->conn_width_slots,pk->ConnId);
	
	//Reserve the used slots
	this->SNMP_reserve_path_resources(this->SNMP_LinkState_dB,pk->ConnLinkPath,pk->conn_base_slot,pk->conn_width_slots,pk->ConnId);
      }
      
      delete pk;
      return;
    }

    if (this==pk->SrcNode) {
	#ifdef STDOUT_PCEP_NOTIFY
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " created PCEP_NotifyEstablished packet...sent TIME: " << this->ParentNet->Now << endl;
	#endif

        SendingSuccess=SendPck(pk);
        if (!SendingSuccess) ErrorMsg("SendPck returned 0... dalay_panic");

        return;
    }

    #ifdef STDOUT_PCEP_NOTIFY
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " arrived PCEP_NotifyEstablished packet...sent TIME: " << this->ParentNet->Now << endl;
    #endif

    SendingSuccess=SendPck(pk);
    if (!SendingSuccess) ErrorMsg("SendPck returned 0... dalay_panic");

    return;
}

//Used only with ProActive PCE
void PCx_Node::ReceivedPck_PCEP_NotifyBlocked(PCEP_NotifyBlockedPck* pk) {
    int SendingSuccess;
    int width_slots, blocked_base_slot;
    vector<CLink*> UsedLinks;

    //TTL test
    if (TTL_expired(pk)) return;

    if (this==pk->DstNode) {
      #ifdef STDOUT_PCEP_NOTIFY
      cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " PCEP_NotifyBlocked packet...arrived at destination TIME: " << this->ParentNet->Now << endl;
      #endif
      
      if ((this->Type!=PCC_PCE) && (this->Type!=PCC_PCE_HPCE))
	ErrorMsg("PCEP_NotifyBlocked PCEP_PCReq_PCC_PCE addressed to a not PCC node");

      if (this->ParentNet->TypeHPCE == PROACTIVE_HPCE) {
	this->SNMP_release_path_resources(this->BGPLS_HPCE_LinkState_dB,pk->ConnLinkPath,pk->conn_pce_SuggestedLabel,pk->conn_width_slots,pk->ConnId);
      }
      else {
	this->SNMP_release_path_resources(this->SNMP_LinkState_dB,pk->ConnLinkPath,pk->conn_pce_SuggestedLabel,pk->conn_width_slots,pk->ConnId);
      }
      
      delete pk;
      return;
    }

    if (this==pk->SrcNode) {
	#ifdef STDOUT_PCEP_NOTIFY
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " created PCEP_NotifyBlocked packet...sent TIME: " << this->ParentNet->Now << endl;
	#endif

        SendingSuccess=SendPck(pk);
        if (!SendingSuccess) ErrorMsg("SendPck returned 0... dalay_panic");

        return;
    }

    #ifdef STDOUT_PCEP_NOTIFY
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " arrived PCEP_NotifyBlocked packet...sent TIME: " << this->ParentNet->Now << endl;
    #endif

    SendingSuccess=SendPck(pk);
    if (!SendingSuccess) ErrorMsg("SendPck returned 0... dalay_panic");

    return;
}

//Used for both ReActive and ProActive PCE
void PCx_Node::ReceivedPck_PCEP_NotifyReleased(PCEP_NotifyReleasedPck* pk) {
    int SendingSuccess;
    int width_slots, base_slot;
    vector<CLink*> UsedLinks;

    //TTL test
    if (TTL_expired(pk)) return;

    if (this==pk->DstNode) {
        if ((this->Type!=PCC_PCE) && (this->Type!=PCC_PCE_HPCE)) {
	    cout << "NODE: " << this->Id << " (" << this->Name << ")" << endl;
            ErrorMsg("PCEP_NotifyReleased addressed to a not PCE node");
	}

        if (pk->ConnLinkPath.size()==0)
            ErrorMsg("ReceivedPck_PCEP_NotifyReleased::No links in the Path... dalay_panic");

	if (this->ParentNet->TypeHPCE == PROACTIVE_HPCE) {
	  #ifdef STDOUT_PCEP_NOTIFY
	  cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " PCEP_NotifyReleased packet...arrived at destination TIME: " << this->ParentNet->Now << endl;
	  cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " BGP database updating" << endl;
	  #endif
	
	  this->SNMP_release_path_resources(this->BGPLS_HPCE_LinkState_dB,pk->ConnLinkPath,pk->conn_base_slot,pk->conn_width_slots,pk->ConnId);
	}
	else {
	  #ifdef STDOUT_PCEP_NOTIFY
	  cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " PCEP_NotifyReleased packet...arrived at destination TIME: " << this->ParentNet->Now << endl;
	  cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " SNMP database updating" << endl;
	  #endif
	
	  this->SNMP_release_path_resources(this->SNMP_LinkState_dB,pk->ConnLinkPath,pk->conn_base_slot,pk->conn_width_slots,pk->ConnId);
	}

	delete pk;
        return;
    }

    if (this==pk->SrcNode) {
	#ifdef STDOUT_PCEP_NOTIFY
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " created PCEP_NotifyReleased packet...sent TIME: " << this->ParentNet->Now << endl;
	#endif

        SendingSuccess=SendPck(pk);
        if (!SendingSuccess) ErrorMsg("SendPck returned 0... dalay_panic");

        return;
    }

    #ifdef STDOUT_PCEP_NOTIFY
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " arrived PCEP_NotifyReleased packet...sent TIME: " << this->ParentNet->Now << endl;
    #endif

    SendingSuccess=SendPck(pk);
    if (!SendingSuccess) ErrorMsg("SendPck returned 0... dalay_panic");

    return;
}

void PCx_Node::ReceivedPck_PCEP_NotifyLinkState(PCEP_NotifyLinkStatePck* pk) {
  int TableEntry;
  CLink* UsedLink=pk->Link_Flooded;
  
    //TTL test
    if (TTL_expired(pk)) return;

    //At Destination node
    if (this==pk->DstNode) {
      if (this->Type != PCC_PCE_HPCE)
	ErrorMsg("PCEP_NotifyLinkState not addressed to the HPCE");
      
      #ifdef STDOUT_PCEP_NOTIFY
      cout << "NODE: " << Id << " (" << Name << "): arrived PCEP_NotifyLinkState packet... destination TIME: " << this->ParentNet->Now << endl;
      cout << "NODE: " << Id << " (" << Name << "): arrived PCEP_NotifyLinkState for link " << pk->Link_Flooded->FromNode->Id << "->" << pk->Link_Flooded->ToNode->Id << " TIME: " << this->ParentNet->Now << endl;
      #endif
      
      if (this->ParentNet->TypeHPCE == STANDARD_HPCE) {
	//Search the link_state in the database
        TableEntry=0;
        while ((TableEntry<this->SNMP_HPCE_InterDomain_LinkState_dB.size()) && (this->SNMP_HPCE_InterDomain_LinkState_dB[TableEntry]->Link!=UsedLink))
	  TableEntry++;
      
        if (TableEntry==this->SNMP_HPCE_InterDomain_LinkState_dB.size())
	  ErrorMsg("PCEP_NotifyLinkState link state not found at the HPCE");
      
        //Update important fields in the database
        this->SNMP_HPCE_InterDomain_LinkState_dB[TableEntry]->InstallationTime=this->ParentNet->Now;
        this->SNMP_HPCE_InterDomain_LinkState_dB[TableEntry]->LambdaStatus=pk->LambdaStatus;
        this->SNMP_HPCE_InterDomain_LinkState_dB[TableEntry]->FreeLambdas=pk->free_lambdas;
      }
      
      if (this->ParentNet->TypeHPCE == BGP_HPCE) {
        //Search the link_state in the database
        TableEntry=0;
        while ((TableEntry<this->BGPLS_HPCE_LinkState_dB.size()) && (this->BGPLS_HPCE_LinkState_dB[TableEntry]->Link!=UsedLink))
	  TableEntry++;
      
        if (TableEntry==this->BGPLS_HPCE_LinkState_dB.size())
	  ErrorMsg("PCEP_NotifyLinkState link state not found at the HPCE");
      
        //Update important fields in the database
        this->BGPLS_HPCE_LinkState_dB[TableEntry]->InstallationTime=this->ParentNet->Now;
        this->BGPLS_HPCE_LinkState_dB[TableEntry]->LambdaStatus=pk->LambdaStatus;
        this->BGPLS_HPCE_LinkState_dB[TableEntry]->FreeLambdas=pk->free_lambdas;
      }
      
      delete pk;
      return;
    }

    //At Source node
    if (this==pk->SrcNode) {
	#ifdef STDOUT_PCEP_NOTIFY
        cout << "NODE: " << Id << " (" << Name << "): created PCEP_NotifyLinkState packet... sent TIME: " << this->ParentNet->Now << endl;
	#endif

        SendPck(pk);
        return;
    }

    //At Intermediate nodes
    #ifdef STDOUT_PCEP_NOTIFY
    cout << "NODE: " << Id << " (" << Name << "): arrived PCEP_NotifyLinkState packet... sent TIME: " << this->ParentNet->Now << endl;
    #endif

    SendPck(pk);
    return;
}

void PCx_Node::ReceivedPck_PCEP_PCReq(PCEP_PCReqPck* pk) {
    //TTL test
    if (TTL_expired(pk)) return;

    //At Destination node
    if (this==pk->DstNode) {
      
	//The specific processing function is used depending on the PCEP_Req subtype
        switch (pk->ReqType) {
	  case Req_PCC_PCE:    
	    this->ReceivedPck_PCEP_PCReq_PCC_PCE(pk);
            break;
	  case Req_PCE_HPCE:    
            this->ReceivedPck_PCEP_PCReq_PCE_HPCE(pk);
            break;
	  case Req_HPCE_PCE: 
            this->ReceivedPck_PCEP_PCReq_HPCE_PCE(pk);
            break ;
	  case Req_PCE_PCE:
	    this->ReceivedPck_PCEP_PCReq_PCE_PCE(pk);
            break ;
        }

        delete pk;
        return;
    }

    //At Source node
    if (this==pk->SrcNode) {
	#ifdef STDOUT_PCEP_REQREP
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " created PCEP_Request packet...sent TIME: " << this->ParentNet->Now << endl;
	#endif

        SendPck(pk);
        return;
    }

    //At Intermediate nodes
    #ifdef STDOUT_PCEP_REQREP
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " arrived PCEP_Request packet...sent TIME: " << this->ParentNet->Now << endl;
    #endif

    SendPck(pk);
    return;
}

void PCx_Node::ReceivedPck_PCEP_PCRep(PCEP_PCRepPck* pk) {
    //TTL test
    if (TTL_expired(pk)) return;

    //At Destination node
    if (this==pk->DstNode) {
    
	//The specific processing function is used depending on the PCEP_Rep subtype
        switch (pk->RepType) {
        case Rep_PCE_PCC:
            this->ReceivedPck_PCEP_PCRep_PCE_PCC(pk);
            break;
        case Rep_HPCE_PCE:
            this->ReceivedPck_PCEP_PCRep_HPCE_PCE(pk);
            break;
        case Rep_PCE_HPCE:
            this->ReceivedPck_PCEP_PCRep_PCE_HPCE(pk);
            break ;
	case Rep_PCE_PCE:
            this->ReceivedPck_PCEP_PCRep_PCE_PCE(pk);
            break ;
        }

        delete pk;
        return;
    }

    //At Source node
    if (this==pk->SrcNode) {
	#ifdef STDOUT_PCEP_REQREP
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " created PCEP_Reply packet...sent TIME: " << this->ParentNet->Now << endl;
	#endif

        SendPck(pk);
        return;
    }

    //At Intermediate nodes
#ifdef STDOUT_PCEP_REQREP
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " arrived PCEP_Reply packet...sent TIME: " << this->ParentNet->Now << endl;
#endif

    SendPck(pk);
    return;
}

//The packet is at destination
void PCx_Node::ReceivedPck_PCEP_PCReq_PCC_PCE(PCEP_PCReqPck* pk) {
    int peso;
    int lambdas=this->ParentNet->W;
    int SuggestedLabel=-1;

    vector<CNode*> n=pk->Conn->NodePath;
    vector<CLink*> l=pk->Conn->LinkPath;
    int FreeLambdas=0;
    vector<int> LabelSet;
    vector<CNode*> e;
    vector<CDomain*> d;

    if (this->Type==PCC) ErrorMsg("PCEP_PCReq_PCC_PCE addressed to a PCC node");

    #ifdef STDOUT_PCEP_REQREP
    cout << "PCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " PCEP_Request [PCC_PCE] packet... arrived at destination TIME: " << this->ParentNet->Now << endl;
    #endif

    //If the destination is outside this domain forward the request to the HPCE
    if ((this->ParentNet->TypeHPCE != NO_HPCE) && (pk->Conn->Type==INTER_DOMAIN_CONN) && (pk->Conn->EdgeNodePath.size()==0) && (pk->Conn->DomainPath.size()==0)) {
	#ifdef STDOUT_PCEP_REQREP
        cout << "PCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " PCEP_Request [PCE_HPCE] packet... forwarded to HPCE: " << this->ParentNet->Now << endl;
	#endif

        PCEP_PCReqPck* P_pk=new PCEP_PCReqPck(this,this->ParentNet->HPCE_Node,128,128,this->ParentNet->Now,pk->Conn,pk->RestorationFlag,Req_PCE_HPCE);
        WritePckNodeEvent* P_eve=new WritePckNodeEvent(this,P_pk,this->ParentNet->Now);
        this->ParentNet->genEvent(P_eve);

        return;
    }

    //Last hop in ERO is removed beacuse it will be included as source of the computed path in this domain
    if (n.size()) n.pop_back();

    //This part is executed only in multi-domain
    if ((pk->Conn->EdgeNodePath.size()!=0) || (pk->Conn->DomainPath.size()!=0)) {
	#ifdef STDOUT_PCEP_REQREP
        cout << "PCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " PCEP_Req expanding the path computed at HPCE" << endl;
	#endif

        peso=this->PCE_ExpandEdgeDomainSequence(pk,&n,&l);

	#ifdef STDOUT_PCEP_REQREP
        cout << "PCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " Path computed at PCE: ";
        for (int i=0;i<n.size();i++)
            cout << n[i]->Id << " ";
        cout << " TIME: " << this->ParentNet->Now << endl;
	#endif

        if (peso==0) {
            //Replies with a NO_PATH PCRep message
            PCEP_PCRepPck* P_pk=new PCEP_PCRepPck(this,pk->SrcNode,128,128,this->ParentNet->Now,pk->Conn,NO_PATH,pk->RestorationFlag,Rep_PCE_PCC);
            WritePckNodeEvent* P_eve=new WritePckNodeEvent(this,P_pk,this->ParentNet->Now);
            this->ParentNet->genEvent(P_eve);

            return;
        }

        //Replies with a PCRep message
        PCEP_PCRepPck* P_pk=new PCEP_PCRepPck(this,pk->SrcNode,128,128,this->ParentNet->Now,pk->Conn,n,l,e,d,SuggestedLabel,YES_PATH,pk->RestorationFlag,Rep_PCE_PCC);
        WritePckNodeEvent* P_eve=new WritePckNodeEvent(this,P_pk,this->ParentNet->Now);
        this->ParentNet->genEvent(P_eve);

        return;
    }
    
    //Routing using the proper scheme
    if (this->ParentNet->PCEP_NotifyMessages) {
        peso=this->ParentDomain->PathTable->Flexible_PCE_SNMP_DetInfo_LeastCongestedPath(pk->ConnSource, pk->ConnDestination, &n, &l, pk->Conn);
    }
    else {
        switch (this->ParentNet->RoutingMode_PCE) {
        case OFF:
            peso=this->ParentDomain->PathTable->PCE_NoInfo_RandomPath(pk->SrcNode, pk->ConnDestination, &n, &l, pk->Conn);
            break;
        case AGGREGATED:
            peso=this->ParentDomain->PathTable->PCE_AggInfo_LeastCongestedPath(pk->SrcNode, pk->ConnDestination, &n, &l, &FreeLambdas, pk->Conn);
            break;
        case DETAILED:
            peso=this->ParentDomain->PathTable->Flexible_PCE_DetInfo_LeastCongestedPath(pk->SrcNode, pk->ConnDestination, &n, &l, &FreeLambdas, &LabelSet, pk->Conn);
            break;
        }
    }
    //Routing failed: PCRep containing the NO_PATH information
    if (peso==0) {
        PCEP_PCRepPck* P_pk=new PCEP_PCRepPck(this,pk->SrcNode,128,128,this->ParentNet->Now,pk->Conn,NO_PATH,pk->RestorationFlag,Rep_PCE_PCC);
        this->Add_CPU_EVENT_PCE_RSA(P_pk);
	
        return;
    }
    //Wavelength Assignment on the computed path using the proper scheme
    if (this->pce_wa_enabled) {
        if (this->ParentNet->PCEP_NotifyMessages) {
            switch (this->ParentNet->Provisioning_DefaultPolicy) {
            case FF:
                SuggestedLabel=this->PathTable->Flexible_PCE_SNMP_WavelengthAssignmentFF(l,pk->Conn_width_slots,pk->Conn->Id,pk->RestorationFlag,pk->Conn);
                break;
            default:
                ErrorMsg("PCE WA not supported");
            }
            if (SuggestedLabel==-1)
                ErrorMsg("No lambdas available on the LeastCongestedPath using SNMP database PCC_PCE");
        }
        else {
            switch (this->ParentNet->RoutingMode_PCE) {
            case OFF:
                SuggestedLabel=-1;
                break;
            case AGGREGATED:
                SuggestedLabel=-1;
                break;
            case DETAILED: {
                switch (this->ParentNet->Provisioning_DefaultPolicy) {
                case FF:
                    SuggestedLabel=this->PathTable->Flexible_PCE_WavelengthAssignmentFF(l,pk->Conn_width_slots,pk->Conn->Id);
                    break;
                default:
                    ErrorMsg("PCE WA not supported");
                }
                if (SuggestedLabel==-1)
                    ErrorMsg("No lambdas available on the LeastCongestedPath using OSPF-TE database - 1");
                break;
            }
            }
        }
    }
    //Update the SNMP database considering the connection as established
    if (this->ParentNet->ProactivePCE) {
	#ifdef STDOUT_PCEP_REQREP
        cout << "PCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " path computed updating SNMP database TIME: " << this->ParentNet->Now << endl;
        cout << "PCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " along path: ";
        for (int i=0; i<n.size(); i++) {
            cout << n[i]->Id << " ";
        }
        cout << "with suggested label " << SuggestedLabel << endl;
	#endif
	
	if (this->ParentNet->TypeHPCE==PROACTIVE_HPCE) {
	  PCEP_NotifyEstablishedPck* n_e_pk=new PCEP_NotifyEstablishedPck(this,this->ParentNet->HPCE_Node,128,128,this->ParentNet->Now,pk->Conn,pk->RestorationFlag);
          n_e_pk->SubType=NOTIFY_ROUTED;
	  n_e_pk->conn_width_slots=pk->Conn_width_slots;
	  n_e_pk->conn_pce_SuggestedLabel=SuggestedLabel;
	  n_e_pk->ConnLinkPath=l;
	  
	  WritePckNodeEvent* n_e_eve=new WritePckNodeEvent(n_e_pk->SrcNode,n_e_pk,this->ParentNet->Now);
          this->ParentNet->genEvent(n_e_eve);
	}

        //Updating the SNMP database in the PCE
        this->SNMP_reserve_path_resources(this->SNMP_LinkState_dB,l,SuggestedLabel,pk->Conn_width_slots,pk->Conn->Id);
    }
    
    #ifdef STDOUT_PCEP_REQREP
    cout << "PCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " Path computed at PCE: ";
    for (int i=0;i<n.size();i++)
        cout << n[i]->Id << " ";
    cout << " FreeLambdas: " << FreeLambdas << " TIME: " << this->ParentNet->Now << endl;
    #endif

    //Replies with a PCRep message
    PCEP_PCRepPck* P_pk=new PCEP_PCRepPck(this,pk->SrcNode,128,128,this->ParentNet->Now,pk->Conn,n,l,e,d,SuggestedLabel,YES_PATH,pk->RestorationFlag,Rep_PCE_PCC);
    this->Add_CPU_EVENT_PCE_RSA(P_pk);
      
    return;
};

void PCx_Node::ReceivedPck_PCEP_PCReq_PCE_HPCE(PCEP_PCReqPck* pk) {
    int SuggestedLabel=-1;
    int peso;
    vector<CNode*> n;
    vector<CLink*> l;
    vector<CNode*> e;
    vector<CDomain*> d;

    if (this->Type!=PCC_PCE_HPCE) ErrorMsg("PCEP_PCReq_PCE_HPCE addressed to a not HPCE node");

    #ifdef STDOUT_PCEP_REQREP
    cout << "HPCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " received PCEP_Request packet...sent TIME: " << this->ParentNet->Now << endl;
    #endif
    
    //If HPCE_DistributedComputing is active HPCE start the session, otherwise it locally performs path computation.
    if ((this->ParentNet->TypeHPCE == STANDARD_HPCE) && (this->ParentNet->HPCE_DistributedComputing)) {
      PathComputationSession* session = new PathComputationSession(pk,pk->Conn,this->ParentNet);
      
      //TODO this database is not yet managed
      //this->DistributedComputationSession_dB.push_back(session);
      
      session->start();
      
      return;
    }

    if (this->ParentNet->TypeHPCE == STANDARD_HPCE) {
      switch (this->ParentNet->RoutingMode_HPCE) {
    	case OFF: 
	  peso=this->ParentNet->HPCE_PathTable->HPCE_NoInfo_EdgeSequence(pk->ConnSource, pk->ConnDestination, &e, &l, pk->Conn);
	  break;
	case AGGREGATED:
	  peso=this->ParentNet->HPCE_PathTable->HPCE_AggInfo_EdgeSequence(pk->ConnSource, pk->ConnDestination, &e, &l, pk->Conn);
	  break;
    	case DETAILED:
	  peso=this->ParentNet->HPCE_PathTable->HPCE_DetInfo_EdgeSequence(pk->ConnSource, pk->ConnDestination, &e, &l, pk->Conn);
	  break;
      }
    }

    if ((this->ParentNet->TypeHPCE == BGP_HPCE) || (this->ParentNet->TypeHPCE == PROACTIVE_HPCE)) {
      peso=this->ParentNet->PathTable->Flexible_HPCE_BGP_DetInfo_LeastCongestedPath(pk->ConnSource, pk->ConnDestination, &n, &l, pk->Conn);
    }
    
    //Routing failed: PCRep containing the NO_PATH information
    if (peso==0) {
	#ifdef STDOUT_PCEP_REQREP
	cout << "HPCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " NO PATH...sent TIME: " << this->ParentNet->Now << endl;
	#endif
      
        PCEP_PCRepPck* P_pk=new PCEP_PCRepPck(this,pk->SrcNode,128,128,this->ParentNet->Now,pk->Conn,NO_PATH,pk->RestorationFlag,Rep_HPCE_PCE);
        this->Add_CPU_EVENT_PCE_RSA(P_pk);
	
	//WritePckNodeEvent* P_eve=new WritePckNodeEvent(this,P_pk,this->ParentNet->Now);
        //this->ParentNet->genEvent(P_eve);

        return;
    }
    
    //Wavelength Assignment on the computed path using the proper scheme
    if (this->hpce_wa_enabled) {
      
      if (this->ParentNet->TypeHPCE == STANDARD_HPCE) {
	ErrorMsg("WA not supported with STANDARD_HPCE and without HPCE_DistributedComputing... please set hpce_wa_enabled=0");
      }
      
      if ((this->ParentNet->TypeHPCE == BGP_HPCE) || (this->ParentNet->TypeHPCE == PROACTIVE_HPCE)) {
	switch (this->ParentNet->RoutingMode_HPCE) {
          case OFF:
          case AGGREGATED:
	    SuggestedLabel=-1;
	    break;
          case DETAILED:
	    if (this->ParentNet->Provisioning_DefaultPolicy==FF) {
	      SuggestedLabel=this->ParentNet->PathTable->Flexible_HPCE_WavelengthAssignmentFF(l,pk->Conn_width_slots,pk->Conn->Id,pk->RestorationFlag,pk->Conn);
	    }
	    else
	      ErrorMsg("WA type not supported ant the pPCE");
	    
	    if (SuggestedLabel==-1)
	      ErrorMsg("No lambdas available on the LeastCongestedPath using OSPF-TE database - 2");
	   
	    break;
	  default:
	    ErrorMsg("RoutingMode_HPCE not supported");
	}
      }
    }

    #ifdef STDOUT_PCEP_REQREP
    cout << "HPCE PCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " Node sequence computed at HPCE: ";
    for (int i=0;i<n.size();i++)
        cout << n[i]->Id << " ";
    cout << endl;
    cout << "HPCE PCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " Edge sequence computed at HPCE: ";
    for (int i=0;i<e.size();i++)
        cout << e[i]->Id << " ";
    cout << endl;
    cout << "HPCE PCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " Link sequence computed at HPCE: ";
    for (int i=0;i<l.size();i++) {
        if (l[i]!=NULL) cout << l[i]->Id << " ";
        else cout << "NULL ";
    }
    cout << endl;
    cout << "HPCE PCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " Suggested Label: " << SuggestedLabel << endl;
    #endif

    //Compute the domain sequence
    if (this->ParentNet->HPCE_SimulationMode==DOMAIN_SEQUENCE) {
        this->HPCE_EdgeToDomain_SequenceConversion(e,&d);
        e.clear();

	#ifdef STDOUT_PCEP_REQREP
        cout << "HPCE PCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " Domain sequence computed at HPCE: ";
        for (int i=0;i<d.size();i++)
            cout << d[i]->Name << " ";
        cout << endl;
	#endif
    }
    
    //Reserve the used slots 
    if (this->ParentNet->TypeHPCE == PROACTIVE_HPCE) {
      #ifdef STDOUT_PCEP_REQREP
      cout << "HPCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " reserving allocated resources" << endl;
      #endif
	
      this->SNMP_reserve_path_resources(BGPLS_HPCE_LinkState_dB,l,SuggestedLabel,pk->Conn->width_slots,pk->Conn->Id);
    }

    //Replies with a PCRep message
    PCEP_PCRepPck* P_pk=new PCEP_PCRepPck(this,pk->SrcNode,128,128,this->ParentNet->Now,pk->Conn,n,l,e,d,SuggestedLabel,YES_PATH,pk->RestorationFlag,Rep_HPCE_PCE);
    this->Add_CPU_EVENT_PCE_RSA(P_pk);
    
    //WritePckNodeEvent* P_eve=new WritePckNodeEvent(this,P_pk,this->ParentNet->Now);
    //this->ParentNet->genEvent(P_eve);

    return;
}

void PCx_Node::ReceivedPck_PCEP_PCReq_HPCE_PCE(PCEP_PCReqPck* pk) {
    int peso;
    int SuggestedLabel=-1;

    int FreeLambdas=-1;
    vector<int> LabelSet;
    
    vector<CNode*> n_spazza=pk->Conn->NodePath;
    vector<CLink*> l_spazza=pk->Conn->LinkPath;
    
    vector<CNode*> n=pk->Conn->NodePath;
    vector<CLink*> l=pk->Conn->LinkPath;
    
    vector<CNode*> e;
    vector<CDomain*> d;

    if (this->Type==PCC) ErrorMsg("PCEP_PCReq_HPCE_PCE addressed to a PCC node");

    #ifdef STDOUT_PCEP_REQREP
    cout << "PCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " PCEP_Request [HPCE_PCE " << pk->FromNode->Id << "->" << pk->ToNode->Id << "] packet... arrived at destination TIME: " << this->ParentNet->Now << endl;
    #endif

    //FromNode and ToNode must be in this domain
    if ((pk->FromNode->ParentDomain!=this->ParentDomain) || (pk->ToNode->ParentDomain!=this->ParentDomain))
      ErrorMsg("PCEP_Request [HPCE_PCE] addressed to a wrong PCE");
    
    //Routing using the proper scheme
    switch (this->ParentNet->RoutingMode_PCE) {
        case OFF:
            peso=this->ParentDomain->PathTable->PCE_NoInfo_RandomPath(pk->FromNode,pk->ToNode,&n,&l,pk->Conn);
            break;
        case AGGREGATED:
            peso=this->ParentDomain->PathTable->PCE_AggInfo_LeastCongestedPath(pk->FromNode,pk->ToNode,&n,&l,&FreeLambdas,pk->Conn);
            break;
        case DETAILED:
            peso=this->ParentDomain->PathTable->Flexible_PCE_DetInfo_LeastCongestedPath(pk->FromNode,pk->ToNode,&n,&l,&FreeLambdas,&LabelSet,pk->Conn);
	    break;
    }

    //Routing failed: PCRep containing the NO_PATH information
    if (peso==0) {
        PCEP_PCRepPck* rep_pk=new PCEP_PCRepPck(this,pk->SrcNode,128,128,this->ParentNet->Now,pk->Conn,NO_PATH,pk->RestorationFlag,Rep_PCE_HPCE);
	rep_pk->ComputationSession=pk->ComputationSession;
	rep_pk->FromNode=pk->FromNode;
	rep_pk->ToNode=pk->ToNode;
	rep_pk->FreeLambdas=FreeLambdas;
	
	this->Add_CPU_EVENT_PCE_RSA(rep_pk);
	
	//WritePckNodeEvent* eve=new WritePckNodeEvent(this,rep_pk,this->ParentNet->Now);
        //this->ParentNet->genEvent(eve);

        return;
    }

    #ifdef STDOUT_PCEP_REQREP
    cout << "PCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " Path computed at PCE: ";
    for (int i=0;i<n.size();i++)
        cout << n[i]->Id << " ";
    cout << " TIME: " << this->ParentNet->Now << endl;
    #endif
    
    //Replies with a PCRep message
    PCEP_PCRepPck* rep_pk=new PCEP_PCRepPck(this,pk->SrcNode,128,128,this->ParentNet->Now,pk->Conn,n,l,e,d,SuggestedLabel,YES_PATH,pk->RestorationFlag,Rep_PCE_HPCE);
    rep_pk->ComputationSession=pk->ComputationSession;
    rep_pk->FromNode=pk->FromNode;
    rep_pk->ToNode=pk->ToNode;
    rep_pk->LabelSet=LabelSet;
    rep_pk->FreeLambdas=FreeLambdas;
    
    this->Add_CPU_EVENT_PCE_RSA(rep_pk);
    
    //WritePckNodeEvent* eve=new WritePckNodeEvent(this,rep_pk,this->ParentNet->Now);
    //this->ParentNet->genEvent(eve);

    return;
}

void PCx_Node::ReceivedPck_PCEP_PCReq_PCE_PCE(PCEP_PCReqPck* pk) {
}

void PCx_Node::ReceivedPck_PCEP_PCRep_PCE_PCC(PCEP_PCRepPck* pk) {
    int i;
    CNet* net=this->ParentNet;

    #ifdef STDOUT_PCEP_REQREP
    cout << "PCC NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " PCEP_Reply packet... arrived at destination TIME: " << this->ParentNet->Now << endl;
    #endif

    if (pk->RestorationFlag) {
        if (pk->NoPath) {
	    #ifdef STDOUT_PCEP_REQREP
            cout << "PCC NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " REFUSED_RESTORATION (No Path from PCE) " << endl;
	    cout << "PCC NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " -> connection blocked restoration (No Path from PCE) " << endl;
	    #endif

            this->ParentNet->ConnRefused_Restoration++; this->ParentNet->ConnRefused_RestorationREP++;
            this->ParentNet->ConnRefusedRouting_Restoration++; this->ParentNet->ConnRefusedRouting_RestorationREP++;

	    #ifdef STDOUT
	    cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << pk->Conn->Id << " CONNECTION MEMORY (erasing) size: " << this->ParentNet->ActiveConns.size() << " TIME: " << this->ParentNet->Now<< endl;
	    #endif

	    this->connection_memory_erasing(pk->Conn);

            pk->Conn->Status=REFUSED_RESTORATION;
	    
	    //Connection will be deleted by the original Release event
            return;
        }
        
	#ifdef STDOUT_PCEP_REQREP
        cout << "PCC NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " ADMITTED_RESTORATION for signaling attempt: " << pk->Conn->RestorationAttempt << endl;
	#endif

        this->ParentNet->ConnAdmittedPath_Restoration++;
        this->ParentNet->ConnAdmittedPath_RestorationREP++;

        pk->Conn->Status=ADMITTED_PATH_RESTORATION;
    }
    else {
        if (pk->NoPath) {
            if (this!=pk->Conn->SourceNode) {
                this->GeneratePathErr_EdgeNode(pk->Conn,this->ParentNet->Now);

                return;
            }

	    #ifdef STDOUT_PCEP_REQREP
            cout << "PCC NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " -> connection blocked (No Path from PCE) " << endl;
	    #endif
	    
	    int attempt = pk->Conn->ProvisioningAttempt-1;

            pk->Conn->Status=REFUSED;
	    
	    net->ConnRequested++; net->ConnRequestedREP++;

	    net->ConnRefused++; net->ConnRefusedREP++;
	    
	    net->ConnBlocked++; net->ConnBlockedREP++;
	    net->ConnBlockedRouting++; net->ConnBlockedRoutingREP++;
	    
	    net->ConnRequestedATT[attempt]++; net->ConnRequestedATT_REP[attempt]++;
	    net->ConnBlockedATT[attempt]++; net->ConnBlockedATT_REP[attempt]++;
	    net->ConnBlockedRoutingATT[attempt]++; net->ConnBlockedRoutingATT_REP[attempt]++;
	    
	    delete pk->Conn;
	    return;
        }

	#ifdef STDOUT_PCEP_REQREP
        cout << "PCC NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " type: " << pk->Conn->Type << " ADMITTED for signaling attempt: " << pk->Conn->ProvisioningAttempt << endl;
	#endif

        pk->Conn->Status=ADMITTED_PATH;

        this->ParentNet->ConnAdmittedPath++;
        this->ParentNet->ConnAdmittedPathREP++;
    }

    //The paths of the connection are cleared
    pk->Conn->NodePath.clear();
    pk->Conn->LinkPath.clear();

    /*Connection Admitted along the selected path - Writing NodePath and LinkPath in the CConnection*/
    pk->Conn->NodePath=pk->NodePath;
    pk->Conn->LinkPath=pk->LinkPath;

    if (pk->EdgeNodePath.size())
      pk->Conn->EdgeNodePath=pk->EdgeNodePath;
    if (pk->DomainPath.size())
      pk->Conn->DomainPath=pk->DomainPath;

    #ifdef STDOUT_PCEP_REQREP
    cout << "PCC NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " admitted along path: ";
    for (int j=0; j<pk->Conn->NodePath.size(); j++) {
        cout << pk->Conn->NodePath[j]->Id << " ";
    }
    cout << endl;
    #endif

    if (pk->SuggestedLabel_ON) {
      #ifdef STDOUT_PCEP_REQREP
      cout << "PCC NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " SUGGESTED LABEL ON" << endl;
      #endif
      
      pk->Conn->PCE_SuggestedLabel_ON=1;
      pk->Conn->PCE_SuggestedLabel=pk->SuggestedLabel;
    }
    else {
      #ifdef STDOUT_PCEP_REQREP
      cout << "PCC NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " SUGGESTED LABEL OFF" << endl;
      #endif
      
      pk->Conn->PCE_SuggestedLabel_ON=0;
      pk->Conn->PCE_SuggestedLabel=-1;
    }

    if (this==pk->Conn->SourceNode) //This is the connection source node
        this->ConnEstablish(pk->Conn,this->ParentNet->Now);
    else //This is a domain edge node
        this->ConnEstablish_IntermediateDomain(pk->Conn,this->ParentNet->Now);

    return;
}

void PCx_Node::ReceivedPck_PCEP_PCRep_HPCE_PCE(PCEP_PCRepPck* pk) {
    int peso;
    int SuggestedLabel=-1;
    vector<CNode*> e=pk->EdgeNodePath;
    vector<CDomain*> d=pk->DomainPath;
    vector<CNode*> n;
    vector<CLink*> l;

    if (this->Type==PCC) ErrorMsg("PCEP_PCRep_HPCE_PCE addressed to a PCC node");

    #ifdef STDOUT_PCEP_REQREP
    cout << "PCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " PCEP_Reply packet... arrived at destination TIME: " << this->ParentNet->Now << endl;
    #endif
    
    //Forward the reply packet to the PCC
    //TODO This is because up to now if HPCE_DistributedComputing is used, the HPCE provides the node_sequence
    if ((this->ParentNet->TypeHPCE==BGP_HPCE) || (this->ParentNet->TypeHPCE==PROACTIVE_HPCE) || ((this->ParentNet->TypeHPCE==STANDARD_HPCE) && (this->ParentNet->HPCE_DistributedComputing)) ) {
	#ifdef STDOUT_PCEP_REQREP
        cout << "PCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " PCEP_Reply packet...... forwarded to PCC TIME: " << this->ParentNet->Now << endl;
	#endif
	
	if (pk->NoPath==YES_PATH) {
	  PCEP_PCRepPck* rep_pk=new PCEP_PCRepPck(this,pk->Conn->SourceNode,128,128,this->ParentNet->Now,pk->Conn,pk->NodePath,pk->LinkPath,pk->EdgeNodePath,pk->DomainPath,pk->SuggestedLabel,pk->NoPath,pk->RestorationFlag,Rep_PCE_PCC);
	  WritePckNodeEvent* rep_eve=new WritePckNodeEvent(this,rep_pk,this->ParentNet->Now);
	  this->ParentNet->genEvent(rep_eve);
	}
	else {
	  PCEP_PCRepPck* rep_pk=new PCEP_PCRepPck(this,pk->Conn->SourceNode,128,128,this->ParentNet->Now,pk->Conn,pk->NoPath,pk->RestorationFlag,Rep_PCE_PCC);
	  WritePckNodeEvent* rep_eve=new WritePckNodeEvent(this,rep_pk,this->ParentNet->Now);
	  this->ParentNet->genEvent(rep_eve);
	}

        return;
    }

    if (pk->NoPath==YES_PATH)
        peso=this->PCE_ExpandEdgeDomainSequence(pk,&n,&l);

    //Routing failed: PCRep containing the NO_PATH information
    if ((pk->NoPath==NO_PATH) || (peso==0)) {
        PCEP_PCRepPck* P_pk=new PCEP_PCRepPck(this,pk->ConnSource,128,128,this->ParentNet->Now,pk->Conn,NO_PATH,pk->RestorationFlag,Rep_PCE_PCC);
        WritePckNodeEvent* P_eve=new WritePckNodeEvent(this,P_pk,this->ParentNet->Now);
        this->ParentNet->genEvent(P_eve);

        return;
    }

    #ifdef STDOUT
    cout << "PCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " computed path: ";
    for (int i=0; i<n.size(); i++) {
        cout << n[i]->Id << " ";
    }
    cout << endl;
    #endif

    //Routing success
    PCEP_PCRepPck* P_pk=new PCEP_PCRepPck(this,pk->ConnSource,128,128,this->ParentNet->Now,pk->Conn,n,l,e,d,SuggestedLabel,YES_PATH,pk->RestorationFlag,Rep_PCE_PCC);
    WritePckNodeEvent* P_eve=new WritePckNodeEvent(this,P_pk,this->ParentNet->Now);
    this->ParentNet->genEvent(P_eve);

    return;
}

void PCx_Node::ReceivedPck_PCEP_PCRep_PCE_HPCE(PCEP_PCRepPck* pk) {
    vector<CNode*> node_path;
    vector<CLink*> link_path;
    vector<CNode*> e;
    vector<CDomain*> d;
    PCx_Node* dest_pce=pk->Conn->SourceNode->ParentDomain->PCE_Node;
    int RoutingSuccess, RestorationFlag;
    PCEP_PCRepPck* rep_pk;
    int base_slot;
  
    if (this->Type!=PCC_PCE_HPCE) ErrorMsg("PCEP_PCRep_PCE_HPCE not addressed to the HPCE node");

    #ifdef STDOUT_PCEP_REQREP
    cout << "HPCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " PCEP_Reply [PCE_HPCE] packet... arrived at destination ";
    cout << " NoPath=" << pk->NoPath << " TIME: " << this->ParentNet->Now << endl;
    #endif

    pk->ComputationSession->ReceivedReply(pk); 
    
    if (pk->ComputationSession->LastReply()) {
      RoutingSuccess=pk->ComputationSession->close(&node_path,&link_path,&base_slot);
      RestorationFlag=pk->ComputationSession->RestorationFlag;
      delete pk->ComputationSession;
      
      #ifdef STDOUT_PCEP_REQREP
      cout << "HPCE NODE: " << Id << " (" << Name << "): conn id " << pk->Conn->Id << " ComputationSession CLOSED";
      cout << " nodes: ";
      for (int i=0; i<node_path.size(); i++) {
        cout << node_path[i]->Id << " ";
      }
      cout << " links: ";
      for (int i=0; i<link_path.size(); i++) {
        cout << link_path[i]->Id << " ";
      }
      cout << "SUGGESTED base slot: " << base_slot << endl;
      #endif
      
      //TODO Derivare da node_path la sequenza dei domini e la sequenza degli edge
      
      if (RoutingSuccess) {
	rep_pk = new PCEP_PCRepPck(this,dest_pce,128,128,this->ParentNet->Now,pk->Conn,node_path,link_path,e,d,base_slot,YES_PATH,RestorationFlag,Rep_HPCE_PCE);
	this->Add_CPU_EVENT_PCE_RSA(rep_pk);
	
	//WritePckNodeEvent* rep_pk_eve = new WritePckNodeEvent(this,rep_pk,this->ParentNet->Now);
	//this->ParentNet->genEvent(rep_pk_eve);
      }
      else {
	rep_pk = new PCEP_PCRepPck(this,dest_pce,128,128,this->ParentNet->Now,pk->Conn,NO_PATH,RestorationFlag,Rep_HPCE_PCE);
	this->Add_CPU_EVENT_PCE_RSA(rep_pk);
	
	//WritePckNodeEvent* rep_pk_eve = new WritePckNodeEvent(this,rep_pk,this->ParentNet->Now);
	//this->ParentNet->genEvent(rep_pk_eve);
      }
    }
}

void PCx_Node::ReceivedPck_PCEP_PCRep_PCE_PCE(PCEP_PCRepPck* pk) {
}

void PCx_Node::ReceivedPck_PCEP_PCReport(PCEP_PCReportPck* pk) {
    int SendingSuccess;
  int width_slots, blocked_base_slot;
  CNode *ActualNode, *NextNode;
  int SuggestedLabel;

  //TTL test
  if (TTL_expired(pk)) return;

  if (this==pk->DstNode) {
    
    if ((this->Type!=PCC_PCE) && (this->Type!=PCC_PCE_HPCE))
	ErrorMsg("PCEP_PCReport addressed to a not PCE node");
    
     #ifdef STDOUT
     cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " PCEP_PCReport packet...arrived at destination TIME: " << this->ParentNet->Now << endl;
     #endif
    
    pk->SetupSession->ReceivedReply(pk->SrcNode,pk->Conn,pk->ReplyType);
  
    //If this is the last reply close the session
    if (pk->SetupSession->LastReply()) {
      #ifdef STDOUT
      cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " PCEP_INTERDOMAIN_SESSION CLOSED at TIME: " << this->ParentNet->Now << endl;
      #endif
      
      pk->SetupSession->close();
    
      delete pk->SetupSession;
    }
  
    delete pk;
    return;
  }

  if (this==pk->SrcNode) {
    #ifdef STDOUT
    //cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " created PCEP_PCReport packet...sent TIME: " << this->ParentNet->Now << endl;
    #endif

    SendingSuccess=SendPck(pk);
    if (!SendingSuccess) ErrorMsg("SendPck returned 0... dalay_panic");

    return;
  }

  #ifdef STDOUT
  //cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " arrived PCEP_PCReport packet...sent TIME: " << this->ParentNet->Now << endl;
  #endif

  SendingSuccess=SendPck(pk);
  if (!SendingSuccess) ErrorMsg("SendPck returned 0... dalay_panic");

  return;
}

void PCx_Node::ReceivedPck_PCEP_PCInit(PCEP_PCInitPck* pk) {
  int SendingSuccess;
  int width_slots, blocked_base_slot;
  CNode *ActualNode, *NextNode;
  int SuggestedLabel=-1;
  PCx_Node *previous_controller=NULL;
  
  //TTL test
  if (TTL_expired(pk)) return;

  if (this==pk->DstNode) {
    #ifdef STDOUT
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " PCEP_PCInit packet... arrived at destination TIME: " << this->ParentNet->Now << endl;
    #endif
      
    if ((this->Type!=PCC_PCE) && (this->Type!=PCC_PCE_HPCE))
        ErrorMsg("PCEP_PCInit addressed to a not PCE node");
    
    //Contruisce una CConnection di appoggio con solo percorso fino ad edge successivo --- RICORDARSI DELETE ---
    CConnection* conn_segment = new CConnection(pk->ConnId,this->ParentNet,pk->ConnSource,pk->ConnDestination);
    conn_segment->width_slots=pk->Conn->width_slots;
    conn_segment->base_slot=-1;
    
    //Inizializza nodepath e linkpath del segmento della connessione
    for (int i=0; i<pk->Conn->NodePath.size()-1; i++) {
      ActualNode=pk->Conn->NodePath[i];
      NextNode=pk->Conn->NodePath[i+1];
      
      if (ActualNode->ParentDomain == this->ParentDomain) {
        conn_segment->NodePath.push_back(ActualNode);
	
        if (NextNode->ParentDomain != this->ParentDomain) {
            conn_segment->NodePath.push_back(NextNode);
        }
	
        if ((NextNode->ParentDomain == this->ParentDomain) && (i==pk->Conn->NodePath.size()-2)) {
            conn_segment->NodePath.push_back(NextNode);
        }
      }
    }
    
    for (int i=0; i<conn_segment->NodePath.size()-1; i++) {
      ActualNode=conn_segment->NodePath[i];
      NextNode=conn_segment->NodePath[i+1];
      
      conn_segment->LinkPath.push_back(this->ParentNet->GetLinkPtrFromNodes(ActualNode,NextNode));
    }
    
    #ifdef STDOUT
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " segment in this domain: ";
    for (int i=0; i<conn_segment->NodePath.size(); i++) {
      cout << conn_segment->NodePath[i]->Id << " ";
    }
    cout << "- LinkPath size: " << conn_segment->LinkPath.size();
    cout << endl;
    #endif
    
    //Wavelength Assignment
    if (pk->Conn->Type == INTER_DOMAIN_CONN) {
        if (this->pce_wa_enabled) {
            switch (this->ParentNet->Provisioning_DefaultPolicy) {
                case FF:
                    SuggestedLabel=this->PathTable->Flexible_PCE_SNMP_WavelengthAssignmentFF(conn_segment->LinkPath,conn_segment->width_slots,conn_segment->Id,0,conn_segment);
                    break;
                default:
                    ErrorMsg("OPENFLOW CONTROLLER: WA not supported");
            }
        }
        else {
            ErrorMsg("pce_wa_enabled should be set to 1");
        }
        
        #ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " computed base slot: " << SuggestedLabel << endl;
        #endif
    }
    
    //Check availability of the suggested wavelenght on the local SNMP database
    if (pk->Conn->Type == INTER_DOMAIN_CONN_ALIEN) {
        #ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " suggested base slot: " << pk->Conn->SuggestedBaseSlot_ALIEN << endl;
        #endif
        
        if (SNMP_check_path_resources(this->SNMP_LinkState_dB,conn_segment->LinkPath,pk->Conn->SuggestedBaseSlot_ALIEN,conn_segment->width_slots,conn_segment->Id)) {
            SuggestedLabel = pk->Conn->SuggestedBaseSlot_ALIEN;
        }
        else {
            #ifdef STDOUT
            cout << "NODE: " << Id << " (" << Name << "): conn id " << conn_segment->Id << " conn blocked ";
            cout << " with suggested label: " << pk->Conn->SuggestedBaseSlot_ALIEN << endl;
            #endif
            
            //TODO update specific statistics here
            
            SuggestedLabel = -1;
        }
    }
    
    //Può accadere perché routing interdominio è stato calcolato in una fase precedente
    if (SuggestedLabel==-1) {
        
      PCEP_PCReportPck* rep_pk = new PCEP_PCReportPck(this,pk->SrcNode,100,64,this->ParentNet->Now,conn_segment,pk->SetupSession,NO_PATH);
      WritePckNodeEvent* rep_eve = new WritePckNodeEvent(this,rep_pk,this->ParentNet->Now);
      this->ParentNet->genEvent(rep_eve);
      
      //Anche in caso di errore si invia PCInit al controllore precedente per continuare catena BRPC
      //Invece in caso di successo il PCInit al controllore precedente viene inviato al termine della FlowModSession
      if (pk->SetupSession->SessionType==SEQUENTIAL_SESSION) { 
          int j=0;
          while (this->Id!=pk->Conn->controllers_sequence[j]->Id) j++;
          
          if (j==0) {
              previous_controller=NULL;
          } else {
              previous_controller = pk->Conn->controllers_sequence[j-1];
          }
          
          if (previous_controller!=NULL) {
              #ifdef STDOUT
              cout << "NODE: " << Id << " (" << Name << "): conn id " << conn_segment->Id << " SEQUENTIAL_SESSION PCInit forwarded ";
              #endif
              
            PCEP_PCInitPck* init_pk = new PCEP_PCInitPck(this,previous_controller,100,64,this->ParentNet->Now,pk->Conn,pk->SetupSession);
            WritePckNodeEvent* init_eve = new WritePckNodeEvent(this,init_pk,this->ParentNet->Now);
            this->ParentNet->genEvent(init_eve);
        }
      }
      
      delete pk;
      return;
    }
  
    //Write the selected slot in the connection
    conn_segment->base_slot=SuggestedLabel;

    #ifdef STDOUT_PCEP_REQREP
    cout << "OPENFLOW CONTROLLER NODE: " << Id << " (" << Name << "): conn id " << conn_segment->Id << " path computed updating SNMP database TIME: " << this->ParentNet->Now << endl;
    cout << "OPENFLOW CONTROLLER NODE: " << Id << " (" << Name << "): conn id " << conn_segment->Id << " along path: ";
    for (int i=0; i<conn_segment->NodePath.size(); i++) {
	cout << conn_segment->NodePath[i]->Id << " ";
    }
    cout << "with suggested label: " << SuggestedLabel << endl;
    #endif
    
    //Updating the SNMP database in the OpenFlow controller
    this->SNMP_reserve_path_resources(this->SNMP_LinkState_dB,conn_segment->LinkPath,conn_segment->base_slot,conn_segment->width_slots,conn_segment->Id);

    //Trigger the FlowModeSession
    vector<CConnection*> conns;
    conns.push_back(conn_segment);
    
    FlowModeSession* intra_session = new FlowModeSession(this->ParentNet, this, conns,INTER_DOMAIN_SESSION,pk->SrcNode);
    intra_session->ParentInterDomain_Session = pk->SetupSession;
    intra_session->start(conns);
    
    delete pk;
    return;
  }

  if (this==pk->SrcNode) {
    #ifdef STDOUT
    //cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " created PCEP_PCInit packet... sent TIME: " << this->ParentNet->Now << endl;
    #endif

    SendingSuccess=SendPck(pk);
    if (!SendingSuccess) ErrorMsg("SendPck returned 0... dalay_panic");

    return;
  }

  #ifdef STDOUT
  //cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " arrived PCEP_PCInit packet...sent TIME: " << this->ParentNet->Now << endl;
  #endif

  SendingSuccess=SendPck(pk);
  if (!SendingSuccess) ErrorMsg("SendPck returned 0... dalay_panic");

  return;
}

//Used by the PCE of the source domain, when the PCEP_PCRepPck from the HPCE is received 
int PCx_Node::PCE_ExpandEdgeDomainSequence(PCEP_PCRepPck* pk, vector<CNode*>* n, vector<CLink*>* l) {
    int peso, i;
    CNode* src;
    CNode* dst;

    //Using the sequence of edge nodes
    if (this->ParentNet->HPCE_SimulationMode==EDGE_SEQUENCE) {
        if (pk->EdgeNodePath.size()==0) 
	  ErrorMsg("PCE_ExpandEdgeSequence HPCE_SimulationMode==EDGE_SEQUENCE but EdgeNodePath.size==0");
        
	i=0; //Indentifies source and destination in this domain
        while (this->ParentDomain!=pk->EdgeNodePath[i]->ParentDomain) i++;

        src=pk->EdgeNodePath[i];
        dst=pk->EdgeNodePath[i+1];

	#ifdef STDOUT
        cout << "PCE_ExpandEdgeSequence (PCEP Reply EDGE) for conn id " << pk->Conn->Id << " expanding: " << src->Id << "->" << dst->Id << endl;
	#endif

        if (src->ParentDomain!=dst->ParentDomain) {
            n->push_back(pk->EdgeNodePath[i]);
            n->push_back(pk->EdgeNodePath[i+1]);
            l->push_back(src->GetLinkToNode(pk->EdgeNodePath[i+1]));

            return 1;
        }

        switch (this->ParentNet->RoutingMode_PCE) {
        case OFF:
            peso=this->ParentDomain->PathTable->NoInfo_RandomPath(src, dst, n, l, pk->Conn);
            break;
        case AGGREGATED:
            peso=this->ParentDomain->PathTable->AggInfo_LeastCongestedPath(src, dst, n, l, pk->Conn);
            break;
        case DETAILED:
            peso=this->ParentDomain->PathTable->Flexible_DetInfo_LeastCongestedPath(src, dst, n, l, pk->Conn);
            break;
        }

        if ((peso!=0) && (pk->EdgeNodePath.size()>=i+2)) {
            n->push_back(pk->EdgeNodePath[i+2]);
            l->push_back(dst->GetLinkToNode(pk->EdgeNodePath[i+2]));
        }

        return peso;
    }

    //Using the sequence of domains
    if (this->ParentNet->HPCE_SimulationMode==DOMAIN_SEQUENCE) {
        if (pk->DomainPath.size()==0) 
	  ErrorMsg("HPCE_SimulationMode==DOMAIN_SEQUENCE but DomainPath.size==0");

	src=pk->ConnSource;
        dst=pk->DomainPath[1]->PCE_Node;//TODO Attenzione stiamo usando il PCE come nodo interno al dominio !!!

	#ifdef STDOUT
        cout << "PCE_ExpandEdgeSequence (PCEP Reply DOMAIN) for conn id " << pk->Conn->Id << " expanding: " << src->Id << "->" << dst->Id << endl;
	#endif

        switch (this->ParentNet->RoutingMode_PCE) {
        case OFF:
            peso=this->ParentDomain->PathTable->NoInfo_RandomPath(src, dst, n, l, pk->Conn);
            break;
        case AGGREGATED:
            peso=this->ParentDomain->PathTable->AggInfo_LeastCongestedPath(src, dst, n, l, pk->Conn);
            break;
        case DETAILED:
            peso=this->ParentDomain->PathTable->Flexible_DetInfo_LeastCongestedPath(src, dst, n, l, pk->Conn);
            break;
        }

        return peso;
    }
}

//Used by the PCE of intermediate and destination domains, when the PCEP_PCReqPck from the edge node is received 
int PCx_Node::PCE_ExpandEdgeDomainSequence(PCEP_PCReqPck* pk, vector<CNode*>* n, vector<CLink*>* l) {
    int peso, i;
    CNode* src;
    CNode* dst;

    //Using the sequence of edge nodes
    if (this->ParentNet->HPCE_SimulationMode==EDGE_SEQUENCE) {
        if (pk->Conn->EdgeNodePath.size()==0) 
	  ErrorMsg("HPCE_SimulationMode==EDGE_SEQUENCE but EdgeNodePath.size==0");
    
        i=0; //Indentifies source and destination in this domain
        while (this->ParentDomain!=pk->Conn->EdgeNodePath[i]->ParentDomain) i++;

        src=pk->Conn->EdgeNodePath[i];
        dst=pk->Conn->EdgeNodePath[i+1];

	#ifdef STDOUT
        cout << "PCE_ExpandEdgeSequence (PCEP Request EDGE) for conn id " << pk->Conn->Id << " expanding: " << src->Id << "->" << dst->Id << endl;
	#endif
	
        if (src->ParentDomain!=dst->ParentDomain) {
            n->push_back(pk->Conn->EdgeNodePath[i]);
            n->push_back(pk->Conn->EdgeNodePath[i+1]);
            l->push_back(src->GetLinkToNode(pk->Conn->EdgeNodePath[i+1]));

            return 1;
        }

        switch (this->ParentNet->RoutingMode_PCE) {
        case OFF:
            peso=this->ParentDomain->PathTable->NoInfo_RandomPath(src, dst, n, l, pk->Conn);
            break;
        case AGGREGATED:
            peso=this->ParentDomain->PathTable->AggInfo_LeastCongestedPath(src, dst, n, l, pk->Conn);
            break;
        case DETAILED:
            peso=this->ParentDomain->PathTable->Flexible_DetInfo_LeastCongestedPath(src, dst, n, l, pk->Conn);
            break;
        }

        if ((peso!=0) && (dst!=pk->Conn->DestinationNode) && (pk->Conn->EdgeNodePath.size()>=i+2)) {
            n->push_back(pk->Conn->EdgeNodePath[i+2]);
            l->push_back(dst->GetLinkToNode(pk->Conn->EdgeNodePath[i+2]));
        }

        return peso;
    }

     //Using the sequence of domains
    if (this->ParentNet->HPCE_SimulationMode==DOMAIN_SEQUENCE) {
        if (pk->Conn->DomainPath.size()==0) 
	  ErrorMsg("HPCE_SimulationMode==DOMAIN_SEQUENCE but DomainPath.size==0");
	
        i=0; //Indentifies source and destination in this domain
        while (this->ParentDomain!=pk->Conn->DomainPath[i]) i++;

        src=pk->SrcNode;
        if (this->ParentDomain->PathTable->DistanceMatrix[this->Id][pk->ConnDestination->Id] < MAX)
            dst=pk->ConnDestination;
        else
            dst=pk->Conn->DomainPath[i+1]->PCE_Node; //TODO Attenzione stiamo usando il PCE come nodo interno al dominio !!!

	#ifdef STDOUT
        cout << "PCE_ExpandEdgeSequence (PCEP Request DOMAIN) for conn id " << pk->Conn->Id << " expanding: " << src->Id << "->" << dst->Id << endl;
	#endif

        switch (this->ParentNet->RoutingMode_PCE) {
        case OFF:
            peso=this->ParentDomain->PathTable->NoInfo_RandomPath(src, dst, n, l, pk->Conn);
            break;
        case AGGREGATED:
            peso=this->ParentDomain->PathTable->AggInfo_LeastCongestedPath(src, dst, n, l, pk->Conn);
            break;
        case DETAILED:
            peso=this->ParentDomain->PathTable->Flexible_DetInfo_LeastCongestedPath(src, dst, n, l, pk->Conn);
            break;
        }

        return peso;
    }
}

void PCx_Node::PCC_PCE_PathRequest(CNode* source, CConnection* conn) {
    CNode* pce=this->ParentDomain->PCE_Node;
    double StartTime=this->ParentNet->Now;
    int RestorationFlag=0;

    conn->Status=REQUESTED_PATH;

    #ifdef STDOUT_PCEP_REQREP
    cout << "PCC NODE: " << Id << " (" << Name << "): conn id " << conn->Id << " created PCEP_Request packet: " << this->ParentNet->Now << endl;
    #endif

    PCEP_PCReqPck* pk=new PCEP_PCReqPck(source,pce,128,128,StartTime,conn,RestorationFlag,Req_PCC_PCE);
    WritePckNodeEvent* eve=new WritePckNodeEvent(pk->SrcNode,pk,StartTime);
    this->ParentNet->genEvent(eve);
}

void PCx_Node::PCE_PathRequest_Restoration(CConnection* conn) {
    CNode* PCE_Node=this->ParentDomain->PCE_Node;
    double StartTime=this->ParentNet->Now;
    int RestorationFlag=1;

    conn->Status=REQUESTED_PATH_RESTORATION;

    //Here an additional epsilon delay is inserted so that the PCReq is sent after the TearDown and NotifyReleased
    PCEP_PCReqPck* pk=new PCEP_PCReqPck(conn->SourceNode,PCE_Node,128,128,StartTime+0.01E-3,conn,RestorationFlag,Req_PCC_PCE);
    WritePckNodeEvent* eve=new WritePckNodeEvent(pk->SrcNode,pk,StartTime+0.01E-3);
    this->ParentNet->genEvent(eve);
}

void PCx_Node::HPCE_EdgeToDomain_SequenceConversion(vector<CNode*> path_node, vector<CDomain*>* path_domain) {
    CDomain* CurrentDomain=NULL;

    if (path_node.size()==0)
        ErrorMsg("Invalid input Edge Path: no elements");

    path_domain->clear();

    for (int i=0;i<path_node.size();i++) {
        if (path_node[i]->ParentDomain!=CurrentDomain) {
            path_domain->push_back(path_node[i]->ParentDomain);
            CurrentDomain=path_node[i]->ParentDomain;
        }
    }
    return;
}

void PCx_Node::ConnEstablish_OpenFlow(vector<CConnection*>& conns) {
  int peso;
  int SuggestedLabel;
  CNet* net=this->ParentNet;
  vector<CNode*> n;
  vector<CLink*> l;
  FlowModeSession* intra_session;
  InterDomainSession* inter_session;
  CConnection* conn;
  
  for (int i=0; i<conns.size(); i++) {
    conn=conns[i];
    
    if (conn->Type==INTRA_DOMAIN_CONN) {
      #ifdef STDOUT
      cout << "OPENFLOW CONTROLLER: establishing INTRA_DOMAIN conn id " << conn->Id << " TIME: " << net->Now << endl;
      #endif
      
      net->ConnRequested++; net->ConnRequestedREP++;
      net->ConnRequestedATT[0]++; net->ConnRequestedATT_REP[0]++;
      
      //Routing
      peso=this->ParentDomain->PathTable->Flexible_PCE_SNMP_DetInfo_LeastCongestedPath(conn->SourceNode,conn->DestinationNode,&n,&l,conn);
    
      if (peso==0) {
        //If the connection is refused no reply is provided to the source node
    
        conn->Status=REFUSED;
    
        net->ConnRefused++; net->ConnRefusedREP++;
        net->ConnBlocked++; net->ConnBlockedREP++;
        net->ConnBlockedRouting++; net->ConnBlockedRoutingREP++;
    
        //Only one attempt is supported with OPENFLOW
        net->ConnBlockedATT[0]++; net->ConnBlockedATT_REP[0]++;
        net->ConnBlockedRoutingATT[0]++; net->ConnBlockedRoutingATT_REP[0]++;
    
        #ifdef STDOUT
        cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << conn->Id << " INTRADOMAIN CONNECTION REFUSED " << endl;
        cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << conn->Id << " CONNECTION MEMORY (erasing) size: " << this->ParentNet->ActiveConns.size() << " TIME: " << this->ParentNet->Now<< endl;
        #endif
    
        /*Rimuove il puntatore alla connessione nel vector di connessioni attive in tutta la rete*/
        vector<CConnection*>::iterator it_net=this->ParentNet->ActiveConns.begin();
        while (conn!=*it_net) it_net++;
        this->ParentNet->ActiveConns.erase(it_net);
    
        delete conn;
	
        //continue to next connection in the conns
        continue;
      }
  
      //Write node and link path in the connection
      conn->NodePath=n;
      conn->LinkPath=l;
  
      //Wavelength Assignment
      if (this->pce_wa_enabled) {
        switch (this->ParentNet->Provisioning_DefaultPolicy) {
            case FF:
            SuggestedLabel=this->PathTable->Flexible_PCE_SNMP_WavelengthAssignmentFF(l,conn->width_slots,conn->Id,0,conn);
            break;
        default:
            ErrorMsg("OPENFLOW CONTROLLER: WA not supported");
        }
      }
      else {
        SuggestedLabel=-1;
      }
 
      //Non dovrebbe accadere mai... perché già il routing è stato fatto con leastcongested
      if (SuggestedLabel==-1) {
        ErrorMsg("PCE_SNMP_WavelengthAssignmentFF did not find an available lambdas");
      }
  
      //Write the selected slot in the connection
      conn->base_slot=SuggestedLabel;

      //Setting the CConnection
      conn->Status=ADMITTED_PATH;
  
      this->ParentNet->ConnEstablished++; this->ParentNet->ConnEstablishedREP++;
      this->ParentNet->ConnEstablishedATT[0]++; this->ParentNet->ConnEstablishedATT_REP[0]++;
  
      #ifdef STDOUT_PCEP_REQREP
      cout << "OPENFLOW CONTROLLER NODE: " << Id << " (" << Name << "): conn id " << conn->Id << " path computed updating SNMP database TIME: " << this->ParentNet->Now << endl;
      cout << "OPENFLOW CONTROLLER NODE: " << Id << " (" << Name << "): conn id " << conn->Id << " along path: ";
      for (int i=0; i<n.size(); i++) {
        cout << n[i]->Id << " ";
      }
      cout << "with suggested label: " << SuggestedLabel << endl;
      #endif
    
      //Updating the SNMP database in the OpenFlow controller
      this->SNMP_reserve_path_resources(this->SNMP_LinkState_dB,conn->LinkPath,conn->base_slot,conn->width_slots,conn->Id);
      
      //Initialize and start the OpenFlow session to notificate each node
      intra_session=new FlowModeSession(net,this,conns,INTRA_DOMAIN_SESSION,conns[0]->NodePath[0]);
      intra_session->start(conns);
      
      //continue to next connection in the conns
      continue;
    }
    
    if ((conn->Type==INTER_DOMAIN_CONN) || (conn->Type==INTER_DOMAIN_CONN_ALIEN)) {
      #ifdef STDOUT
      cout << "OPENFLOW CONTROLLER: establishing INTER_DOMAIN conn id " << conn->Id << " TIME: " << net->Now << endl;
      #endif
      
      //peso=this->ParentDomain->ParentNet->PathTable->Flexible_LINKS_DetInfo_LeastCongestedPath(conn->SourceNode,conn->DestinationNode,&n,&l,conn);
      peso=this->ParentDomain->ParentNet->PathTable->Flexible_LINKS_AggInfo_LeastCongestedPath(conn->SourceNode,conn->DestinationNode,&n,&l,conn);
      
      if (peso==0) {
        #ifdef STDOUT
        cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << conn->Id << " INTERDOMAIN CONNECTION REFUSED - ROUTING" << endl;
        #endif
	
        //If the connection is refused no reply is provided to the source node
    
        conn->Status=REFUSED;
	  
        net->ConnRequested++; net->ConnRequestedREP++;
        net->ConnRequestedATT[0]++; net->ConnRequestedATT_REP[0]++;
    
        net->ConnRefused++; net->ConnRefusedREP++;
        net->ConnBlocked++; net->ConnBlockedREP++;
        net->ConnBlockedRouting++; net->ConnBlockedRoutingREP++;
    
        //Only one attempt is supported with OPENFLOW
        net->ConnBlockedATT[0]++; net->ConnBlockedATT_REP[0]++;
        net->ConnBlockedRoutingATT[0]++; net->ConnBlockedRoutingATT_REP[0]++;
    
        #ifdef STDOUT
        cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << conn->Id << " CONNECTION MEMORY (erasing) size: " << this->ParentNet->ActiveConns.size() << " TIME: " << this->ParentNet->Now<< endl;
        #endif
    
        /*Rimuove il puntatore alla connessione nel vector di connessioni attive in tutta la rete*/
        vector<CConnection*>::iterator it_net=this->ParentNet->ActiveConns.begin();
        while (conn!=*it_net) it_net++;
        this->ParentNet->ActiveConns.erase(it_net);
    
        delete conn;
	
        //continue to next connection in the conns
        continue;
      }
      
      //Write node and link path in the connection
      conn->NodePath=n;
      conn->LinkPath=l;
  
      //Wavelength Assignment cannot be performed here, demanded to remote controllers
      if (conn->Type==INTER_DOMAIN_CONN_ALIEN) {
          //Perform Spectrum Assignment
          conn->SuggestedBaseSlot_ALIEN = this->ParentDomain->ParentNet->PathTable->Flexible_LINKS_WavelengthAssignmentFF(conn->LinkPath,conn->width_slots,conn->Id,0,conn);
          SuggestedLabel = conn->SuggestedBaseSlot_ALIEN;
          
            #ifdef STDOUT_PCEP_REQREP
            if (SuggestedLabel==-1) cout << "OPENFLOW CONTROLLER NODE: " << Id << " (" << Name << "): conn id " << conn->Id << " no lambda" << endl;
            #endif
      }
      else { //if (conn->Type==INTER_DOMAIN_CONN)
          //Spectrum Assignment is performed locally, no spectrum continuty among different domains
          SuggestedLabel=-1;
      }
      
      #ifdef STDOUT_PCEP_REQREP
      cout << "OPENFLOW CONTROLLER NODE: " << Id << " (" << Name << "): conn id " << conn->Id << " path computed TIME: " << this->ParentNet->Now << endl;
      cout << "OPENFLOW CONTROLLER NODE: " << Id << " (" << Name << "): conn id " << conn->Id << " along path: ";
      for (int i=0; i<n.size(); i++) {
            cout << n[i]->Id << " ";
      }
      cout << "with suggested label: " << SuggestedLabel << endl;
      #endif
 
      inter_session=new InterDomainSession(net,this,conn);
      inter_session->start_parallel_session(conn);
      //inter_session->start_sequential_session(conn);
      
      //continue to next connection in the conns
      continue;
    }
  }
  return;
}

void PCx_Node::ConnRestoration_OpenFlow(vector<CConnection*>& conns) {
  int peso;
  int SuggestedLabel;
  CNet* net=this->ParentNet;
  vector<CNode*> n;
  vector<CLink*> l;
  FlowModeSession* Session;
  vector<CConnection*> admitted_conns;
  InterDomainSession* inter_session;
  
  if (conns.size() != 1) {
      ErrorMsg("Only connection vector of size 1 is currently supported... dalay panic");
  }
  
  CConnection* conn=conns[0];

  n.clear();
  l.clear();
    
  conn->ElapsedTime=(conn->DisruptionTime)-(conn->GenerationTime + conn->SignalingTime);
  conn->OldLabel=conn->base_slot;
    
  //Deleted disrupted path
  conn->NodePath.clear();
  conn->LinkPath.clear();
  
  if (conn->Type == INTRA_DOMAIN_CONN) {
      
     #ifdef STDOUT
     cout << "OPENFLOW CONTROLLER: restoring INTRA_DOMAIN conn id " << conn->Id << " TIME: " << this->ParentNet->Now << endl;
     #endif 
  
    //Routing
    peso=this->ParentDomain->PathTable->Flexible_PCE_SNMP_DetInfo_LeastCongestedPath(conn->SourceNode,conn->DestinationNode,&n,&l,conn);
  
    if (peso==0) {
        conn->Status=REFUSED_RESTORATION;
    
        this->ParentNet->ConnRefused_Restoration++;
        this->ParentNet->ConnRefused_RestorationREP++;

        this->ParentNet->ConnRefusedRouting_Restoration++;
        this->ParentNet->ConnRefusedRouting_RestorationREP++;
        this->ParentNet->ConnRefusedRouting_RestorationATT[conn->RestorationAttempt-1]++;
    
        //Rimuove il puntatore alla connessione nel vector di connessioni attive in tutta la rete
        vector<CConnection*>::iterator it_net=this->ParentNet->ActiveConns.begin();
        while (conn!=*it_net) it_net++;
        this->ParentNet->ActiveConns.erase(it_net);
        
        delete conn;
        return;
    }
  
    //Write node and link path in the connection
    conn->NodePath=n;
    conn->LinkPath=l;
    
    //Wavelength Assignment
    if (this->pce_wa_enabled) {
        switch (this->ParentNet->Provisioning_DefaultPolicy) {
        case FF:
            SuggestedLabel=this->PathTable->Flexible_PCE_SNMP_WavelengthAssignmentFF(l,conn->width_slots,conn->Id,1,conn);
            break;
        default:
            ErrorMsg("OPENFLOW CONTROLLER: WA not supported");
        }
    }
    else {
        SuggestedLabel=-1;
    }
 
    //Non dovrebbe accadere mai... perché già il routing è stato fatto con leastcongested
    if (SuggestedLabel==-1) {
        ErrorMsg("PCE_SNMP_WavelengthAssignmentFF did not find an available lambdas");
    }
  
    //Write the selected slot in the connection
    conn->base_slot=SuggestedLabel;

    //Setting the CConnection
    conn->Status=ADMITTED_PATH_RESTORATION;
    
    this->ParentNet->ConnRestored++;
    this->ParentNet->ConnRestoredREP++;
    this->ParentNet->ConnRestoredATT[0]++;
  
    #ifdef STDOUT_PCEP_REQREP
    cout << "OPENFLOW CONTROLLER NODE: " << Id << " (" << Name << "): conn id " << conn->Id << " restoration path computed updating SNMP database TIME: " << this->ParentNet->Now << endl;
    cout << "OPENFLOW CONTROLLER NODE: " << Id << " (" << Name << "): conn id " << conn->Id << " along path: ";
    for (int i=0; i<n.size(); i++) {
        cout << n[i]->Id << " ";
    }
    cout << "with suggested label " << SuggestedLabel << endl;
    #endif
  
    //Updating the SNMP database in the OpenFlow controller
    this->SNMP_reserve_path_resources(this->SNMP_LinkState_dB,conn->LinkPath,conn->base_slot,conn->width_slots,conn->Id);
  
    admitted_conns.push_back(conn);
  
    //Open Session
    Session=new FlowModeSession(net,this,admitted_conns,INTRA_DOMAIN_SESSION,conns[0]->NodePath[0]);
    Session->RestorationFlag=1;
  
    //Start the OpenFlow session to notificate each node
    Session->start(admitted_conns);
  
    return;
  }
  
  if ((conn->Type==INTER_DOMAIN_CONN) || (conn->Type==INTER_DOMAIN_CONN_ALIEN)) {
    
    #ifdef STDOUT
    cout << "OPENFLOW CONTROLLER: restoring INTER_DOMAIN conn id " << conn->Id << " TIME: " << this->ParentNet->Now << endl;
    #endif   
    
    //routing
    peso=this->ParentDomain->ParentNet->PathTable->Flexible_LINKS_AggInfo_LeastCongestedPath(conn->SourceNode,conn->DestinationNode,&n,&l,conn);
      
    if (peso==0) {
        conn->Status=REFUSED_RESTORATION;
    
        this->ParentNet->ConnRefused_Restoration++;
        this->ParentNet->ConnRefused_RestorationREP++;

        this->ParentNet->ConnRefusedRouting_Restoration++;
        this->ParentNet->ConnRefusedRouting_RestorationREP++;
        this->ParentNet->ConnRefusedRouting_RestorationATT[conn->RestorationAttempt-1]++;
    
        //Rimuove il puntatore alla connessione nel vector di connessioni attive in tutta la rete
        vector<CConnection*>::iterator it_net=this->ParentNet->ActiveConns.begin();
        while (conn!=*it_net) it_net++;
        this->ParentNet->ActiveConns.erase(it_net);
    
        delete conn;
        return;
    }
    
    //Write node and link path in the connection
    conn->NodePath=n;
    conn->LinkPath=l;
    
    //Wavelength Assignment cannot be performed here, demanded to remote controllers
    if (conn->Type==INTER_DOMAIN_CONN_ALIEN) {
          //Perform Spectrum Assignment
          conn->SuggestedBaseSlot_ALIEN = this->ParentDomain->ParentNet->PathTable->Flexible_LINKS_WavelengthAssignmentFF(conn->LinkPath,conn->width_slots,conn->Id,0,conn);
          SuggestedLabel = conn->SuggestedBaseSlot_ALIEN;
    }
    else { //if (conn->Type==INTER_DOMAIN_CONN)
          //Spectrum Assignment is performed locally, no spectrum continuty among different domains
          SuggestedLabel=-1;
    }
    
    #ifdef STDOUT_PCEP_REQREP
    cout << "OPENFLOW CONTROLLER NODE: " << Id << " (" << Name << "): conn id " << conn->Id << " path computed TIME: " << this->ParentNet->Now << endl;
    cout << "OPENFLOW CONTROLLER NODE: " << Id << " (" << Name << "): conn id " << conn->Id << " along path: ";
    for (int i=0; i<n.size(); i++) {
      cout << n[i]->Id << " ";
    }
    cout << "with suggested label: " << SuggestedLabel << endl;
    #endif
 
    inter_session=new InterDomainSession(net,this,conn);
    inter_session->start_parallel_session(conn);
    //inter_session->start_sequential_session(conn);
      
    return;
  }
}

void PCx_Node::ConnRelease_OpenFlow(CConnection* conn) {
  CNode *ActualNode, *NextNode, *ActualController;
  vector<CNode*> n=conn->NodePath;
  vector<CLink*> l=conn->LinkPath;
  int UsedLambda=conn->base_slot;
  
  #ifdef STDOUT
  cout << "NODE: " << this->Id << " (" << this->Name << " OPENFLOW CONTROLLER: releasing conn id " << conn->Id << " " << endl;
  cout << "NODE: " << this->Id << " (" << this->Name << "): conn id " << conn->Id << " CONNECTION MEMORY (erasing) size: " << this->ParentNet->ActiveConns.size() << " TIME: " << this->ParentNet->Now<< endl;
  #endif
  
  /*Rimuove il puntatore alla connessione nel vector di connessioni attive in tutta la rete*/
  vector<CConnection*>::iterator it_net=this->ParentNet->ActiveConns.begin();
  while (conn!=*it_net) it_net++;
  this->ParentNet->ActiveConns.erase(it_net);

  conn->Status=RELEASING;
  
  if (conn->Type==INTRA_DOMAIN_CONN) {
  
    //Aggiornare database SNMP
    this->SNMP_release_path_resources(this->SNMP_LinkState_dB,conn->LinkPath,conn->base_slot,conn->width_slots,conn->Id);
  
    //Notify all the nodes for flow deletion
    for (int i=0; i<n.size(); i++) {
      ActualNode=n[i];
      vector<CConnection*> conns;
    
      conns.push_back(conn);
    
      OFP_FlowModPck* del_pk=new OFP_FlowModPck(this,ActualNode,128,128,this->ParentNet->Now,conns,FlowMod_DELETE);
      WritePckNodeEvent* eve=new WritePckNodeEvent(this,del_pk,this->ParentNet->Now);
      this->ParentNet->genEvent(eve);
    }
  
    conn->Status=RELEASED;
    delete conn;
  }
  
  //Current implementation, there is no communication among controllers, each controller autonomously release its segment
  if ((conn->Type==INTER_DOMAIN_CONN) || (conn->Type==INTER_DOMAIN_CONN_ALIEN)) {
    for (int i=0; i<conn->controllers_sequence.size(); i++) {
      ActualController=conn->controllers_sequence[i];
      
      //Si genera una connessione virtuale ad hoc - RICORDASI DELETE dei segmenti e della conn principale
      CConnection* conn_segment = new CConnection(conn->Id,this->ParentNet,conn->SourceNode,conn->DestinationNode);
      conn_segment->width_slots=conn->width_slots;
      
      //Copia il NodePath e il base_slot
      for (int j=0; j<conn->NodePath.size()-1; j++) {
        ActualNode=conn->NodePath[j];
        NextNode=conn->NodePath[j+1];
      
        if (ActualNode->ParentDomain == ActualController->ParentDomain) {
            conn_segment->NodePath.push_back(ActualNode);
            conn_segment->base_slot=conn->base_slot_path[j];
	
            if (NextNode->ParentDomain != ActualController->ParentDomain) {
                conn_segment->NodePath.push_back(NextNode);
            }
	
            if ((NextNode->ParentDomain == ActualController->ParentDomain) && (j==conn->NodePath.size()-2)) {
                conn_segment->NodePath.push_back(NextNode);
            }
        }
      }
      
      //Copia il LinkPath
      for (int j=0; j<conn_segment->NodePath.size()-1; j++) {
        ActualNode=conn_segment->NodePath[j];
        NextNode=conn_segment->NodePath[j+1];
      
        conn_segment->LinkPath.push_back(this->ParentNet->GetLinkPtrFromNodes(ActualNode,NextNode));
      }
    
      //Each controller release the locally owned segment
      conn->controllers_sequence[i]->ConnRelease_OpenFlow_segment(conn_segment);
    }
    
    delete conn;
  }
  
  return;
}

void PCx_Node::ConnRelease_OpenFlow_segment(CConnection* conn) {
  CNode *ActualNode;
  vector<CNode*> n=conn->NodePath;
  vector<CLink*> l=conn->LinkPath;
  int UsedLambda=conn->base_slot;
  
  #ifdef STDOUT
  if (conn->Status == DISRUPTED)
      cout << "NODE: " << this->Id << " (" << this->Name << ") OPENFLOW CONTROLLER: stub releasing local segment conn id " << conn->Id << " local links size " << conn->LinkPath.size() << endl;
  else
      cout << "NODE: " << this->Id << " (" << this->Name << ") OPENFLOW CONTROLLER: releasing local segment conn id " << conn->Id << " local links size " << conn->LinkPath.size() << endl;
  #endif
  
  //Aggiornare database SNMP
  this->SNMP_release_path_resources(this->SNMP_LinkState_dB,conn->LinkPath,conn->base_slot,conn->width_slots,conn->Id);
  
  //Notify all the nodes for flow deletion
  for (int i=0; i<n.size(); i++) {
    ActualNode=n[i];
    vector<CConnection*> conns;
    
    conns.push_back(conn);
    
    OFP_FlowModPck* del_pk=new OFP_FlowModPck(this,ActualNode,128,128,this->ParentNet->Now,conns,FlowMod_DELETE);
    WritePckNodeEvent* eve=new WritePckNodeEvent(this,del_pk,this->ParentNet->Now);
    this->ParentNet->genEvent(eve);
  }
  
  delete conn;
  return;
}

void PCx_Node::StubRelease_OpenFlow(CConnection* conn) {
  CNode *ActualNode, *NextNode, *ActualController;
  vector<CConnection*> conns;

  //Vector used for generating OFP_packets
  conns.push_back(conn);
  
  //For intra domain connections simply notify the nodes
  if (conn->Type==INTRA_DOMAIN_CONN) {
  
    #ifdef STDOUT
    cout << "OPENFLOW CONTROLLER: stub releasing INTRA_DOMAIN conn id " << conn->Id << " " << endl;
    #endif
  
    //Aggiornare database SNMP
    this->SNMP_release_path_resources(this->SNMP_LinkState_dB,conn->LinkPath,conn->base_slot,conn->width_slots,conn->Id);
    
    //Notify all the nodes for flow deletion
    for (int i=0; i<conn->NodePath.size(); i++) {
        ActualNode=conn->NodePath[i];

        OFP_FlowModPck* del_pk=new OFP_FlowModPck(this,ActualNode,128,128,this->ParentNet->Now,conns,FlowMod_DELETE);
        WritePckNodeEvent* eve=new WritePckNodeEvent(this,del_pk,this->ParentNet->Now);
        this->ParentNet->genEvent(eve);
    }
 
    return;
  }
  
  if ((conn->Type==INTER_DOMAIN_CONN) || (conn->Type==INTER_DOMAIN_CONN_ALIEN)) {
      
    #ifdef STDOUT
    cout << "OPENFLOW CONTROLLER: stub releasing INTER_DOMAIN conn id " << conn->Id << " " << endl;
    #endif  
      
    for (int i=0; i<conn->controllers_sequence.size(); i++) {
      ActualController=conn->controllers_sequence[i];
      
      //Si genera una connessione virtuale ad hoc - RICORDASI DELETE dei segmenti e della conn principale
      //Il delete viene fatto nella ConnRelease_OpenFlow_segment
      CConnection* conn_segment = new CConnection(conn->Id,this->ParentNet,conn->SourceNode,conn->DestinationNode);
      conn_segment->width_slots=conn->width_slots;
      conn_segment->Status=DISRUPTED;
      
      //Copia il NodePath e il base_slot
      for (int j=0; j<conn->NodePath.size()-1; j++) {
        ActualNode=conn->NodePath[j];
        NextNode=conn->NodePath[j+1];
      
        if (ActualNode->ParentDomain == ActualController->ParentDomain) {
            conn_segment->NodePath.push_back(ActualNode);
            conn_segment->base_slot=conn->base_slot_path[j];
	
            if (NextNode->ParentDomain != ActualController->ParentDomain) {
                conn_segment->NodePath.push_back(NextNode);
            }
	
            if ((NextNode->ParentDomain == ActualController->ParentDomain) && (j==conn->NodePath.size()-2)) {
                conn_segment->NodePath.push_back(NextNode);
            }
        }
      }
      
      //Copia il LinkPath
      for (int j=0; j<conn_segment->NodePath.size()-1; j++) {
        ActualNode=conn_segment->NodePath[j];
        NextNode=conn_segment->NodePath[j+1];
      
        conn_segment->LinkPath.push_back(this->ParentNet->GetLinkPtrFromNodes(ActualNode,NextNode));
      }
    
      //Each controller release the locally owned segment
      conn->controllers_sequence[i]->ConnRelease_OpenFlow_segment(conn_segment);
    }  
      
    return;
  }
}

void PCx_Node::ReceivedPck_OFP_FlowModPck(OFP_FlowModPck* pk) {
    //TTL test
    if (TTL_expired(pk)) return;
    
    //cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " pk id " << pk->Id << " type: " << pk->OFP_PckType << " received " << endl;

    //At Destination node
    if (this==pk->DstNode) {
	//The specific processing function is used depending on the PCEP_Req subtype
        switch (pk->OFP_PckType) {
	  case FlowMod_ADD:    
	    this->ReceivedPck_OFP_FlowModPck_ADD(pk);
            break;
	  case FlowMod_DELETE:
	    this->ReceivedPck_OFP_FlowModPck_DEL(pk);
            break;
	  case FlowMod_ACK:    
            this->ReceivedPck_OFP_FlowModPck_ACK(pk);
            break;
	  case FlowMod_CLOSE:    
            this->ReceivedPck_OFP_FlowModPck_CLOSE(pk);
            break;
	  default: ErrorMsg("OFP_FlowModPck type not known");
        }

        return;
    }
    
    //At Source node
    if (this==pk->SrcNode) {
	#ifdef STDOUT_PCEP_REQREP
	//for (int i=0; i<pk->FlowEntries.size();i++) 
	  //cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->FlowEntries[i]->ConnId << " pk id " << pk->Id << " created OFP_FlowModPck packet...sent to node " << pk->DstNode->Id << " TIME: " << this->ParentNet->Now << endl;
	#endif
	
	pk->ProcTime=PROC_OFP_FLOWMOD;

        SendPck(pk);
        return;
    }

    //At Intermediate nodes
    #ifdef STDOUT_PCEP_REQREP
    //for (int i=0; i<pk->FlowEntries.size();i++) 
      //cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->FlowEntries[i]->ConnId << " pk id " << pk->Id << " arrived OFP_FlowModPck packet...sent TIME: " << this->ParentNet->Now << endl;
    #endif

    SendPck(pk);
    return;
}
    
void PCx_Node::ReceivedPck_OFP_FlowModPck_ADD(OFP_FlowModPck* pk) {
  CLink* IncomingLink;
  
  #ifdef STDOUT_PCEP_REQREP
  cout << "NODE: " << Id << " (" << Name << "): OFP_FlowModPck [ADD] packet with FlowEntries: " << pk->FlowEntries.size() << " TIME: " << this->ParentNet->Now <<endl;
  for (int i=0; i<pk->FlowEntries.size();i++) {
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->FlowEntries[i]->ConnId << " pk id " << pk->Id << " OFP_FlowModPck [ADD] packet... arrived at destination TIME: " << this->ParentNet->Now << endl;
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->FlowEntries[i]->ConnId << " pk id " << pk->Id << " CrossConnection command inserted in data plane buffer TIME: " << this->ParentNet->Now << endl;
  }
  #endif
  
  //Configure the node physical layer
  
  for (int j=0; j<pk->FlowEntries.size(); j++) {
    int i=0;
    
    while ((this!=pk->FlowEntries[j]->ConnNodePath[i]) && (i<pk->FlowEntries[j]->ConnNodePath.size())) i++;
    
    if (i==pk->FlowEntries[j]->ConnNodePath.size()) {
      #ifdef STDOUT_PCEP_REQREP
      cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->FlowEntries[j]->ConnId << " this node is not traversed by this connection" << endl;
      #endif
      
      continue;
    }
  
    //If this is not the source node, reserve the spectrum on the incoming link...
    //In questo modo se un nodo non deve fare nulla per una certa connessione non fa niente
    if ((i!=0) && (i!=pk->FlowEntries[j]->ConnNodePath.size())) {
      IncomingLink=this->GetLinkFromNode(pk->FlowEntries[j]->ConnNodePath[i-1]);

      if (this->check_available_link_spectrum(pk->FlowEntries[j]->base_slot,pk->FlowEntries[j]->width_slots,IncomingLink)) {
	this->reserve_link_spectrum(pk->FlowEntries[j]->base_slot,pk->FlowEntries[j]->width_slots,IncomingLink);
      }
      else {
	ErrorMsg("Reply with a OFP_FlowModPck_NACK packet --- Expected available resource is busy --- dalay_panic");
	
	//Reply with a OFP_FlowModPck_ERR packet
	//OFP_ErrorPck* err_pk=new OFP_ErrorPck(this,pk->SrcNode,128,128,this->ParentNet->Now,pk->Conn,pk->Session);
	//WritePckNodeEvent* eve=new WritePckNodeEvent(this,err_pk,this->ParentNet->Now);
	//this->ParentNet->genEvent(eve);
      }
    }
  }
  
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

void PCx_Node::ReceivedPck_OFP_FlowModPck_DEL(OFP_FlowModPck* pk) {
  CLink* IncomingLink;
  
  #ifdef STDOUT_PCEP_REQREP
  cout << "NODE: " << Id << " (" << Name << ")" << " pk id " << pk->Id << " OFP_FlowModPck [DEL] packet with FlowEntries: " << pk->FlowEntries.size() << " TIME: " << this->ParentNet->Now <<endl;
  for (int i=0; i<pk->FlowEntries.size();i++)
   cout << "NODE: " << Id << " (" << Name << "): conn id " <<pk->FlowEntries[i]->ConnId << " pk id " << pk->Id << " OFP_FlowModPck [DEL] packet... arrived at destination TIME: " << this->ParentNet->Now << endl;
  #endif
  
  //Remove configuration in the node physical layer
  
  for (int j=0; j<pk->FlowEntries.size(); j++) {
    int i=0;
    
    //If this is not the source node, release the spectrum on the incoming link
    while ((this!=pk->FlowEntries[j]->ConnNodePath[i]) && (i<pk->FlowEntries[j]->ConnNodePath.size())) i++;
    
    if (i==pk->FlowEntries[j]->ConnNodePath.size()) {
      #ifdef STDOUT
      cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->FlowEntries[j]->ConnId << " pk id " << pk->Id << " this node is not traversed by this connection" << endl;
      #endif
      
      continue;
    }
    
    if ((i!=0) && (i!=pk->FlowEntries[j]->ConnNodePath.size())) {
      IncomingLink=this->GetLinkFromNode(pk->FlowEntries[j]->ConnNodePath[i-1]);
      
      if (this->check_busy_link_spectrum(pk->FlowEntries[j]->base_slot,pk->FlowEntries[j]->width_slots,IncomingLink)) {
	this->release_link_spectrum(pk->FlowEntries[j]->base_slot,pk->FlowEntries[j]->width_slots,IncomingLink);
  
      //There is not reply in case of DEL
      }
      else {
	ErrorMsg("Expected busy resource is available --- dalay_panic");
      }
    }
  }
  
  delete pk;
  return;
}

void PCx_Node::ReceivedPck_OFP_FlowModPck_CLOSE(OFP_FlowModPck* pk) {
  #ifdef STDOUT_PCEP_REQREP
  for (int i=0; i<pk->FlowEntries.size();i++)
  cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->FlowEntries[i]->ConnId << " pk id " << pk->Id << " OFP_FlowModPck [CLOSE] packet... arrived at destination TIME: " << this->ParentNet->Now << endl;
  #endif
  
  pk->Session->close();
    
  delete pk->Session;
  
  delete pk;
  return;
}

void PCx_Node::ReceivedPck_OFP_FlowModPck_ACK(OFP_FlowModPck* pk) {
  if (this->Type==PCC) ErrorMsg("OFP_FlowModPck_ACK not addressed to the OPENFLOW_CONTROLLER");
  #ifdef STDOUT_PCEP_REQREP
  for (int i=0; i<pk->Session->Conns.size();i++)
    cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Session->Conns[i]->Id << " pk id " << pk->Id << " from node " << pk->SrcNode->Id << " OFP_FlowModPck [ACK] packet... arrived at destination TIME: " << this->ParentNet->Now << endl;
  #endif
  
  pk->Session->ReceivedReply(pk->SrcNode);
  
  //If this is the last reply close the session
  if (pk->Session->LastReply()) {
    #ifdef STDOUT_PCEP_REQREP
    for (int i=0; i<pk->Session->Conns.size();i++)
      cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->Session->Conns[i]->Id << " pk id " << pk->Id << " from node " << pk->SrcNode->Id << " OFP_FlowModPck [ACK] packet... last arrived at destination TIME: " << this->ParentNet->Now << endl;
    #endif
    
    pk->Session->close();
    
    delete pk->Session;
  }
  
  delete pk;
  return;
}


void PCx_Node::ReceivedPck_OFP_LightpathPckIN(OFP_LightpathPckIN* pk) {
    //TTL test
    if (TTL_expired(pk)) return;

    //At Destination node
    if (this==pk->DstNode) {
      
      #ifdef STDOUT_PCEP_REQREP
      for (int i=0; i<pk->Conns.size(); i++) {
        if (pk->RestorationFlag==0)
            cout<< "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnIds[i] << " OFP_LightpathPckIN packet (PROVISIONING)...received at destination TIME: " << this->ParentNet->Now << endl;
        if (pk->RestorationFlag==1)
            cout<< "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnIds[i] << " OFP_LightpathPckIN packet (RESTORATION)...received at destination TIME: " << this->ParentNet->Now << endl;
      
      }
      #endif
      
      this->Add_CPU_EVENT_PCE_RSA(pk); //packet has to be processed by the CPU
          
      return;
    }
    
    //At Source node
    if (this==pk->SrcNode) {
        #ifdef STDOUT_PCEP_REQREP
	for (int i=0; i<pk->Conns.size(); i++) 
	cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnIds[i] << " created OFP_LightpathPckIN packet...sent to node " << pk->DstNode->Id << " TIME: " << this->ParentNet->Now << endl;
	#endif
	
	pk->ProcTime=PROC_OFP_LIGHTPATH_IN;

        SendPck(pk);
        return;
    }

    //At Intermediate nodes simply forward
    #ifdef STDOUT_PCEP_REQREP
	for (int i=0; i<pk->Conns.size(); i++) 
	cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnIds[i] << " arrived OFP_LightpathPckIN packet...sent TIME: " << this->ParentNet->Now << endl;
    #endif

    SendPck(pk);
    return;
}

void PCx_Node::ReceivedPck_OFP_LightpathPckOUT(OFP_LightpathPckOUT* pk) {
  double SessionTime, ActualValue_TotSignTime, ActualValue_TotRestTime;
  CConnection* conn=pk->Conn;
  
    //TTL test
    if (TTL_expired(pk)) return;

    //At Destination node
    if (this==pk->DstNode) {
      if (pk->Conn->Status==ADMITTED_PATH_RESTORATION) {
	
        pk->Conn->Status=RESTORED;
	
        SessionTime=this->ParentNet->Now - pk->Conn->DisruptionTime;
        pk->Conn->RestorationTime=SessionTime;
	  
        #ifdef STDOUT
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " OFP_LightpathPckOUT packet...received at destination " << pk->DstNode->Id << " TIME: " << this->ParentNet->Now << endl;
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " RESTORED restoration time " << SessionTime << " TIME: " << this->ParentNet->Now << endl;
        #endif 
	
        ReleaseConnEvent* eve=new ReleaseConnEvent(conn->SourceNode,conn,this->ParentNet->Now + (pk->Conn->DurationTime-pk->Conn->ElapsedTime),1);
        ParentNet->genEvent(eve);
	
        //Statistic update
        if (this->ParentNet->ConnRestoredREP==0) {
            this->ParentNet->TotRestTimeREP=SessionTime;
        }
        else {
            ActualValue_TotRestTime=this->ParentNet->TotRestTimeREP;
            ActualValue_TotRestTime=(ActualValue_TotRestTime*(this->ParentNet->ConnRestoredREP-1) + SessionTime)/(this->ParentNet->ConnRestoredREP);
            this->ParentNet->TotRestTimeREP=ActualValue_TotRestTime;
        }
	
        if (SessionTime > this->ParentNet->MaxRestTimeREP) {
        this->ParentNet->MaxRestTimeREP = SessionTime;
        }	  
	
        delete pk;
        return;
      }

      pk->Conn->Status=ESTABLISHED;
      
      SessionTime=this->ParentNet->Now - pk->ConnGenerationTime;
      pk->Conn->SignalingTime=SessionTime;
      
      #ifdef STDOUT
      cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " OFP_LightpathPckOUT packet...received at destination " << pk->DstNode->Id << " TIME: " << this->ParentNet->Now << endl;
      cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " ESTABLISHED setup time " << SessionTime  << " domains: " << pk->Conn->controllers_sequence.size() << endl;
      #endif
      
      //The ReleaseConnEvent is placed at [pk->Conn->DurationTime] seconds after signaling conclusion.
      ReleaseConnEvent* eve=new ReleaseConnEvent(conn->SourceNode,conn,this->ParentNet->Now + conn->DurationTime,0);
      ParentNet->genEvent(eve);
  
      //Statistic update
      if (this->ParentNet->ConnEstablishedREP==0) {
        this->ParentNet->TotSignTimeREP=SessionTime;
      }
      else {
        ActualValue_TotSignTime=this->ParentNet->TotSignTimeREP;
        ActualValue_TotSignTime=(ActualValue_TotSignTime*(this->ParentNet->ConnEstablishedREP-1) + SessionTime)/(this->ParentNet->ConnEstablishedREP);
        this->ParentNet->TotSignTimeREP=ActualValue_TotSignTime;
      }
      
      delete pk;
      return;
    }
    
    //At Source node
    if (this==pk->SrcNode) {
	#ifdef STDOUT_PCEP_REQREP
        cout << "NODE: " << Id << " (" << Name << "): conn id " << pk->ConnId << " created OFP_LightpathPckOUT packet...sent to node " << pk->DstNode->Id << " TIME: " << this->ParentNet->Now << endl;
	#endif
	
        SendPck(pk);
        return;
    }

    //At Intermediate nodes simply forward
    SendPck(pk);
    return;
}


void PCx_Node::SNMP_reserve_path_resources(vector<LinkState*> database, vector<CLink*> UsedLinks, int base_slot, int width_slots, int conn_id) {
  int TableEntry;
  
  if (UsedLinks.size()==0) {
    WarningMsg("SNMP_reserve_path_resources::No links in the Path... dalay_panic");
    return;
  }

  //Reserve resources is the SNMP database
  for (int i=0; i<UsedLinks.size(); i++) {
    TableEntry=0;
    while (database[TableEntry]->Link!=UsedLinks[i]) TableEntry++;

    database[TableEntry]->InstallationTime=this->ParentNet->Now;
    
    for (int j=base_slot; j<base_slot+width_slots; j++) {
      switch (database[TableEntry]->LambdaStatus[j]) {
	case 0:
	  database[TableEntry]->FreeLambdas--;
	  database[TableEntry]->LambdaStatus[j]=1;
	  break;
	case 1:
	  database[TableEntry]->LambdaStatus[j]=2;
	  
	  cout << "NODE: " << Id << " (" << Name << "): conn id " << conn_id << " RESERVE path resources NOT ALIGNED (1->2) TIME: " << this->ParentNet->Now << endl;
	  WarningMsg("SNMP_LinkStateTable is not aligned in the PCE node: SNMP_reserve_path_resources (case 1)");
	  break;
	default:
	  //cout << "NODE: " << Id << " (" << Name << "): conn id " << conn_id << " RESERVE path resources NOT ALIGNED (2->?) TIME: " << this->ParentNet->Now << endl;
	  ErrorMsg("SNMP_LinkStateTable is not aligned in the PCE node: SNMP_reserve_path_resources (case default)");
      }
    }
  }
}

//Returns 0 if no resources are available on the path, 1 otherwise
int PCx_Node::SNMP_check_path_resources(vector<LinkState*> database, vector<CLink*> UsedLinks, int base_slot, int width_slots, int conn_id) {
  int TableEntry;
  
  if (UsedLinks.size()==0) {
    WarningMsg("SNMP_check_path_resources::No links in the Path... dalay_panic");
    return 0;
  }

  //Check resources in the SNMP database
  for (int i=0; i<UsedLinks.size(); i++) {
    TableEntry=0;
    while (database[TableEntry]->Link!=UsedLinks[i]) TableEntry++;
    
    for (int j=base_slot; j<base_slot+width_slots; j++) {
      switch (database[TableEntry]->LambdaStatus[j]) {
	case 0:
	  //resource is available
	  break;
	case 1:
	  //resource is not available
        return 0;
	  break;
    case 2: 
        ErrorMsg("TODO resource is in status 2... please check");
        break;
	default:
	  ErrorMsg("SNMP_LinkStateTable is not aligned in the PCE node: SNMP_reserve_path_resources (case default)");
      }
    }
  }
  
  return 1;
}

void PCx_Node::SNMP_release_path_resources(vector<LinkState*> database, vector<CLink*> UsedLinks, int base_slot, int width_slots, int conn_id) {
  int TableEntry;
  
  if (UsedLinks.size()==0)
    ErrorMsg("SNMP_release_path_resources::No links in the Path... dalay_panic");

  //In this case we have received a NotifyEstablished with ProActive, first action required is to cancel proactive reservation
  for (int i=0; i<UsedLinks.size(); i++) {
    TableEntry=0;
    while (database[TableEntry]->Link!=UsedLinks[i]) TableEntry++;
    
    for (int j=base_slot; j<base_slot+width_slots; j++) {
      switch (database[TableEntry]->LambdaStatus[j]) {
	case 0:
	  ErrorMsg("SNMP_LinkStateTable is not aligned in the PCE node: SNMP_release_path_resources");
	  break;
	case 1:
	  database[TableEntry]->FreeLambdas++;
	  database[TableEntry]->LambdaStatus[j]=0;
	  break;
	case 2: 
	  database[TableEntry]->LambdaStatus[j]=1;
	  
	  //cout << "NODE: " << Id << " (" << Name << "): conn id " << conn_id << " RELEASE path resources NOW ALIGNED (2->1) TIME: " << this->ParentNet->Now << endl;
	  //WarningMsg("SNMP_LinkStateTable is now aligned in the PCE node: SNMP_release_path_resources (case 2)");
	  break;
	default:
	  ErrorMsg("SNMP_LinkStateTable is not aligned in the PCE node: SNMP_release_path_resources (case default)");
      }
    }
  }
}

void PCx_Node:: SNMP_dump_database(vector<LinkState*> database) {
  LinkState* TableEntry;
  
  for (int i=0; i<database.size(); i++) {
    TableEntry=database[i]; 
    
    cout << "SNMP link id: " << TableEntry->Link->Id << " " << TableEntry->Link->FromNode->Id << "->" << TableEntry->Link->ToNode->Id << " freelambdas " << TableEntry->Link->FreeLambdas << endl; 
  }
}

void PCx_Node::Add_CPU_EVENT_PCE_RSA(PCEP_PCRepPck* pk) {
  
  #ifdef STDOUT
  cout << "PCE CPU NODE: " << this->Id << " (" << this->Name << "): conn id " <<pk->Conn->Id << " CPU queued path computation TIME: " << ParentNet->Now << endl;
  #endif
  
  this->NodeCPU_Buffer->Add(pk); 
      
  if (this->NodeCPU_Buffer->PckSize()==1) {
      
    //Place the next CPU Event
    CPU_Event* pc_eve = new CPU_Event(this, ParentNet->Now + ParentNet->pce_routing_time, CPU_EVENT_PCE_RSA);
    ParentNet->genEvent(pc_eve);
  }
  else {
      
    //Niente perché vuol dire che c'è già schedulato un CrossConnectionEvent, che quando arriverà si occuperà di piazzare il prossimo evento
  }
  return;
}

void PCx_Node::Add_CPU_EVENT_PCE_RSA(OFP_LightpathPckIN* pk) {
  
  #ifdef STDOUT
  for (int i=0; i<pk->Conns.size(); i++)
    cout << "PCE CPU NODE: " << this->Id << " (" << this->Name << "): conn id " <<pk->ConnIds[i] << " CPU queued path computation TIME: " << ParentNet->Now << endl;
  #endif
  
  this->NodeCPU_Buffer->Add(pk); 
      
  if (this->NodeCPU_Buffer->PckSize()==1) {
      
    //Place the next CPU Event
    CPU_Event* pc_eve = new CPU_Event(this, ParentNet->Now + ParentNet->pce_routing_time, CPU_EVENT_PCE_RSA);
    ParentNet->genEvent(pc_eve);
  }
  else {
      
    //Niente perché vuol dire che c'è già schedulato un CrossConnectionEvent, che quando arriverà si occuperà di piazzare il prossimo evento
  }
  return;
}


