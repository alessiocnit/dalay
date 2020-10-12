#include "NodeBuffer.h"

NodeBuffer::NodeBuffer(CNode* node):
        CBuffer::CBuffer(node) {
    this->Type=NODE_BUFFER;
}

NodeBuffer::~NodeBuffer() {
}

