/*----------------------------------------------------------------------------*\

file:   an_sis3302.cxx
author: Matthias W. Smith
email:  mwsmith2@uw.edu

about:  Performs FFTs and plots those as well waveforms for simple
        digitizer input.

\*----------------------------------------------------------------------------*/

//-- std includes ------------------------------------------------------------//
#include <stdio.h>
#include <time.h>
#include <cstring>
#include <iostream>

//--- other includes ---------------------------------------------------------//
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TROOT.h"
#include "TStyle.h"

#define USE_ROOT 1
#include "midas.h"

//--- project includes -------------------------------------------------------//
#include "experim.h"
#include "fid.h"
#include "common.hh"

//--- globals ----------------------------------------------------------------//

// The analyzer name (client name) as seen by other MIDAS clients   
char *analyzer_name = (char *)"online-monitor"; // MWS set

// analyzer_loop is called with this interval in ms (0 to disable)  
INT analyzer_loop_period = 500;

// default ODB size 
INT odb_size = DEFAULT_ODB_SIZE;

// ODB structures 
RUNINFO runinfo;
GLOBAL_PARAM global_param;
EXP_PARAM exp_param;

int analyze_trigger_event(EVENT_HEADER * pheader, void *pevent);
int analyze_scaler_event(EVENT_HEADER * pheader, void *pevent);

INT analyzer_init(void);
INT analyzer_exit(void);
INT analyzer_loop(void);
INT ana_begin_of_run(INT run_number, char *error);
INT ana_end_of_run(INT run_number, char *error);
INT ana_pause_run(INT run_number, char *error);
INT ana_resume_run(INT run_number, char *error);

// My stop hook.
INT tr_stop_hook(INT run_number, char *error);

//-- Bank list --------------------------------------------------------------//

// We anticipate three SIS3302 digitizers, but only use one for now.
BANK_LIST trigger_bank_list[] = {
  {"16_0", TID_WORD, SIS_3316_LN * SIS_3316_CH, NULL},
  {"02_0", TID_WORD, SIS_3302_LN * SIS_3302_CH, NULL},
  {""}
};

//-- Event request list -----------------------------------------------------//

ANALYZE_REQUEST analyze_request[] = {
  {"online-monitor",                // equipment name 
   {1,                         // event ID 
    TRIGGER_ALL,               // trigger mask 
    GET_NONBLOCKING,           // get some events 
    "SYSTEM",                  // event buffer 
    TRUE,                      // enabled 
    "", "",},
   analyze_trigger_event,      // analyzer routine 
   NULL,                       // module list 
   trigger_bank_list,          // list 
   10000,                      // RWNT buffer size 
  },
  
  {""}
};

namespace {

// Histograms for a single SIS3302 digitizer.
std::string figdir;
}

//-- Analyzer Init ---------------------------------------------------------//

INT analyzer_init()
{
  HNDLE hDB, hkey;
  char str[80];
  int i, size;

  // Register my own stop hook.
  cm_register_transition(TR_STOP, tr_stop_hook, 900);  

  // following code opens ODB structures to make them accessible
  // from the analyzer code as C structures 
  cm_get_experiment_database(&hDB, NULL);
  db_find_key(hDB, 0, "/Params/html-dir", &hkey);
  
  if (hkey) {
    size = sizeof(str);
    db_get_data(hDB, hkey, str, &size, TID_STRING);
    if (str[strlen(str) - 1] != DIR_SEPARATOR) {
      strcat(str, DIR_SEPARATOR_STR);
    }
  }
  
  // Filepaths are too long, so moving to /tmp
  //  figdir = std::string(str) + std::string("static/");
  figdir = std::string("/tmp");
  gROOT->SetBatch(true);
  gStyle->SetOptStat(false);

  // Check if we need to put the images in.
  char keyname[] = "Custom/Images/sis3302_ch03_fft.gif/Background";
  cm_get_experiment_database(&hDB, NULL);
  db_find_key(hDB, 0, keyname, &hkey);

  if (hkey) {
    return SUCCESS;
  }
   
  //---- user code to book histos ------------------------------------------//
  for (i = 0; i < SIS_3316_CH; i++) {
    char title[32], name[32], key[64], figpath[64];

    // Now add to the odb
    sprintf(name, "sis3302_ch%02i_wf", i);
    sprintf(key, "/Custom/Images/%s.gif/Background", name);
    sprintf(figpath, "%s/%s.gif", figdir.c_str(), name);
    db_create_key(hDB, 0, key, TID_STRING);
    db_set_value(hDB, 0, key, figpath, sizeof(figpath), 1, TID_STRING);

    // Now add to the odb
    sprintf(name, "sis3302_ch%02i_fft", i);
    sprintf(key, "/Custom/Images/%s.gif/Background", name);
    sprintf(figpath, "%s/%s.gif", figdir.c_str(), name);
    db_create_key(hDB, 0, key, TID_STRING);
    db_set_value(hDB, 0, key, figpath, sizeof(figpath), 1, TID_STRING);
  }

  for (i = 0; i < SIS_3302_CH; i++) {
    char title[32], name[32], key[64], figpath[64];

    // Now add to the odb
    sprintf(name, "sis3302_ch%02i_wf", i);
    sprintf(key, "/Custom/Images/%s.gif/Background", name);
    sprintf(figpath, "%s/%s.gif", figdir.c_str(), name);
    db_create_key(hDB, 0, key, TID_STRING);
    db_set_value(hDB, 0, key, figpath, sizeof(figpath), 1, TID_STRING);

    // Now add to the odb
    sprintf(name, "sis3302_ch%02i_fft", i);
    sprintf(key, "/Custom/Images/%s.gif/Background", name);
    sprintf(figpath, "%s/%s.gif", figdir.c_str(), name);
    db_create_key(hDB, 0, key, TID_STRING);
    db_set_value(hDB, 0, key, figpath, sizeof(figpath), 1, TID_STRING);
  }

  return SUCCESS;
}

//-- Analyzer Exit ---------------------------------------------------------//

INT analyzer_exit()
{
  return CM_SUCCESS;
}

//-- Begin of Run ----------------------------------------------------------//

INT ana_begin_of_run(INT run_number, char *error)
{
  return CM_SUCCESS;
}

//-- End of Run ------------------------------------------------------------//

INT ana_end_of_run(INT run_number, char *error)
{
  FILE *f;
  time_t now;
  char str[256];
  int size;
  double n;
  HNDLE hDB;
  BOOL flag;

  cm_get_experiment_database(&hDB, NULL);

  // update run log if run was written and running online 

  size = sizeof(flag);
  db_get_value(hDB, 0, "/Logger/Write data", &flag, &size, TID_BOOL, TRUE);
  if (flag && runinfo.online_mode == 1) {
    // update run log 
    size = sizeof(str);
    str[0] = 0;
    db_get_value(hDB, 0, "/Logger/Data Dir", str, &size, TID_STRING, TRUE);
    if (str[0] != 0)
      if (str[strlen(str) - 1] != DIR_SEPARATOR)
        strcat(str, DIR_SEPARATOR_STR);
    strcat(str, "runlog.txt");

    f = fopen(str, "a");

    time(&now);
    strcpy(str, ctime(&now));
    str[10] = 0;

    fprintf(f, "%s\t%3d\t", str, runinfo.run_number);

    strcpy(str, runinfo.start_time);
    str[19] = 0;
    fprintf(f, "%s\t", str + 11);

    strcpy(str, ctime(&now));
    str[19] = 0;
    fprintf(f, "%s\t", str + 11);

    size = sizeof(n);
    db_get_value(hDB, 0, "/Equipment/fe-sis3302/Statistics/Events sent",
                 &n, &size, TID_DOUBLE, TRUE);

    fprintf(f, "%dk\t", (int) (n / 1000.0 + 0.5));
    fprintf(f, "%s\n", exp_param.comment);

    fclose(f);
  }

  return CM_SUCCESS;
}

//-- Pause Run -------------------------------------------------------------//


INT ana_pause_run(INT run_number, char *error)
{
  return CM_SUCCESS;
}

//-- Resume Run ------------------------------------------------------------//

INT ana_resume_run(INT run_number, char *error)
{
  return CM_SUCCESS;
}

//-- Analyzer Loop ---------------------------------------------------------//

INT analyzer_loop()
{
  return CM_SUCCESS;
}

//-- Analyze Events --------------------------------------------------------//

INT analyze_trigger_event(EVENT_HEADER * pheader, void *pevent)
{
  // We need these for each FID, so keep them allocated.
  static char title[32], name[32];
  static std::vector<double> wf;
  static std::vector<double> tm;
  static TCanvas c1("c1", "Online Monitor", 360, 270);
  static TH1F *ph_wfm = nullptr;
  static TH1F *ph_fft = nullptr;

  unsigned int ch, idx;
  WORD *pvme;
  float *pfreq;

  // Look for the first SIS3302 traces.
  if (bk_locate(pevent, "02_0", &pvme) != 0) {

    cm_msg(MINFO, "online_analyzer", "Processing a sis3302 event.");

    wf.resize(SIS_3302_LN);
    tm.resize(SIS_3302_LN);

    // Set up the time vector.
    for (idx = 0; idx < SIS_3302_LN; idx++){
      tm[idx] = -1.0 + idx * 0.0001;  // @10 MHz, t = [-1ms, 9ms]
    }

    // Copy and analyze each channel's FID separately.
    for (ch = 0; ch < SIS_3302_CH; ++ch) {
      std::copy(&pvme[ch*SIS_3302_LN], 
                &pvme[(ch + 1)*SIS_3302_LN], 
                wf.begin());

      auto myfid = fid::FID(wf, tm);

      sprintf(name, "sis3302_ch%02i_wf", ch);
      sprintf(title, "Channel %i Trace", ch);
      ph_wfm = new TH1F(name, title, SIS_3316_LN, myfid.tm()[0], 
                        myfid.tm()[myfid.tm().size() - 1]);
      
      sprintf(name, "sis3302_ch%02i_fft", ch);
      sprintf(title, "Channel %i Fourier Transform", ch);
      ph_fft = new TH1F(name, title, SIS_3316_LN, myfid.fftfreq()[0], 
                        myfid.fftfreq()[myfid.fftfreq().size() - 1]);

      // One histogram gets the waveform and another with the fft power.
      for (idx = 0; idx < myfid.power().size(); ++idx){
        ph_wfm->SetBinContent(idx, myfid.wf()[idx]);
        ph_fft->SetBinContent(idx, myfid.power()[idx]);
      }

      // The waveform has more samples.
      for (; idx < SIS_3302_LN; ++idx) {
        ph_wfm->SetBinContent(idx, myfid.wf()[idx]);
      }
      
      c1.SetLogx(0);
      c1.SetLogy(0);
      ph_wfm->Draw();
      c1.Print(TString::Format("%s/%s.gif", 
                                  figdir.c_str(), 
                                  ph_wfm->GetName()));
      c1.SetLogx(1);
      c1.SetLogy(1);
      ph_fft->Draw();
      c1.Print(TString::Format("%s/%s.gif", 
                                  figdir.c_str(), 
                                  ph_fft->GetName()));

      if (ph_wfm != nullptr) {
        delete ph_wfm;
      }

      if (ph_fft != nullptr) {
        delete ph_fft;
      }
    }
  }

  // Look for the first SIS3316 traces.
  if (bk_locate(pevent, "16_0", &pvme) != 0) {

    cm_msg(MINFO, "online_analyzer", "Processing a sis3316 event.");

    wf.resize(SIS_3316_LN);
    tm.resize(SIS_3316_LN);

    // Set up the time vector.
    for (idx = 0; idx < SIS_3316_LN; idx++){
      tm[idx] = -1.0 + idx * 0.0001;  // @10 MHz, t = [-1ms, 9ms]
    }

    // Copy and analyze each channel's FID separately.
    for (ch = 0; ch < SIS_3316_CH; ++ch) {

      std::copy(&pvme[ch*SIS_3316_LN], 
                &pvme[(ch + 1)*SIS_3316_LN], 
                wf.begin());

      auto myfid = fid::FID(wf, tm);

      sprintf(name, "sis3316_ch%02i_wf", ch);
      sprintf(title, "Channel %i Trace", ch + 1);
      ph_wfm = new TH1F(name, title, SIS_3316_LN, myfid.tm()[0], 
                        myfid.tm()[myfid.tm().size() - 1]);
      
      sprintf(name, "sis3316_ch%02i_fft", ch);
      sprintf(title, "Channel %i Fourier Transform", ch + 1);
      ph_fft = new TH1F(name, title, SIS_3316_LN, myfid.fftfreq()[0], 
                        myfid.fftfreq()[myfid.fftfreq().size() - 1]);

      // One histogram gets the waveform and another with the fft power.
      for (idx = 0; idx < myfid.power().size(); ++idx){
        ph_wfm->SetBinContent(idx, myfid.wf()[idx]);
        ph_fft->SetBinContent(idx, myfid.power()[idx]);
      }

      // The waveform has more samples.
      for (; idx < SIS_3316_LN; ++idx) {
        ph_wfm->SetBinContent(idx, myfid.wf()[idx]);
      }
      
      c1.SetLogx(0);
      c1.SetLogy(0);
      ph_wfm->Draw();
      c1.Print(TString::Format("%s/%s.gif", figdir.c_str(), ph_wfm->GetName()));

      c1.SetLogx(1);
      c1.SetLogy(1);
      ph_fft->Draw();
      c1.Print(TString::Format("%s/%s.gif", figdir.c_str(), ph_fft->GetName()));

      if (ph_wfm != nullptr) {
        delete ph_wfm;
      }

      if (ph_fft != nullptr) {
        delete ph_fft;
      }
    }
  }

  return CM_SUCCESS;
}

//-- Run Control Hooks -----------------------------------------------------//
INT tr_stop_hook(INT run_number, char *error) {

  //DATA part
  HNDLE hDB, hkey;
  INT status;
  char str[256], filename[256];
  int size;

  TFile *pf_sis3302;
  TFile *pf_sis3316;
  TFile *pf_final;

  TTree *pt_old_sis3302;
  TTree *pt_old_sis3316;
  TTree *pt_sis3302;
  TTree *pt_sis3316;
    
  cm_get_experiment_database(&hDB, NULL);
  db_find_key(hDB, 0, "/Logger/Data dir", &hkey);
    
  if (hkey) {
    size = sizeof(str);
    db_get_data(hDB, hkey, str, &size, TID_STRING);
    if (str[strlen(str) - 1] != DIR_SEPARATOR) {
      strcat(str, DIR_SEPARATOR_STR);
    }
  }

  // Open all the files.
  sprintf(filename, "%sfe_sis3302_run_%05i.root", str, run_number);
  pf_sis3302 = new TFile(filename);
  if (!pf_sis3302->IsZombie()) {
    pt_old_sis3302 = (TTree *)pf_sis3302->Get("t_sis3302");
    std::cout << "Opened sis3302 file.\n";
  } else {
    std::cout << "Failed to open sis3302 file.\n";
  }

  sprintf(filename, "%s/fe_sis3316_run_%05i.root", str, run_number);
  pf_sis3316 = new TFile(filename);
  if (!pf_sis3316->IsZombie()) {
    pt_old_sis3316 = (TTree *)pf_sis3316->Get("t_sis3316");
    std::cout << "Opened sis3316 file.\n";
  } else {
    std::cout << "Failed to open sis3316 file.\n";
  }

  sprintf(filename, "%s/run_%05i.root", str, run_number);
  pf_final = new TFile(filename, "recreate");

  // Copy the trees from the other two and remove them.
  pt_sis3302 = pt_old_sis3302->CloneTree();
  pt_sis3316 = pt_old_sis3316->CloneTree();

  pt_sis3302->Print();
  pt_sis3316->Print();

  pf_final->Write();

  delete pf_final;
  delete pf_sis3302;
  delete pf_sis3316;

  // Make sure the file was written and clean up.
  pf_final = new TFile(filename);
  
  if (!pf_final->IsZombie()) {
    char cmd[256];
    sprintf(cmd, "rm %s/fe_sis33*_run_%05i.root", str, run_number);
    system(cmd);
  }
    
  delete pf_final;

  return CM_SUCCESS;
}
