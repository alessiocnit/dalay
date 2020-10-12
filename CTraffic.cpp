#include "CTraffic.h"

#include "Statistics.h"

#include "CPck.h"
#include "WritePckNodeEvent.h"
#include "GenerateConnEvent.h"
#include "CConnection.h"

//Constructor loads from configuration file class member values of: 1. File, 2. TrafficType, 3. LinesATime
CTraffic::CTraffic(CNet* parent_net) {
    this->Net = parent_net;

    int tm_node;

    this->Net->TrafficType = STATISTIC;
    //this->Net->TrafficType = STATIC;

    if (this->Net->TrafficType == STATISTIC) {
      Net->LogFile << "(CTraffic)::Traffic Type set to STATISTIC" << endl;
      
      if (this->Net->TrafficMatrix_File) {
        this->TrafficMatrixFileName = "traffic_matrix.in";
        Net->LogFile << "(CTraffic)::Traffic Matrix loaded from file: " << TrafficMatrixFileName << endl;

        this->TrafficMatrixFile.open(TrafficMatrixFileName.c_str(),ios::in);
        if (TrafficMatrixFile.is_open()) {
            Net->LogFile << "(CTraffic)::Traffic Matrix file is open: " << TrafficMatrixFileName << endl;
        }
        else {
            Net->LogFile << "(CTraffic)::Traffic Matrix file cannot be open " << TrafficMatrixFileName << endl;
            ErrorMsg("(CTraffic)::Traffic Matrix file cannot be open");
        }

        this->TrafficMatrixFile >> tm_node;
        if (tm_node != this->Net->Node.size()) {
            Net->LogFile << "(CTraffic)\tError. Number of nodes in traffic matrix file not coherent!\n";
            ErrorMsg("Number of nodes in traffic matrix file not coherent!");
        }

        this->Net->Sum_TM = 0;
        for (int i=0; i < tm_node; i++)
            for (int j=0; j < tm_node; j++) {
                this->TrafficMatrixFile >> Net->TM[i][j];  // LOADING TM[i][j]
                if (this->TrafficMatrixFile.fail()) {
                    Net->LogFile << "(CTraffic)\tError. Traffic matrix file not coherent!\n";
                    ErrorMsg("Traffic matrix file not coherent!");
                }
                this->Net->Sum_TM += Net->TM[i][j];
            }
        this->TrafficMatrixFile.close();
      }
      else {
        Net->LogFile << "(CTraffic)::No TRAFFIC_MATRIX defined: supposing UNIFORM distribution!!!" << endl;
        for (int i=0; i<Net->Node.size(); i++)
            for (int j=0; j<Net->Node.size(); j++)
                this->Net->TM[i][j] = (i==j) ? 0 : 1;
        this->Net->Sum_TM = (Net->Node.size()) * (Net->Node.size() - 1);
      }
      
      this->Print_TrafficMatrix();
    }
    else {
      Net->LogFile << "(CTraffic)::Traffic Type set to STATIC" << endl;
      
      this->TrafficFileName="connections.in";
    }
    
    return;
}

CTraffic::~CTraffic() {
}

//Loads the timeline with Generate*Events
void CTraffic::Load() {
    /********* Questa parte viene eseguita quando si caricano le statistiche del traffico **********/
    if (this->Net->TrafficType==STATISTIC) {
	Net->LogFile << "(CTraffic)::First connection will be briefly generated" << endl;
      
        this->FirstConnection_Generation();
        return;
    }
    if (this->Net->TrafficType==STATIC) {
      Net->LogFile << "(CTraffic)::Reading file connections.in" << endl;
      
      this->FileConnections_Generation();
      return;
    }
    else ErrorMsg("TrafficType not supported");
};

CConnection* CTraffic::CreateConnection(int id,double GenerationTime,double DurationTime) {
    int NodeId_s, NodeId_d, i, j, number, found, acc;

    number = randint(1, Net->Sum_TM, &(Net->SrcDstSeed));
    acc=0;
    found=0;
    for (i=0; i<Net->Node.size() && !found; i++)
        for (j=0; j<Net->Node.size() && !found; j++)
            if (i != j) {
                acc += Net->TM[i][j];
                if (acc>=number) {
                    NodeId_s=i;
                    NodeId_d=j;
                    found = 1;
                }
            }

    //Generate connection duration with exponential distribution
    CConnection* conn=new CConnection(id,Net,GenerationTime,DurationTime,Net->GetNodePtrFromNodeID(NodeId_s),Net->GetNodePtrFromNodeID(NodeId_d));
    return conn;
};

void CTraffic::FirstConnection_Generation() {
    double FirstConnection_GenerationTime;
    double FirstConnection_DurationTime;

    FirstConnection_GenerationTime = negexp(Net->MeanInterarrivalTime,&(Net->InterarrivalSeed));
    FirstConnection_DurationTime = negexp(Net->MeanDurationTime,&(Net->DurationSeed));

    cout << "\tGenerating the first connection (MeanInterarrivalTime, MeanDurationTime) "  << Net->MeanInterarrivalTime << " " << Net->MeanDurationTime << endl;

    CConnection* conn = CreateConnection(0,FirstConnection_GenerationTime,FirstConnection_DurationTime);
    GenerateConnEvent* eve=new GenerateConnEvent(conn,conn->GenerationTime);
    this->Net->genEvent(eve);

    cout << "\tFirst connection generated (GenerationTime, DurationTime) "  << FirstConnection_GenerationTime << " " << FirstConnection_DurationTime << endl;

    return;
}

void CTraffic::FileConnections_Generation() {
  ifstream input_file;
  int src_id,dst_id,slots;
  double generation_time, holding_time;
  CNode *Src, *Dst;
  
  int conns=10;
  
  char first_line_comment[512];
  
  input_file.open(this->TrafficFileName.c_str(),ios::in);
  input_file.getline (first_line_comment,512);
  
  for (int i=0; i<conns; i++) {
    input_file >> src_id;
    input_file >> dst_id;
    input_file >> slots;
    input_file >> generation_time;
    input_file >> holding_time;
    
    Src=this->Net->GetNodePtrFromNodeID(src_id);
    Dst=this->Net->GetNodePtrFromNodeID(dst_id);
    
    CConnection* conn = new CConnection(i, this->Net, generation_time, holding_time, Src, Dst);
    GenerateConnEvent* eve=new GenerateConnEvent(conn,conn->GenerationTime);
    this->Net->genEvent(eve);
  }
  
  input_file.close();
  
  return;
}

void CTraffic::Print_TrafficMatrix() {
    this->Net->LogFile << "----------------------------------" << endl;
    this->Net->LogFile << "-------- Traffic Matrix ----------" << endl;
    this->Net->LogFile << "----------------------------------" << endl;

    for (int i=0; i<this->Net->Node.size(); i++) {
        for (int j=0; j<this->Net->Node.size(); j++)
            this->Net->LogFile << this->Net->TM[i][j] << " ";
        this->Net->LogFile << endl;
    }
    this->Net->LogFile << "----------------------------------" << endl;
}
