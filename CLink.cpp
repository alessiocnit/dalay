#include "CLink.h"

#include "Statistics.h"
#include "CNet.h"
#include "CNode.h"
#include "LinkBuffer.h"

CLink::CLink(int id, double len, CNode* fromnode, CNode* tonode, double bitrate, int metric) {
    this->Id=id;
    this->Status=UP;
    this->Type=INTRA_DOMAIN;

    this->length=len;
    this->FromNode=fromnode;
    this->ToNode=tonode;
    this->ParentNet=this->FromNode->ParentNet;
    this->Trx_Bitrate=bitrate;
    this->LinkInterArrival=0;
    this->DistanceMetric=metric;
    this->FreeLambdas=this->ParentNet->W;

    //this->LSA_LastGenerationTime_FromNode=0;
    this->LSA_LastGenerationTime_FromNode=randint(0,this->ParentNet->OSPF_MinLSInterval,&this->ParentNet->OSPF_FirstGenerationSeed);
    this->LSA_LastSequenceNumber_FromNode=1;
    this->LSA_NextScheduled_FromNode=0;
    this->LSA_NextScheduledTime_FromNode=0;
    
    //this->LSA_LastGenerationTime_ToNode=0;
    this->LSA_LastGenerationTime_ToNode=randint(0,this->ParentNet->OSPF_MinLSInterval,&this->ParentNet->OSPF_FirstGenerationSeed);
    this->LSA_NextScheduled_ToNode=0;
    this->LSA_NextScheduledTime_ToNode=0;
    this->LSA_LastSequenceNumber_ToNode=1;

    this->PropagationDelay=len/LIGHTSPEED;
    //this->PropagationDelay=2.5e-3;

    for (int i=0;i<this->ParentNet->W;i++)
        this->LambdaStatus.push_back(0);
};

CLink::~CLink() {
    delete this->ControlPlane_Buffer;
};

void CLink::CreateBuffer(int MaxByte) {
    LinkBuffer* buffer=new LinkBuffer(this->FromNode,this);
    this->ControlPlane_Buffer=buffer;
};

void CLink::print() {
    cout << "\n-----------------------------------" << endl;
    cout << "Link id " << this->Id << " " << this->FromNode->Name << " - " << this->ToNode->Name << endl;
    cout << "Status: " << this->Status;
    cout << " LambdaStatus: " << flush;
    for (int i=0; i<this->LambdaStatus.size(); i++)
        cout << this->LambdaStatus[i] << " " << flush;
    cout << endl;
    cout << "-----------------------------------" << endl;
}
