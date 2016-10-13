/******************************************************************************\

Name:   fe_steady_trigger.cxx
Author: Matthias W. Smith
Email:  mwsmith2@uw.edu

About:  The code implements a MIDAS frontend to be used as a
        stable, constant trigger for an Acromag DIO board.

\******************************************************************************/


//--- std includes -----------------------------------------------------------//
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
using std::string;

//--- other includes ---------------------------------------------------------//
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "TFile.h"
#include "TTree.h"
#include "midas.h"

//--- project includes -------------------------------------------------------//
#include "dio_trigger_board.hh"

//--- globals ----------------------------------------------------------------//

extern "C" {
  
  // The frontend name (client name) as seen by other MIDAS clients
  char *frontend_name = (char*) "steady-trigger";

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
      {"steady-trigger",     // equipment name 
       { 1, 0,          // event ID, trigger mask 
         "SYSTEM",      // event buffer 
         EQ_POLLED,     // equipment type 
         0,             // not used 
         "MIDAS",       // format 
         TRUE,          // enabled 
         RO_RUNNING,    // read only when running 
         25,            // poll for 25ms 
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
std::atomic<bool> use_steady_trigger;
std::atomic<bool> run_in_progress;
std::atomic<bool> trigger_sent;
std::atomic<bool> triggering;
std::atomic<double> trigger_time;
std::atomic<double> trigger_rate;
std::thread trigger_thread;

int trigger_port = 6;
char board_id = 'a';
int resolution_us = 100;
int trigger_mask = 0xff;
}

void trigger_loop(); 
void load_params();

//--- Load Param from ODB ------------------------------------------------//
void load_params() 
{
  HNDLE hDB, hkey;
  INT status, size;
  BOOL mstatus;
  char *str = new char[1024];
  int bytes_written = 0;
  
  cm_get_experiment_database(&hDB, NULL);
  db_find_key(hDB, 0, "Params/use-steady-trigger", &hkey);
  if (hkey) {
    size = sizeof(mstatus);
    db_get_data(hDB, hkey, &mstatus, &size, TID_BOOL);
    use_steady_trigger = mstatus;
  }

  db_find_key(hDB, 0, "Equipment/steady-trigger/Settings", &hkey);
  if (hkey) {
    size = 1024;
    db_copy_json_values(hDB, hkey, &str, &size, &bytes_written);
  }

  std::stringstream ss;
  ss << str;

  boost::property_tree::ptree pt;
  boost::property_tree::read_json(ss, pt);

  trigger_rate = pt.get<double>("trigger_rate");
  board_id = pt.get<char>("board_id");
  resolution_us = pt.get<int>("resolution_us");
  trigger_port = pt.get<int>("trigger_port");
  trigger_mask = pt.get<int>("trigger_mask");
}

//--- Frontend Init -------------------------------------------------//
INT frontend_init() 
{
  // Reload from ODB
  load_params();

  return SUCCESS;
}

//--- Frontend Exit ------------------------------------------------//
INT frontend_exit()
{
  return SUCCESS;
}

//--- Begin of Run --------------------------------------------------*/
INT begin_of_run(INT run_number, char *error)
{
  run_in_progress = true;
  triggering = true;

  // Reload from ODB
  load_params();

  if (use_steady_trigger) {
    trigger_thread = std::thread(&trigger_loop);
  }

  return SUCCESS;
}

//--- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error)
{
  triggering = false;
  run_in_progress = false;

  // Clean up thread if it was launched.
  if (trigger_thread.joinable()) {
    trigger_thread.join();
  }

  return SUCCESS;
}

//--- Pause Run -----------------------------------------------------*/
INT pause_run(INT run_number, char *error)
{
  triggering = false;
  return SUCCESS;
}

//--- Resuem Run ----------------------------------------------------*/
INT resume_run(INT run_number, char *error)
{
  triggering = true;
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
    
  return trigger_sent? 1 : 0;
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

//--- Event readout ----------------------------------------------------------//

INT read_trigger_event(char *pevent, INT off)
{
  char *bk_name = "TRIG";
  double *pdata;

  bk_init32(pevent);
  bk_create(pevent, bk_name, TID_DOUBLE, &pdata);

  *(pdata++) = trigger_time;

  bk_close(pevent, pdata);

  trigger_sent = false;

  return bk_size(pevent);
}

//--- Trigger Loop -----------------------------------------------------------//
void trigger_loop()
{
  using namespace std::chrono;
  using namespace daq;

  DioTriggerBoard *trigger;
  auto last_trigger = steady_clock::now();
  unsigned long long trigger_period_us = 1.0e6 / trigger_rate;

  switch (::board_id) {
  case 'a':
    trigger = new DioTriggerBoard(0x0, BOARD_A, trigger_port);
    break;

  case 'b':
    trigger = new DioTriggerBoard(0x0, BOARD_B, trigger_port);
    break;

  case 'c':
    trigger = new DioTriggerBoard(0x0, BOARD_C, trigger_port);   
    break;

  default:
    trigger = new DioTriggerBoard(0x0, BOARD_D, trigger_port);  
    break;
  }

  while (run_in_progress) {

    while (triggering) {

      // Get a timestamp to compare to the last trigger.
      auto t1 = steady_clock::now();
      auto dtn = t1.time_since_epoch() - last_trigger.time_since_epoch();

      // If enough time has passed, trigger, otherwise wait.
      if (duration_cast<microseconds>(dtn).count() > trigger_period_us) { 

        trigger->FireTriggers(::trigger_mask);
        trigger_sent = true;
        last_trigger = steady_clock::now();

      } else {

        usleep(resolution_us);
      }
    }

    // Wait for triggers to start again.
    usleep(1000);
  }
}
