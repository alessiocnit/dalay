#ifndef OSPF_LsaPck_H
#define OSPF_LsaPck_H

#include "CPck.h"
#include "CNode.h"

using namespace std;

class CPck;
class CNode;

class OSPF_LsaPck:public CPck {
public:

    CNode* Advertising_Node;  //Node generating the LSA
    CLink* Link_Flooded;
    long sequence_number;
    unsigned long free_lambdas;
    vector<int> LambdaStatus;

    OSPF_LsaPck();
    OSPF_LsaPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, LinkState* ls);
    OSPF_LsaPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, OSPF_LsaPck* pk);
    OSPF_LsaPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, CNode* AdvertisingNode, CLink* AdvertisedLink);
    ~OSPF_LsaPck();

    void LoadLsaFields(LinkState* ls);
    virtual void arrived(CNode* node);

};

#endif
