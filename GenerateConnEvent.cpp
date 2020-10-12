#include "GenerateConnEvent.h"

#include "defs.h"
#include "Statistics.h"

#include "CNet.h"
#include "CDomain.h"
#include "PCx_Node.h"
#include "CConnection.h"
#include "StartExperimentEvent.h"
#include "CPck.h"
#include "OFP_LightpathPckIN.h"
#include "WritePckNodeEvent.h"

class WritePckNodeEvent;

GenerateConnEvent::GenerateConnEvent() {
}

GenerateConnEvent::GenerateConnEvent(int conn_id, CNet* net) {

    CConnection* NextConn=new CConnection(conn_id,net);

    this->Conn=NextConn;
    this->eventTime=NextConn->GenerationTime;
    this->eventType=GENERATE_CONN;
    this->Comment="GENERATE_CONN";
    this->InNode=NextConn->SourceNode;
};

GenerateConnEvent::GenerateConnEvent(CConnection* connection, double time) {
    this->eventTime=time;
    this->eventType=GENERATE_CONN;
    this->Comment="GENERATE_CONN";

    this->Conn=connection;
    this->InNode=connection->SourceNode;
}

GenerateConnEvent::~GenerateConnEvent() {
};

void GenerateConnEvent::execute() {
    int ActualConnection_ID=this->Conn->Id;
    CNet* ParentNet=this->Conn->Net;
    CNet* net=this->Conn->Net;
    vector<CConnection*> conns; //for supporting requests of multiple connections with LIGHTPATH_IN

    //The OpenFlow signaling is used
    if (net->DomainArchitecture==OPENFLOW_CONTROLLER) {
      
      if (this->InNode->ParentDomain->PCE_Node==NULL) 
	ErrorMsg("net->DomainArchitecture==OPENFLOW_CONTROLLER: the pointer to the OpenFlow Controller of this domain is NULL");
      
      PCx_Node* OpenflowController=this->InNode->ParentDomain->PCE_Node;
      conns.push_back(this->Conn);
      
      OFP_LightpathPckIN* in_pk = new OFP_LightpathPckIN(this->Conn->SourceNode,OpenflowController,100,64,net->Now,conns);
      WritePckNodeEvent* eve = new WritePckNodeEvent(this->Conn->SourceNode,in_pk,net->Now);
      net->genEvent(eve);
      
      this->Conn->Status=REQUESTED;
     
      #ifdef STDOUT
      cout << "NODE: " << this->Conn->SourceNode->Id << " (" << this->Conn->SourceNode->Name << "): conn id " << this->Conn->Id << " CONNECTION MEMORY (writing) size: " << this->Conn->Net->ActiveConns.size() << " TIME: " << this->Conn->Net->Now<< endl;
      #endif
      
      net->ActiveConns.push_back(this->Conn);
    }
    
    //The PCE is not utilized during provisioning
    if (net->DomainArchitecture==NOPCE) {
        //If the ConnectionAdmissionControl is successfull the RSVP signaling is triggered with the ConnEstablish function by the source node
        
        this->Conn->RoutingFlag=1; //Connection will be routed at the source node
        
        if (this->InNode->ConnectionAdmissionControl(this->Conn)) {
	    #ifdef STDOUT
            cout << "NODE: " << this->InNode->Id << " (" << this->InNode->Name << "): conn id " << this->Conn->Id << " ADMITTED for first signaling attempt" << endl;
	    #endif

            this->Conn->Status=ADMITTED_PATH;
	    
	    net->ConnAdmittedPath++; net->ConnAdmittedPathREP++;
	    net->ConnAdmittedPathATT[0]++; net->ConnAdmittedPathATT_REP[0]++;
	    
            this->InNode->ConnEstablish(this->Conn,this->Conn->GenerationTime);
        }
        else {
	    #ifdef STDOUT
            cout << "NODE: " << this->Conn->SourceNode->Id << " (" << this->Conn->SourceNode->Name << "): conn id " << this->Conn->Id << " REFUSED before first signaling attempt" << endl;
	    #endif
	    
            this->Conn->Status=REFUSED;
	    
	    net->ConnRequested++; net->ConnRequestedREP++;
	    
	    net->ConnRefused++; net->ConnRefusedREP++;
	    
	    net->ConnBlocked++; net->ConnBlockedREP++;
	    net->ConnBlockedRouting++; net->ConnBlockedRoutingREP++;

	    net->ConnRequestedATT[0]++; net->ConnRequestedATT_REP[0]++;
	    net->ConnBlockedATT[0]++; net->ConnBlockedATT_REP[0]++;
	    net->ConnBlockedRoutingATT[0]++; net->ConnBlockedRoutingATT_REP[0]++;

            delete this->Conn;
        }
    }
    
    //There PCE is utilized during provisioning
    if (net->DomainArchitecture==PCE) {
        if (this->InNode->ParentDomain->PCE_Node==NULL)
            ErrorMsg("[net->DomainArchitecture==PCE]: the pointer to the PCE of this domain is NULL");

        PCx_Node* PCC_Src=static_cast <PCx_Node*> (this->InNode);

        PCC_Src->PCC_PCE_PathRequest(this->InNode,this->Conn);
    }
    
    /*Durante il primo esperimento Experiment_ID=0 la funzione RepEnd calcola
    	1. Numero di connessioni per ripetizione
    	2. Tempo per ripetizione
    I due parametri poi saranno diversamente
    Il blocco ottenuto nel primo esperimento viene cancellato perché contiene il transitorio*/
    if (ParentNet->Experiment_ID==0) {
        if (this->ExperimentEnd(ParentNet->Experiment_ID, &ParentNet->TimeForRep)) {
            StartExperimentEvent* Repetition_eve=new StartExperimentEvent(ParentNet, ParentNet->Now, ParentNet->TimeForRep);
            ParentNet->genEvent(Repetition_eve);
        }
    }

    #ifdef FIXED_REQUESTS
    if (ActualConnection_ID==FIXED_REQUESTS) ParentNet->SimEnd=1; //Stoppa la simulazione dopo TOT connessioni generate
    #endif

    //If the simulation is over return, else it generates the next GenerateConnectionEvent
    //SCRIVERE QUI CHE SE TRAFFIC STATIC NON SERVE GENERARE LA PROSSIMA CONNESSIONE
    if ((net->SimEnd) || (net->TrafficType == STATIC)) {
        cout << endl << "Simulation point...[ok]" << endl;
        return;
    }
    else {
        GenerateConnEvent* eve=new GenerateConnEvent(ActualConnection_ID+1,ParentNet);
        ParentNet->genEvent(eve);
    }
};

//Ritorna 0 se sono state generate meno di 101 connessioni
//Ritorna 1 se ci sono stati almeno 250 connessioni rifiutate o 10000 connessioni generate
//Ritorna 0 se meno di 200 blocchi e meno di 10000 di connessioni generate
int GenerateConnEvent::ExperimentEnd(int Experiment_ID, double *TimeForRep) {
    CNet* net=this->InNode->ParentNet;

    if (net->SimulationMode==PROV) {
      if (Experiment_ID==0) {
        if ((net->ConnRequested) < 101) return 0;
        if ((net->ConnRefused > 101) || ((net->ConnRequested) > 2500)) {
	  
            *TimeForRep=net->Now;
	    
	    cout << "\t...connection requested in transient: " << net->ConnRequested << endl;
	    cout << "\t...connection refused in transient: " << net->ConnRefused << endl;
	    cout << "\t...time per experiment: " << *TimeForRep << endl;
	    
            return 1;
        }
        return 0;
      }
    }
    
    if (net->SimulationMode==REST) {
      if (Experiment_ID==0) {
	
        if (net->ConnRequested < 10000) 
	  return 0;
	
	*TimeForRep=10 * net->MeanDurationTime;
	
	cout << "\t...connection requested in transient: " << net->ConnRequested << endl;
	cout << "\t...connection refused in transient: " << net->ConnRefused << endl;
	cout << "\t...time per experiment: " << *TimeForRep << endl;
	
	return 1;
      }
    }
    
    return 0;
}


