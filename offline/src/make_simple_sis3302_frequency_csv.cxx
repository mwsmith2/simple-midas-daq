/*===========================================================================*\

author: Matthias W. Smith
email:  mwsmith2@uw.edu
date:   2015/09/04

about:  The program runs on a simple-daq output file and extracts times
        and frequencies.

usage:

./make_simple_frequency_csv <data-file> <output-file> [channel]

\*===========================================================================*/


//--- std includes ----------------------------------------------------------//
#include <iostream>
#include <cmath>

//--- other includes --------------------------------------------------------//
#include <boost/filesystem.hpp>
#include "TFile.h"
#include "TTree.h"
#include "fid.h"

using namespace std;

#define SIS_3302_CH 8
#define SIS_3302_LN 100000

struct sis_3302 {
  ULong64_t system_clock;
  ULong64_t device_clock[SIS_3302_CH];
  UShort_t trace[SIS_3302_CH][SIS_3302_LN];
};

int main(int argc, char *argv[])
{
  string datafile;
  string outfile;
  ushort channel;
  const int FID_LEN = 50000;
  const double dt = 0.0002;
  double t0 = 0.0;

  sis_3302 data;

  // And the ROOT variables.
  TFile *pf;
  TTree *pt;

  if (argc < 3) {
    cout << "Insufficent arguments, usage:" << endl;
    cout << "./make_simple_sis3302_frequencies_csv <data-file> <output-file>";
    cout << endl;
    exit(1);
  }

  datafile = string(argv[1]);
  outfile = string(argv[2]);

  if (argc > 3) {
    channel = atoi(argv[3]);
  } else {
    channel = 0;
  }

  // Now get the input data.
  pf = new TFile(datafile.c_str());
  pt = (TTree *)pf->Get("t_sis3302");

  vector<double> wf(FID_LEN, 0.0);
  vector<double> tm(FID_LEN, 0.0);
  UShort_t trace[FID_LEN];
  ULong64_t clock;

  for (int i = 0; i < FID_LEN; ++i) {
    tm[i] = i * dt;
  }

  pt->SetBranchAddress("sis_3302_0", &data.system_clock);

  // Make sure we can write the output data.
  boost::filesystem::path p(outfile);
  boost::filesystem::create_directories(p.remove_filename());

  ofstream out(outfile);
  out.precision(12);

  for (int i = 0; i < pt->GetEntries(); ++i) {

    cout << "Round " << i << endl;

    if (i % (pt->GetEntries() / 10) == 0) {
      printf("%i/%lli complete", i, pt-> GetEntries());
    }

    pt->GetEntry(i);

    if (i == 0) {
      t0 = clock;
    }

    for (int n = 0; n < FID_LEN; ++n) {
      wf[n] = (double)data.trace[0][n * 2 + 1];
    }

    fid::Fid myfid(wf, tm);

    // int i_wf = myfid.i_wf();
    // myfid.set_f_wf(i_wf + 10000);
    myfid.DiagnosticInfo(cout);

    out << data.device_clock[0] / 1.0e6 << ", " << myfid.GetFreq() << endl;
  }

  out.close();

  return 0;
}
