#include "PCEP_PCRepPck.h"

#include "CNet.h"
#include "CDomain.h"
#include "PCx_Node.h"
#include "CConnection.h"

PCEP_PCRepPck::PCEP_PCRepPck(CNode* src,CNode* dst,int dim,int ttl,double msgGenTime,CConnection* conn,
                             vector<CNode*> node_path,
                             vector<CLink*> link_path,
                             vector<CNode*> edge_node_path,
                             vector<CDomain*> domain_path,
                             int SuggestedLabel,
                             int NoPath,
			     int rest_flag,
			     PCEP_RepType type):CPck::CPck(src,dst,dim,ttl,msgGenTime) {
	  
    this->Type=PCEP_PCRep;
    this->Comment="PCEP_PCRepPck";
    this->ProcTime=PROC_PCEP_PCREP;
    this->RepType=type;

    this->Conn=conn;
    this->ConnSource=conn->SourceNode;
    this->ConnDestination=conn->DestinationNode;
    this->NoPath=NoPath;
    this->RestorationFlag=rest_flag;

    //This builder should be used only when a path has been found
    if (this->NoPath)
        ErrorMsg("PCEP_PCRepPck wrong builder (builder with path)");

    this->NodePath=node_path; //NodeSequence
    this->LinkPath=link_path; //LinkSequence
    this->EdgeNodePath=edge_node_path; //Sequence of the edge nodes
    this->DomainPath=domain_path; //DomainSequance

    if (SuggestedLabel>=0) {
        this->SuggestedLabel_ON=1;
        this->SuggestedLabel=SuggestedLabel;
    }
    else {
        this->SuggestedLabel_ON=0;
        this->SuggestedLabel=-1;
    }
}

PCEP_PCRepPck::PCEP_PCRepPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, CConnection* conn,
			     int NoPath,
			     int rest_flag,
			     PCEP_RepType type):CPck::CPck(src,dst,dim,ttl,msgGenTime) {
			       
    this->Type=PCEP_PCRep;
    this->Comment="PCEP_PCRepPck";
    this->ProcTime=PROC_PCEP_PCREP;
    this->RepType=type;

    this->Conn=conn;
    this->ConnSource=conn->SourceNode;
    this->ConnDestination=conn->DestinationNode;
    this->NoPath=NoPath;
    this->RestorationFlag=rest_flag;
    
    //This builder should be used only when a path has NOT been found
    if (!this->NoPath)
        ErrorMsg("PCEP_PCRepPck wrong builder (builder NOPATH)");
}

PCEP_PCRepPck::~PCEP_PCRepPck() {
    //cout << "Destroyed PCEP_PCRep\n";
}

void PCEP_PCRepPck::arrived(CNode *node) {
    node->ReceivedPck_PCEP_PCRep(this);
}

