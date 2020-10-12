#include "ProcessedPckEvent.h"

#include "CNet.h"
#include "CPck.h"
#include "CNode.h"
#include "NodeBuffer.h"

ProcessedPckEvent::ProcessedPckEvent(CNode* node, CPck* pk, double time) {
    this->eventTime=time;
    this->eventType=PROCESSED_PCK;
    this->Comment="PROCESSED_PCK";

    this->AtNode=node;
    this->Pck=pk;
};

ProcessedPckEvent::~ProcessedPckEvent() {
};

void ProcessedPckEvent::execute() {
    double ProcessingTime;
    double Now=this->eventTime;
    CPck* current_pck;
    CPck* next_pck;

    current_pck = this->AtNode->InBuffer->RemoveNextPck();
    
    if (this->AtNode->InBuffer->PckSize()) {
        next_pck=this->AtNode->InBuffer->NextPck();

        ProcessingTime=next_pck->ProcTime;

        ProcessedPckEvent* p_eve=new ProcessedPckEvent(this->AtNode,next_pck,Now + ProcessingTime);
        this->AtNode->ParentNet->genEvent(p_eve);
    }

    this->Pck->arrived(AtNode);
};
