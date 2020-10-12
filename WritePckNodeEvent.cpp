#include "WritePckNodeEvent.h"

#include "CNet.h"
#include "CPck.h"
#include "CNode.h"
#include "PCx_Node.h"
#include "CLink.h"
#include "CDomain.h"
#include "NodeBuffer.h"
#include "ProcessedPckEvent.h"

WritePckNodeEvent::WritePckNodeEvent(CNode* node, CPck* pk, double time) {
    this->eventType=WRITE_PCK_NODE;
    this->eventTime=time;
    this->Comment="WRITE_PCK_NODE";

    this->AtNode=node;
    this->InBuffer=node->InBuffer;
    this->Pck=pk;
    
    //cout << "Building WritePckNodeEvent pk id " << pk->Id << " TIME " << this->AtNode->ParentNet->Now << endl;
}

WritePckNodeEvent::~WritePckNodeEvent() {
}

void WritePckNodeEvent::execute() {
    double ProcessingTime, Now=this->AtNode->ParentNet->Now;
    CNet* net=this->AtNode->ParentNet;

    net->TOT_PckCounter++;
    net->TOT_PckCounterREP++;
    
    //cout << "Executing WritePckNodeEvent pk id " << this->Pck->Id << " TIME " << this->AtNode->ParentNet->Now << endl;
    
    if (this->AtNode->Id==this->AtNode->ParentNet->HPCE_NodeId) {
      //if (this->Pck->Type==OFP_FlowMod) cout << "OFP packet at controller id " << this->Pck->Id << endl;
      
      net->TOT_ControllerPckCounter++;
      net->TOT_ControllerPckCounterREP++;
    }

    switch (this->Pck->Type) {
    case GENERIC:
        break;
    case RSVP_PATH:
    case RSVP_PATH_ERR:
    case RSVP_RESV:
    case RSVP_RESV_ERR:
    case RSVP_TEARDOWN:
        net->RSVP_PckCounter++;
        net->RSVP_PckCounterREP++;
        break;
    case OSPF_LinkFailure:
    case OSPF_LinkRepair:
    case OSPF_LSA:
        net->OSPF_PckCounter++;
        net->OSPF_PckCounterREP++;
        break;
    case PCEP_PCReq:
    case PCEP_PCRep:
    case PCEP_NotifyBlocked:
    case PCEP_NotifyEstablished:
    case PCEP_NotifyReleased:
    case PCEP_NotifyLinkState:
    case PCEP_PCInit:
    case PCEP_PCReport:
        net->PCEP_PckCounter++;
        net->PCEP_PckCounterREP++;
        break;
    case OFP_FlowMod: 
    case OFP_Error:
    case OFP_Lightpath_IN:
    case OFP_Lightpath_OUT:
        net->OFP_PckCounter++;
        net->OFP_PckCounterREP++;
        break;
    default:
        ErrorMsg("WritePckNodeEvent: Packet type not supported");
    }
    
    this->InBuffer->Add(this->Pck);
    
    //cout << "Executing WritePckNodeEvent pk id " << this->Pck->Id << " buffer size " << this->InBuffer->ListPck.size() << endl;

    if (this->InBuffer->ListPck.size()==1) {
        ProcessingTime=this->Pck->ProcTime;

        ProcessedPckEvent* p_eve=new ProcessedPckEvent(this->AtNode,this->Pck,Now + ProcessingTime);
        this->AtNode->ParentNet->genEvent(p_eve);
    }
    else {
        //If there are at least two packets the next ProcessedPckEvent will place the ProcessedPckEvent for the current packet
    }
}

