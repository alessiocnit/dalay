#include "DataPlaneBuffer.h"

DataPlaneBuffer::DataPlaneBuffer(CNode* node):
        CBuffer::CBuffer(node) {
    this->Type=DATAPLANE_BUFFER;
}

DataPlaneBuffer::~DataPlaneBuffer(){

}

