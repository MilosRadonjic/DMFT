#include "SIAM.h"
#include "routines.h"
#include "Broyden.h"
#include "GRID.h"
#include "Result.h"
#include "Input.h"

#ifdef _OMP
#include <omp.h>
#endif

//================== Constructors/DEstructors ====================//

void SIAM::Defaults()
{
  U = 2.0;
  T = 0.05;
  epsilon = 0;
  
  //broyden parameters
  MAX_ITS = 100; //default 100
  Accr = 1e-9; //default 1e-9

  //broadening
  eta = 5e-2;
   
  //options
  CheckSpectralWeight = false; //default false
  UseMPT_Bs = false; //default false
  isBethe = false;

  UseLatticeSpecificG = false;
  t = 0.5;
  LatticeType = DOStypes::SemiCircle;
}

SIAM::SIAM()
{
  Defaults();
}

SIAM::SIAM(const char* ParamsFN)
{
  Defaults();
  this->ParamsFN.assign(ParamsFN);
  cout << "-- INFO -- SIAM: Params File Name set to:" << this->ParamsFN << endl;

  Input input(ParamsFN);

  input.ReadParam(U,"SIAM::U");
  input.ReadParam(T,"SIAM::T");
  input.ReadParam(epsilon,"SIAM::epsilon");
  input.ReadParam(eta,"SIAM::eta");
  input.ReadParam(MAX_ITS,"SIAM::MAX_ITS");
  input.ReadParam(Accr,"SIAM::Accr");
  input.ReadParam(CheckSpectralWeight, "SIAM::CheckSpectralWeight");
  input.ReadParam(UseMPT_Bs,"SIAM::UseMPT_Bs");
  input.ReadParam(isBethe,"SIAM::isBethe");
}

SIAM::~SIAM()
{

}

//========================= INITIALIZERS ===========================//

void SIAM::SetT(double T)
{
  this->T = T;
}

void SIAM::SetEpsilon(double epsilon)
{
  this->epsilon = epsilon;  
}


void SIAM::SetU(double U)
{
  this->U = U;
}

void SIAM::SetUTepsilon(double U, double T, double epsilon)
{
  SetU(U);
  SetT(T);
  SetEpsilon(epsilon);
}


void SIAM::SetBroydenParameters(int MAX_ITS, double Accr)
{
  this->MAX_ITS = MAX_ITS;
  this->Accr = Accr;
}

void SIAM::SetBroadening(double eta)
{
  this->eta = eta;
}

void SIAM::SetIsBethe(bool isBethe)
{
  this->isBethe = isBethe;
}

void SIAM::SetUseLatticeSpecificG(bool UseLatticeSpecificG, double t, int LatticeType)
{
  this->UseLatticeSpecificG = UseLatticeSpecificG;
  this->t = t;
  this->LatticeType = LatticeType;
}

//========================= RUN SIAM EITH FIXED Mu ==========================//

bool SIAM::Run(Result* r) //output
{  
  this->r = r;
  N = r->grid->get_N();
  grid = r->grid;
  get_fermi();

  Clipped = false;
  
  if ((r->mu==0)&&(epsilon==-U/2.0)) 
    SymmetricCase = true;
  else 
    SymmetricCase = false;  
  
  printf("    -------%s SIAM: mu=%.3f, U=%.3f, T=%.3f, epsilon=%.3f -------\n", (SymmetricCase) ? "Symmetric" : "Asymmetric", r->mu, U, T, epsilon);
  
 
  //----- initial guess ------// 
  r->n = 0.5;  
  mu0 = r->mu0;  
  if ((!SymmetricCase)and(UseMPT_Bs))
     MPT_B = epsilon;
  else
     MPT_B = 0.0;

  complex<double>* V = new complex<double>[2];
  V[0] = mu0; 
  V[1] = MPT_B;
  //---------------------------//

  //------ SOLVE SIAM ---------//
  double mu0inits [] = {0.0, 1.0, -1.0, -0.8, 2.0, 1.5, -1.5,  2.5, -2.5, 
                        -2.0, 0.05, 0.8, 0.1, -0.1, 0.3, -0.3, 0.5, -0.5, -0.05, 
                        0.4, -0.4, 0.6, -0.6, 2.3, -2.3, 2.8, -2.8, 1.8, -1.8  }; 
  if (SymmetricCase) 
    //mu0 and n are known => there's no solving of system of equations
    SolveSiam(V);
  else 
  { int c = 0;
    while ( UseBroyden<SIAM>(2, MAX_ITS, Accr, &SIAM::SolveSiam, this, V) != 1 ) 
    { c++;
      if ( c > sizeof(mu0inits)/sizeof(double) - 1 )
      {
        printf("\n\n\n\n==== ERROR ====: SIAM Failed to converge!!!\n\n\n\n");
        return true;
      }
      V[0] = mu0inits[c]; 
      V[1] = MPT_B;
      printf("==================== ====================== ========== TRYING new mu0 int!!! c = %d, mu0init = %f\n\n\n",c, mu0inits[c]);
    };
    //use broyden to solve system of two equations
    
  }
  delete [] V;
  //----------------------------//
  
  //output spectral weight if optioned
  if (CheckSpectralWeight)
  {
    printf("        Spectral weight G: %fe\n", -imag(TrapezIntegralMP(N, r->G, r->omega))/pi);
    printf("        Spectral weight G0: %fe\n", -imag(TrapezIntegralMP(N, r->G0, r->omega))/pi);
  }

  r->mu0 = mu0;

  return Clipped;
}

//========================== RUN SIAM With FIXED n ==============================//
// applicable ONLY in solving Clean Hubbard Model which implies epsilon = 0 and NIDOS is needed on input.
//NOTE that MPT Bs will ALWAYS be one iteration late. They will converge to their real values
//when the DMFT loop converges. First DMFT Iteration is ALWAYS solved WITHOUT MPT Bs.

//TODO in case of asym NIDOS, mu and mu0 are not known EVEN FOR n=0.5 !!!!

bool SIAM::Run_CHM(Result* r) //output
{  
  this->r = r;
  N = r->grid->get_N();
  grid = r->grid;
  get_fermi();
  
  Clipped = false;
  
  epsilon = 0;

  if (r->n==0.5) HalfFilling = true; 
  else HalfFilling = false;
  
  printf("    ------- SIAM for CHM: n=%.3f, U=%.3f, T=%.3f, epsilon=%.3f -------\n", r->n, U, T, epsilon);
  
  if (HalfFilling) 
  {
    r->mu = 0.5*U;
    mu0 = 0.0;
    MPT_B = 0.0;
    MPT_B0 = 0.0;
    SymmetricCase = true;
  }

  //------initial guess---------//
  complex<double>* V = new complex<double>[1];
  V[0] = mu0; //initial guess is always the last mu0. in first DMFT iteration it is 0
  //---------------------------//

  printf("     MPT: B = %fe, B0 = %fe\n", MPT_B, MPT_B0);  

  //----------------- CALCULATION ----------------------//
  if (HalfFilling)//and (SymmetricCase))
    get_G0();
  else
    UseBroyden<SIAM>(1, MAX_ITS, Accr, &SIAM::get_G0, this, V);  

  printf("    mu0 = %f\n", mu0);
  
  get_As();
  get_Ps();
  get_SOCSigma();

  V[0] = r->mu;
  
  if (HalfFilling)//and (SymmetricCase))
  { if (isBethe)
    {  
      get_Sigma();
      get_G();
    }
    else
      get_G_CHM();
  }
  else
  { if (isBethe)
      UseBroyden<SIAM>(1, MAX_ITS, Accr, &SIAM::get_G, this, V);  
    else
      UseBroyden<SIAM>(1, MAX_ITS, Accr, &SIAM::get_G_CHM, this, V);
  }
  MPT_B = get_MPT_B();
  MPT_B0 = get_MPT_B0();

  printf("    mu = %f\n", r->mu);

  delete [] V;
  //-----------------------------------------------------//

  //output spectral weight if optioned
  if (CheckSpectralWeight)
  {
    printf("    n0: %.6f\n", get_n(r->G0));
    printf("    n:  %.6f\n", get_n(r->G));
  }

  // fill in DOS
  #pragma omp parallel for
  for (int i=0; i<N; i++)
    r->DOS[i] = - imag(r->G[i]) / pi;

  r->mu0 = mu0;

  return false;
}

//=================================== FUNCTIONS ===================================//

double SIAM::get_fermi(int i)
{
  return 1.0 / ( 1.0 + exp( r->omega[i]/T ) );
}

void SIAM::get_fermi()
{  
  #pragma omp parallel for
  for (int i=0; i<N; i++) 
  { //printf("tid: %d i: %d\n",omp_get_thread_num(),i);
    r->fermi[i] = get_fermi(i);
  }
}

void SIAM::get_G0()
{
  #pragma omp parallel for
  for (int i=0; i<N; i++) 
    r->G0[i] = complex<double>(1.0)
               / ( complex<double>(r->omega[i] + mu0, eta)
                   - r->Delta[i] ); 

}

void SIAM::get_G0(complex<double>* V)
{
  mu0 = real(V[0]);

  get_G0();

  V[0] = mu0 + get_n(r->G0) - r->n;
} 


double SIAM::get_n(complex<double> X[])
{
  double* g = new double[N];
  #pragma omp parallel for
  for (int i=0; i<N; i++) 
    g[i]=-(1/pi)*imag(X[i])*r->fermi[i];
  
  double n = TrapezIntegralMP(N, g, r->omega);
  delete [] g;
  return n; 
}

void SIAM::get_As() 
{
  #pragma omp parallel for
  for (int i=0; i<N; i++)
  { //printf("tid: %d i: %d\n",omp_get_thread_num(),i);
    r->Ap[i] = -imag(r->G0[i]) * r->fermi[i] / pi;
    r->Am[i] = -imag(r->G0[i]) * (1.0 - r->fermi[i]) / pi;
  }
}

void SIAM::get_Ps()
{
  double** p1 = new double*[N];
  double** p2 = new double*[N];

  #pragma omp parallel for
  for (int i=0; i<N; i++) 
  { 
      p1[i] = new double[N];
      p2[i] = new double[N];
      for (int j=0; j<N; j++)
      {  
         p1[i][j] = r->Am[j] * grid->interpl(r->Ap, r->omega[j] - r->omega[i]);
         p2[i][j] = r->Ap[j] * grid->interpl(r->Am, r->omega[j] - r->omega[i]);
      }

      //get Ps by integrating                           
      r->P1[i] = pi * TrapezIntegral(N, p1[i], r->omega);
      r->P2[i] = pi * TrapezIntegral(N, p2[i], r->omega);

      delete [] p1[i];
      delete [] p2[i];
  }

  delete [] p1;
  delete [] p2;
}

void SIAM::get_SOCSigma()
{
    double** s = new double*[N];
    #pragma omp parallel for 
    for (int i=0; i<N; i++) 
    { //printf("tid: %d i: %d\n",omp_get_thread_num(),i);
      s[i] = new double[N];
      for (int j=0; j<N; j++) 
      {  //printf("tid: %d j: %d\n",omp_get_thread_num());
         s[i][j] =   grid->interpl(r->Ap, r->omega[i] - r->omega[j]) * r->P2[j] 
                   + grid->interpl(r->Am, r->omega[i] - r->omega[j]) * r->P1[j];
      }
                         
      //integrate
  
      r->SOCSigma[i] = complex<double>(0.0, - U*U * TrapezIntegral(N, s[i], r->omega) );    
     
      if (ClipOff( r->SOCSigma[i] )) Clipped = true ;
      delete [] s[i];
    }
    delete [] s;
  //int i;
  //cin >> i;
  if (Clipped) printf("    !!!Clipping SOCSigma!!!!\n");
  grid->KramarsKronig( r->SOCSigma );
}

double SIAM::get_MPT_B0()
{
  if (!UseMPT_Bs) return 0.0;
  
  complex<double>* b0 = new complex<double>[N]; //integrand function
  #pragma omp parallel for
  for (int i=0; i<N; i++) 
    b0[i] = r->fermi[i] * r->Delta[i] * r->G0[i];
  
  double mpt_b0 = epsilon - 1.0  * (2.0 * r->n - 1.0) * imag(TrapezIntegralMP(N, b0, r->omega))
                           / ( pi * r->n * (1.0 - r->n) ) ;
  delete [] b0;
  return mpt_b0;
}

double SIAM::get_MPT_B()
{
  if (!UseMPT_Bs) return 0.0;
  
  complex<double>* b = new complex<double>[N];
  #pragma omp parallel for
  for (int i=0; i<N; i++) 
    b[i] = r->fermi[i] * r->Delta[i] * r->G[i]
           * ( (2.0 / U) * r->Sigma[i] - 1.0 );
  
  double mpt_b = epsilon - 1.0/( pi * r->n * (1.0 - r->n) ) 
                           * imag(TrapezIntegralMP(N, b, r->omega));
  delete [] b;
  return mpt_b;
}

double SIAM::get_b()
{ //we used mu0 as (mu0 - epsilon - U*n) in G0, now we're correcting that
  if (!SymmetricCase)
    return ( (1.0 - 2.0 * r->n) * U - r->mu + (mu0 + epsilon + U * r->n) 
                             - MPT_B0 + MPT_B ) 
           /             ( r->n * (1.0 - r->n) * sqr(U) );
  else return 0;
}

void SIAM::get_Sigma()
{
 
  if (!SymmetricCase)
  { //printf("going through asymmetric\n");
    double b = get_b();    
    #pragma omp parallel for
    for (int i=0; i<N; i++) 
      r->Sigma[i] =  U*r->n + r->SOCSigma[i] 
                              / ( 1.0 - b * r->SOCSigma[i] );
    
  }
  else
    #pragma omp parallel for
    for (int i=0; i<N; i++) 
      r->Sigma[i] =  U * r->n + r->SOCSigma[i];

}

//---------------- Get G -------------------------------//

void SIAM::get_G()
{
  if (UseLatticeSpecificG) 
    #pragma omp parallel for
    for (int i=0; i<N; i++) 
    { complex<double> com = r->omega[i] + r->mu - r->Sigma[i];
      r->G[i] = LS_get_G(LatticeType, t, com);
    }
  else
  {
  
  
  #pragma omp parallel for
  for (int i=0; i<N; i++) 
  {    
    r->G[i] =  1.0
               / (r->omega[i] + r->mu - epsilon - r->Delta[i] - r->Sigma[i]) ;
    
    if (ClipOff(r->G[i])) Clipped = true;
  }
  
  if (Clipped) printf("    !!!!Clipping G!!!!\n");

  }
}

void SIAM::get_G(complex<double>* V)
{
  r->mu = real(V[0]);

  get_G();

  V[0] = r->mu + get_n(r->G) - r->n;
} 

//---------------- Get G for CHM -------------------------//

void SIAM::get_G_CHM()
{ 

  get_Sigma();   
  
  if (UseLatticeSpecificG) 
    #pragma omp parallel for
    for (int i=0; i<N; i++) 
    { complex<double> com = r->omega[i] + r->mu - r->Sigma[i];
      r->G[i] = LS_get_G(LatticeType, t, com);
    }
  else
  {

  complex<double>** g = new complex<double>*[N];
  #pragma omp parallel for
  for (int i=0; i<N; i++) 
  {
      
    //treat integrand carefully 
    double D = 0.0;
    complex<double> LogTerm = 0.0;
    if (abs(imag(r->Sigma[i]))<0.1) 
    {
      D = grid->interpl(r->NIDOS, r->mu + r->omega[i] - real(r->Sigma[i]));
      LogTerm = complex<double>(D, 0.0) * log( (r->mu + r->omega[i] - r->Sigma[i] + r->omega[N-1])
                                              /(r->mu + r->omega[i] - r->Sigma[i] - r->omega[N-1]) );
    }

    //create integrand array
    g[i] = new complex<double>[N];  
    for (int j=0; j<N; j++)
      g[i][j] = complex<double>(r->NIDOS[j] - D, 0.0) 
             / ( r->mu + r->omega[i] - r->omega[j] - r->Sigma[i] ); 
    
  
    //integrate to get G 
    r->G[i] = TrapezIntegral(N, g[i], r->omega) + LogTerm ; 

    if (ClipOff(r->G[i])) Clipped = true;
    delete [] g[i];
  }
  
  delete [] g;    
  
  if (Clipped) printf("    !!!!Clipping G!!!!\n");

  }
}

void SIAM::get_G_CHM(complex<double>* V)
{
  r->mu = real(V[0]);

  get_G_CHM();

  V[0] = r->mu + get_n(r->G) - r->n;
} 
//------------------------------------------------------//


void SIAM::SolveSiam(complex<double>* V)
{
  mu0 = real(V[0]);
  MPT_B = real(V[1]);

  //--------------------//
  get_G0();

  r->n = get_n(r->G0);
  MPT_B0 = get_MPT_B0();  

  get_As();
  get_Ps();
  get_SOCSigma();
  get_Sigma();   
  get_G();

  //--------------------//

  V[0] = mu0 + (get_n(r->G) - r->n); //we need to satisfy (get_n(G) == n) and 
  V[1] = get_MPT_B();                //                (MPT_B == get_MPT_B())
}


//================================= ROUTINES =================================//

//-----------------------Miscellaneous---------------------------------//

bool SIAM::ClipOff(complex<double> &X)
{
  if (imag(X)>0) 
  {
    X = complex<double>(real(X),-1e-5);
    return true;
  }
  else
    return false;
}

//------------------------ IMAG Axis ---------------------------//


double SIAM::MatsFreq(int m)
{
  return 2.0*pi*T*(m+0.5);
}

void SIAM::GetGfOnImagAxis(int M, complex<double> * G_out)
{ 
  complex<double>* g = new complex<double>[N];
  double* mf = new double[M];
  for(int m=0; m<M; m++)
  { 
    mf[m]=MatsFreq(m);
    for(int i=0; i<N; i++)
      g[i] = imag(r->G[i]) / complex<double>( -r->omega[i], mf[m] );
    
    G_out[m] = -1/(pi)*TrapezIntegral(N, g, r->omega);
  }
  delete [] g;
  delete [] mf;
}
