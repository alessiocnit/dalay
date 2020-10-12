#include "RSVP_PathPck.h"

#include "Statistics.h"
#include "CDomain.h"

#include "CNet.h"

RSVP_PathPck::~RSVP_PathPck() {
    //cout << "Destroyed RSVP_PATH\n";
}

RSVP_PathPck::RSVP_PathPck(CNode* src,CNode* dst,int dim, int ttl,double msgGenTime,CConnection* conn,int RestFlag):
        CPck::CPck(src,dst,dim,ttl,msgGenTime) {
    this->Type=RSVP_PATH;
    this->Comment="RSVP_PATH";

    this->RestorationFlag=RestFlag;
    this->Conn=conn;
    this->ConnType=conn->Type;

    this->ProcTime=PROC_RSVP_PATH;

    this->NodePath=conn->NodePath;
    this->LinkPath=conn->LinkPath;
    this->width_slots=conn->width_slots;

    this->LabelSet_ON=1;
    for (int i=0; i< src->ParentNet-> W; i++)
        this->LabelSet.push_back(1);

    this->SuggestedLabel_ON=0;
    this->SuggestedLabel=-1;
    
    if (RestFlag==0)
      this->SuggestedWA=this->SrcNode->ParentNet->Provisioning_DefaultPolicy;
    else
      this->SuggestedWA=this->SrcNode->ParentNet->Restoration_DefaultPolicy;

    if (this->SrcNode->ParentNet->Provisioning_DomainPCE==1) {
        if (conn->PCE_SuggestedLabel_ON) {
            this->SuggestedLabel_ON=1;
            this->SuggestedLabel=conn->PCE_SuggestedLabel;
        }
        else {
            this->SuggestedLabel_ON=0;
            this->SuggestedLabel=-1;
        }
    }

    this->ConnSourceNode=conn->SourceNode;
    this->ConnDestinationNode=conn->DestinationNode;
    
    return;
}

void RSVP_PathPck::arrived(CNode *node) {
    node->ReceivedPck_RSVP_PATH(this);
}
