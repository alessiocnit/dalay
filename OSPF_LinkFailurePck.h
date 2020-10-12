#ifndef OSPF_LinkFailurePck_H
#define OSPF_LinkFailurePck_H

#include "CPck.h"
#include "CNode.h"

using namespace std;

class CPck;

class OSPF_LinkFailurePck:public CPck {
public:
    CLink* DisruptedLink;
    double FailureDetectionTime;

    OSPF_LinkFailurePck();
    OSPF_LinkFailurePck(CNode* src,CNode* dst,int dim,int ttl,double msgGenTime,CLink* link);

    ~OSPF_LinkFailurePck();

    virtual void arrived(CNode* node);
};

#endif

