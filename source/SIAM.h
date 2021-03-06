//**********************************************************//
//              SIAM at Arbitrary Filling                   //
//                    by Jaksha Vuchichevicc                //
//**********************************************************//

#include <iostream>
#include <complex>

class Result;
class GRID;

using namespace std;

//======================= SIAM Class ==========================================//

class SIAM
{
  private:

    void Defaults();
    string ParamsFN;

    Result* r;

    //--impurity parameters--//
    double U;			//on-site repulsion
    double T;			//temperature
    double epsilon;		//impurity energy level

    //---BROADENING---//
    double eta;
    
    //--bath parameters--// 
    int DOStype_CHM;		//used in RunCHM for calculation of G
    double t_CHM;
    double mu;			//global chemical potential
    double mu0;			//fictious chemical potential
    bool isBethe;		//Set this to true when using bethe lattice specific self-consistency to use simplified expression for G

    //----lattice---------//
    bool UseLatticeSpecificG;
    int LatticeType;
    double t;
    
    //--don't touch this---//
    bool SymmetricCase;
    bool HalfFilling;

    //-- Broyden solver options--//
    double Accr;
    int MAX_ITS;  

    //--MPT Higher order correlations--//
    double MPT_B;
    double MPT_B0;
    
    //--storage arrays--//
    GRID* grid;
    int N;

     //--get functions--//
    double get_fermi(int i);
    double get_n(complex<double> X[]);

    //--get procedures--//
    void get_fermi();
    void get_G0();
    void get_G0(complex<double>* V);
    void get_As();
    void get_Ps();  
    void get_SOCSigma();
    double get_MPT_B();
    double get_MPT_B0();
    double get_b();
    void get_Sigma();
    void get_G();
    void get_G(complex<double>* V); //used by broyden in solving systems of equations
    void get_G_CHM();
    void get_G_CHM(complex<double>* V); //used by broyden in solving systems of equations

    bool ClipOff(complex<double> &X);
    bool Clipped;

    //--imaginary axis--// 
    double MatsFreq(int n);

    //--- SIAM solver ---//
    void SolveSiam(complex<double>* V);
  public:
    //------ OPTIONS -------//
    bool UseMPT_Bs;		//if true program uses MPT higher coerrelations B and B0
    bool CheckSpectralWeight;   //if true program prints out spectral weights of G and G0 after each iteration
    void SetBroydenParameters(int MAX_ITS, double Accr);
    void SetBroadening(double eta);
    void SetDOStype_CHM(int DOStype, double t, const char* FileName ="");
    void SetIsBethe(bool isBethe);
    void SetT(double T);
    void SetU(double U);
    void SetEpsilon(double epsilon);
    void SetUTepsilon(double U, double T, double epsilon);

    void SetUseLatticeSpecificG(bool UseLatticeSpecificG, double t, int LatticeType);

    //--Constructors/destructors--//
    SIAM();  
    SIAM(const char* ParamsFN);
    ~SIAM();
    
    //get G on inamginary axis
    void GetGfOnImagAxis(int Nmax, complex<double>* G_out);
    
    //--------RUN SIAM--------//
    
    bool Run(Result* r); 
    bool Run_CHM(Result* r); 

    //--print out routines--//
    void PrintModel();

  //---- FRIENDS -----//
  //function that will be calling private member functions in case of solving (systems of) equations
  friend bool UseBroyden(int, int, double, void (SIAM::*)(complex<double>*), SIAM*, complex<double>*);
   
};
