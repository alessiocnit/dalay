#ifndef STARTEXPERIMENTEVENT_H
#define STARTEXPERIMENTEVENT_H

#include "CEvent.h"
#include "CNode.h"

using namespace std;

class StartExperimentEvent: public CEvent {
public:
    CNet* ParentNet;
    double DurationExperiment;

    StartExperimentEvent(CNet* net, double event_time, double duration_experiment);
    ~StartExperimentEvent();

    void execute();
};

#endif

