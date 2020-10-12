#include "LinkFailureEvent.h"

#include "defs.h"
#include "Statistics.h"

#include "CNet.h"
#include "CDomain.h"
#include "LinkRepairEvent.h"
#include "PCx_Node.h"
#include "CConnection.h"

//Node x and y have to be adjacent
LinkFailureEvent::LinkFailureEvent(CLink* l, double time) {
    this->eventTime=time;
    this->eventType=LINK_FAILURE;
    this->Comment="LINK_FAILURE";

    this->Node_1=l->FromNode;
    this->Node_2=l->ToNode;
    this->Link_1=l;
    this->Link_2=this->Node_2->GetLinkToNode(this->Node_1);
    this->ParentNet=this->Node_1->ParentNet;
    this->ParentDomain=this->Node_1->ParentDomain;
}

LinkFailureEvent::~LinkFailureEvent() {
};

void LinkFailureEvent::execute() {
    if (this->ParentNet->SimulationMode!=REST)
        ErrorMsg("LinkFailure event in PROV simulation mode");

    #ifdef FIXED_FAILURES
    if (this->ParentNet->FailureCounter==FIXED_FAILURES)
        this->ParentNet->SimEnd=1;
    #endif

    if (this->ParentNet->SimEnd==0) {
        #ifdef STDOUT
        cout << endl;
        cout << "LinkFailureEvent TIME: " << this->eventTime << " failure of link: " << this->Link_1->Id << ", " << this->Link_2->Id << " from " << this->Node_1->Id << " (" << this->Node_1->Name << ") to " << this->Node_2->Id << " (" << this->Node_2->Name << ") " << endl;
        cout << "LinkRepairEvent placed at TIME: " << this->eventTime+0.04*(this->ParentNet->TimeForRep) << endl;
        cout << "Next LinkFailureEvent placed at TIME: " << this->eventTime+this->ParentNet->TimeForRep << endl;
        #endif

        //Generate the RepairEvent
        LinkRepairEvent* r_eve=new LinkRepairEvent(this->Link_1, this->eventTime+0.04*(this->ParentNet->TimeForRep));
        ParentNet->genEvent(r_eve);

        //Generate the NEXT FailureEvent
        int NextFailureLink_ID;
        CLink* NextFailureLink;
	
        do {
            NextFailureLink_ID=randint(0,this->ParentNet->Link.size()-1,&this->ParentNet->LinkFailureSeed);
            NextFailureLink=this->ParentNet->GetLinkPtrFromLinkID(NextFailureLink_ID);
        } while (NextFailureLink->Type==INTER_DOMAIN);
	
        this->ParentNet->last_failure_generation_time=ParentNet->Now;
        this->ParentNet->FailureCounter++;
	
        LinkFailureEvent* f_eve=new LinkFailureEvent(NextFailureLink, this->eventTime+this->ParentNet->TimeForRep);
        ParentNet->genEvent(f_eve);
	
        if ((this->Link_1->Status==DOWN) || (this->Link_2->Status==DOWN)) 
            ErrorMsg("Double failure upon link");
	
        //Change the STATUS of the FAILED links
        this->Link_1->Status=DOWN;
        this->Link_2->Status=DOWN;
	
        //Computes the number of disrupted connections from the Link occupation...
        this->ParentNet->CentralizedConnDisrupted=0;
	
        for (int i=0; i<this->ParentNet->ActiveConns.size(); i++) {
	  
            CConnection* ActualConn=this->ParentNet->ActiveConns[i];
            //cout << i << " " << this->ParentNet->ActiveConns[i]->Id << endl;
            for (int j=0; j<ActualConn->LinkPath.size(); j++) { 
	    
                CLink* ActualLink=this->ParentNet->ActiveConns[i]->LinkPath[j];
                if ((ActualLink==this->Link_1) || (ActualLink==this->Link_2)) {
	      
                    this->ParentNet->CentralizedConnDisrupted++;
                    break;
                }
            }
        }
	
        #ifdef STDOUT
        cout << "Number of disrupted connections: " << this->ParentNet->CentralizedConnDisrupted << endl;
        #endif
	
        //Write the pointer to the faulted links
        this->ParentNet->DisruptedLinks.push_back(this->Link_1);
        this->ParentNet->DisruptedLinks.push_back(this->Link_2);

        int j;
        //Updating the local view in neighbour nodes, each nodes manages the IncomingLink
        j=this->Node_1->SearchLocalLink(this->Link_2,this->Node_1->IncomingLink_dB);
        this->Node_1->IncomingLink_dB[j]->LocalStatus=DOWN;
        j=this->Node_2->SearchLocalLink(this->Link_1,this->Node_2->IncomingLink_dB);
        this->Node_2->IncomingLink_dB[j]->LocalStatus=DOWN;

        //Updating the matrix
        for (int i=0; i<NMAX; i++)
            for (int j=0; j<NMAX; j++)
                G_l[i][j]=this->ParentNet->G[i][j];
            
        G_l[this->Node_1->Id][this->Node_2->Id]=0;
        G_l[this->Node_2->Id][this->Node_1->Id]=0;

        //Delete the Pathtables computed without failures
        delete this->ParentNet->PathTable;
        delete this->ParentDomain->PathTable;
        for (int i=0; i<this->ParentDomain->Node.size(); i++)
            delete this->ParentDomain->Node[i]->PathTable;
	
        //Compute the Pathtable with the failure
        CPathTable* PathTable_FAULTED_PCE = new CPathTable(this->ParentNet,this->Link_1,this->Link_2,this->ParentNet->RoutingHopThreshold_PCE,1);
        CPathTable* PathTable_FAULTED_CNET =new CPathTable(this->ParentNet,this->Link_1,this->Link_2,this->ParentNet->RoutingHopThreshold_CNet,1,1); 
	
        //Replace the Pathtable at the Domain PCE with the one computed after the failure
        this->ParentDomain->PathTable = PathTable_FAULTED_PCE;
	
        //Replace the Pathtable at the nodes in the domain with the one computed after the failure
        for (int i=0; i<this->ParentDomain->Node.size(); i++)
            this->ParentDomain->Node[i]->PathTable=new CPathTable(this->ParentNet, this->ParentDomain->Node[i], this->ParentDomain->PathTable);

        //Replace the Pathtable at CNet level with the one computed after the failure
        this->ParentNet->PathTable = PathTable_FAULTED_CNET;
	
        //Failure detection
        this->Node_1->FailureDetected_OpenFlow(this->Link_2);
        this->Node_2->FailureDetected_OpenFlow(this->Link_1);

        return;
    }

    if (ParentNet->SimEnd==1) return;
};
