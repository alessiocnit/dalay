#include "CPU_Buffer.h"

CPU_Buffer::CPU_Buffer(CNode* node):
        CBuffer::CBuffer(node) {
    this->Type=CPU_BUFFER;
}

CPU_Buffer::~CPU_Buffer(){

}

