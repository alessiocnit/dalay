#include "LinkBuffer.h"

LinkBuffer::LinkBuffer(CNode* node, CLink* link):
        CBuffer::CBuffer(node) {
    this->Type=LINK_BUFFER;
    this->AtLink=link;
}

LinkBuffer::~LinkBuffer() {
    this->ListPck.clear();
}
