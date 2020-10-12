#include "DeliveredPckEvent.h"

#include "CPck.h"
#include "CLink.h"
#include "CNet.h"
#include "LinkBuffer.h"
#include "WritePckNodeEvent.h"

DeliveredPckEvent::DeliveredPckEvent(CNode* node,LinkBuffer* fb, double time) {
    this->eventTime=time;
    this->eventType=DELIVERED_PCK;
    this->Comment="DELIVERED_PCK";

    this->AtNode=node;
    this->FromBuffer=fb;
};

DeliveredPckEvent::~DeliveredPckEvent() {};

void DeliveredPckEvent::execute() {
    double Now=this->eventTime;
    CPck *DeliveredPck, *NextPck;
    CNode* DstNode=this->FromBuffer->AtLink->ToNode;
    CLink* Link=this->FromBuffer->AtLink;

    DeliveredPck=this->FromBuffer->NextPck();

    WritePckNodeEvent* r_eve=new WritePckNodeEvent(DstNode,DeliveredPck, Now + Link->PropagationDelay);
    this->AtNode->ParentNet->genEvent(r_eve);

    this->FromBuffer->RemoveNextPck();

    if (this->FromBuffer->PckSize()!=0) {
        NextPck=this->FromBuffer->NextPck();

        DeliveredPckEvent* d_eve=new DeliveredPckEvent(this->AtNode,this->FromBuffer,Now + NextPck->Dim*8/Link->Trx_Bitrate);
        this->AtNode->ParentNet->genEvent(d_eve);
    }
};


