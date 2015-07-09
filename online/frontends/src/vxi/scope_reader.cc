#include "scope_reader.h"
using namespace std;

/*vxi11_user error codes:
  *  (From published VXI-11 protocol, section B.5.2)
  *  0	no error
  *  1	syntax error
  *  3	device not accessible
  *  4	invalid link identifier
  *  5	parameter error
  *  6	channel not established
  *  8	operation not supported
  *  9	out of resources
  *  11	device locked by another link
  *  12	no lock held by this link
  *  15	I/O timeout
  *  17	I/O error
  *  21	invalid address
  *  23	abort
  *  29	channel already established
If you get one of these and can't think of an obvious reason, try restarting the scope. If this doesn't work, then good luck!
 */

double getTime(chrono::system_clock::time_point reference_time) {
  return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now()-reference_time).count()/1000.;
}

scope_reader::scope_reader(const char* ip_address, chrono::system_clock::time_point reference_time_in) {
  cl = new CLINK;
  ipAdd = ip_address;
  err = vxi11_open_device(ipAdd, cl);
  stopped=false;
  reference_time = reference_time_in;
  send("WFMP:BYT_N 2;NR_P 10000;ENC BIN;BN_F RI;BYT_O LSB;:HEAD OFF\n");
}

scope_reader::~scope_reader() {
  vxi11_close_device(ipAdd, cl);
  delete cl;
}

//Send a command to the scope
int scope_reader::send(string cmd) {
  if(cmd[cmd.size()-1] != '\n') cmd+='\n';
  err = vxi11_send(cl, cmd.c_str());
  return err;
}

//Query scope, returning an integer
int scope_reader::iQuery(string query) {
  send(query);
  char ret[16];
  vxi11_receive(cl, ret, sizeof ret); 
  return atoi(ret);
}

//Query scope, returning a double
double scope_reader::dQuery(string query) {
  send(query);
  char ret[16];
  vxi11_receive(cl, ret, sizeof ret); 
  return atof(ret);
}

//Query scope, returning a string
string scope_reader::sQuery(string query) {
  send(query);
  char ret[1000];
  vxi11_receive(cl, ret, sizeof ret); 
  return string(ret);
}

//stop waveform acquisition
int scope_reader::stop() {
  stopped=true;
  tom=getTime(reference_time);
  return send("ACQ:STATE 0\n");
}

//start waveform acquisition
int scope_reader::start() {
  stopped=false;
  return send("ACQ:STATE 1\n");
}

//Retrieve the waveform from channel ch of the scope, storing times in time and waveform in wfm, both length N arrays
int scope_reader::getWfm(char ch, int N, double* time, double* wfm, double& start_time) {
  if(ch>'4' || ch<'1') {
    cout << "Channel must be between 1 and 4" << endl;
    return 0;
  }
  if(!stopped) tom=getTime(reference_time);
  start_time = tom;
  
  string query("DAT:SOU CH");
  query+= ch;
  send(query);

  double dt = dQuery("WFMP:XIN?\n")*1000.;
  double t0 = dQuery("WFMP:XZE?\n")*1000.;
  double dV = dQuery("WFMP:YMU?\n")*1000.;
  send("CURV?\n");

  char res[20011];
  int n = vxi11_receive(cl, res, 20011);

  //Header has form #n<length of waveform><waveform>, where n is number of digits in <length of buffer>; this means header has n+2 chars, where n<=9. Waveform is returned as binary string with each data point containing 2 bytes. Use bitwise logic to convert pairs of bytes into a short and multiply by dV to get voltage in mV
  int head = res[1] - '0' + 2;
  for(int i=0; i<N; i++) {
    short wfmbin = (res[2*i+head+1]<<8) | (res[2*i+head] & 255);
    wfm[i] = wfmbin * dV;
    time[i] = t0 + i*dt;
    //if(i%100 == 0) cout << time[i] << ':' << wfm[i] << " , ";
  }
  return n;
}

int scope_reader::getWfm(int ch, int N, double* time, double* wfm, double &start_time) {
  return getWfm((char)('0'+ch), N, time, wfm, start_time);
}
