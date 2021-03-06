#include <iostream>
#include "CHM.h"

using namespace std;

class Result;

namespace Distributions
{
  const int Uniform = 0;
  const int Gaussian = 1;
}


class TMT : public CHM
{
  private:
    void Defaults();
    string ParamsFN;    

    double W;
    int Distribution;
    int Nimp;
    double* Egrid;
    double* mu0grid;

    double P(double epsilon);
    void Avarage(Result** R);
    void MakeEgrid();

    void PrepareResult(Result* R, double mu, double mu0, double* ReDelta, double* ImDelta);
    bool DoSIAM(Result* R, double epsilon);


    //--- OpenMP ---//
    int AverageNt;
    int SiamNt;
    int KramarsKronigNt;
    double ExitSignal;
    
  public:
    TMT();
    TMT(const char* ParamsFN);
    ~TMT();

    void SetWDN(double W, int Distribution, int Nimp);
    void SetUseBethe(bool UseBethe);
    double get_W() { return W; };
 
    bool SolveSIAM();
    void Slave(int myrank);

    void SendExitSignal();
};
