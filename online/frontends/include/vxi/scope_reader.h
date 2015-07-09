#ifndef SCOPE_READOUT_H
#define SCOPE_READOUT_H

#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include "vxi11_user.h"
using namespace std;

double getTime(chrono::system_clock::time_point reference_time = chrono::system_clock::time_point()); //gets time in s from reference_time; default reference_time is Jan 1 1970; resolution is ms

class scope_reader {
  CLINK *cl;
  const char* ipAdd; //ip address of scope
  int err; //error code; errors listed at beginning of source file
  double tom; //time of measurement
  bool stopped;
  chrono::system_clock::time_point reference_time;

 public:
  scope_reader(const char* ip_address = "10.95.100.12", chrono::system_clock::time_point reference_time_in = chrono::system_clock::now());
  ~scope_reader();
  void clearBuffer();
  int send(string cmd);
  int iQuery(string query);
  double dQuery(string query);
  string sQuery(string query);
  int stop();
  int start();
  int getWfm(char ch, int N, double* time, double* wfm, double& start_time);
  int getWfm(int ch, int N, double* time, double* wfm, double& start_time);
};

#endif
