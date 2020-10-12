#ifndef LINKBUFFER_H
#define LINKBUFFER_H

#include "CBuffer.h"

using namespace std;

class CLink;
//class CBuffer;

class LinkBuffer:public CBuffer {
public:
    CLink*	AtLink;

    LinkBuffer(CNode* node, CLink* link);
    ~LinkBuffer();
};

#endif

