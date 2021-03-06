#include <iostream>

class Result;
class GRID;

using namespace std;

class Loop
{
  protected:
    void Defaults();
    string ParamsFN;

    Result* r;

    GRID* grid;
    int N;

    //---- Broyden options ----//
    bool UseBroyden;
    bool ForceBroyden;
    double BroydenStartDiff;

    //---- Mixer Options ------//
    int NtoMix;
    int * Coefs;

    //---- Loop Options -------//
    int MAX_ITS;
    double Accr;

    //---- PrintOut/Debugging optins----//
    bool PrintIntermediate;
    bool HaltOnIterations;
    bool ForceSymmetry;
    
    
    //---- Functions to be overridden---//
    virtual bool SolveSIAM();
    virtual void CalcDelta();

    virtual void ReleaseMemory();

  public:
    Loop();
    Loop(const char* ParamsFN);
    ~Loop();
    
    bool Run(Result* r);
    
    void SetGrid(GRID* grid);
    void SetMixerOptions(int NtoMix, const int * Coefs);
    void SetBroydenOptions(bool UseBroyden, bool ForceBroyden, double BroydenStartDiff);
    void SetLoopOptions(int MAX_ITS, double Accr);
    void SetPrintOutOptions(bool PrintIntermediate, bool HaltOnIterations);
};
