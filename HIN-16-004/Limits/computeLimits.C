#ifndef computeLimits_C
#define computeLimits_C

#include "TString.h"

#include <vector>
#include <sstream>

#include "../Fitter/Macros/Utilities/resultUtils.h"
#include "runLimit_RaaNS_Workspace.C"

using namespace std;

const bool usebatch=true;

void computeLimits(
                   const char* ACTag, // ACTag: where to look for combined workspaces in Limits/CombinedWorkspaces/
                   bool doSyst= false,
                   double CL=0.95, // Confidence Level for limits computation
                   int calculatorType = 2,
                   int testStatType = 2,
                   bool useCLs = false
)
// define calc type and test stat type:
//
// calculatorType = 0 Freq calculator
// calculatorType = 1 Hybrid calculator
// calculatorType = 2 Asymptotic calculator
// calculatorType = 3 Asymptotic calculator using nominal Asimov data sets (not using fitted parameter values but nominal ones)
//
// testStatType = 0 LEP
//              = 1 Tevatron
//              = 2 Profile Likelihood two sided
//              = 3 Profile Likelihood one sided (i.e. = 0 if mu < mu_hat)
//              = 4 Profile Likelihood signed ( pll = -pll if mu < mu_hat)
//              = 5 Max Likelihood Estimate as test statistic
//              = 6 Number of observed event as test statistic
//
// 0,3,true for frequentist CLs; 2,3,true for asymptotic CLs; 0,2,false for FC

{
  // list of files
  vector<TString> theFiles = combFileList(ACTag, doSyst ? "wSyst" : "woSyst");
  if ( theFiles.empty() )
  {
    cout << "# [Error]: No combined workspaces found" << endl;
    return;
  }
  
  // File to save results
  ostringstream strs;
  strs << CL;
  string str = strs.str();
  TString sCL(str);
  sCL.Remove(0,sCL.First('.')+1);
  string limitsFileName = string("csv/") + "cLimits_" + string(sCL) + "_" + string(ACTag);
  if ( calculatorType == 0 ) limitsFileName = limitsFileName + "_Freq";
  else if ( calculatorType == 1 ) limitsFileName = limitsFileName + "_Hybr";
  else if ( calculatorType == 2 ) limitsFileName = limitsFileName + "_Asym";
  else if ( calculatorType == 3 ) limitsFileName = limitsFileName + "_AsymAsi";
  if ( testStatType == 0 ) limitsFileName = limitsFileName + "_LEP";
  else if ( testStatType == 1 ) limitsFileName = limitsFileName + "_Tev";
  else if ( testStatType == 2 ) limitsFileName = limitsFileName + "_2SPL";
  else if ( testStatType == 3 ) limitsFileName = limitsFileName + "_1SPL";
  else if ( testStatType == 4 ) limitsFileName = limitsFileName + "_SPL";
  else if ( testStatType == 5 ) limitsFileName = limitsFileName + "_MaxL";
  else if ( testStatType == 6 ) limitsFileName = limitsFileName + "_NOE";
  if ( doSyst ) limitsFileName = limitsFileName + "_wSyst.csv";
  else limitsFileName = limitsFileName + "_woSyst.csv";
  
  ofstream file(limitsFileName.c_str());
  file << CL << endl;
  
  // Confidence interval computation
  int cnt=1;
  for (vector<TString>::const_iterator it=theFiles.begin(); it!=theFiles.end(); it++)
  {
    cout << ">>>>>>> Computing " << CL*100 << "% " << "limits for analysis bin " << cnt << endl;
    cout << "Using combined PbPb-PP workspace " << cnt << " / " << theFiles.size() << ": " << *it << endl;
    
    if (!usebatch) {
       pair<double,double> lims = runLimit_RaaNS_Workspace(*it, "RFrac2Svs1S_PbPbvsPP", "simPdf_syst", "workspace", "dOS_DATA", ACTag, CL, calculatorType, testStatType, useCLs);

       anabin thebin = binFromFile(*it);

       file << thebin.rapbin().low() << ", " << thebin.rapbin().high() << ", "
          << thebin.ptbin().low() << ", " << thebin.ptbin().high() << ", "
          << thebin.centbin().low() << ", " << thebin.centbin().high() << ", "
          << lims.first << ", " << lims.second << endl;
    } else {
       TString exports;
       exports += Form("export it=%s; ",it->Data());
       exports += Form("export ACTag=%s; ",ACTag);
       exports += Form("export CL=%f; ",CL);
       exports += Form("export calculatorType=%i; ",calculatorType);
       exports += Form("export testStatType=%i; ",testStatType);
       exports += Form("export useCLs=%i; ",useCLs);
       exports += Form("export pwd_=%s; ", gSystem->pwd());
       TString command("qsub -k oe -q cms@llrt3 -l nodes=1:ppn=23 ");
       command += Form("-N limits_bin%i ",cnt);
       command += "-V ";
       command += Form("-o %s ", gSystem->pwd());
       command += Form("-v it,ACTag,CL,calculatorType,testStatType,useCLs,pwd_ ");
       command += "runbatch_limits.sh";
       TString command_full = exports + command;
       cout << command_full.Data() << endl;
       system(command_full.Data());
       system("sleep 1");
    }
    
    cnt++;
  } // loop on the files
  file.close();
  cout << "Closed " << limitsFileName << endl << endl;
}

#endif // #ifndef computeLimits_C
