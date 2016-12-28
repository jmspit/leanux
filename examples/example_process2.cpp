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
#include "process.hpp"
#include "oops.hpp"
#include "system.hpp"
#include "util.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>


using namespace std;

int main( int argc, char *argv[] ) {
  try {
    leanux::init();

    // two snapshot maps to hold per-pid statistics
    leanux::process::ProcPidStatMap snap1, snap2;
    // to hold the time interval
    struct timeval t1,t2;
    // to hold system wide cpu usage
    leanux::cpu::CPUStatsMap cpusnap1, cpusnap2, cpudelta;

    // CPU topology
    leanux::cpu::CPUTopology topology;
    leanux::cpu::getCPUTopology( topology );

    while ( true ) {
      // first CPU snapshot
      leanux::cpu::getCPUStats( cpusnap1 );
      // first pid snapshot
      leanux::process::getAllProcPidStat( snap1 );
      // first time
      gettimeofday( &t1, 0 );
      // wait
      leanux::util::Sleep( 60, 0 );
      // clear terminal
      cout << "\e[1;1H\e[2J";
      // second pid snapshot
      leanux::process::getAllProcPidStat( snap2 );
      // second cpu snapshot
      leanux::cpu::getCPUStats( cpusnap2 );
      // second time
      gettimeofday( &t2, 0 );
      // get delta of the two pid snaps
      leanux::process::ProcPidStatDeltaVector delta;
      deltaProcPidStats( snap1, snap2, delta );
      // delta time in seconds
      double dt = leanux::util::deltaTime( t1, t2 );
      // delta of the CPU snapshots
      leanux::cpu::deltaStats( cpusnap1, cpusnap2, cpudelta );
      // calc total CPU
      leanux::cpu::CPUStat cpu_total;
      leanux::cpu::getCPUTotal( cpudelta, cpu_total );

      // setup a pid sorter and sort
      leanux::process::StatsSorter my_sorter( leanux::process::StatsSorter::top );
      sort( delta.begin(), delta.end(), my_sorter );

      // track some total values
      double sutime = 0;
      double sstime = 0;
      unsigned long sminflt = 0;
      unsigned long smajflt = 0;
      double sdelayacct_blkio_ticks = 0;

      // print header
      cout << setw(6) << "pid";
      cout << setw(2) << "Q";
      cout << setw(18) << "comm";
      cout << setw(7) << "utime";
      cout << setw(7) << "stime";
      cout << setw(7) << "iotime";
      cout << setw(8) << "minflt";
      cout << setw(8) << "majflt";
      cout << setw(9) << "rss";
      cout << setw(9) << "vsize";
      cout << setw(28) << "wchan";
      cout << endl;
      cout << setfill('-') << setw(109) << '-' << setfill(' ') << endl;
      // loop over pid delta
      int c = 0;
      for ( leanux::process::ProcPidStatDeltaVector::const_iterator i = delta.begin(); i != delta.end(); i++, c++ ) {
        cout << setw(6) << (*i).pid;
        cout << setw(2) << snap2[(*i).pid].state;
        cout << setw(18) << snap2[(*i).pid].comm;
        cout << setw(7) << fixed << setprecision(3) << (*i).utime / dt;
        cout << setw(7) << fixed << setprecision(3) << (*i).stime / dt;
        cout << setw(7) << fixed << setprecision(3) << (*i).delayacct_blkio_ticks / dt;
        cout << setw(8) << (*i).minflt;
        cout << setw(8) << (*i).majflt;
        cout << setw(9) << leanux::util::ByteStr( (*i).rss, 3 );
        cout << setw(9) << leanux::util::ByteStr( (*i).vsize, 3 );
        cout << setw(28) << leanux::process::getWChan( (*i).pid );
        cout << endl;
        sutime += (*i).utime;
        sstime += (*i).stime;
        sminflt += (*i).minflt;
        smajflt += (*i).majflt;
        sdelayacct_blkio_ticks += (*i).delayacct_blkio_ticks;
        if ( c == 16 ) break;
      }
      cout << setfill('-') << setw(109) << '-' << setfill(' ') << endl;
      cout << setw(6+2+18) << "total";
      cout << setw(7) << fixed << setprecision(3) << sutime / dt;
      cout << setw(7) << fixed << setprecision(3) << sstime / dt;
      cout << setw(7) << fixed << setprecision(3) << sdelayacct_blkio_ticks / dt;
      cout << setw(8) << sminflt;
      cout << setw(8) << smajflt;
      cout << endl;
      cout << setw(6+2+18) << "system wide";
      cout << setw(7) << fixed << setprecision(3) << (cpu_total.user+cpu_total.nice) / dt;
      cout << setw(7) << fixed << setprecision(3) << cpu_total.system / dt;
      cout << endl;
      // note that the pid snapshots can miss processes that are too short lived to be captured
      // twice, but which still eat cpu. by comparing the 'total' against the 'system wide'
      // you can see what you don't see in the pid snapshots. if all pids are long lived,
      // the total and system wide values should be close.

    }


  }
  catch ( leanux::Oops &oops ) {
    cout << oops << endl;
    return 1;
  }
  return 0;
}
