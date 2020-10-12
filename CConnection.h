#ifndef CCONNECTION_H
#define CCONNECTION_H

#include "defs.h"
#include "CPathTable.h"

class ReleaseConnEvent;
using namespace std;

enum ConnStatus {
    REQUESTED, 	//0

    REQUESTED_PATH,
    ADMITTED_PATH,	//2
    BLOCKED_PATH,	//3
    RELEASING,	//4
    RELEASED,	//5
    ESTABLISHED,	//6
    REFUSED,	//7

    DISRUPTED,			//8

    REQUESTED_PATH_RESTORATION,
    ADMITTED_PATH_RESTORATION,
    BLOCKED_PATH_RESTORATION,	//11
    REFUSED_RESTORATION,		//12
    RESTORED			//13
};

class CNet;
class CNode;
class CLink;
struct Path;

class CConnection {
public:
    int Id;
    ConnStatus Status;
    ConnectionType Type;
    
    static int active_cconnections;

    CNet*  Net;
    CNode* SourceNode;
    CNode* DestinationNode;

    //--- Inter-Domain connection are routed on multiple Path
    vector<Path*> ActualPath;

    //--- Paths during provisioning
    vector<Path*> PathsBlocked;

    vector<Path*> PathsBlocked_Forward;
    vector<Path*> PathsBlocked_EmptyLabelSet;
    vector<Path*> PathsBlocked_UnacceptableLabel;
    vector<Path*> PathsBlocked_RemoteRouting;

    vector<Path*> PathsBlocked_Backward;
    vector<Path*> PathsBlocked_Collision;
    vector<Path*> PathsBlocked_LinkDown;
    //---------------------------------------//

    //Paths during restoration
    vector<Path*> PathsBlocked_Restoration;

    vector<Path*> PathsBlocked_Forward_Restoration;
    vector<Path*> PathsBlocked_EmptyLabelSet_Restoration;

    vector<Path*> PathsBlocked_Backward_Restoration;
    vector<Path*> PathsBlocked_Collision_Restoration;
    vector<Path*> PathsBlocked_LinkDown_Restoration;
    //---------------------------------------//

    int ProvisioningAttempt;
    int RestorationAttempt;

    /*For both unidirectional and bidirectional LSPs*/
    vector<CNode*> NodePath;
    vector<CLink*> LinkPath;

    vector<CDomain*> DomainPath;
    vector<CNode*> EdgeNodePath;
    
    vector<PCx_Node*> controllers_sequence;

    int PCE_SuggestedLabel_ON;
    int PCE_SuggestedLabel;
    
    int SuggestedBaseSlot_ALIEN;
    
    /*This is used in multidomain networks where OEO conversion is applied at edge nodes*/
    vector<int> base_slot_path;
    vector<int> width_slots_path;

    ReleaseConnEvent* ConnRelease;

    //For flexible connection
    int base_slot;
    int width_slots;

    double GenerationTime;
    double DurationTime;
    double ElapsedTime;
    double DisruptionTime;

    double SignalingTime;
    double RestorationTime;

    int OldLabel;
    
    //If 0 routing has been not performed locally. If 1 routing has been performed locally
    int RoutingFlag; 

    CConnection(int id, CNet* net);
    CConnection(int id, CNet* net, double time, double duration, CNode* source, CNode* dest);
    CConnection(int id, CNet* net, CNode* source, CNode* dest);
    ~CConnection();

    void print();
};

#endif
