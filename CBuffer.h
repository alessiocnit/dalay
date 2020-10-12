#ifndef CBUFFER_H
#define CBUFFER_H

#include "CPck.h"

using namespace std;

class CPck;
class CNode;

enum BufferType {NODE_BUFFER, LINK_BUFFER, DATAPLANE_BUFFER, CPU_BUFFER};

class CBuffer {
public:
    int TotalByte;
    int MaxByte; //Max queue size [byte]
    CNode* AtNode;

    BufferType Type;

    list<CPck*> ListPck; //The queue of packets

    CBuffer();
    CBuffer(CNode* AtNode);
    ~CBuffer();

    int PckSize();
    int ByteSize();
    void Add(CPck* pk);
    CPck* RemoveNextPck();
    CPck* NextPck();
};

#endif
