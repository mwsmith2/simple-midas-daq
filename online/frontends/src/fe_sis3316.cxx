/********************************************************************\

Name:   fe_sis3316.cxx
Author: Matthias W. Smith
Email:  mwsmith2@uw.edu

About:  The code implements a MIDAS frontend that wraps basic routines
        for Struck SIS3316 VME devices.  Its usage is foreseen as 
        a general frontend for testing NMR probes and shimming the 
        g-2 magnets. Its sister analyzer is an_vme_basic.

\********************************************************************/

//--- std includes -------------------------------------------------//
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <string>
using std::string;

//--- other includes -----------------------------------------------//
#include "midas.h"
#include "TFile.h"
#include "TTree.h"

//--- project includes ---------------------------------------------//
#include "event_manager_basic.hh"
#include "common.hh"


//--- globals ------------------------------------------------------//

extern "C" {
  
  // The frontend name (client name) as seen by other MIDAS clients
  char *frontend_name = (char*) "fe-sis3316";

  // The frontend file name, don't change it.
  char *frontend_file_name = (char*) __FILE__;
  
  // frontend_loop is called periodically if this variable is TRUE
  BOOL frontend_call_loop = FALSE;
  
  // A frontend status page is displayed with this frequency in ms.
  INT display_period = 1000;
  
  // maximum event size produced by this frontend
  INT max_event_size = 0x400000;

  // maximum event size for fragmented events (EQ_FRAGMENTED)
  INT max_event_size_frag = 0x1000000;  
  
  // buffer size to hold events
  INT event_buffer_size = 0x8000000;
  
  // Function declarations
  INT frontend_init();
  INT frontend_exit();
  INT begin_of_run(INT run_number, char *error);
  INT end_of_run(INT run_number, char *error);
  INT pause_run(INT run_number, char *error);
  INT resume_run(INT run_number, char *error);

  INT frontend_loop();
  INT read_trigger_event(char *pevent, INT off);
  INT poll_event(INT source, INT count, BOOL test);
  INT interrupt_configure(INT cmd, INT source, PTYPE adr);

  // Equipment list

  EQUIPMENT equipment[] = 
    {
      {"fe-sis3316",     // equipment name 
       { 1, 0,          // event ID, trigger mask 
         "SYSTEM",      // event buffer 
         EQ_POLLED,     // equipment type 
         0,             // not used 
         "MIDAS",       // format 
         TRUE,          // enabled 
         RO_RUNNING |   // read only when running 
         RO_ODB,        // and update ODB 
         500,           // poll for 500ms 
         0,             // stop run after this event limit 
         0,             // number of sub events 
         0,             // don't log history 
         "", "", "",
       },
       read_trigger_event,      // readout routine 
      },
      
      {""}
    };

} //extern C

RUNINFO runinfo;

// Anonymous namespace for my "globals"
namespace {
TFile* root_file;
TTree* t;
bool run_in_progress = false;
bool write_midas = true;
daq::event_data data;
daq::EventManagerBasic* event_manager;
}

//--- Frontend Init -------------------------------------------------//
INT frontend_init() 
{
  string conf_file;
  HNDLE hDB, hkey;
  INT status, size;
  char str[128];
  
  cm_get_experiment_database(&hDB, NULL);
  db_find_key(hDB, 0, "Params/config-dir", &hkey);
    
  if (hkey) {
    size = sizeof(str);
    db_get_data(hDB, hkey, str, &size, TID_STRING);
    if (str[strlen(str) - 1] != DIR_SEPARATOR) {
      strcat(str, DIR_SEPARATOR_STR);
    }
  }

  conf_file = std::string(str);
  conf_file += std::string("fe_sis3316.json");

  event_manager = new daq::EventManagerBasic(conf_file);
  return SUCCESS;
}

//--- Frontend Exit ------------------------------------------------//
INT frontend_exit()
{
  delete event_manager;
  return SUCCESS;
}

//--- Begin of Run --------------------------------------------------*/
INT begin_of_run(INT run_number, char *error)
{
  //HW part
  event_manager->BeginOfRun();
  event_manager->ResizeEventData(data);
  cm_msg(MINFO, frontend_name, "event manager loaded.");

  //DATA part
  HNDLE hDB, hkey;
  INT status;
  char str[256], filename[256];
  int size;
    
  cm_get_experiment_database(&hDB, NULL);
  db_find_key(hDB, 0, "/Logger/Data dir", &hkey);
    
  if (hkey) {
    size = sizeof(str);
    db_get_data(hDB, hkey, str, &size, TID_STRING);
    if (str[strlen(str) - 1] != DIR_SEPARATOR) {
      strcat(str, DIR_SEPARATOR_STR);
    }
  }
    
  db_find_key(hDB, 0, "/Runinfo", &hkey);
  if (db_open_record(hDB, hkey, &runinfo, sizeof(runinfo), MODE_READ,
		     NULL, NULL) != DB_SUCCESS) {
    cm_msg(MERROR, "analyzer_init", "Cannot open \"/Runinfo\" tree in ODB");
    return 0;
  }
  
  // Get the run number out of the MIDAS database.
  strcpy(filename, str);
  sprintf(str, "run_%05d.root", runinfo.run_number);
  strcat(filename, str);

  // Set up the ROOT data output.
  root_file = new TFile(filename, "recreate");
  t = new TTree("t", "t");
  t->SetAutoSave(0);
  t->SetAutoFlush(0);

  int count;
  char branch_vars[100];
  char branch_name[100];

  count = 0;
  for (auto &sis : data.sis_3316_vec) {

    sprintf(branch_name, "sis_3316_%i", count);
    sprintf(branch_vars, "system_clock/l:device_clock[%i]/l:trace[%i][%i]/s",
	    SIS_3316_CH, SIS_3316_CH, SIS_3316_LN);
    
    t->Branch(branch_name, &data.sis_3316_vec[count++], branch_vars);
  }

  run_in_progress = true;

  return SUCCESS;
}

//--- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error)
{
  event_manager->EndOfRun();

  // Make sure we write the ROOT data.
  if (run_in_progress) {
    t->Write();
    root_file->Close();
    
    delete root_file;
    run_in_progress = false;
  }
  
  return SUCCESS;
}

//--- Pause Run -----------------------------------------------------*/
INT pause_run(INT run_number, char *error)
{
  event_manager->PauseRun();
  return SUCCESS;
}

//--- Resuem Run ----------------------------------------------------*/
INT resume_run(INT run_number, char *error)
{
  event_manager->ResumeRun();
  return SUCCESS;
}

//--- Frontend Loop -------------------------------------------------*/

INT frontend_loop()
{
  // If frontend_call_loop is true, this routine gets called when
  // the frontend is idle or once between every event
  return SUCCESS;
}

//-------------------------------------------------------------------*/

/********************************************************************\
  
  Readout routines for different events

\********************************************************************/

//--- Trigger event routines ----------------------------------------*/

unsigned int failure_count = 0;

INT poll_event(INT source, INT count, BOOL test) {
  unsigned int i;

  // fake calibration
  if (test) {
    for (i = 0; i < count; i++) {
      usleep(10);
    }
    return 0;
  }
    
  return ((event_manager->HasEvent() == true)? 1 : 0);
}

//--- Interrupt configuration ---------------------------------------*/

INT interrupt_configure(INT cmd, INT source, PTYPE adr)
{
  switch (cmd) {
  case CMD_INTERRUPT_ENABLE:
    break;
  case CMD_INTERRUPT_DISABLE:
    break;
  case CMD_INTERRUPT_ATTACH:
    break;
  case CMD_INTERRUPT_DETACH:
    break;
  }
  return SUCCESS;
}

//--- Event readout -------------------------------------------------*/

INT read_trigger_event(char *pevent, INT off)
{
  using namespace daq;
  static unsigned long long num_events;
  static unsigned long long events_written;

  int count = 0;
  char bk_name[10];
  WORD *pdata;

  // ROOT output
  if (run_in_progress) {

    // Copy the data
    count = 0;
    for (auto &sis : event_manager->GetCurrentEvent().sis_3316_vec) {
      data.sis_3316_vec[count++] = sis;
    }

    // Pop the event now that we are done copying it.
    event_manager->PopCurrentEvent();

    // Now that we have a copy of the latest event, fill the tree.
    t->Fill();
    num_events++;

    if (num_events % 1000 == 1) {

      t->AutoSave("SaveSelf,FlushBaskets");
      root_file->Flush();

    }
  }

  // And MIDAS output.
  bk_init32(pevent);

  if (write_midas) {
    
    count = 0;
    for (auto &sis : data.sis_3316_vec) {
      
      sprintf(bk_name, "S%01iTR", count++);
      bk_create(pevent, bk_name, TID_WORD, &pdata);
      std::copy(&sis.trace[0][0], 
                &sis.trace[0][0] + SIS_3316_CH*SIS_3316_LN, 
                pdata);
      pdata += sizeof(sis.trace) / sizeof(sis.trace[0][0]);
      bk_close(pevent, pdata);
    }
  }
  return bk_size(pevent);
}
