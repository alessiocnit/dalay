#ifndef CPCK_H
#define CPCK_H

#include "defs.h"

using namespace std;

class CNode;

enum PckType {
    GENERIC,

    RSVP_PATH,
    RSVP_PATH_ERR,
    RSVP_RESV,
    RSVP_RESV_ERR,
    RSVP_TEARDOWN,

    OSPF_LinkFailure,
    OSPF_LinkRepair,
    OSPF_LSA,

    PCEP_PCReq,
    PCEP_PCRep,
    PCEP_PCReq_H,
    PCEP_PCRep_H,
    PCEP_NotifyBlocked,
    PCEP_NotifyEstablished,
    PCEP_NotifyReleased,
    PCEP_NotifyLinkState,
    PCEP_PCInit,
    PCEP_PCReport,
    
    OFP_Lightpath_IN,
    OFP_Lightpath_OUT,
    OFP_FlowMod,
    OFP_Error
};

class CPck {
public:
    CNode*   SrcNode;
    CNode*   DstNode;

    PckType  Type;
    string   Comment;

    int Id;
    int Dim;		// dimension [byte]
    int TTL;
    double MsgGenTime;	// packet generation time

    double ProcTime;

    CPck() {};
    CPck(CPck* pck);
    CPck(CNode* src,CNode* dst,int dim,int ttl,double gen_time);
    virtual ~CPck();

    void print();
    virtual void arrived(CNode* node);
};

#endif
