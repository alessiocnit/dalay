#ifndef PCEP_NOTIFYLINKSTATEPCK_H
#define PCEP_NOTIFYLINKSTATEPCK_H

#include "defs.h"
#include "CPck.h"

class CNode;
class CLink;
class OSPF_LsaPck;

class PCEP_NotifyLinkStatePck:public CPck {
  public:
    CNode* Advertising_Node;  //Node generating the LSA
    CLink* Link_Flooded;
    long sequence_number;
    unsigned long free_lambdas;
    vector<int> LambdaStatus;

    PCEP_NotifyLinkStatePck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, OSPF_LsaPck* lsa_pk);
    ~PCEP_NotifyLinkStatePck();

    virtual void arrived(CNode* node);
};

#endif // PCEP_NOTIFYLINKSTATEPCK_H
