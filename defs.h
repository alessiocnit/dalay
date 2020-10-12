#ifndef DEFS_H
#define DEFS_H

#include <list>
#include <deque>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

using namespace std;

//Simulation parameters (EXTERNAL loaded from command line, EUROPENET loaded in EurpeNet.cpp)
enum SimMode {PROV,REST}; //EUROPENET
enum Flood_Mode {OFF,AGGREGATED,DETAILED}; 
enum TieBreak {FF,LF,RANDOM,HYBRID}; 
enum Signaling_Mode {RSVP, PCEP_PPD, PCEP_BRPC, OPENFLOW};

//Multidomain simulation parameters - all EUROPENET
enum Domain_Architecture {NOPCE, PCE, OPENFLOW_CONTROLLER};
enum MultiDomain_Architecture {HPCE_CentrRouting, HPCE_DistrRouting};
enum MultiDomain_ResponseHPCE {DOMAIN_SEQUENCE, EDGE_SEQUENCE, NODE_SEQUENCE};
enum MultiDomain_ResponsePCE  {NOINFO, HOPCOUNT, BOTTLENECK, LABELSET}; 

//Other parameters
enum LinkStatus {UP,DOWN};
enum ConnectionType {INTRA_DOMAIN_CONN, INTER_DOMAIN_CONN, INTER_DOMAIN_CONN_ALIEN};
enum Block_Type {FORWARD,BACKWARD};
enum Block_SubType {COLLISION, EMPTY_LABELSET, LINK_DOWN, REMOTE_ROUTING};

enum SDN_Restoration_Type {IND,BUND};

enum HPCE_Type {NO_HPCE, STANDARD_HPCE, BGP_HPCE, PROACTIVE_HPCE};

enum FlowModeType {INTRA_DOMAIN_SESSION, INTER_DOMAIN_SESSION};

//Lightspeed in optical fibers n=1.5 [km/s]
#define LIGHTSPEED  2E5
//Maximum number of network nodes
#define NMAX 99
//Utilized as bound number
#define MAX 999999


#define NO_PATH  1
#define YES_PATH 0

#define ERRORS
#define WARNINGS

//#define FLOODING

#define FIXED_REQUESTS 10000
//#define FIXED_FAILURES 8

//Debugging print options
//#define STDOUT
//#define STDOUT_FLOODING
//#define STDOUT_CPATHTABLE
//#define STDOUT_PCEP_REQREP
//#define STDOUT_PCEP_NOTIFY

//#define MEMORY_DEBUG
//#define DEBUG
//#define LS_length

#define PROC_RSVP_PATH 2e-3
#define PROC_RSVP_RESV 2e-3
#define PROC_RSVP_PATHERR 0.1e-3
#define PROC_RSVP_RESVERR 2e-3
#define PROC_RSVP_PATHTEAR 2e-3
#define PROC_PCEP_OPEN_SESSION 0
#define PROC_PCEP_PCREQ 0.1e-3
#define PROC_PCEP_PCREP 0.1e-3
#define PROC_PCEP_NOTIFY 0.1e-3
#define PROC_PCEP_PCINIT 0.1e-3
#define PROC_PCEP_PCREPORT 0.1e-3

#define PROC_OSPF_LSA 2e-3
#define PROC_OSPF_LINKFAILURE 2e-3
#define PROC_OSPF_LINKREPAIR 2e-3

#define PROC_OFP_FLOWMOD 0.1e-3
#define PROC_OFP_LIGHTPATH_IN 0.1e-3
#define PROC_OFP_LIGHTPATH_OUT 0.1e-3

void ErrorMsg(string message);
void WarningMsg(string message);
void IntegerToString(int n, string* s);

#endif
