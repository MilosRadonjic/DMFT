#include "Result.h"
#include "GRID.h"

Result::Result(GRID* grid)
{
  Initialize(grid);
}

Result::Result(const Result &result)
{
  Initialize(result.grid);
  CopyFrom(result);
}

Result::~Result()
{
  ReleaseMemory();
}

void Result::Reset()
{
  ReleaseMemory();
  Initialize(grid);
}

void Result::Reset(GRID* grid)
{
  ReleaseMemory();
  Initialize(grid);
}

void Result::Initialize(GRID* grid)
{
  this->grid = grid;
  
  int N = grid->get_N();
  
  omega = new double[N];
  grid->assign_omega(omega);

  Delta = new complex<double>[N];
  fermi = new double[N];
  G0 = new complex<double>[N];
  Ap = new double[N];
  Am = new double[N];
  P1 = new double[N];
  P2 = new double[N];
  SOCSigma = new complex<double>[N];
  Sigma = new complex<double>[N];
  G = new complex<double>[N];
  DOS = new double[N];
  NIDOS = new double[N];
  DOSmed = new double[N];

  n=0.0;
  mu=0.0;
  mu0=0.0;
}

void Result::ReleaseMemory()
{
  delete [] omega;

  delete [] Delta;
  delete [] fermi;          
  delete [] G0;  
  delete [] Ap;          
  delete [] Am;
  delete [] P1;         
  delete [] P2;
  delete [] SOCSigma;
  delete [] Sigma;
  delete [] G;
  delete [] DOS;
  delete [] NIDOS;
  delete [] DOSmed;
}

void Result::PrintResult(const char* ResultFN)
{ 
  FILE *f;
  f = fopen(ResultFN, "w+");
  
  fprintf(f,"# n = %le mu = %le mu0=%le\n",n,mu,mu0);   

  int N = grid->get_N();
  int i;
  for (i=0; i<N; i++)
  { 
     // loop through and store the numbers into the file
    fprintf(f, "%.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le\n", 
                   omega[i], fermi[i],					//1 2
                   real(Delta[i]), imag(Delta[i]),			//3 4
                   real(G0[i]), imag(G0[i]), 				//5 6
                   Ap[i], Am[i], P1[i], P2[i],				//7 8 9 10 
                   real(SOCSigma[i]), imag(SOCSigma[i]), 		//11 12
                   real(Sigma[i]), imag(Sigma[i]),			//13 14
                   real(G[i]), imag(G[i]),				//15 16
                   DOS[i], NIDOS[i], DOSmed[i]);			//17 18 19 
                   
                
  }
  fclose(f);
}

void Result::ReadFromFile(const char* ResultFN)
{ 
  FILE *f;
  f = fopen(ResultFN, "r");

  char rstLine[1000];
  fgets ( rstLine, 1000, f );

  int N = grid->get_N();
  int i;
  for (i=0; i<N; i++)
  { double o, fer, rd, id, rg0, ig0, ap, am, p1, p2, rsocs, isocs, rs, is, rg, ig, dos, nidos, dosmed;
     // loop through and store the numbers into the file
    fscanf(f, "%le %le %le %le %le %le %le %le %le %le %le %le %le %le %le %le %le %le %le\n", 
                   &o, &fer,			//1 2
                   &rd, &id,			//3 4
                   &rg0, &ig0, 			//5 6
                   &ap, &am, &p1, &p2,		//7 8 9 10 
                   &rsocs, &isocs, 		//11 12
                   &rs, &is,			//13 14
                   &rg, &ig,			//15 16
                   &dos, &nidos, &dosmed);	//17 18 19 

   /* printf("%.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le %.15le\n",
                   o, fer,			//1 2
                   rd, id,			//3 4
                   rg0, ig0, 			//5 6
                   ap, am, p1, p2,		//7 8 9 10 
                   rsocs, isocs, 		//11 12
                   rs, is,			//13 14
                   rg, ig,			//15 16
                   dos, nidos, dosmed);*/
    omega[i] = o;
    fermi[i] = fer;					
    Delta[i] = complex<double>(rd,id);			
    G0[i] = complex<double>(rg0,ig0); 				
    Ap[i] = ap; Am[i]=am; P1[i]=p1; P2[i]=p2;		
    SOCSigma[i] = complex<double>(rsocs,isocs); 	
    Sigma[i] = complex<double>(rs,is);			
    G[i] = complex<double>(rg,ig);			
    DOS[i] = dos; NIDOS[i] = nidos; DOSmed[i]=dosmed;	                                  
  }
  fclose(f);
}


void Result::CopyFrom(const Result &result)
{
  Reset(result.grid);

  int N = result.grid->get_N();

  for (int i=0; i<N; i++)
  {
    //printf("m"); 
    omega[i] = result.omega[i];
    Delta[i] = result.Delta[i];
    fermi[i] = result.fermi[i];
    G0[i] = result.G0[i];
    Ap[i] = result.Ap[i];
    Am[i] = result.Am[i];
    P1[i] = result.P1[i];
    P2[i] = result.P2[i];
    SOCSigma[i] = result.SOCSigma[i];
    Sigma[i] = result.Sigma[i];
    G[i] = result.G[i];
    DOS[i] = result.DOS[i];
    NIDOS[i] = result.NIDOS[i];
    DOSmed[i] = result.DOSmed[i];
  }

  n = result.n;
  mu = result.mu;
  mu0 = result.mu0;
}
/*
void SIAM::PrintModel()
{
  FILE *ModelFile, *InitImageFile;
  char FNModel[100], FNInitImage[100];
  sprintf(FNModel, "model.U%.3f.T%.3f", U, T);
  sprintf(FNInitImage, "initimage.U%.3f.T%.3f", U, T);
  ModelFile= fopen(FNModel,"w");
  InitImageFile= fopen(FNInitImage,"w");
          
  int nw = 200;  
  double dw = 0.025;
               
  for(int i=-nw; i<=nw; i++)
  {  fprintf(ModelFile, "%.15le  %.15le    %.15le\n", i*dw, -1.0/pi*imag(grid->interpl(G,i*dw)), dw ); 
     fprintf(InitImageFile, "%.15le   %.15le\n", i*dw, -1.0/pi*imag(grid->interpl(G,i*dw))); 
  }

  fclose(ModelFile);
  fclose(InitImageFile);
}
*/
