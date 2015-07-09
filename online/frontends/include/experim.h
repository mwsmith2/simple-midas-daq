/********************************************************************\

  Name:         experim.h
  Created by:   ODBedit program

  Contents:     This file contains C structures for the "Experiment"
                tree in the ODB and the "/Analyzer/Parameters" tree.

                Additionally, it contains the "Settings" subtree for
                all items listed under "/Equipment" as well as their
                event definition.

                It can be used by the frontend and analyzer to work
                with these information.

                All C structures are accompanied with a string represen-
                tation which can be used in the db_create_record function
                to setup an ODB structure which matches the C structure.

  Created on:   Thu Sep 25 14:03:53 2014

\********************************************************************/

#define EXP_PARAM_DEFINED

typedef struct {
  char      comment[80];
} EXP_PARAM;

#define EXP_PARAM_STR(_name) const char *_name[] = {\
"[.]",\
"Comment = STRING : [80] Test",\
"",\
NULL }

#define EXP_EDIT_DEFINED

typedef struct {
  char      comment[80];
} EXP_EDIT;

#define EXP_EDIT_STR(_name) const char *_name[] = {\
"[.]",\
"Comment = LINK : [35] /Experiment/Run Parameters/Comment",\
"",\
NULL }

#ifndef EXCL_GLOBAL

#define GLOBAL_PARAM_DEFINED

typedef struct {
  float     adc_threshold;
} GLOBAL_PARAM;

#define GLOBAL_PARAM_STR(_name) const char *_name[] = {\
"[.]",\
"ADC Threshold = FLOAT : 5",\
"",\
NULL }

#endif

#ifndef EXCL_VME_BASIC

#define VME_BASIC_SETTINGS_DEFINED

typedef struct {
  BYTE      io506;
} VME_BASIC_SETTINGS;

#define VME_BASIC_SETTINGS_STR(_name) const char *_name[] = {\
"[.]",\
"IO506 = BYTE : 7",\
"",\
NULL }

#define VME_BASIC_COMMON_DEFINED

typedef struct {
  WORD      event_id;
  WORD      trigger_mask;
  char      buffer[32];
  INT       type;
  INT       source;
  char      format[8];
  BOOL      enabled;
  INT       read_on;
  INT       period;
  double    event_limit;
  DWORD     num_subevents;
  INT       log_history;
  char      frontend_host[32];
  char      frontend_name[32];
  char      frontend_file_name[256];
  char      status[256];
  char      status_color[32];
  BOOL      hidden;
} VME_BASIC_COMMON;

#define VME_BASIC_COMMON_STR(_name) const char *_name[] = {\
"[.]",\
"Event ID = WORD : 1",\
"Trigger mask = WORD : 0",\
"Buffer = STRING : [32] SYSTEM",\
"Type = INT : 2",\
"Source = INT : 0",\
"Format = STRING : [8] MIDAS",\
"Enabled = BOOL : y",\
"Read on = INT : 257",\
"Period = INT : 500",\
"Event limit = DOUBLE : 0",\
"Num subevents = DWORD : 0",\
"Log history = INT : 0",\
"Frontend host = STRING : [32] cenpa-nmr-daq5",\
"Frontend name = STRING : [32] basic vme",\
"Frontend file name = STRING : [256] modules/fe_vme_basic.cxx",\
"Status = STRING : [256] basic vme@cenpa-nmr-daq5",\
"Status color = STRING : [32] #00FF00",\
"Hidden = BOOL : n",\
"",\
NULL }

#endif

