#include <complex>
using namespace std;

class GRID;

class Result
{
  public:
    Result(GRID* grid);
    Result(const Result &result);
    ~Result();

    void Reset();
    void Reset(GRID* grid);
 
    GRID* grid;

    double n;
    double mu;
    double mu0;

    double* omega;		//omega grid
    double* fermi;		//fermi function
    double* Ap;			//spectral functions
    double* Am;
    double* P1;			//polarizations
    double* P2;
    complex<double>* SOCSigma;	//Second order contribution in sigma
    complex<double>* Sigma;	//Sigma interpolating between exact limiting cases
    complex<double>* G;		//Greens function on real axis
    complex<double>* Delta;	//Bath
    complex<double>* G0;	//auxillary Green's function
    double* DOS;		//quasi-particle density of states (typical DOS in TMT)
    double* NIDOS;		//non-interacting density of states
    double* DOSmed;		//medium DOS in TMT, can be used as an auxiallry DOS in other cases

    void PrintResult(const char* ResultFN);
    void ReadFromFile(const char* ResultFN);
    void CopyFrom(const Result &result);

  private:
    void Initialize(GRID* grid);
    void ReleaseMemory();
};
