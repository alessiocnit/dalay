#include "StartExperimentEvent.h"

#include "defs.h"
#include "Statistics.h"

#include "CNet.h"
#include "LinkFailureEvent.h"

StartExperimentEvent::StartExperimentEvent(CNet* net, double event_time, double duration_experiment) {
    this->eventTime=event_time;
    this->eventType=START_EXPERIMENT;
    this->Comment="START_EXPERIMENT";

    this->ParentNet=net;
    this->DurationExperiment=duration_experiment;
};

StartExperimentEvent::~StartExperimentEvent() {
};

void StartExperimentEvent::execute() {
    #ifdef STDOUT
    cout << "Rep Ended at: " << setprecision (10) << ParentNet->Now << endl;
    #endif

    if (this->ParentNet->SimEnd) return;

    ParentNet->TimeStartREP=ParentNet->Now;
    
    //Registering previous experiment stats (stats are not registred for Experiment_ID=0)
    if (ParentNet->Experiment_ID > 0) {
      cout << "\t...experiment id: " << ParentNet->Experiment_ID;
      
      if (ParentNet->SimulationMode==PROV) {
        cout << " - refused conn prov: " << ParentNet->ConnBlockedREP;
        cout << " - requested: " << ParentNet->ConnRequestedREP << endl;
      }
      
      ParentNet->GeneralBlock.push_back(( (double) (ParentNet->ConnBlockedREP))/(ParentNet->ConnRequestedATT_REP[0]));
      ParentNet->ForwardBlock.push_back(( (double) (ParentNet->ConnBlockedForwardREP))/(ParentNet->ConnRequestedATT_REP[0]));
      ParentNet->BackwardBlock.push_back(((double) (ParentNet->ConnBlockedBackwardREP))/(ParentNet->ConnRequestedATT_REP[0]));
      ParentNet->RoutingBlock.push_back(( (double) (ParentNet->ConnBlockedRoutingREP))/(ParentNet->ConnRequestedATT_REP[0]));

      ParentNet->TotSignTime.push_back(ParentNet->TotSignTimeREP);

      ParentNet->RSVP_NodeRate.push_back(((double) ParentNet->RSVP_PckCounterREP)/(ParentNet->Node.size()*ParentNet->TimeForRep));
      ParentNet->OSPF_NodeRate.push_back(((double) ParentNet->OSPF_PckCounterREP)/(ParentNet->Node.size()*ParentNet->TimeForRep));
      ParentNet->PCEP_NodeRate.push_back(((double) ParentNet->PCEP_PckCounterREP)/(ParentNet->Node.size()*ParentNet->TimeForRep));ParentNet->OFP_NodeRate.push_back(((double) ParentNet->OFP_PckCounterREP)/(ParentNet->Node.size()*ParentNet->TimeForRep));
      ParentNet->TOT_NodeRate.push_back(((double) ParentNet->TOT_PckCounterREP)/(ParentNet->Node.size()*ParentNet->TimeForRep));
      ParentNet->TOT_ControllerNodeRate.push_back(((double) ParentNet->TOT_ControllerPckCounterREP)/(ParentNet->TimeForRep));
	
      for (int i=0; i< ParentNet->ProvisioningAttempt_MAX; i++) {
            ParentNet->GeneralBlockATT[i].push_back(( (double) (ParentNet->ConnBlockedATT_REP[i]))/(ParentNet->ConnRequestedATT_REP[0]));
            ParentNet->ForwardBlockATT[i].push_back(( (double) (ParentNet->ConnBlockedForwardATT_REP[i]))/(ParentNet->ConnRequestedATT_REP[0]));
            ParentNet->BackwardBlockATT[i].push_back(((double) (ParentNet->ConnBlockedBackwardATT_REP[i]))/(ParentNet->ConnRequestedATT_REP[0]));
            ParentNet->RoutingBlockATT[i].push_back(( (double) (ParentNet->ConnBlockedRoutingATT_REP[i]))/(ParentNet->ConnRequestedATT_REP[0]));
      }
        
      //If simulation has been launched in restoration mode
      if (ParentNet->SimulationMode==REST) {
            if (ParentNet->ConnDisruptedREP!=0) {
	      cout << " point: " << ParentNet->MeanInterarrivalTime << "-" << ParentNet->MeanDurationTime;
	      cout << " - refused prov: " << ParentNet->ConnBlockedREP;
	      //cout << " - disrupted links: " << ParentNet->DisruptedLinks[0]->Id << "," << ParentNet->DisruptedLinks[1]->Id;
	      cout << " - disrupted: " << ParentNet->ConnDisruptedREP;
	      cout << " - refused rest: " << ParentNet->ConnRefused_RestorationREP << endl;
	      
	      for (int i=0; i<ParentNet->RestorationAttempt_MAX; i++) {
		//ParentNet->GeneralBlockATT_Restoration[i].push_back(( (double) (ParentNet->ConnBlocked_RestorationATT_REP[i]+ParentNet->ConnRefusedRouting_RestorationATT_REP[i]))/(ParentNet->ConnDisruptedREP));
		
		int conn_restored=0;
		for (int j=0; j<=i; j++) 
		  conn_restored=conn_restored + ParentNet->ConnRestoredATT_REP[j];
		
		ParentNet->GeneralBlockATT_Restoration[i].push_back(( 1 - (double) (conn_restored)/(ParentNet->ConnDisruptedREP)));
	      }
	      
	      ParentNet->GeneralBlock_Restoration.push_back(( (double) ParentNet->ConnRefused_RestorationREP)/(ParentNet->ConnDisruptedREP));
	      ParentNet->ForwardBlock_Restoration.push_back(( (double) (ParentNet->ConnBlockedForward_RestorationREP))/(ParentNet->ConnDisruptedREP));
	      ParentNet->BackwardBlock_Restoration.push_back(((double) (ParentNet->ConnBlockedBackward_RestorationREP))/(ParentNet->ConnDisruptedREP));
	      
	      ParentNet->TotRestTime.push_back(ParentNet->TotRestTimeREP);
	      ParentNet->MaxRestTime.push_back(ParentNet->MaxRestTimeREP);
	      
	      for (int i=0; i<ParentNet->RestorationAttempt_MAX; i++) {
		if (ParentNet->TotRestTimeATT_REP[i]!=0)
		  ParentNet->TotRestTimeATT[i].push_back(ParentNet->TotRestTimeATT_REP[i]);
	      }
	      
	      //Restoration ---> Experiment consistency check
	      if (ParentNet->ConnDisruptedREP != ParentNet->ConnRestoredREP + ParentNet->ConnRefused_RestorationREP) {
		cout << "ConnDisruptedREP: " << ParentNet->ConnDisruptedREP << endl;
		cout << "ConnAdmittedPath_RestorationREP: " << ParentNet->ConnAdmittedPath_RestorationREP << endl;
		cout << "ConnRestoredREP: " << ParentNet->ConnRestoredREP << endl;
		
		cout << "ConnRefused_RestorationREP: " << ParentNet->ConnRefused_RestorationREP << endl;
		cout << "ConnRefusedRouting_RestorationREP: " << ParentNet->ConnRefusedRouting_RestorationREP << endl;
		cout << "ConnRefusedMaxAtt_RestorationREP: " << ParentNet->ConnRefusedMaxAtt_RestorationREP << endl;
		
		cout << "ConnBlocked_RestorationREP: " << ParentNet->ConnBlocked_RestorationREP << endl;
		cout << "ConnBlockedForward_RestorationREP: " << ParentNet->ConnBlockedForward_RestorationREP << endl;
		cout << "ConnBlockedBackward_RestorationREP: " << ParentNet->ConnBlockedBackward_RestorationREP << endl;
		
		cout << "Connections Restored Attempt: ";
		for (int i=0; i<ParentNet->RestorationAttempt_MAX; i++) 
		  cout << " " << ParentNet->ConnRestoredATT_REP[i];
		cout << endl;
		cout << "Connections Refused Routing Attempt: ";
		for (int i=0; i<ParentNet->RestorationAttempt_MAX; i++)
		  cout << " " << ParentNet->ConnRefusedRouting_RestorationATT_REP[i];
		cout << endl;
		cout << "Connections Blocked Attempt: ";
		for (int i=0; i<ParentNet->RestorationAttempt_MAX; i++)
		  cout << " " << ParentNet->ConnBlocked_RestorationATT_REP[i];
		cout << endl;
		cout << "Connections Blocked Forward Attempt: ";
		for (int i=0; i<ParentNet->RestorationAttempt_MAX; i++)
		  cout << " " << ParentNet->ConnBlockedForward_RestorationATT_REP[i];
		cout << endl;
		cout << "Connections Blocked Backward Attempt: ";
		for (int i=0; i<ParentNet->RestorationAttempt_MAX; i++)
		  cout << " " << ParentNet->ConnBlockedBackward_RestorationATT_REP[i];
		cout << endl;
    
		ErrorMsg("ConnRestoredREP + ConnRefused_RestorationREP != ConnDisruptedREP");
	      }
            }
            else cout << " - no disrupted paths" << endl;
      }
    }
    
    //Provisioning ---> Experiment consistency check
    if (ParentNet->ConnRequestedREP != ParentNet->ConnEstablishedREP + ParentNet->ConnRefusedREP) {
        cout << "ConnRequestedREP: " << ParentNet->ConnRequestedREP << endl;
        cout << "ConnEstablishedREP: " << ParentNet->ConnEstablishedREP << endl;
        cout << "ConnRefusedREP: " << ParentNet->ConnRefusedREP << endl;
        cout << "ConnBlockedREP: " << ParentNet->ConnBlockedREP << endl;
        cout << "-- ConnBlockedRoutingREP: " << ParentNet->ConnBlockedRoutingREP << endl;
        cout << "-- ConnBlockedForwardREP: " << ParentNet->ConnBlockedForwardREP << endl;
        cout << "-- ConnBlockedBackwardREP: " << ParentNet->ConnBlockedBackwardREP << endl;
        ErrorMsg ("Experiment not consistent... dalay panic");
    }

    /*Experiment ended... statistic reset !!!*/
    //Provisioning stats
    ParentNet->ConnRequestedREP=0;
    ParentNet->ConnAdmittedPathREP=0;
    ParentNet->ConnRefusedREP=0;
    ParentNet->ConnBlockedREP=0;
    ParentNet->ConnEstablishedREP=0;
    ParentNet->ConnBlockedBackwardREP=0;
    ParentNet->ConnBlockedForwardREP=0;
    ParentNet->ConnBlockedRoutingREP=0;

    //Detailed Provisioning Statistics Reset
    ParentNet->ConnRequestedATT_REP.clear();
    ParentNet->ConnAdmittedPathATT_REP.clear();
    ParentNet->ConnEstablishedATT_REP.clear();
    ParentNet->ConnBlockedATT_REP.clear();
    ParentNet->ConnBlockedRoutingATT_REP.clear();
    ParentNet->ConnBlockedForwardATT_REP.clear();
    ParentNet->ConnBlockedBackwardATT_REP.clear();

    for (int i=0; i<ParentNet->ProvisioningAttempt_MAX; i++) {
        ParentNet->ConnRequestedATT_REP.push_back(0);
	ParentNet->ConnAdmittedPathATT_REP.push_back(0);
        ParentNet->ConnEstablishedATT_REP.push_back(0);
        ParentNet->ConnBlockedATT_REP.push_back(0);
        ParentNet->ConnBlockedRoutingATT_REP.push_back(0);
        ParentNet->ConnBlockedForwardATT_REP.push_back(0);
        ParentNet->ConnBlockedBackwardATT_REP.push_back(0);
    }

    //Time stats
    ParentNet->TotSignTimeREP=0;
    
    //Pck stats
    ParentNet->RSVP_PckCounterREP=0;
    ParentNet->OSPF_PckCounterREP=0;
    ParentNet->PCEP_PckCounterREP=0;
    ParentNet->OFP_PckCounterREP=0;
    ParentNet->TOT_PckCounterREP=0;
    ParentNet->TOT_ControllerPckCounterREP=0;

    //Restoration stats
    if (ParentNet->SimulationMode==REST) {
      ParentNet->TotRestTimeREP=0;
      ParentNet->MaxRestTimeREP=0;
      
      ParentNet->ConnDisruptedREP=0;
      ParentNet->ConnAdmittedPath_RestorationREP=0;
      ParentNet->ConnRefused_RestorationREP=0;
      ParentNet->ConnRefusedRouting_RestorationREP=0;
      ParentNet->ConnRefusedMaxAtt_RestorationREP=0;
      ParentNet->ConnRestoredREP=0;
      ParentNet->ConnBlocked_RestorationREP=0;
      ParentNet->ConnBlockedBackward_RestorationREP=0;
      ParentNet->ConnBlockedForward_RestorationREP=0;
      
      ParentNet->ConnBlocked_RestorationATT_REP.clear();
      ParentNet->ConnRefusedRouting_RestorationATT_REP.clear();
      ParentNet->ConnBlockedForward_RestorationATT_REP.clear();
      ParentNet->ConnBlockedBackward_RestorationATT_REP.clear();
      ParentNet->ConnRestoredATT_REP.clear();
      
      ParentNet->TotRestTimeATT_REP.clear();
        
      for (int i=0; i<ParentNet->RestorationAttempt_MAX; i++) {
	ParentNet->ConnBlocked_RestorationATT_REP.push_back(0);
	ParentNet->ConnRefusedRouting_RestorationATT_REP.push_back(0);
	ParentNet->ConnBlockedForward_RestorationATT_REP.push_back(0);
	ParentNet->ConnBlockedBackward_RestorationATT_REP.push_back(0);
	ParentNet->ConnRestoredATT_REP.push_back(0);
	
	ParentNet->TotRestTimeATT_REP.push_back(0);
      }
    }

    //Generate the first link failure event if the simulation has been launched in restoration mode
    if ((ParentNet->SimulationMode==REST) && (ParentNet->Experiment_ID==0)) {
        int FirstFailureLink_ID;
        CLink* FirstFailureLink;

        do {
	  FirstFailureLink_ID=randint(0,ParentNet->Link.size()-1,&ParentNet->LinkFailureSeed);
	  FirstFailureLink=this->ParentNet->GetLinkPtrFromLinkID(FirstFailureLink_ID);
	} while (FirstFailureLink->Type==INTER_DOMAIN);
	
	/*FirstFailureLink_ID=0;
	FirstFailureLink=this->ParentNet->GetLinkPtrFromLinkID(FirstFailureLink_ID);*/
	
        LinkFailureEvent* f_eve=new LinkFailureEvent(FirstFailureLink, ParentNet->TimeStartREP+0.01*(this->DurationExperiment));
        ParentNet->genEvent(f_eve);
    }

    double mu=0,sigma2=0,CI=0;
    //Simulation end conditions
    #ifndef FIXED_REQUESTS
    #ifndef FIXED_FAILURES
    if (ParentNet->SimulationMode==PROV)
        if ((ParentNet->Experiment_ID) && (ParentNet->ConfidenceInterval(1024,2048,10,90,ParentNet->GeneralBlockATT[0],&mu,&sigma2,&CI)>=0)) {
            ParentNet->SimEnd=1;
            return;
        }
    if (ParentNet->SimulationMode==REST) {
        if ((ParentNet->Experiment_ID) && (ParentNet->ConfidenceInterval(1024,2048,10,90,ParentNet->GeneralBlock_Restoration,&mu,&sigma2,&CI)>=0)) {
            ParentNet->SimEnd=1;
            return;
        }
    }
    #endif
    #endif

    ParentNet->Experiment_ID++;

    StartExperimentEvent* Repetition_eve=new StartExperimentEvent(ParentNet, ParentNet->Now + this->DurationExperiment, this->DurationExperiment);
    ParentNet->genEvent(Repetition_eve);

    return;
}
