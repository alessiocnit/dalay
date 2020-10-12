#ifndef PCx_NODE_H
#define PCx_NODE_H

#include "defs.h"
#include "CNode.h"

using namespace std;

class PathComputationSession;
class FlowModeSession;

struct PathComputationRequest;

struct OngoingConn {
    int id;
    double RTT_pce;
    double RTT_sd;
    double RepliedTime;
    int Label;

    CNode* ConnSource;
    CNode* ConnDestination;

    vector<CLink*> pathlink;
};

class PCx_Node:public CNode {
public:
    int pce_wa_enabled;
    int hpce_wa_enabled;

    //SNMP Link State database
    vector<LinkState*> SNMP_LinkState_dB;

    //HPCE database
    vector<LinkState*> SNMP_HPCE_InterDomain_LinkState_dB;
    vector<LinkState*> BGPLS_HPCE_LinkState_dB;
    
    //HPCE database for stored requests
    vector<PathComputationRequest*> PCEP_StoredRequests;
    
    //DistributedComputation database
    vector<PathComputationSession*> DistributedComputationSession_dB;
    
    //OpenFlow controller database
    vector<FlowModeSession*> FlowModeSession_dB;

    PCx_Node(int id, string name, CNet* pn);
    virtual ~PCx_Node();

    void PCC_PCE_PathRequest(CNode* source,CConnection* conn);
    void PCE_PathRequest_Restoration(CConnection* conn);
    int PCE_ExpandEdgeDomainSequence(PCEP_PCRepPck* pk, vector<CNode*>* n, vector<CLink*>* l);
    int PCE_ExpandEdgeDomainSequence(PCEP_PCReqPck* pk, vector<CNode*>* n, vector<CLink*>* l);

    void HPCE_EdgeToDomain_SequenceConversion(vector<CNode*> pathnode, vector<CDomain*>* pathdomain);

    //Processing of PCEP PCReq packet types
    virtual void ReceivedPck_PCEP_PCReq(PCEP_PCReqPck* pk);
    void ReceivedPck_PCEP_PCReq_PCC_PCE(PCEP_PCReqPck* pk);
    void ReceivedPck_PCEP_PCReq_PCE_HPCE(PCEP_PCReqPck* pk);
    void ReceivedPck_PCEP_PCReq_HPCE_PCE(PCEP_PCReqPck* pk);
    void ReceivedPck_PCEP_PCReq_PCE_PCE(PCEP_PCReqPck* pk);

    //Processing of PCEP PCRep packet types
    virtual void ReceivedPck_PCEP_PCRep(PCEP_PCRepPck* pk);
    void ReceivedPck_PCEP_PCRep_PCE_PCC(PCEP_PCRepPck* pk);
    void ReceivedPck_PCEP_PCRep_HPCE_PCE(PCEP_PCRepPck* pk);
    void ReceivedPck_PCEP_PCRep_PCE_HPCE(PCEP_PCRepPck* pk);
    void ReceivedPck_PCEP_PCRep_PCE_PCE(PCEP_PCRepPck* pk);

    //Notify messages are used for updating the SNMP Link State database
    virtual void ReceivedPck_PCEP_NotifyEstablished(PCEP_NotifyEstablishedPck* pk);
    virtual void ReceivedPck_PCEP_NotifyBlocked(PCEP_NotifyBlockedPck* pk);
    virtual void ReceivedPck_PCEP_NotifyReleased(PCEP_NotifyReleasedPck* pk);
    virtual void ReceivedPck_PCEP_NotifyLinkState(PCEP_NotifyLinkStatePck* pk);
    virtual void ReceivedPck_PCEP_PCInit(PCEP_PCInitPck* pk);
    virtual void ReceivedPck_PCEP_PCReport(PCEP_PCReportPck* pk);
    
    //OpenFlow FlowMod messages
    virtual void ReceivedPck_OFP_FlowModPck(OFP_FlowModPck* pk);
    void ReceivedPck_OFP_FlowModPck_ADD(OFP_FlowModPck* pk);
    void ReceivedPck_OFP_FlowModPck_DEL(OFP_FlowModPck* pk);
    void ReceivedPck_OFP_FlowModPck_ACK(OFP_FlowModPck* pk);
    void ReceivedPck_OFP_FlowModPck_CLOSE(OFP_FlowModPck* pk);
    
    //OpenFlow messages
    virtual void ReceivedPck_OFP_LightpathPckIN(OFP_LightpathPckIN* pk);
    virtual void ReceivedPck_OFP_LightpathPckOUT(OFP_LightpathPckOUT* pk);
    
    //OpenFlow controller functions
    void ConnEstablish_OpenFlow(vector<CConnection*>& conns);
    void ConnRelease_OpenFlow(CConnection* conn);
    void StubRelease_OpenFlow(CConnection* conn);
    void ConnRestoration_OpenFlow(vector<CConnection*>& conns);
    void ConnRelease_OpenFlow_segment(CConnection* conn);
    
    void SNMP_reserve_path_resources(vector<LinkState*> database, vector<CLink*> UsedLinks, int base_slot, int width_slots, int conn_id);
    void SNMP_release_path_resources(vector<LinkState*> database, vector<CLink*> UsedLinks, int base_slot, int width_slots, int conn_id);
    void SNMP_dump_database(vector<LinkState*> database);
    int SNMP_check_path_resources(vector<LinkState*> database, vector<CLink*> UsedLinks, int base_slot, int width_slots, int conn_id);
    
    void Add_CPU_EVENT_PCE_RSA(PCEP_PCRepPck* pk);
    void Add_CPU_EVENT_PCE_RSA(OFP_LightpathPckIN* pk);
};

#endif

