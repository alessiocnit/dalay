#include "CStat.h"

CStat::CStat(double min, double max, int bins, string file_name) {
    this->min=min;
    this->max=max;
    this->bins=bins;

    this->distribution_samples=0;
    this->out_ditribution_samples=0;

    this->OutFileName=file_name;

    this->delta=(max-min)/bins;

    for (int i=0; i<this->bins; i++) {
        this->data_distribution.push_back(0);
    }
}


CStat::~CStat() {
}

void CStat::AddSample_Distribution(double sample) {
    int bin;

    this->distribution_samples++;

    if ((sample < this->min) || (sample > this->max)) {
        this->out_ditribution_samples++;
        return;
    }

    bin = (int) ((sample - min)/delta);

    this->data_distribution[bin]++;
}

void CStat::AddSample(double sample) {
    this->data.push_back(sample);
}

void CStat::print_data() {
    int Total=0;
    ofstream output_file;

    output_file.open(this->OutFileName.c_str(),ios::out);

    for (int i=0; i<this->data_distribution.size(); i++) {
        Total+=this->data_distribution[i];
    }

    output_file << "#--- CStat::print_data() ---" << endl;
    output_file << "#- Samples: " << this->distribution_samples << endl;
    output_file << "#- Out of range samples: " << this->out_ditribution_samples << endl;
    output_file << "#- Min: " << this->min << endl;
    output_file << "#- Max: " << this->max << endl;
    output_file << "#- Bins: " << this->bins << endl;
    output_file << "#- Delta: " << this->delta << endl;

    output_file << "#- Average: " << this->ConfidenceInterval(95) << endl;

    for (int i=0; i<this->data_distribution.size(); i++) {
        output_file << this->min + i * this->delta << " ";
        output_file << this->data_distribution[i]  << " ";
        output_file << ((double) this->data_distribution[i])/Total << endl;
    }

    output_file.close();
}

double CStat::ConfidenceInterval(int cl) {

    double mu;
    double sigma2;
    double CI;

    double zeta;
    double Relative_CI;

    switch (cl) {
    case 80:
        zeta=1.28;
        break;
    case 85:
        zeta=1.44;
        break;
    case 90:
        zeta=1.645;
        break;
    case 95:
        zeta=1.96;
        break;
    case 99:
        zeta=2.575;
        break;
    default:
        ErrorMsg("Confidence level not supported !");
    }

    /**************** Computation ********************/
    mu=0;
    for (int i=0; i<this->data.size(); i++)
        mu+=this->data[i];
    mu=mu/this->data.size();

    sigma2=0;
    for (int i=0; i<this->data.size(); i++)
        sigma2+=pow(this->data[i]-mu,2);
    sigma2=sigma2/(this->data.size()-1);

    CI=zeta*sqrt(sigma2/(this->data.size()));
    if (mu!=0) Relative_CI=(2*(CI))/(mu);
    else Relative_CI=0;
    /**************************************************/

    cout << "Samples " << this->data.size() << " average " << mu << " sigma " << sqrt(sigma2) << " CI " << Relative_CI << endl;
    return mu;
}
