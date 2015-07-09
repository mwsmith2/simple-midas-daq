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

//-- Bank list --------------------------------------------------------------//

// We anticipate three SIS3302 digitizers, but only use one for now.
BANK_LIST trigger_bank_list[] = {
  {"S0TR", TID_WORD, SIS_3302_LN * SIS_3302_CH, NULL},
  {"OSCV", TID_DOUBLE, TEK_SCOPE_LN * TEK_SCOPE_CH, NULL},
  {"OSCT", TID_DOUBLE, TEK_SCOPE_LN * TEK_SCOPE_CH, NULL},
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
  char keyname[] = "Custom/Images/osci_ch03_fft.gif/Background";
  cm_get_experiment_database(&hDB, NULL);
  db_find_key(hDB, 0, keyname, &hkey);

  if (hkey) {
    return SUCCESS;
  }
   
  //---- user code to book histos ------------------------------------------//
  for (i = 0; i < TEK_SCOPE_CH; i++) {
    char title[32], name[32], key[64], figpath[64];

    // Now add to the odb
    sprintf(name, "osci_ch%02i_wf", i);
    sprintf(key, "/Custom/Images/%s.gif/Background", name);
    sprintf(figpath, "%s/%s.gif", figdir.c_str(), name);
    db_create_key(hDB, 0, key, TID_STRING);
    db_set_value(hDB, 0, key, figpath, sizeof(figpath), 1, TID_STRING);

    // Now add to the odb
    sprintf(name, "osci_ch%02i_fft", i);
    sprintf(key, "/Custom/Images/%s.gif/Background", name);
    sprintf(figpath, "%s/%s.gif", figdir.c_str(), name);
    db_create_key(hDB, 0, key, TID_STRING);
    db_set_value(hDB, 0, key, figpath, sizeof(figpath), 1, TID_STRING);
  }

  for (i = 0; i < SIS_3302_CH; i++) {
    char title[32], name[32], key[64], figpath[64];

    // Now add to the odb
    sprintf(name, "sis_ch%02i_wf", i);
    sprintf(key, "/Custom/Images/%s.gif/Background", name);
    sprintf(figpath, "%s/%s.gif", figdir.c_str(), name);
    db_create_key(hDB, 0, key, TID_STRING);
    db_set_value(hDB, 0, key, figpath, sizeof(figpath), 1, TID_STRING);

    // Now add to the odb
    sprintf(name, "sis_ch%02i_fft", i);
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
  double *poscv;
  double *posct;
  float *pfreq;

  // Look for the first SIS3302 traces.
  if (bk_locate(pevent, "S0TR", &pvme) != 0) {

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

      sprintf(name, "sis_ch%02i_wf", ch);
      sprintf(title, "Channel %i Trace", ch);
      ph_wfm = new TH1F(name, title, TEK_SCOPE_LN, myfid.tm()[0], 
                        myfid.tm()[myfid.tm().size() - 1]);
      
      sprintf(name, "sis_ch%02i_fft", ch);
      sprintf(title, "Channel %i Fourier Transform", ch);
      ph_fft = new TH1F(name, title, TEK_SCOPE_LN, myfid.fftfreq()[0], 
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

  // Look for the first TEKSCOPE traces.
  if ((bk_locate(pevent, "OSCV", &poscv) != 0) &&
      (bk_locate(pevent, "OSCT", &posct) != 0)) {

    cm_msg(MINFO, "online_analyzer", "Processing a tekscope event.");

    wf.resize(TEK_SCOPE_LN);
    tm.resize(TEK_SCOPE_LN);

    // Copy and analyze each channel's FID separately.
    for (ch = 0; ch < TEK_SCOPE_CH; ++ch) {

      std::copy(&poscv[ch*TEK_SCOPE_LN], 
                &poscv[(ch + 1)*TEK_SCOPE_LN], 
                wf.begin());

      std::copy(&posct[ch*TEK_SCOPE_LN], 
                &posct[(ch + 1)*TEK_SCOPE_LN], 
                tm.begin());

      auto myfid = fid::FID(wf, tm);

      sprintf(name, "osci_ch%02i_wf", ch);
      sprintf(title, "Channel %i Trace", ch + 1);
      ph_wfm = new TH1F(name, title, TEK_SCOPE_LN, myfid.tm()[0], 
                        myfid.tm()[myfid.tm().size() - 1]);
      
      sprintf(name, "osci_ch%02i_fft", ch);
      sprintf(title, "Channel %i Fourier Transform", ch + 1);
      ph_fft = new TH1F(name, title, TEK_SCOPE_LN, myfid.fftfreq()[0], 
                        myfid.fftfreq()[myfid.fftfreq().size() - 1]);

      // One histogram gets the waveform and another with the fft power.
      for (idx = 0; idx < myfid.power().size(); ++idx){
        ph_wfm->SetBinContent(idx, myfid.wf()[idx]);
        ph_fft->SetBinContent(idx, myfid.power()[idx]);
      }

      // The waveform has more samples.
      for (; idx < TEK_SCOPE_LN; ++idx) {
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

