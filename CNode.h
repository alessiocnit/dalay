#ifndef CNODE_H
#define CNODE_H

#include "CLink.h"

using namespace std;

enum NodeType {DEFAULT, PCC, PCC_PCE, PCC_PCE_HPCE};
enum NodeLocation {EDGE_DOMAIN, IN_DOMAIN};

class CNet;
class CLink;
class CNode;
class CDomain;
class NodeBuffer;
class DataPlaneBuffer;
class CPU_Buffer;
class CConnection;

class CPathTable;
class CPathState_Table;

class CPck;

class RSVP_PathPck;
class RSVP_ResvPck;
class RSVP_ErrPck;
class RSVP_TearDownPck;
class RSVP_PathErrPck;

class OSPF_LinkFailurePck;
class OSPF_LinkRepairPck;
class OSPF_LsaPck;

class PCEP_PCReqPck;
class PCEP_PCRepPck;
class PCEP_NotifyEstablishedPck;
class PCEP_NotifyBlockedPck;
class PCEP_NotifyReleasedPck;
class PCEP_NotifyLinkStatePck;
class PCEP_PCInitPck;
class PCEP_PCReportPck;

class OFP_FlowModPck;
class OFP_LightpathPckIN;
class OFP_LightpathPckOUT;

//Routing table contains for each network node: 1.Next hop node 2.Distance as number of hops
struct dBS {
    CNode* ToNode;
    CLink* Through_link;
    int    NHop;
};

struct link_dBS {
    CLink* LocalPointer;
    LinkStatus LocalStatus;
    vector<int> LocalLambdaStatus;
};

struct DestinationDomainNode_entry {
    CNode* 	ToNode;
    CNode* 	DstDomainNode;
    int 	TraversedDomains;
};

struct LinkState {
    CLink* Link;
    CNode* AdvertisingNode;
    unsigned long SequenceNumber;
    double InstallationTime;

    int FreeLambdas;
    LinkStatus Status;
    vector<int> LambdaStatus;
};

class CNode {
  public:
    int Id;
    string Name;
    NodeType Type;
    NodeLocation Location;

    CNet* ParentNet;
    CDomain* ParentDomain;
    CPathTable* PathTable;
    CPathState_Table* PathState_dB;

    //Node routing table
    vector<dBS> dB;

    //The buffer for incoming packets
    NodeBuffer* InBuffer;
    
    //The buffer for the interface toward data plane
    DataPlaneBuffer* DataBuffer;
    
    //The buffer for operation requiring CP utilization (Routing, Spectrum Assignment)
    CPU_Buffer* NodeCPU_Buffer;

    //Pointers to links originating at this node
    vector<CLink*> OutLink;
    vector<CLink*> InLink;

    //Database keeping the status of link incoming (outgoing) in (from) this node
    vector<link_dBS*> OutgoingLink_dB;
    vector<link_dBS*> IncomingLink_dB;

    //Link State database
    vector<LinkState*> LinkState_dB;

    //Pointers to connections originating and terminating at this node
    vector<CConnection*> OriginatedConns;
    vector<CConnection*> TerminatedConns;

    //Matrix containing for each node the destination node inside the domain
    vector<DestinationDomainNode_entry> Destination_DomainNode_dB;

    //Buffer containing the RSVP_PathPck waiting for a reply of the PCE
    vector<RSVP_PathPck*> Bufferized_PathPck;

    CNode();
    CNode(int id, string name,CNet* pn);
    virtual ~CNode();

    //Returns pointer to link between this node and node p. If direct connection does not exist it returns NULL
    CLink* GetLinkToNode(CNode* p);
    CLink* GetInterDomainLinkToNode(CNode* p);
    CLink* GetLinkFromNode(CNode* node);
    CLink* CreateNewLink(int id, CNode* ToNode, double length, int maxbyte, double bitrate, int metric);
    void AddNewLink(int id, CNode* ToNode, double length, int maxbyte, double bitrate, int metric);
    void RoutingTable_AddRecord(CNode* tonode, int nhop, CLink* thr);
    void Destination_DomainNode_Table_AddRecord(CNode* to_node, CNode* dst_node);
    void Fill_InLink();

    int TTL_expired(CPck* pk);
    int SendPck(CPck* pck);
    int SendPck(CPck* pck,CNode* TowardNode);

    void FailureDetected(CLink* link);
    void FailureDetected_OpenFlow(CLink* link);
    void RepairDetected(CLink* link);
    void RepairDetected_OpenFlow(CLink* link);

    void ConnEstablish(CConnection* conn, double StartTime);
    void ConnEstablish_IntermediateDomain(CConnection* conn, double StartTime);
    void GeneratePathErr_EdgeNode(CConnection* conn, double StartTime);

    void ConnRelease(CConnection* conn);
    void ConnStubRelease(CConnection* conn);
    int ConnectionAdmissionControl(CConnection* conn);
    int ConnectionAdmissionControl_IntermediateDomain(CConnection* conn, RSVP_PathPck* pk);
    int ConnectionDistributedRestoration(CConnection* conn);

    void CreateLinkDatabase();
    int SearchLocalLink(CLink* link, vector<link_dBS*> link_dB);
    int SearchLinkState(CLink* link);
    int ScanLinkDatabase();

    void ReceivedPck_GENERIC(CPck* pk);

    //RSVP-TE packets
    void ReceivedPck_RSVP_PATH(RSVP_PathPck* pk);
    void ReceivedPck_RSVP_PATH_ERR(RSVP_PathErrPck* pk);
    void ReceivedPck_RSVP_RESV(RSVP_ResvPck* pk);
    void ReceivedPck_RSVP_RESV_ERR(RSVP_ErrPck* pk);
    void ReceivedPck_RSVP_TEARDOWN(RSVP_TearDownPck* pk);

    //OSPF-TE packets
    void ReceivedPck_OSPF_LinkFailure(OSPF_LinkFailurePck* pk);
    void ReceivedPck_OSPF_LinkRepair(OSPF_LinkRepairPck* pk);
    void ReceivedPck_OSPF_LSA(OSPF_LsaPck* pk);

    //PCE packets
    virtual void ReceivedPck_PCEP_PCReq(PCEP_PCReqPck* pk)=0;
    virtual void ReceivedPck_PCEP_PCRep(PCEP_PCRepPck* pk)=0;
    virtual void ReceivedPck_PCEP_NotifyEstablished(PCEP_NotifyEstablishedPck* pk)=0;
    virtual void ReceivedPck_PCEP_NotifyBlocked(PCEP_NotifyBlockedPck* pk)=0;
    virtual void ReceivedPck_PCEP_NotifyReleased(PCEP_NotifyReleasedPck* pk)=0;
    virtual void ReceivedPck_PCEP_NotifyLinkState(PCEP_NotifyLinkStatePck* pk)=0;
    virtual void ReceivedPck_PCEP_PCInit(PCEP_PCInitPck* pk)=0;
    virtual void ReceivedPck_PCEP_PCReport(PCEP_PCReportPck* pk)=0;
    
    virtual void ReceivedPck_OFP_FlowModPck(OFP_FlowModPck* pk)=0;
    virtual void ReceivedPck_OFP_LightpathPckIN(OFP_LightpathPckIN* pk)=0;
    virtual void ReceivedPck_OFP_LightpathPckOUT(OFP_LightpathPckOUT* pk)=0;
    
    //WA locally executed at the connection destination node
    int Distributed_WavelengthAssignmentFF(RSVP_ResvPck* pk);
    int Distributed_WavelengthAssignmentLF(RSVP_ResvPck* pk);
    int Distributed_WavelengthAssignmentRANDOM(RSVP_ResvPck* pk);
    
    int Flexible_Distributed_WavelengthAssignmentFF(RSVP_ResvPck* pk);

    int LinkStateBased_WavelengthAssignmentRANDOM(RSVP_PathPck* pk);
    int LinkStateBased_WavelengthAssignmentFF(RSVP_PathPck* pk);

    void PathErrPckManage_ConnSourceNode_Provisioning(RSVP_PathErrPck* pk);
    void PathErrPckManage_ConnSourceNode_Restoration(RSVP_PathErrPck* pk);

    int Generate_LsaPck(CLink* AdvertisedLink);
    int LabelSet_Test(vector<int> LabelSet);

    void LabelSet_Update(RSVP_PathPck* pk, link_dBS* link);

    void Bufferize_PathPck(RSVP_PathPck* pk);
    RSVP_PathPck* DeBufferize_PathPck(CConnection* conn);

    void print();
    
    LinkState* Install_SelfOriginated_LinkState(OSPF_LsaPck* pk);
    LinkState* Install_Received_LinkState(OSPF_LsaPck* pk);
    
    //Flexible
    int check_available_link_spectrum (int base, int width, CLink* link);
    int check_busy_link_spectrum (int base, int width, CLink* link);
    
    void reserve_link_spectrum(int base, int width, CLink* link);
    void release_link_spectrum(int base, int width, CLink* link);
    
    void reserve_incoming_link_spectrum_db(int base, int width, CLink* link);
    void reserve_outgoing_link_spectrum_db(int base, int width, CLink* link);
    void release_incoming_link_spectrum_db(int base, int width, CLink* link);
    void release_outgoing_link_spectrum_db(int base, int width, CLink* link);
    
    void connection_memory_erasing(CConnection* conn);
    
    void Add_CPU_EVENT_SRC_RSA(RSVP_PathPck* pk);
};

#endif

