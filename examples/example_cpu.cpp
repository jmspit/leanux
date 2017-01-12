//========================================================================
// Copyright (C) 2015-2016 Jan-Marten Spit http://www.o-rho.com/leanux
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, distribute with modifications, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
// THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name(s) of the above copyright
// holders shall not be used in advertising or otherwise to promote the
// sale, use or other dealings in this Software without prior written
// authorization.
//========================================================================

//========================================================================
//  Author: Jan-Marten Spit
//========================================================================
#include "cpu.hpp"
#include "system.hpp"
#include "oops.hpp"
#include "util.hpp"

#include <iomanip>
#include <iostream>

using namespace std;

int main() {
  try {
    leanux::init();
    // show the number of physical CPUs,
    // the number of cores and the
    // number of logical CPUs.
    leanux::cpu::CPUTopology topology;
    leanux::cpu::getCPUTopology( topology );
    cout << "CPU topology: "
         << topology.physical << " physical, "
         << topology.cores << " cores, "
         << topology.logical << " logical" << endl << endl;

    leanux::cpu::CPUInfo info;
    getCPUInfo( info );
    // we make use of the << operator on info
    // instead of acessing the CPUInfo attributes directly.
    cout << "CPU info:" << endl << info << endl << endl;

    leanux::cpu::LoadAvg lavg;
    leanux::cpu::getLoadAvg( lavg );
    cout << "5, 10 and 15 minute load averages:" << endl;
    cout << lavg.avg5_ << " " << lavg.avg10_ << " " << lavg.avg15_ << endl << endl;

    //running and blocked processes
    leanux::cpu::SchedInfo q = leanux::cpu::getSchedInfo();
    cout << "processes (forks)  : " << q.processes << endl;
    cout << "avg forks/s        : " << q.processes/leanux::system::getUptime() << endl;
    cout << "context switches   : " << q.ctxt << endl;
    cout << "avg ctxt switch/s  : " << q.ctxt/leanux::system::getUptime() << endl;
    cout << "processes running  : " << q.running << endl;
    cout << "processes blocked  : " << q.blocked << endl << endl;

    cout << "CPU usage:" << endl;
    //two maps of stats keyed by cpu number
    leanux::cpu::CPUStatsMap stats1;
    leanux::cpu::CPUStatsMap stats2;
    leanux::cpu::CPUStatsMap delta;
    //get the first sample
    leanux::cpu::getCPUStats( stats1 );
    //wait a bit
    int sleep_interval = 2;
    leanux::util::Sleep( sleep_interval, 0 );
    //get the second sample
    leanux::cpu::getCPUStats( stats2 );
    // header
    cout << fixed << setprecision(1);
    cout << left << setw(7) << "CPU#" << " ";
    cout << right << setw(7) << "user" << " ";
    cout << right << setw(7) << "nice" << " ";
    cout << right << setw(7) << "system" << " ";
    cout << right << setw(7) << "idle" << " ";
    cout << right << setw(7) << "iowait" << " ";
    cout << right << setw(7) << "irq" << " ";
    cout << right << setw(7) << "softirq" << " ";
    cout << right << setw(7) << "steal" << " ";
    cout << right << setw(7) << "guest" << " ";
    cout << right << setw(11) << "guest_nice" << " ";
    cout << endl;
    if ( leanux::cpu::deltaStats( stats1, stats2, delta ) ) {
      for ( leanux::cpu::CPUStatsMap::const_iterator i = delta.begin(); i != delta.end(); ++i ) {
        double scale = (double)sleep_interval / 100.0;
        cout << "CPU" << left << setw(3) << i->first << " ";
        cout << right << setw(7) << i->second.user / scale << "%";
        cout << right << setw(7) << i->second.nice / scale << "%";
        cout << right << setw(7) << i->second.system / scale << "%";
        cout << right << setw(7) << i->second.idle / scale << "%";
        cout << right << setw(7) << i->second.iowait / scale << "%";
        cout << right << setw(7) << i->second.irq / scale << "%";
        cout << right << setw(7) << i->second.softirq / scale << "%";
        cout << right << setw(7) << i->second.steal / scale << "%";
        cout << right << setw(7) << i->second.guest / scale << "%";
        cout << right << setw(11) << i->second.guest_nice / scale << "%";
        cout << left << endl;
      }
    }
  }
  catch ( leanux::Oops &oops ) {
    cerr << oops << endl;
    return 1;
  }
  return 0;
}
