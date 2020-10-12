#ifndef OSPF_LinkRepairPck_H
#define OSPF_LinkRepairPck_H

#include "CPck.h"
#include "CNode.h"

using namespace std;

class CPck;

class OSPF_LinkRepairPck:public CPck {
public:
    CLink* RepairedLink;

    OSPF_LinkRepairPck();
    OSPF_LinkRepairPck(CNode* src,CNode* dst,int dim,int ttl,double msgGenTime,CLink* link);

    ~OSPF_LinkRepairPck();

    virtual void arrived(CNode* node);
};

#endif

