#include "defs.h"

#include "CNet.h"
#include "CDomain.h"
#include "CTopology.h"
#include "CTraffic.h"
#include "PCx_Node.h"
#include "Statistics.h"

//Required only for MEMORY DEBUG
#include "CConnection.h"
#include "FlowModeSession.h"
#include "OFP_ErrorPck.h"
#include "OFP_FlowModPck.h"
#include "OFP_LightpathPckIN.h"
#include "OFP_LightpathPckOUT.h"

using namespace std;

class CNet;
class CPathTable;

int CConnection::active_cconnections;
int FlowModeSession::active_flowmodesessions;
int OFP_ErrorPck::active_pk;
int OFP_FlowModPck::active_pk;
int OFP_LightpathPckIN::active_pk;
int OFP_LightpathPckOUT::active_pk;

void input_data(CNet *net);

int main (int argc, char* argv[]) {

    if (argc!=7) {
      cout << "Input parameters they should be:" << endl;
      cout << " 1. Network topology (STRING: use the string EuroD if the file is EuroD.topology)" << endl;
      cout << " 2. Number of slots (INTEGER: typical values are 32,40,80,320)" << endl;
      cout << " 3. Interarrival time (DOUBLE: mean interarrival time expressed in seconds)" << endl;
      cout << " 4. Holding time (DOUBLE: mean holding time expressed in seconds)" << endl;
      cout << " 5. Routing time (DOUBLE expressed in milli-seconds)" << endl;
      cout << " 6. OXC cross connection time (DOUBLE expressed in milli-seconds)" << endl;
      ErrorMsg("Six parameters are necessary...");
    }

    cout << endl;
    cout << "-----------------------------------------" << endl;
    cout << "ENTERING DALAY OBJECT DRIVEN SIMULATOR..." << endl;
    cout << "-----------------------------------------" << endl;
    cout << endl;

    cout << "Initializing network structures..." << endl;
    CNet* net=new CNet();
    cout << "[ok]" << endl << endl;

    // ---------------------------------------------------------------- //
    //Parameters for JOCN OpenFlow
    net->DomainArchitecture=OPENFLOW_CONTROLLER; //enum {NOPCE,PCE,OPENFLOW_CONTROLLER}
    net->SDN_RestorationType=IND; //enum{IND,BUND}
    // ---------------------------------------------------------------- //
    
    //net->multi_domain_interconnection = OEO;
    net->multi_domain_interconnection = CONTINUITY; //used for alien wavelength
    
    //OSPF simulation parameters
    net->Provisioning_DomainPCE=1; 	//PCE is asked during provisioning
    net->RestorationPCE=1;		//PCE is asked during restoration
    
    net->PCEP_NotifyMessages=0;		//Enable generation of PCEP notifications to the PC
    net->ProactivePCE=0; 		//PCE uses ProActive method to update local SNMP database
      
    net->ProvisioningAttempt_MAX=1;
    net->RestorationAttempt_MAX=0;
    net->SmartRestorationMapping=0;
      
    net->FloodingMode=DETAILED; //enum {OFF, AGGREGATED, DETAILED}
    net->OSPF_MinLSInterval=5; //Typical values {5 seconds, 30 seconds}
    
    CConnection::active_cconnections=0;
    FlowModeSession::active_flowmodesessions=0;
    OFP_ErrorPck::active_pk=0;
    OFP_FlowModPck::active_pk=0;
    OFP_LightpathPckIN::active_pk=0;
    OFP_LightpathPckOUT::active_pk=0;
    
    //Command line parameters
    net->NameS=argv[1]; strcpy(net->Name,argv[1]);
    net->W=atoi(argv[2]);
    net->MeanInterarrivalTime=atof(argv[3]);
    net->MeanDurationTime=atof(argv[4]);
    net->pce_routing_time=atof(argv[5])/1000;
    net->distr_routing_time=atof(argv[5])/1000;
    net->oxc_crossconnection_time=atof(argv[6])/1000;

    //General simulation parameters
    net->SimulationMode=PROV; //enum {PROV, REST}

    //Traffic simulation parameters, if 0 matrix is considered uniform otherwise it is loaded from "traffic_matrix.in"
    net->TrafficMatrix_File=0;
    
    net->RoutingMode_PCC= DETAILED;
    net->RoutingMode_PCE= DETAILED;
    net->RoutingMode_HPCE=DETAILED;
    
    net->RoutingHopThreshold_CNet=1; //Paths are computed within RoutingHopThreshold hops from the shortest
    net->RoutingHopThreshold_PCC=0;
    net->RoutingHopThreshold_PCE=1;
    net->RoutingHopThreshold_HPCE=1;
    
    net->InterDomainLink_HopCount=100; //Cost of interdomain links
    
    //Wavelength assignment simulation parameters
    net->Provisioning_DefaultPolicy=FF; //enum {FF, LF, RANDOM}
    net->Restoration_DefaultPolicy=FF;
    
    //PCE simulation parameters
    //net->Provisioning_DomainPCE=0;
    //net->RestorationPCE=0;
    //net->PCEP_NotifyMessages=0; //Enable generation of PCEP notifications to the PCE
    //net->ProactivePCE=0; //The PCE uses ProActive method to update local SNMP database 
    
    //HPCE simulation parameters
    net->TypeHPCE=PROACTIVE_HPCE; //{NO_HPCE, STANDARD_HPCE, BGP_HPCE, PROACTIVE_HPCE}
    net->HPCE_NodeId=5; //For EuroD topology node 5 is Frankfurt
    net->HPCE_SimulationMode=EDGE_SEQUENCE; //enum {DOMAIN_SEQUENCE,EDGE_SEQUENCE}
    net->HPCE_TimeThreshold=0.0;
    net->ResponseMode_HPCE=LABELSET; //enum ResponseMode {NOINFO,HOPCOUNT,BOTTLENECK,LABELSET};
    
    cout << "Loading topology..." << endl; //The file .topology must be included in the src folder
    CTopology* InputTopo;
    InputTopo=new CTopology(net);
    InputTopo->load();
    cout << "[ok]" << endl << endl;
    
    //This option can be activated only with STANDARD_HPCE. If it is 0 hpce_wa_enabled must be set to 0
    net->HPCE_DistributedComputing=0;
    net->HPCE_Node->hpce_wa_enabled=0;
    
    //Multiple signaling attemps during provisioning and restoration
    net->ProvisioningAttempt_MAX=1;
    net->RestorationAttempt_MAX=3;
 
    //Initialization of statistic collectors
    for (int i=0; i<net->ProvisioningAttempt_MAX; i++) {
        net->GeneralBlockATT.push_back(vector <double> ());
        net->ForwardBlockATT.push_back(vector <double> ());
        net->BackwardBlockATT.push_back(vector <double> ());
        net->RoutingBlockATT.push_back(vector <double> ());
    }
    
    for (int i=0; i<net->RestorationAttempt_MAX; i++) {
      net->GeneralBlockATT_Restoration.push_back(vector <double> ());
      net->TotRestTimeATT.push_back(vector <double> ());
    }
      
    for (int i=0; i<net->ProvisioningAttempt_MAX; i++) {
        net->ConnRequestedATT.push_back(0);
        net->ConnRequestedATT_REP.push_back(0);
	
        net->ConnAdmittedPathATT.push_back(0);
        net->ConnAdmittedPathATT_REP.push_back(0);

        net->ConnEstablishedATT.push_back(0);
        net->ConnEstablishedATT_REP.push_back(0);

        net->ConnBlockedATT.push_back(0);
        net->ConnBlockedATT_REP.push_back(0);

        net->ConnBlockedRoutingATT.push_back(0);
        net->ConnBlockedRoutingATT_REP.push_back(0);

        net->ConnBlockedForwardATT.push_back(0);
        net->ConnBlockedForwardATT_REP.push_back(0);

        net->ConnBlockedBackwardATT.push_back(0);
        net->ConnBlockedBackwardATT_REP.push_back(0);
    }
    
    if (net->SimulationMode==REST) {
      for (int i=0; i<net->RestorationAttempt_MAX; i++) {
	net->ConnBlocked_RestorationATT.push_back(0);
	net->ConnBlocked_RestorationATT_REP.push_back(0);
	
	net->ConnRefusedRouting_RestorationATT.push_back(0);
	net->ConnRefusedRouting_RestorationATT_REP.push_back(0);
    
	net->ConnBlockedForward_RestorationATT.push_back(0);
	net->ConnBlockedForward_RestorationATT_REP.push_back(0);
	
	net->ConnBlockedBackward_RestorationATT.push_back(0);
	net->ConnBlockedBackward_RestorationATT_REP.push_back(0);
	
	net->ConnRestoredATT.push_back(0);
	net->ConnRestoredATT_REP.push_back(0);
      }
    }

    /*---------------------------------------------------------------------------------------------------------------------*/
    cout << "\nLoading PathTables..." << endl;
    cout << "\t...loading PathTable in CNet" << flush;
    CPathTable* PathTable;
    PathTable=new CPathTable(net,net->RoutingHopThreshold_CNet,0);
    net->PathTable=PathTable;
    cout << "...[ok]" << endl;

    //This is the CPathTable used by the Parent PCE
    if (net->TypeHPCE == STANDARD_HPCE) {
        cout << "\t...loading PathTable_HPCE in CNet" << flush;
        CPathTable* HPCE_PathTable;
        HPCE_PathTable=new CPathTable(net);
        net->HPCE_PathTable=HPCE_PathTable;
        net->HPCE_PathTable->ParentNode=net->HPCE_Node;
        cout << "...[ok]" << endl;
    }

    //This is the CPathTable used by the PCE
    cout << "\t...loading PathTable in CDomain: " << flush;
    CPathTable* PathTableDomain;
    for (int i=0; i < net->Domain.size(); i++) {
        PathTableDomain=new CPathTable(net,net->Domain[i],net->RoutingHopThreshold_PCE,0);
        net->Domain[i]->PathTable=PathTableDomain;
        cout << i << " ";

        //net->Domain[i]->PathTable->print();
    }
    cout << "...[ok]" << endl;

    //This is the CPathTable used by the source node
    cout << "\t...loading PathTables in CNodes: " << flush;
    CPathTable* PathTableNode;
    for (int i=0; i < net->Node.size(); i++) {
        PathTableNode=new CPathTable(net,net->Node[i],net->Node[i]->ParentDomain->PathTable);
        net->Node[i]->PathTable=PathTableNode;
        cout << i << " ";
    }
    cout << "...[ok]" << endl;
    /*---------------------------------------------------------------------------------------------------------------------------------*/

    cout << "\nLoading routing..." << endl;
    net->InterDomain_RoutingScheme(); //Fill up the CDomain->DomainRoutingTable_db for each domain
    
    //Choose one of the following two lines for filling up the CNodes->RoutingTables
    //net->NetworkLevel_RoutingScheme();
    net->DomainLevel_RoutingScheme();
    cout << "[ok]" << endl << endl;

    cout << "Loading traffic..." << endl;
    CTraffic* Traffic;
    Traffic = new CTraffic(net);
    Traffic->Load();
    cout << "[ok]" << endl << endl;

    #ifdef STDOUT
    cout << "STDOUT mode turned on..." << endl;
    #endif

    net->LogFile.close();
    
    net->execute();
    
    /*//MONITORING LINK RESOURCES
    int counter;
    for (int i=0; i<net->Link.size(); i++) {
      counter=0;
      for (int j=0; j<net->Link[i]->LambdaStatus.size(); j++) {
        counter+=net->Link[i]->LambdaStatus[j];
      }
      cout << "Link " << net->Link[i]->Id << " LambdaStatus: " << counter << endl;
    }
    
    
    PCx_Node* pce_node;
    for (int i=0; i<net->Node.size(); i++) {
      if ((net->Node[i]->Type==PCC_PCE) || (net->Node[i]->Type==PCC_PCE_HPCE)) {
        cout << "Controller node: " << net->Node[i]->Id << endl;
        
        pce_node = static_cast <PCx_Node*> (net->Node[i]);
        pce_node->SNMP_dump_database(pce_node->SNMP_LinkState_dB);
      }
    }
    */
    
    /*int campioni=1000000,finestra=10,somma_campioni=0,campione;
    double media_campioni;
    int min_distr=10,max_distr=100,bins=90,bin;
    int distr_1[bins],distr_2[bins];
    int finestra_campioni[finestra];
    double bin_width=(double) (max_distr-min_distr)/bins;
    
    for (int i=0; i<bins; i++) {
      distr_1[i]=0;
      distr_2[i]=0;
    }
    
    for (int i=0; i<finestra; i++) {
      finestra_campioni[i]=0;
    }
    
    for (int i=0; i<campioni; i++) {
      somma_campioni=0;
      for (int j=0; j<finestra; j++) {
	campione=randint(min_distr,max_distr,&net->RequiredSlotsSeed);
	somma_campioni=somma_campioni+campione;
      }
      media_campioni=(double) somma_campioni/finestra;
      //media_campioni=499.9;
      bin=(bins*(media_campioni-min_distr))/(max_distr-min_distr);
      cout << "Media 1: " << media_campioni << " Bin: " << bin << endl;
      
      distr_1[bin]++;
    }
    //----- SLIDING WINDOWS -----
    somma_campioni=0;
    for (int i=0; i<campioni; i++) {
      campione=randint(min_distr,max_distr,&net->RequiredSlotsSeed);
      
      if (i<finestra) {
	somma_campioni=somma_campioni+campione;
	finestra_campioni[i%finestra]=campione;
      }
      else {
	somma_campioni=somma_campioni+campione-finestra_campioni[i%finestra];
	finestra_campioni[i%finestra]=campione;
      
	media_campioni=(double) somma_campioni/finestra;
	
	bin=(bins*(media_campioni-min_distr))/(max_distr-min_distr);
	cout << "Media 2: " << media_campioni << " Bin: " << bin << endl;
      
	distr_2[bin]++;
      }
    }
    
    cout << "--- STATISTICAL DISTRIBUTION 1 ---" << endl;
    for (int i=0; i<bins; i++)
      cout << "Distribution " << min_distr+i*bin_width << " " << distr_1[i] << endl;
    
    cout << "--- STATISTICAL DISTRIBUTION 2 ---" << endl;
    for (int i=0; i<bins; i++)
      cout << "Distribution " << min_distr+i*bin_width << " " << distr_2[i] << endl;;
    */
    
    /*-----------------------------------------------*/
    /*--- Output provisioning blocking components ---*/
    /*-----------------------------------------------*/
    double mu,sigma2,CI;
    
    cout << endl << "-------------------------------" << endl;
    cout << "--- PROVISIONING STATISTICS ---" << endl;
    cout << "-------------------------------" << endl;

    mu=0,sigma2=0,CI=0;
    net->ConfidenceInterval(5,5000,10,90,net->GeneralBlock,&mu,&sigma2,&CI);
    cout << "GeneralBlock[Provisioning]: " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " " << net->pce_routing_time << " " << net->oxc_crossconnection_time;
    cout << " rep: " << net->GeneralBlock.size() << " block: " << mu << "  sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;

    mu=0,sigma2=0,CI=0;
    net->ConfidenceInterval(5,5000,10,90,net->ForwardBlock,&mu,&sigma2,&CI);
    cout << "ForwardBlock[Provisioning]: " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " " << net->pce_routing_time << " " << net->oxc_crossconnection_time;
    cout << " rep: " << net->ForwardBlock.size() << " block: " << mu << "  sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;

    mu=0,sigma2=0,CI=0;
    net->ConfidenceInterval(5,5000,10,90,net->BackwardBlock,&mu,&sigma2,&CI);
    cout << "BackwardBlock[Provisioning] " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " " << net->pce_routing_time << " " << net->oxc_crossconnection_time;
    cout << " rep: " << net->BackwardBlock.size() << " block: " << mu << "  sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;

    mu=0,sigma2=0,CI=0;
    net->ConfidenceInterval(5,5000,10,90,net->RoutingBlock,&mu,&sigma2,&CI);
    cout << "RoutingBlock[Provisioning] " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " " << net->pce_routing_time << " " << net->oxc_crossconnection_time;
    cout << " rep: " << net->RoutingBlock.size() << " block: " << mu << "  sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;

    //Output signaling time statistics
    mu=0,sigma2=0,CI=0;
    net->ConfidenceInterval(1,5000,10,90,net->TotSignTime,&mu,&sigma2,&CI);
    cout << "SignalingTime[Provisioning] " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " " << net->pce_routing_time << " " << net->oxc_crossconnection_time;
    cout << " rep: " << net->TotSignTime.size() << " time: " << mu << " sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;

    cout << endl << "--------------------------" << endl;
    cout << "--- PACKETS STATISTICS ---" << endl;
    cout << "--------------------------" << endl;
    
    //Output packets statistics
    mu=0,sigma2=0,CI=0;
    net->ConfidenceInterval(1,5000,10,90,net->RSVP_NodeRate,&mu,&sigma2,&CI);
    cout << "RSVP_Packets[Provisioning] - traffic: " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " rep: " << net->RSVP_NodeRate.size() << " load: " << mu << " sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;

    mu=0,sigma2=0,CI=0;
    net->ConfidenceInterval(1,5000,10,90,net->OSPF_NodeRate,&mu,&sigma2,&CI);
    cout << "OSPF_Packets[Provisioning] - traffic: " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " rep: " << net->OSPF_NodeRate.size() << " load: " << mu << " sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;

    mu=0,sigma2=0,CI=0;
    net->ConfidenceInterval(1,5000,10,90,net->PCEP_NodeRate,&mu,&sigma2,&CI);
    cout << "PCEP_Packets[Provisioning] - traffic: " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " rep: " << net->PCEP_NodeRate.size() << " load: " << mu << " sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;

    mu=0,sigma2=0,CI=0;
    net->ConfidenceInterval(1,5000,10,90,net->OFP_NodeRate,&mu,&sigma2,&CI);
    cout << "OFP_Packets[Provisioning] - traffic: " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " rep: " << net->OFP_NodeRate.size() << " load: " << mu << " sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;
    
    mu=0,sigma2=0,CI=0;
    net->ConfidenceInterval(1,5000,10,90,net->TOT_NodeRate,&mu,&sigma2,&CI);
    cout << "TotalPackets[Provisioning] - traffic: " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " rep: " << net->TOT_NodeRate.size() << " load: " << mu << " sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;

    mu=0,sigma2=0,CI=0;
    net->ConfidenceInterval(1,5000,10,90,net->TOT_ControllerNodeRate,&mu,&sigma2,&CI);
    cout << "ControllerPackets[Provisioning] - traffic: " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " rep: " << net->TOT_NodeRate.size() << " load: " << mu << " sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;
    
    /*
    cout << endl << "---------------------------------------" << endl;
    cout << "--- ATTEMPT PROVISIONING STATISTICS ---" << endl;
    cout << "---------------------------------------" << endl;
    
    for (int i=0; i<net->ProvisioningAttempt_MAX; i++) {
        mu=0,sigma2=0,CI=0;
        net->ConfidenceInterval(1,5000,10,90,net->GeneralBlockATT[i],&mu,&sigma2,&CI);
        cout << "GeneralBlock[Provisioning] - traffic: " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " attempt: " << i << " rep: " << net->GeneralBlock.size() << " block: " << mu << "  sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;

        mu=0,sigma2=0,CI=0;
        net->ConfidenceInterval(1,5000,10,90,net->IntraDomainBlock,&mu,&sigma2,&CI);
        cout << "IntraDomainBlock[Provisioning] - traffic: " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " attempt: " << i << " rep: " << net->IntraDomainBlock.size() << " block: " << mu << "  sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;

        mu=0,sigma2=0,CI=0;
        net->ConfidenceInterval(1,5000,10,90,net->InterDomainBlock,&mu,&sigma2,&CI);
        cout << "InterDomainBlock[Provisioning] - traffic: " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " attempt: " << i << " rep: " << net->InterDomainBlock.size() << " block: " << mu << "  sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;

        mu=0,sigma2=0,CI=0;
        net->ConfidenceInterval(1,5000,10,90,net->ForwardBlockATT[i],&mu,&sigma2,&CI);
        cout << "ForwardBlock[Provisioning] - traffic: " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " attempt: " << i << " rep: " << net->ForwardBlock.size() << " block: " << mu << "  sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;

        mu=0,sigma2=0,CI=0;
        net->ConfidenceInterval(1,5000,10,90,net->BackwardBlockATT[i],&mu,&sigma2,&CI);
        cout << "BackwardBlock[Provisioning] - traffic: " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " attempt: " << i << " rep: " << net->BackwardBlock.size() << " block: " << mu << "  sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;

        mu=0,sigma2=0,CI=0;
        net->ConfidenceInterval(1,5000,10,90,net->RoutingBlockATT[i],&mu,&sigma2,&CI);
        cout << "RoutingBlock[Provisioning] - traffic: " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " attempt: " << i << " rep: " << net->RoutingBlock.size() << " block: " << mu << "  sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;

        cout << endl;
    }*/

    /*----------------------------------------------*/
    /*--- Output restoration blocking components ---*/
    /*----------------------------------------------*/
    cout << endl << "------------------------------" << endl;
    cout << "--- RESTORATION STATISTICS ---" << endl;
    cout << "------------------------------" << endl;
    
    if (net->SimulationMode==REST) {
        mu=0,sigma2=0,CI=0;
        net->ConfidenceInterval(5,5000,10,90,net->GeneralBlock_Restoration,&mu,&sigma2,&CI);
        cout << "GeneralBlock[Restoration] " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " " << net->pce_routing_time << " " << net->oxc_crossconnection_time;
	cout << " rep: " << net->GeneralBlock_Restoration.size() << " block: " << mu << "  sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;

        mu=0,sigma2=0,CI=0;
        net->ConfidenceInterval(5,5000,10,90,net->ForwardBlock_Restoration,&mu,&sigma2,&CI);
        cout << "ForwardBlock[Restoration] " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " " << net->pce_routing_time << " " << net->oxc_crossconnection_time;
	cout << " rep: " << net->ForwardBlock_Restoration.size() << " block: " << mu << "  sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;

        mu=0,sigma2=0,CI=0;
        net->ConfidenceInterval(5,5000,10,90,net->BackwardBlock_Restoration,&mu,&sigma2,&CI);
        cout << "BackwardBlock[Restoration] " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " " << net->pce_routing_time << " " << net->oxc_crossconnection_time;
	cout << " rep: " << net->BackwardBlock_Restoration.size() << " block: " << mu << "  sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;

        //Output signaling time statistics
        mu=0,sigma2=0,CI=0;
        net->ConfidenceInterval(1,5000,10,90,net->TotRestTime,&mu,&sigma2,&CI);
        cout << "AvgRecoveryTime[Restoration] " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " " << net->pce_routing_time << " " << net->oxc_crossconnection_time;
	cout << " rep: " << net->TotRestTime.size() << " time: " << mu << " sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;
	
	//Output signaling time statistics
        mu=0,sigma2=0,CI=0;
        net->ConfidenceInterval(1,5000,10,90,net->MaxRestTime,&mu,&sigma2,&CI);
        cout << "MaxRecoveryTime[Restoration] " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " " << net->pce_routing_time << " " << net->oxc_crossconnection_time;
	cout << " rep: " << net->MaxRestTime.size() << " time: " << mu << " sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;
	
	cout << endl;
	
	for (int i=0; i<net->RestorationAttempt_MAX; i++) {
	  mu=0,sigma2=0,CI=0;
	  net->ConfidenceInterval(5,5000,10,90,net->GeneralBlockATT_Restoration[i],&mu,&sigma2,&CI);
	  cout << "GeneralBlockAtt[Restoration] - attempt: " << i << " " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " " << net->pce_routing_time << " " << net->oxc_crossconnection_time;
	  cout << " rep: " << net->GeneralBlockATT_Restoration[i].size() << " block: " << mu << "  sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;
	}
	
	cout << endl;
	
	for (int i=0; i<net->RestorationAttempt_MAX; i++) {
	  mu=0,sigma2=0,CI=0;
	  net->ConfidenceInterval(1,5000,10,90,net->TotRestTimeATT[i],&mu,&sigma2,&CI);
	  cout << "AvgRecoveryTimeAtt[Restoration] - attempt: " << i << " " << net->MeanInterarrivalTime << " " << net->MeanDurationTime << " " << net->pce_routing_time << " " << net->oxc_crossconnection_time;
	  cout << " rep: " << net->TotRestTimeATT[i].size() << " time: " << mu << "  sigma2: " << sigma2 << " CI[radius]: " << CI << " CI[%]: " << 2*(CI/mu)*100 << endl;
	}
    }

    //Other outputs
    cout << endl << "-------------------------------" << endl;
    cout << "--- CONTROL PLANE STATISTIC ---" << endl;
    cout << "-------------------------------" << endl;
    cout << "Control packets dropped: " << net->PckBlocked << endl;
    cout << "ActiveConns: " << net->ActiveConns.size() << endl;

    cout << endl << "----------------------------------------" << endl;
    cout << "--- DETAILED PROVISIONING STATISTICS ---" << endl;
    cout << "----------------------------------------" << endl;
    cout << "Connections Requested: " << net->ConnRequested << endl;
    cout << "- Connections Path Admitted: " << net->ConnAdmittedPath << endl;
    cout << "- Connections Blocked: " << net->ConnBlocked << endl;
    cout << "-- Connections Blocked Routing: " << net->ConnBlockedRouting << endl;
    cout << "-- Connections Blocked Forward: " << net->ConnBlockedForward << endl;
    cout << "---- Connections Blocked EmptyLabelSet: " << net->ConnBlocked_EmptyLabelSet << endl;
    cout << "-- Connections Blocked Backward: " << net->ConnBlockedBackward << endl;
    cout << "---- Connections Blocked Collision: " << net->ConnBlocked_Collision << endl;
    cout << "---- Connections Blocked LinkDown: " << net->ConnBlocked_LinkDown << endl;
    cout << "Connections Refused: " << net->ConnRefused << endl;
    cout << "Connections Established: " << net->ConnEstablished << endl;
    cout << "Connections Released: " << net->ConnReleased << endl << endl;

    cout << endl << "------------------------------------------------" << endl;
    cout << "--- ATTEMPT DETAILED PROVISIONING STATISTICS ---" << endl;
    cout << "------------------------------------------------" << endl;
    cout << "Connections Requested Attempt: ";
    for (int i=0; i<net->ProvisioningAttempt_MAX; i++) cout << " " << net->ConnRequestedATT[i];
    cout << endl;
    cout << "Connections Established Attempt: ";
    for (int i=0; i<net->ProvisioningAttempt_MAX; i++) cout << " " << net->ConnEstablishedATT[i];
    cout << endl;
    cout << "Connections Blocked: ";
    for (int i=0; i<net->ProvisioningAttempt_MAX; i++) cout << " " << net->ConnBlockedATT[i];
    cout << endl;
    cout << "- Connections Blocked Routing Attempt: ";
    for (int i=0; i<net->ProvisioningAttempt_MAX; i++) cout << " " << net->ConnBlockedRoutingATT[i];
    cout << endl;
    cout << "- Connections Blocked Forward Attempt: ";
    for (int i=0; i<net->ProvisioningAttempt_MAX; i++) cout << " " << net->ConnBlockedForwardATT[i];
    cout << endl;
    cout << "- Connections Blocked Backward Attempt: ";
    for (int i=0; i<net->ProvisioningAttempt_MAX; i++) cout << " " << net->ConnBlockedBackwardATT[i];
    cout << endl;
    cout << endl;

    if (net->SimulationMode==REST) {
        cout << endl << "----------------------------------------" << endl;
        cout << "--- DETAILED RESTORATION  STATISTICS ---" << endl;
        cout << "----------------------------------------" << endl;
        cout << "Number of failures: " << net->FailureCounter << endl;
        cout << "Connections Disrupted: " << net->ConnDisrupted << endl;
        cout << "Connections Restored: " << net->ConnRestored << endl;
        cout << "Connections Refused Restoration: " << net->ConnRefused_Restoration << endl;
	cout << "- Connection Refused Routing Restoration: " << net->ConnRefusedRouting_Restoration << endl;
	cout << "- Connection Refused MaxAtt Restoration: " << net->ConnRefusedMaxAtt_Restoration << endl;
        cout << "Connections Blocked Restoration: " << net->ConnBlocked_Restoration << endl;
        cout << "- Connection Blocked Forward Restoration: " << net->ConnBlockedForward_Restoration << endl;
        cout << "- Connection Blocked Backward Restoration: " << net->ConnBlockedBackward_Restoration << endl;
        cout << "Connections Blocked LINK DOWN: " << net->ConnBlocked_LinkDown_Restoration << endl << endl;

        cout << endl << "-----------------------------------------------" << endl;
        cout << "--- ATTEMPT DETAILED RESTORATION STATISTICS ---" << endl;
        cout << "-----------------------------------------------" << endl;
        cout << "Connections Disrupted: " << net->ConnDisrupted << endl;
	cout << "Connections Refused MaxAtt: " << net->ConnRefusedMaxAtt_Restoration << endl;
        cout << "Connections Restored Attempt: ";
        for (int i=0; i<net->RestorationAttempt_MAX; i++) 
	  cout << " " << net->ConnRestoredATT[i];
        cout << endl;
        cout << "Connections Refused Routing Attempt: ";
        for (int i=0; i<net->RestorationAttempt_MAX; i++)
	  cout << " " << net->ConnRefusedRouting_RestorationATT[i];
        cout << endl;
	cout << "Connections Blocked Attempt: ";
        for (int i=0; i<net->RestorationAttempt_MAX; i++)
	  cout << " " << net->ConnBlocked_RestorationATT[i];
        cout << endl;
        cout << "Connections Blocked Forward Attempt: ";
        for (int i=0; i<net->RestorationAttempt_MAX; i++)
	  cout << " " << net->ConnBlockedForward_RestorationATT[i];
        cout << endl;
        cout << "Connections Blocked Backward Attempt: ";
        for (int i=0; i<net->RestorationAttempt_MAX; i++)
	  cout << " " << net->ConnBlockedBackward_RestorationATT[i];
        cout << endl;
        cout << endl;
    }

    cout << endl << "-------------------------------" << endl;
    cout << "------- OSPF STATISTICS -------" << endl;
    cout << "-------------------------------" << endl;
    cout << "LSA Generated: " << net->GeneratedLSA << endl;
    cout << "- Immediate: " << net->ImmediateLSA << endl;
    cout << "- Postponed: " << net->PostponedLSA << endl;
    cout << "LSA Received: " << net->ReceivedLSA << " [It should be " << net->GeneratedLSA*(net->Node.size()-1) << " (LSA Generated)*(Nodes-1)]" << endl;
    cout << "LSA Forwarded: " << net->ForwardedLSA << endl;
    cout << "LSA Dropped: " << net->DroppedLSA << endl;
    cout << "- Too frequent: " << net->TooFrequentLSA << endl;
    cout << "- Out of date: " << net->OutOfDateLSA << endl;
    cout << "- Duplicate: " << net->DuplicateLSA << endl;
    cout << "- SelfOriginated: " << net->SelfOriginatedLSA << endl;
    cout << "LSA Active: " << net->ActiveLSA << endl << endl;

    net->InterArrivalTime->print_data();
    net->HoldingTime->print_data();
    net->TableSize->print_data();
    net->LabelSetSize->print_data();
    
    int general_alu=0;
    cout << endl;
    for (int i=0; i<net->HPCE_Node->BGPLS_HPCE_LinkState_dB.size();i++) {
      for (int j=0; j<net->HPCE_Node->BGPLS_HPCE_LinkState_dB[i]->LambdaStatus.size();j++) {
	//cout << net->HPCE_Node->BGPLS_HPCE_LinkState_dB[i]->LambdaStatus[j] << " ";
	general_alu=general_alu+net->HPCE_Node->BGPLS_HPCE_LinkState_dB[i]->LambdaStatus[j];
	
	if (net->HPCE_Node->BGPLS_HPCE_LinkState_dB[i]->LambdaStatus[j] > 0) {
	  cout << "link: " << net->HPCE_Node->BGPLS_HPCE_LinkState_dB[i]->Link->FromNode->Id << "-" << net->HPCE_Node->BGPLS_HPCE_LinkState_dB[i]->Link->ToNode->Id;
	  cout << " " << net->HPCE_Node->BGPLS_HPCE_LinkState_dB[i]->LambdaStatus[j] << endl;
	}
      }
      //cout << endl;
    }
    cout << "BGP-LS database at H-PCE: " << general_alu << endl;
    
    /*
    cout << "--- Destroying CTraffic..." << endl;
    delete Traffic;
    for (int i=0; i<net->Node.size(); i++) {
    	delete net->Node[i]->PathTable;
    }
    cout << "--- Destroying CPathTable..." << endl;
    delete net->PathTable;

    cout << "--- Destroying CTopology..." << endl;
    delete InputTopo;

    cout << "--- Destroying CNodes..." << endl;
    for (int i=0; i<net->Node.size(); i++) {
    	delete net->Node[i];
    }

    cout << "--- Destroying CLinks..." << endl;
    for (int i=0; i<net->Link.size(); i++) {
    	delete net->Link[i];
    }
    net->Node.clear();
    net->Link.clear();

    net->PCE_Node=NULL;

    cout << "--- Destroying BHTree..." << endl;
    delete net->Tree;

    cout << "--- Destroying CNet..." << endl;
    delete net;*/
}

