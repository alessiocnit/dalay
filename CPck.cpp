#include "CPck.h"
#include "CNode.h"

CPck::CPck(CNode* src,CNode* dst,int dim,int ttl,double gen_time) {
    static int id=0;

    this->Id=id;
    this->Type=GENERIC;
    this->SrcNode=src;
    this->DstNode=dst;
    this->Dim=dim;
    this->TTL=ttl;

    id++;

    this->MsgGenTime=gen_time;

    this->ProcTime=0.1e-3;
    //cout << "Created GENERIC\n";
}

CPck::~CPck() {
    //cout << "Destroyed GENERIC\n";
}

void CPck::print() {
    cout << "Type = " << Type << endl;
    cout << "SrcNodeId = " << SrcNode->Id << endl;
    cout << "DstNodeId = " << DstNode->Id << endl;
    cout << "Dim = " << Dim << endl;
    cout << "TTL = " << TTL << endl;
    cout << "MsgGenTime = " << MsgGenTime << endl;
}

void CPck::arrived(CNode* node) {
    node->ReceivedPck_GENERIC(this);
}


