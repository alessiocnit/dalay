#ifndef CNET_H
#define CNET_H

#include "defs.h"
#include "CNode.h"
#include "CEvent.h"
#include "BHTree.h"
#include "CPathTable.h"
#include "CStat.h"

using namespace std;

class BHTree;
class CConnection;
class CNode;
class PCx_Node;
class CLink;
class CPathTable;
class CDomain;

enum EnumTrafficType {STATISTIC, STATIC};
enum MultiDomainInterconnection {OEO, CONTINUITY};

class CNet  {
public:
    int W;
    int SimEnd;
    int Experiment_ID;
    
    double Now;
    double MeanInterarrivalTime, MeanDurationTime;
    
    BHTree* Tree;

    char Name[128];
    string NameS;

    ofstream LogFile;

    vector<CDomain*> Domain;
    vector<CNode*> Node;
    vector<CLink*> Link;

    vector<CNode*> EdgeNode;
    vector<CLink*> InterDomain_Link;

    vector<CConnection*> ActiveConns;

    vector<CLink*> DisruptedLinks;
    
    //Seeds used in randint functions
    long InterarrivalSeed, DurationSeed, SrcDstSeed, WavelengthAssignmentSeed, PathSelectionSeed, LinkFailureSeed, OSPF_FirstGenerationSeed,RequiredSlotsSeed;
    
    int PckBlocked;
    
    //Topology matrices
    int G[NMAX][NMAX];//Physical Topology Matrix
    int D[NMAX][NMAX];//Domain Topology Matrix
    int F[NMAX][NMAX];//Domain Topology Matrix with EdgeNodes --- This matrix is correctly created but currently not used
    int H[NMAX][NMAX];//Logic topology with EdgeNodes and one logic node per domain

    //Traffic matrix: if the file is not found an uniform traffic matrix is assumed
    EnumTrafficType TrafficType;
    
    //Lambda continuity is applied traversing domains (CONTINUITY) or not (OEO)
    MultiDomainInterconnection multi_domain_interconnection;
    
    int TrafficMatrix_File;
    int TM[NMAX][NMAX], Sum_TM;
    
    //RoutingHopThresholds (number of hop accepted above the shortest)
    //CNet->HPCE_PathTable is always built using only shortest path on the logical topology
    int RoutingHopThreshold_CNet; //For building CNet->PathTable
    int RoutingHopThreshold_PCC;  //For building CNode->PathTable
    int RoutingHopThreshold_PCE;  //For building CDomain->PathTable
    int RoutingHopThreshold_HPCE; //For choosing among built paths in remote computing 
    
    SDN_Restoration_Type SDN_RestorationType;

    //RoutingMode at pcc,pce,hpce
    Flood_Mode RoutingMode_PCC;
    Flood_Mode RoutingMode_PCE;
    Flood_Mode RoutingMode_HPCE;
    
    MultiDomain_ResponsePCE ResponseMode_HPCE;
    Signaling_Mode SignalingMode;
    Domain_Architecture DomainArchitecture;

    //--- Distribution Statistics ------------------------------------------------//
    //----------------------------------------------------------------------------//
    CStat* InterArrivalTime;
    CStat* HoldingTime;
    CStat* TableSize;
    CStat* LabelSetSize;

    //--- Connection Statistics during provisioning -----------------------------//
    //----------------------------------------------------------------------------//
    int ConnRequested, ConnRequestedREP;
    int ConnAdmittedPath, ConnAdmittedPathREP;
    int ConnRefused, ConnRefusedREP;
    int ConnEstablished, ConnEstablishedREP;
    int ConnReleased;
    
    //--- Blocking modes
    int ConnBlocked, ConnBlockedREP;
    int ConnBlockedRouting, ConnBlockedRoutingREP;
    int ConnBlockedForward, ConnBlockedForwardREP;
    int ConnBlockedBackward, ConnBlockedBackwardREP;
    
    //int ConnIntraBlocked, ConnIntraBlockedREP;
    //int ConnInterBlocked, ConnInterBlockedREP;
    //int ConnBlockedMaxAttempt, ConnBlockedMaxAttemptREP;
    
    //----- Forward blocking modes
    int ConnBlocked_EmptyLabelSet;
    
    //int ConnBlocked_UnaccetableLabel;
    //int ConnBlocked_RemoteRouting;
    //----- Backward blocking modes
    int ConnBlocked_Collision;
    int ConnBlocked_LinkDown;
    
    //----- Statistics for multiple provisioning attempts
    vector<int> 
    ConnRequestedATT,ConnRequestedATT_REP,
    ConnAdmittedPathATT,ConnAdmittedPathATT_REP,
    ConnEstablishedATT,ConnEstablishedATT_REP,
    ConnBlockedATT,ConnBlockedATT_REP,
    ConnBlockedRoutingATT,ConnBlockedRoutingATT_REP,
    ConnBlockedForwardATT,ConnBlockedForwardATT_REP,
    ConnBlockedBackwardATT,ConnBlockedBackwardATT_REP;
    
    //--- Connection Statistics during restoration -------------------------------//
    //----------------------------------------------------------------------------//
    int CentralizedConnDisrupted; //Number of disrupted conns... computed in LinkFailureEvent

    int ConnDisrupted, ConnDisruptedREP;
    int ConnAdmittedPath_Restoration, ConnAdmittedPath_RestorationREP;
    int ConnRestored, ConnRestoredREP;
    
    //----- Refusing modes
    int ConnRefused_Restoration, ConnRefused_RestorationREP;
    int ConnRefusedRouting_Restoration, ConnRefusedRouting_RestorationREP;
    int ConnRefusedMaxAtt_Restoration, ConnRefusedMaxAtt_RestorationREP;
    
    //----- Blocking modes
    int ConnBlocked_Restoration, ConnBlocked_RestorationREP;
    int ConnBlockedForward_Restoration, ConnBlockedForward_RestorationREP;
    int ConnBlockedBackward_Restoration, ConnBlockedBackward_RestorationREP;
    
    //----- Forward blocking modes
    
    int ConnBlocked_EmptyLabelSet_Restoration;
    //int ConnBlocked_UnaccetableLabel_Restoration;
    
    //----- Backward blocking modes
    int ConnBlocked_Collision_Restoration;
    int ConnBlocked_LinkDown_Restoration;

    //----- Statistics for multiple restoration attempts
    vector<int> 
    ConnBlocked_RestorationATT, ConnBlocked_RestorationATT_REP,
    ConnRefusedRouting_RestorationATT, ConnRefusedRouting_RestorationATT_REP,
    ConnBlockedForward_RestorationATT, ConnBlockedForward_RestorationATT_REP,
    ConnBlockedBackward_RestorationATT, ConnBlockedBackward_RestorationATT_REP,
    ConnRestoredATT, ConnRestoredATT_REP;
    //----------------------------------------------------------------------------//

    //--- Packets Statistics -----------------------------------------------------//
    //----------------------------------------------------------------------------//
    long int RSVP_PckCounter, RSVP_PckCounterREP;
    long int OSPF_PckCounter, OSPF_PckCounterREP;
    long int PCEP_PckCounter, PCEP_PckCounterREP;
    long int OFP_PckCounter, OFP_PckCounterREP;
    long int TOT_PckCounter, TOT_PckCounterREP;
    long int TOT_ControllerPckCounter, TOT_ControllerPckCounterREP;
    

    vector<double> RSVP_NodeRate, OSPF_NodeRate, PCEP_NodeRate, OFP_NodeRate, TOT_NodeRate, TOT_ControllerNodeRate;
    //----------------------------------------------------------------------------//

    double TimeStartREP;
    double TimeForRep;

    int GeneratedLSA, ReceivedLSA, ForwardedLSA, DroppedLSA, ActiveLSA;
    int SelfOriginatedLSA, DuplicateLSA, TooFrequentLSA, OutOfDateLSA;//Types of dropped LSAs
    int PostponedLSA, ImmediateLSA;//Types of generated LSAs

    double OSPF_MinLSInterval;
    double OSPF_MinLSArrival;
    
    double distr_routing_time;
    double distr_wa_time;
    double pce_routing_time;
    double pce_wa_time;
    double oxc_crossconnection_time;
    
    vector<double> GeneralBlock;
    vector<double> ForwardBlock;
    vector<double> BackwardBlock;
    vector<double> RoutingBlock;

    vector<double> InterDomainBlock;
    vector<double> IntraDomainBlock;

    vector<double> BackwardBlock_Restoration;
    vector<double> ForwardBlock_Restoration;
    vector<double> GeneralBlock_Restoration;
    vector<double> RoutingBlock_Restoration;

    vector< vector<double> > GeneralBlockATT;
    vector< vector<double> > ForwardBlockATT;
    vector< vector<double> > BackwardBlockATT;
    vector< vector<double> > RoutingBlockATT;
    
    vector< vector<double> > GeneralBlockATT_Restoration;

    int ProvisioningAttempt_MAX; //Numero massimo di tentativi di segnalazione durante Provisioning
    int RestorationAttempt_MAX;  //Numero massimo di tentativi di segnalazione durante Restoration

    TieBreak Provisioning_DefaultPolicy;
    TieBreak Restoration_DefaultPolicy;

    SimMode SimulationMode;
    int SmartRestorationMapping;

    int Provisioning_DomainPCE, HPCE_DistributedComputing; 
    int RestorationPCE;
    int HPCE_NodeId;
    MultiDomain_ResponseHPCE HPCE_SimulationMode;
    PCx_Node* HPCE_Node;
    double HPCE_TimeThreshold;
    
    HPCE_Type TypeHPCE;

    //If ProactivePCE is 1 PCE considers lambda as busy after path computation
    int ProactivePCE;
    int PCEP_NotifyMessages;

    Flood_Mode FloodingMode;

    double last_failure_generation_time;
    int FailureCounter;
    
    int InterDomainLink_HopCount;

    //Collecting statistics on provisioning and restoration signaling time
    vector<double> TotSignTime;
    vector<double> TotRestTime;
    vector<double> MaxRestTime;
    
    double TotSignTimeREP;
    double TotRestTimeREP;
    double MaxRestTimeREP;
    
    vector<double> TotSignTime_1;
    vector<double> TotSignTime_2;
    vector<double> TotSignTime_3;
    vector<double> TotSignTime_4;
    vector<double> TotSignTime_5;
    
    double TotSignTimeREP_1;
    double TotSignTimeREP_2;
    double TotSignTimeREP_3;
    double TotSignTimeREP_4;
    double TotSignTimeREP_5;
    
    vector< vector<double> > TotRestTimeATT;
    
    vector<double> TotRestTimeATT_REP;
    //--------------------------------------------------------------------
    
    CPathTable* PathTable;
    CPathTable* HPCE_PathTable;

    CNet();
    ~CNet();

    CNode* CreateNode(int id, string name);

    CDomain* CreateDomain(int id, string name, int pce_id, int* node_list);
    CDomain* GetDomainPtrFromDomainID(int ID);

    CNode* GetNodePtrFromNodeID(int ID);
    CLink* GetLinkPtrFromLinkID(int ID);
    CLink* GetLinkPtrFromNodesID(int a, int b);
    CLink* GetLinkPtrFromNodes(CNode* s, CNode* d);
    void execute();

    void genEvent(CEvent* eve);
    int dijkstra(int G[NMAX][NMAX], int nodi, int x, int y, vector<int>* path);
    int DijkstraDistance(CNode* s, int DistanceTree[]);
    int DijkstraDistance_HPCE(int s, int DistanceTree[]);

    void NetworkLevel_RoutingScheme();
    void DomainLevel_RoutingScheme();
    void InterDomain_RoutingScheme();

    int ConfidenceInterval(int MinRep, int MaxRep, int ci, int cl, vector<double>, double *mu, double *sigma2, double *CI);
    void FileTopologyBuilder(int G[NMAX][NMAX], string TopoPath);
    int EqualLink(vector<int> path1, vector<int> path2);
    int EqualPath(vector<CNode*> path1, vector<CNode*> path2);
    CNode* ForkPath(vector<CNode*> path, vector<CNode*> shortest);
    
    //Used to print the input data
    void print_inputs();
};

#endif
