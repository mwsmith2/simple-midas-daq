/***************************************************************************** \

Name:   simplest_frontend.cxx
Author: Matthias W. Smith
Email:  mwsmith2@uw.edu

About:  A simple example frontend that is synchronized to the
      SyncTrigger class in shim_trigger using a SyncClient.
      Boilerplate code is surrounded with @sync flags and 
      places needing user code are marked with @user.  
        
\******************************************************************************/

//--- std includes -----------------------------------------------------------//
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
using std::string;

//--- other includes ---------------------------------------------------------//
#include "midas.h"

//--- project includes -------------------------------------------------------//
#include "sync_client.hh"

//--- globals ----------------------------------------------------------------//

#define FRONTEND_NAME "simplest-frontend"

extern "C" {
  
  // The frontend name (client name) as seen by other MIDAS clients
  char *frontend_name = (char*)FRONTEND_NAME;

  // The frontend file name, don't change it.
  char *frontend_file_name = (char*) __FILE__;
  
  // frontend_loop is called periodically if this variable is TRUE
  BOOL frontend_call_loop = FALSE;
  
  // A frontend status page is displayed wi`th this frequency in ms.
  INT display_period = 1000;
  
  // maximum event size produced by this frontend
  INT max_event_size = 0x80000; // 80 kB

  // maximum event size for fragmented events (EQ_FRAGMENTED)
  INT max_event_size_frag = 0x800000;  
  
  // buffer size to hold events
  INT event_buffer_size = 0x800000;
  
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
      {FRONTEND_NAME,   //"sync-example", // equipment name 
       { 1, 0,          // event ID, trigger mask 
         "BUF1",        // event buffer (used to be SYSTEM)
         EQ_POLLED |
	 EQ_EB,         // equipment type 
         0,             // not used 
         "MIDAS",       // format 
         TRUE,          // enabled 
         RO_RUNNING |   // read only when running 
         RO_ODB,        // and update ODB 
         10,            // poll for 10ms 
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

// @sync: begin boilerplate
daq::SyncClient *listener;
// @sync: end boilderplate

//--- Frontend Init -------------------------------------------------//
INT frontend_init() 
{
  // @sync: begin boilerplate
  //DATA part
  HNDLE hDB = NULL;
  HNDLE hkey = NULL;
  INT status = 0;
  int size = 0;
  char str[256], filename[256];
    
  string trigger_addr;
  int trigger_port;

  cm_get_experiment_database(&hDB, NULL);
  if (hDB == NULL) {
    std::cerr << "Could not connect to ODB." << std::endl;
  }

  db_find_key(hDB, 0, "Params/trigger-address", &hkey);
  if (hkey) {
    size = sizeof(str);
    db_get_data(hDB, hkey, str, &size, TID_STRING);
  } else {
    std::cerr << "Could not find trigger address in ODB." << std::endl;
    exit(EXIT_FAILURE);
  }
  
  // Grab the trigger address.
  trigger_addr = std::string(str);

  db_find_key(hDB, 0, "Params/trigger-address", &hkey);
  if (hkey) {
    size = sizeof(str);
    db_get_data(hDB, hkey, str, &size, TID_STRING);
  } else {
    std::cerr << "Could not find trigger address in ODB." << std::endl;
    exit(EXIT_FAILURE);
  }

  trigger_port = std::stoi(str);
  listener = new daq::SyncClient(trigger_addr, trigger_port);
  // @sync: end boilderplate
  // Note that if no address is specifed the SyncClient operates
  // on localhost automatically.

  return SUCCESS;
}

//--- Frontend Exit ------------------------------------------------//
INT frontend_exit()
{
  // @sync: begin boilerplate
  delete listener;
  // @sync: end boilerplate

  return SUCCESS;
}

//--- Begin of Run --------------------------------------------------*/
INT begin_of_run(INT run_number, char *error)
{
  // @sync: begin boilerplate
  listener->SetReady();
  // @sync: end boilerplate

  return SUCCESS;
}

//--- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error)
{
  // @sync: begin boilerplate
  listener->UnsetReady();
  // @sync: end boilerplate

  return SUCCESS;
}

//--- Pause Run -----------------------------------------------------*/
INT pause_run(INT run_number, char *error)
{
  return SUCCESS;
}

//--- Resuem Run ----------------------------------------------------*/
INT resume_run(INT run_number, char *error)
{
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

INT poll_event(INT source, INT count, BOOL test) {

  static unsigned int i;

  // fake calibration
  if (test) {
    for (i = 0; i < count; i++) {
      usleep(10);
    }
    return 0;
  }

  // @sync: begin boilerplate
  if (listener->HasTrigger()) {
    // User: Issue trigger here.
    // Note that HasTrigger only returns true once.  It resets the
    // trigger when it reads true to avoid multiple reads.
    return 1;
  }
  // @sync: end boilerplate

  return 0; // User: Check device for event here.
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
  int count = 0;
  char bk_name[10];
  double *pdata;

  // @user: readout routine here.
  // And MIDAS output.
  bk_init32(pevent);

  // Get the time as an example
  sprintf(bk_name, "SFTM");
  bk_create(pevent, bk_name, TID_DOUBLE, &pdata);

  *(pdata++) = daq::systime_us() * 1.0e-6;

  bk_close(pevent, pdata);

  // @sync: begin boilerplate
  // Let the trigger listener know we are ready for the next event.
  listener->SetReady();
  // @sync: end boilerplate

  return bk_size(pevent);
}
