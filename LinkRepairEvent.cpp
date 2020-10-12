#include "LinkRepairEvent.h"

#include "defs.h"

#include "CNet.h"
#include "CDomain.h"

//Node x and y have to be adjacent
LinkRepairEvent::LinkRepairEvent(CLink* l, double time) {
    this->eventTime=time;
    this->eventType=LINK_REPAIR;
    this->Comment="LINK_REPAIR";

    this->Node_1=l->FromNode;
    this->Node_2=l->ToNode;
    this->Link_1=l;
    this->Link_2=this->Node_2->GetLinkToNode(this->Node_1);
    this->ParentNet=this->Node_1->ParentNet;
    this->ParentDomain=this->Node_1->ParentDomain;
}

LinkRepairEvent::~LinkRepairEvent() {
};

void LinkRepairEvent::execute() {

    #ifdef STDOUT
    cout << "TIME: " << this->eventTime << " link repair (" << this->Node_1->Id << "," << this->Node_2->Id << ")" << endl;
    #endif

    string TopoName=this->ParentNet->Name;

    if ((this->Link_1->Status==UP) || (this->Link_2->Status==UP)) 
      ErrorMsg("Double repair upon link");
    
    this->Link_1->Status=UP;
    this->Link_2->Status=UP;

    this->ParentNet->DisruptedLinks.clear();

    int j;
    //Updating the local view in neighbour nodes, each nodes manages the IncomingLink
    j=this->Node_1->SearchLocalLink(this->Link_2,this->Node_1->IncomingLink_dB);
    this->Node_1->IncomingLink_dB[j]->LocalStatus=UP;
    j=this->Node_2->SearchLocalLink(this->Link_1,this->Node_2->IncomingLink_dB);
    this->Node_2->IncomingLink_dB[j]->LocalStatus=UP;
    

    //Delete the Pathtables computed without failures
    delete this->ParentNet->PathTable;
    delete this->ParentDomain->PathTable;
    for (int i=0; i<this->ParentDomain->Node.size(); i++) {
	  delete this->ParentDomain->Node[i]->PathTable;
    }
    
    //Compute the Pathtable with the failure
    CPathTable* Domain_PathTable=new CPathTable(this->ParentNet,this->ParentDomain,this->ParentNet->RoutingHopThreshold_PCE,0);
    CPathTable* CNet_PathTable=new CPathTable(this->ParentNet, this->ParentNet->RoutingHopThreshold_CNet,0);
	  
    //Replace the Pathtable at the Domain PCE with the one computed after the failure
    this->ParentDomain->PathTable=Domain_PathTable;
    
    //Replace the Pathtable at the nodes in the domain with the one computed after the failure
    for (int i=0; i<this->ParentDomain->Node.size(); i++) {
      this->ParentDomain->Node[i]->PathTable=new CPathTable(this->ParentNet,this->ParentDomain->Node[i],this->ParentDomain->PathTable);
    }
    //Replace the Pathtable at CNet level with the one computed after the failure
    this->ParentNet->PathTable=CNet_PathTable;
    
    this->Node_1->RepairDetected(this->Link_2);
    this->Node_2->RepairDetected(this->Link_1);
    
    return;
};

