//========================================================================
// Copyright (C) 2015-2016 Jan-Marten Spit https://github.com/jmspit/leanux
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
#include "net.hpp"
#include "process.hpp"
#include "oops.hpp"
#include "system.hpp"
#include "util.hpp"

#include <iomanip>
#include <iostream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>


using namespace std;

void recurPidChildren( pid_t root, int level, leanux::process::ProcPidStatMap &snap ) {
  list<pid_t> children;
  leanux::process::getAllDirectChildren( root, snap, children );
  for ( list<pid_t>::const_iterator i = children.begin(); i != children.end(); i++ ) {
    cout << setw(level) << " " << *i;
    cout << " " << snap[*i].comm;
    cout << " " << snap[*i].utime;
    cout << " " << snap[*i].rss * leanux::system::getPageSize();
    cout << endl;
    recurPidChildren( *i, level + 2, snap );
  }
}

int main( int argc, char *argv[] ) {
  try {
    leanux::init();
    pid_t pid = 1;
    if ( argc > 1 ) pid = atoi( argv[1] );
    if ( getuid() != 0 ) cout << "i am not root - some info cannot be shown" << endl;
    leanux::process::ProcPidStat stat;
    if ( leanux::process::getProcPidStat( pid, stat ) ) {
      ssize_t c = 21;
      cout << left << setw(c) << "pid" << ": " << stat.pid << endl;
      cout << left << setw(c) << "ppid" << ": " << stat.ppid << endl;
      cout << left << setw(c) << "pgrp" << ": " << stat.pgrp << endl;
      cout << left << setw(c) << "session" << ": " << stat.session << endl;
      cout << left << setw(c) << "tpgid" << ": " << stat.tpgid << endl;
      cout << left << setw(c) << "executable" << ": " << stat.comm << endl;
      cout << left << setw(c) << "state" << ": " << stat.state << endl;
      cout << left << setw(c) << "tty_nr" << ": " << stat.tty_nr << endl;
      cout << left << setw(c) << "minflt" << ": " << stat.minflt << endl;
      cout << left << setw(c) << "cminflt" << ": " << stat.cminflt << endl;
      cout << left << setw(c) << "majflt" << ": " << stat.majflt << endl;
      cout << left << setw(c) << "cmajflt" << ": " << stat.cmajflt << endl;
      cout << left << setw(c) << "utime" << ": " << leanux::util::TimeStrSec( stat.utime ) << endl;
      cout << left << setw(c) << "stime" << ": " << leanux::util::TimeStrSec( stat.stime ) << endl;
      cout << left << setw(c) << "cutime" << ": " << leanux::util::TimeStrSec( stat.cutime ) << endl;
      cout << left << setw(c) << "cstime" << ": " << leanux::util::TimeStrSec( stat.cstime ) << endl;
      cout << left << setw(c) << "priority" << ": " << stat.priority << endl;
      cout << left << setw(c) << "nice" << ": " << stat.nice << endl;
      cout << left << setw(c) << "num_threads" << ": " << stat.num_threads << endl;
      cout << left << setw(c) << "starttime" << ": " << stat.starttime << endl;
      time_t t = leanux::system::getBootTime() + stat.starttime/leanux::system::getUserHz();
      struct timeval tv;
      gettimeofday( &tv, 0 );
      t = tv.tv_sec - t;
      cout << left << setw(c) << "runtime" << ": " << leanux::util::TimeStrSec( t ) << " seconds" << endl;
      cout << left << setw(c) << "vsize" << ": " << leanux::util::ByteStr( stat.vsize, 3 ) << endl;
      cout << left << setw(c) << "rss" << ": " << leanux::util::ByteStr( stat.rss * leanux::system::getPageSize(), 3 ) << endl;
      cout << left << setw(c) << "rsslim" << ": " << leanux::util::ByteStr( stat.rsslim, 3 ) << endl;
      cout << left << setw(c) << "processor" << ": " << stat.processor << endl;
      cout << left << setw(c) << "delayacct_blkio_ticks" << ": " << stat.delayacct_blkio_ticks << endl;
      cout << left << setw(c) << "wchan" << ": " << leanux::process::getWChan(stat.pid) << endl;

      cout << left << setw(c) << "cmdline args" << ": " << leanux::process::getProcCmdLine( stat.pid ) << endl;

      if ( getuid() == 0 ) {
        leanux::process::ProcPidIO io;
        if ( leanux::process::getProcPidIO( stat.pid, io ) ) {
          cout << fixed << setprecision(2);
          cout << left << setw(c) << "rchar" << ": " << leanux::util::ByteStr( io.rchar, 3 ) << endl;
          cout << left << setw(c) << "wchar" << ": " << leanux::util::ByteStr( io.wchar, 3 ) << endl;
          cout << left << setw(c) << "syscr" << ": " << io.syscr << endl;
          cout << left << setw(c) << "syscw" << ": " << io.syscw << endl;
          cout << left << setw(c) << "read_bytes" << ": " << leanux::util::ByteStr( io.read_bytes, 3 ) << endl;
          cout << left << setw(c) << "write_bytes" << ": " << leanux::util::ByteStr( io.write_bytes,3 ) << endl;
        }
      }

      cout << endl << "child processes" << endl;
      list<pid_t> children;
      leanux::process::ProcPidStatMap snap;
      leanux::process::getAllProcPidStat( snap );
      recurPidChildren( pid, 0, snap );

      cout << endl << "open files" << endl;
      cout << setw(8) << "fd" << " file" << endl;
      leanux::process::OpenFileMap files;
      leanux::process::getOpenFiles( pid, files );
      for ( leanux::process::OpenFileMap::const_iterator i = files.begin(); i != files.end(); ++i ) {
        ino_t inode;
        if ( sscanf( i->second.c_str(), "socket:[%lu]", &inode ) == 1 ) {
          leanux::net::TCP4SocketInfo tcp4info;
          leanux::net::UnixDomainSocketInfo udsinfo;
          leanux::net::UDP4SocketInfo udp4info;
          if ( leanux::net::findTCP4SocketByINode( inode, tcp4info ) ) {
            cout << setw(8) << i->first << " " << tcp4info << endl;
          } else if ( leanux::net::findUnixDomainSocketByINode( inode, udsinfo ) ) {
            cout << setw(8) << i->first << " " << udsinfo << endl;
          } else if ( leanux::net::findUDP4SocketByINode( inode, udp4info ) ) {
            cout << setw(8) << i->first << " " << udp4info << endl;
          } else cout << setw(8) << i->first << " " << i->second << endl;
        } else cout << setw(8) << i->first << " " << i->second << endl;
      }

    } else cout << "pid not found (getProcPidStat)" << endl;

  }
  catch ( leanux::Oops &oops ) {
    cout << oops << endl;
    return 1;
  }
  return 0;
}
