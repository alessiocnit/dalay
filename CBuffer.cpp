#include "CBuffer.h"

#include "defs.h"
#include "CNet.h"
#include "DeliveredPckEvent.h"
#include "RSVP_PathPck.h"
#include "RSVP_ResvPck.h"

CBuffer::CBuffer() {}

CBuffer::CBuffer(CNode* node) {
    this->TotalByte=0;

    this->AtNode=node;
    this->MaxByte=128000; //up to now each packet is 128 bytes so the buffer is for 1000 packets
}

CBuffer::~CBuffer() {
}

int CBuffer::PckSize() {
    return this->ListPck.size();
}

int CBuffer::ByteSize() {
    return this->TotalByte;
}

void CBuffer::Add(CPck* pk) {
    this->ListPck.push_back(pk);
    this->TotalByte=this->TotalByte+pk->Dim;

    //cout << " BUFFER SIZE " << this->ListPck.size() << " TYPE: " << this->Type << endl;

    if (this->TotalByte > MaxByte) {
        cout << "Buffer overflow at node: " << this->AtNode->Name << " buffer type: " << this->Type << endl;
        cout << "PckSize: " << this->ListPck.size() << " ByteSize: " << this->ByteSize() << endl;
        int size=this->ListPck.size();
        for (int i=0; i<size; i++) {
            cout << this->ListPck.front()->Comment << " " << this->ListPck.front()->SrcNode->Name << endl;
            this->ListPck.pop_front();
        }
        ErrorMsg("CBuffer overflow... dalay_panic");
    }
}

CPck* CBuffer::NextPck() {
    return ListPck.front();
}

CPck* CBuffer::RemoveNextPck() {
    CPck* next_pck=this->NextPck();

    ListPck.pop_front();
    this->TotalByte=this->TotalByte - next_pck->Dim;

    if (this->TotalByte < 0) ErrorMsg("CBuffer negative overflow... dalay_panic");
    
    return next_pck;

    //cout << "NODE: " << this->AtNode->Name << " BUFFER packets: " << this->ListPck.size() << " type " << this->Type << endl;
}


