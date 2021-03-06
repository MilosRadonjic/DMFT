#include <iostream>
#include "Broyden.h"
#include "nrutil.h"
#include "routines.h"

using namespace std;

double abs(complex<double> x)
{
  return  sqrt(   sqr( real(x) ) 
                + sqr( imag(x) ) ); 
}


//========================== USER INTERFACE =================================//

//------default constructor-------//
Broyden::Broyden()
{ 
  Initialized = false;
}


//----call this to set parameters Broyden----//
void Broyden::SetParameters(int N, int MAX_ITS, double alpha, double omega0, double Accr)
{
  this->N = N;
  this->MAX_ITS = MAX_ITS;
  this->alpha = alpha;
  this->omega0 = omega0;
  this->Accr = Accr;
  //LastReset = 0;
}

//---call this to initialize Broyden for use---//
void Broyden::TurnOn(int it)
{
  if (Initialized)
  {
    cout << "----------ERROR: Broyden already turned on!" << endl;
    exit(1);
  }
  LastReset = it;
  PrepareArrays();
  Initialized = true;
  //cout << "    !!!!  Broyden On  !!!!"<< endl;
}

//---when done always turn off broyden---//
void Broyden::TurnOff()
{ 
  if (!Initialized)
  {
    cout << "----------ERROR: Broyden already turned off!" << endl;
    exit(1);
  }
  ReleaseMemory();
  Initialized = false;
  //cout << "    !!!!  Broyden Off  !!!!"<< endl;
}

//---reset broyden to start over---//
void Broyden::Reset(int it)
{ //if you reset at iteration n, the next iteration must be n+1
  if (!Initialized)
  {
    cout << "---------ERROR: Can't reset - Broyden turned off!" << endl;
    exit(1);
  }
  LastReset = it;
  ReleaseMemory();
  PrepareArrays();
  cout << "    !!! Broyden Reset !!!" << endl;
}

//-----calculates new V and alters Vnew to be new V---//
int Broyden::CalculateNew(complex<double> Vnew[], int it)
{
  it -= LastReset;

  if (!Initialized) { printf("ERROR: Broyden not initialized!\n"); exit(1); }

  for (int i=0; i<N; i++)
  {
    Fold[i] = F[i];
    F[i] = Vnew[i] - V[i]; //subtract what was input and what was output in previous iteration   
  }

  if (it>1)
  { 
    add_Ds(it);
    add_As(it);
    if (not get_Betas(it)) return -1;
    get_Us(it);
    get_cs(it);
  }

  for (int i=0; i<N; i++)
  {
    Vold[i] = V[i];
    V[i] = Vold[i] + ((it>1) ? complex<double>(alpha) : 1.0) * F[i]
                   - ((it>1) ? CorrTerm(it, i) : 0.0); 
    Vnew[i] = V[i]; 
  } 

  if (it>1)
  {  double MaxDiff = 0;
     for (int i=0; i<N; i++)
       if( abs( Vnew[i] - Vold[i] ) > MaxDiff ) 
         MaxDiff = abs( Vnew[i] - Vold[i] );
     CurrentDiff = MaxDiff;
     printf("    Broyden: Diff = %le\n", MaxDiff);
     if (MaxDiff < Accr) 
     {  printf("    Broyden: !!! Converged !!!\n");
        return 1;
     }
     else return 0;
  }
  else return 0;
}




//----allocates arrays----//
void Broyden::PrepareArrays()
{
  V = new complex<double> [N];
  Vold = new complex<double> [N];
  F = new complex<double> [N];
  Fold = new complex<double> [N];

  for (int j=0; j<N; j++)
  {
    V[j] = 0.0;
    Vold[j] = 0.0;
    F[j] = 0.0;
    Fold[j] = 0.0;
  }
  
  DV = new complex<double>* [MAX_ITS];
  DF = new complex<double>* [MAX_ITS];
  U = new complex<double>* [MAX_ITS];

  c = new complex<double> [MAX_ITS];
  A = new complex<double>* [MAX_ITS];
  Beta = new complex<double>* [MAX_ITS];
  for (int it=0; it<MAX_ITS; it++)
  { 
    A[it] = new complex<double> [MAX_ITS];
    Beta[it] = new complex<double> [MAX_ITS];
    
    DV[it] = new complex<double> [N];
    DF[it] = new complex<double> [N];
    U[it] = new complex<double> [N]; 

    c[it]=0.0;
    for (int j=0; j<MAX_ITS; j++)
    {
      A[it][j]=0.0;
      Beta[it][j]=0.0;      
    }
    for (int j=0; j<N; j++)
    {
      DV[it][j] = 0.0;
      DF[it][j] = 0.0;
      U[it][j] = 0.0;     
    }
  }
  //Initialized = true;
}

//----deallocates arrays------//
void Broyden::ReleaseMemory()
{
  delete [] V;
  delete [] Vold;
  delete [] F;
  delete [] Fold;
  delete [] c;
  
  for (int it=0; it<MAX_ITS; it++)
  {
    delete [] Beta[it];
    delete [] DV[it];
    delete [] DF[it];
    delete [] U[it]; 
    delete [] A[it];
  }
  delete [] Beta;
  delete [] DV;
  delete [] DF;
  delete [] U;
  delete [] A;

  //Initialized = false;
}

//============================== IMPLEMENTATION ============================//

void Broyden::add_Ds(int it)
{ 
  complex<double> sum = 0.0;
  for(int i=0; i<N; i++)    
  {
    DF[it-1][i] = F[i]-Fold[i];
    DV[it-1][i] = V[i]-Vold[i];
    sum += DF[it-1][i]*DF[it-1][i];
  }
  sum = sqrt(sum);
  for(int i=0; i<N; i++)
  {  
    DF[it-1][i] /= sum;
    DV[it-1][i] /= sum;
  }
}

void Broyden::add_As(int it)
{
  for(int i=1; i<=it-1; i++)
  {
    A[i][it-1] = Multiply(DF[i],DF[it-1], N);
    A[it-1][i] = conj(A[i][it-1]);
  }
}

bool Broyden::get_Betas(int it)
{
  complex<double>** t = new complex<double>*[it];
  for (int i=1; i<=it-1; i++)
  {
    t[i] = new complex<double>[it];
    for (int j=1; j<=it-1; j++)
      t[i][j] = ((i-j==0) ? omega0*omega0 : 0) + A[i][j];    
  }
  if ( not InverseMatrix(t, Beta, it-1) ) return false;
  for (int i=1; i<=it-1; i++)
    delete [] t[i];
  delete [] t;
  return true;
}

void Broyden::get_Us(int it)
{ 
  for (int n=1; n<=it-1; n++)
    for (int i=0; i<N; i++)
      U[n][i] = complex<double>(alpha) * DF[n][i] + DV[n][i];
}

void Broyden::get_cs(int it)
{
  for (int k=1; k<=it-1; k++)
    c[k] = Multiply(F, DF[k], N);
}

complex<double> Broyden::CorrTerm(int it, int i)
{
  complex<double> sum = 0.0;
  for (int n=1; n<=it-1; n++)
    for (int k=1; k<=it-1; k++)
      sum += c[k] * Beta[k][n] * U[n][i]; 
  return sum;    
}

//=======================KRONECKER DELTA=============================//

int Broyden::KDelta(int i, int j)
{
  return (i-j==0) ? 1 : 0;
}


//====================== VECTOR MULTIPLICATION ======================//

complex<double> Broyden::Multiply(complex<double> A[], complex<double> B[], int n)
{
  complex<double> sum=0.0;
  for (int i=0; i<n; i++)
    sum += A[i] * conj(B[i]);  
  return sum;
}

//====================== MATRIX INVERSION ==========================//

//---------------LU decomposition------------------//
#define NRANSI
#define TINY 1.0e-20;

bool Broyden::ludcmp(complex<double> **a, int n, int *indx)
{
	double d;
	int i,imax,j,k;
	double big,dum,temp;
	complex<double> sum, cdum;
	double *vv;

	vv=nrvector(1,n);
	d=1.0;
	for (i=1;i<=n;i++) {
		big=0.0;
		for (j=1;j<=n;j++)
			if ((temp=abs(a[i][j])) > big) big=temp;
		if (big == 0.0) { //nrerror("Singular matrix in routine ludcmp"); 
                                  printf("!!! ERROR !!!==== Broyden : Singular matrix in routine ludcmp============\n");  
                                  return false;
                                }
		vv[i]=1.0/big;
	}
	for (j=1;j<=n;j++) {
		for (i=1;i<j;i++) {
			sum=a[i][j];
			for (k=1;k<i;k++) sum -= a[i][k]*a[k][j];
			a[i][j]=sum;
		}
		big=0.0;
		for (i=j;i<=n;i++) {
			sum=a[i][j];
			for (k=1;k<j;k++)
				sum -= a[i][k]*a[k][j];
			a[i][j]=sum;
			if ( (dum=vv[i]*abs(sum)) >= big) {
				big=dum;
				imax=i;
			}
		}
		if (j != imax) {
			for (k=1;k<=n;k++) {
				cdum=a[imax][k];
				a[imax][k]=a[j][k];
				a[j][k]=cdum;
			}
			d = -d;
			vv[imax]=vv[j];
		}
		indx[j]=imax;
		if (a[j][j] == complex<double>(0.0,0.0)) a[j][j]=TINY;
		if (j != n) {
			cdum=complex<double>(1.0)/(a[j][j]);
			for (i=j+1;i<=n;i++) a[i][j] *= cdum;
		}
	}
	free_vector(vv,1,n);

  return true;

}
#undef TINY
#undef NRANSI

//----------- backtracing--------------//
void Broyden::lubksb(complex<double> **a, int n, int *indx, complex<double> b[])
{
	int i,ii=0,ip,j;
	complex<double> sum;

	for (i=1;i<=n;i++) {
		ip=indx[i];
		sum=b[ip];
		b[ip]=b[i];
		if (ii)
			for (j=ii;j<=i-1;j++) sum -= a[i][j]*b[j];
		else if (sum != complex<double>(0.0,0.0)) ii=i;
		b[i]=sum;
	}
	for (i=n;i>=1;i--) {
		sum=b[i];
		for (j=i+1;j<=n;j++) sum -= a[i][j]*b[j];
		b[i]=sum/a[i][i];
	}
}

//------- function to be called -------//
//a input matrix, y output. a gets destroyed.
bool Broyden::InverseMatrix(complex<double> **a, complex<double> **y, int n)
{
  complex<double>* col = new complex<double> [n+1];
  int i,j;
  int* indx = new int[n+1];
 
  if (not ludcmp(a,n,indx)) return false;
                                  
  for(j=1;j<=n;j++) 
  {
    for(i=1;i<=n;i++) col[i]=0.0;
    col[j]=1.0;
    lubksb(a,n,indx,col);
    for(i=1;i<=n;i++) y[i][j]=col[i];
  }
  delete [] col;
  delete [] indx;
  return true;
}
  
//=====================================================================//


