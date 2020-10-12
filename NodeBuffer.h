#ifndef NODEBUFFER_H
#define NODEBUFFER_H

#include "CBuffer.h"

using namespace std;

class CNode;

class NodeBuffer:public CBuffer {
public:
    CNode* ParentNode;

    NodeBuffer(CNode* node);
    ~NodeBuffer();
};

#endif
