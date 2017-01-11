//========================================================================
//
// This file is part of the leanux toolkit.
//
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

/**
 * @file
 * leanux::process c++ source file.
 */
#include "process.hpp"
#include "oops.hpp"
#include "system.hpp"
#include "util.hpp"

#include <fstream>
#include <sstream>
#include <iostream>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


 namespace leanux {

  namespace process {

    bool getProcPidStat( pid_t pid, ProcPidStat &stat ) {
      std::stringstream path;
      stat.pid = pid;
      stat.comm = "";
      path << "/proc/" << pid << "/stat";
      std::ifstream ifs( path.str().c_str() );
      std::string s;
      getline( ifs, s );
      if ( ifs.good() ) {
        size_t p = 0, q = 0;
        for ( p = 0; p < s.length(); p++ ) {
          if ( !(isdigit( s[p] ) || s[p] == ' ' ) ) {
            if ( s[p] != '(' ) throw Oops( __FILE__, __LINE__, "parse failure on " + path.str() ); else {
              //here begins the 'cmd' field
              for ( q = p+1; q < s.length() && (s[q] != ')' || (q<s.length()-1 && s[q+1] == ')')); q++ ) {
                stat.comm += s[q];
              }
              p = q+2;
              break;
            }
          }
        }
        unsigned long utime;
        unsigned long stime;
        unsigned long cutime;
        unsigned long cstime;
        unsigned long long delayacct_blkio_ticks;
        if ( 21 <= sscanf( s.substr(p).c_str(), "%c %d %d %d %d %d %*u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %*d %llu %lu %ld %lu"
                                                " %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*d" // ignore 26-startcode to 38-exit_signal
                                                "%d %*u %*u %llu",
                                                &stat.state,
                                                &stat.ppid,
                                                &stat.pgrp,
                                                &stat.session,
                                                &stat.tty_nr,
                                                &stat.tpgid,
                                                &stat.minflt,
                                                &stat.cminflt,
                                                &stat.majflt,
                                                &stat.cmajflt,
                                                &utime,
                                                &stime,
                                                &cutime,
                                                &cstime,
                                                &stat.priority,
                                                &stat.nice,
                                                &stat.num_threads,
                                                &stat.starttime,
                                                &stat.vsize,
                                                &stat.rss,
                                                &stat.rsslim,
                                                &stat.processor,
                                                &delayacct_blkio_ticks ) ) {
          stat.wchan = process::getWChan( pid );
          stat.utime = utime / (double)leanux::system::getUserHz();
          stat.stime = stime / (double)leanux::system::getUserHz();
          stat.cutime = cutime / (double)leanux::system::getUserHz();
          stat.cstime = cstime / (double)leanux::system::getUserHz();
          stat.delayacct_blkio_ticks = delayacct_blkio_ticks / (double)leanux::system::getUserHz();
        } else throw Oops( __FILE__, __LINE__, "parse failure on " + path.str() );
      } else return false;
      return true;
    };


    std::string getWChan( pid_t pid ) {
      std::stringstream ss;
      ss << "/proc/" << pid << "/wchan";
      std::string result = "";
      try {
        result = util::fileReadString( ss.str() );
      } catch ( ... ) {
        result = "";
      }
      return result;
    }

    bool getProcPidIO( pid_t pid, ProcPidIO &io ) {\
      std::stringstream ss;
      ss << "/proc/" << pid << "/io";
      std::ifstream ifs( ss.str().c_str() );
      if ( ifs.good() ) {
        while ( ifs.good() && !ifs.eof() ) {
          std::string s;
          getline( ifs, s );
          unsigned long t;
          if ( sscanf( s.c_str(), "rchar: %lu", &t ) == 1 ) io.rchar = t;
          else if ( sscanf( s.c_str(), "wchar: %lu", &t ) == 1 ) io.wchar = t;
          else if ( sscanf( s.c_str(), "syscr: %lu", &t ) == 1 ) io.syscr = t;
          else if ( sscanf( s.c_str(), "syscw: %lu", &t ) == 1 ) io.syscw = t;
          else if ( sscanf( s.c_str(), "read_bytes: %lu", &t ) == 1 ) io.read_bytes = t;
          else if ( sscanf( s.c_str(), "write_bytes: %lu", &t ) == 1 ) io.write_bytes = t;
          else if ( sscanf( s.c_str(), "cancelled_write_bytes: %lu", &t ) == 1 ) io.cancelled_write_bytes = t;
        }
      } else return false;
      return true;
    }

    std::string getProcCmdLine( pid_t pid ) {
      std::stringstream ss;
      ss << "/proc/" << pid << "/cmdline";
      std::string result = "";
      FILE *file = fopen( ss.str().c_str(), "r" );
      if ( file ) {
        char c = fgetc(file);
        while ( !feof(file) && !ferror(file) ) {
          if ( c != 0 ) result += c; else result += " ";
          c = fgetc(file);
        }
        fclose( file );
      }
      return result;
    }

    void getAllProcPidStat( ProcPidStatMap &stats ) {
      stats.clear();
      std::string path = "/proc";
      DIR *d;
      struct dirent *dir;
      d = opendir( path.c_str() );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( isdigit( dir->d_name[0] ) ) {
            pid_t pid = atoi( dir->d_name );
            if ( pid ) {
              ProcPidStat stat;
              if ( getProcPidStat( pid, stat ) ) {
                stats[pid] = stat;
              }
            }
          }
        }
      } else throw Oops( __FILE__, __LINE__, errno );
      closedir( d );
    };

    void getAllDirectChildren( pid_t parent, const ProcPidStatMap &snap, std::list<pid_t> &children ) {
      children.clear();
      for ( ProcPidStatMap::const_iterator i = snap.begin(); i != snap.end(); ++i ) {
        if ( i->second.ppid == parent ) children.push_back( i->first );
      }
    }

    void getOpenFiles( pid_t pid, OpenFileMap &files ) {
      files.clear();
      std::stringstream path;
      path << "/proc/" << pid << "/fd";
      DIR *d;
      struct dirent *dir;
      d = opendir( path.str().c_str() );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          std::stringstream link;
          link << path.str() << "/" << dir->d_name;
          unsigned long fd = strtoul( dir->d_name, NULL, 10 );
          if ( fd ) {
            char buf[4096];
            int r = readlink( link.str().c_str(), buf, sizeof(buf) );
            if ( r != -1 && r < (int)sizeof(buf) ) {
              buf[r] = 0;
              files[fd] = buf;
            }
          }
        }
      }
      closedir( d );
    }

    void deltaProcPidStats( const ProcPidStatMap &snap1, const ProcPidStatMap &snap2, ProcPidStatDeltaVector &delta ) {
      delta.clear();
      for ( ProcPidStatMap::const_iterator s2 = snap2.begin(); s2 != snap2.end(); ++s2 ) {
        ProcPidStatDelta dt;
        dt.pid = s2->first;
        ProcPidStatMap::const_iterator s1 = snap1.find( s2->first );
        if ( s1 != snap1.end() ) {
          dt.utime = s2->second.utime - s1->second.utime;
          dt.stime = s2->second.stime - s1->second.stime;
          dt.minflt = s2->second.minflt - s1->second.minflt;
          dt.majflt = s2->second.majflt - s1->second.majflt;
          dt.delayacct_blkio_ticks = s2->second.delayacct_blkio_ticks - s1->second.delayacct_blkio_ticks;
          dt.state = s2->second.state;
          dt.rss = s2->second.rss;
          dt.vsize = s2->second.vsize;
          dt.comm = s2->second.comm;
          dt.wchan = s2->second.wchan;
          delta.push_back( dt );
        } else {
          // with only end-interval statistics...we return simply the stats of snap2
          dt.utime = s2->second.utime;
          dt.stime = s2->second.stime;
          dt.minflt = s2->second.minflt;
          dt.majflt = s2->second.majflt;
          dt.delayacct_blkio_ticks = s2->second.delayacct_blkio_ticks;
          dt.state = s2->second.state;
          dt.rss = s2->second.rss;
          dt.vsize = s2->second.vsize;
          dt.comm = s2->second.comm;
          dt.wchan = s2->second.wchan;
          delta.push_back( dt );
        }
      }
    }

    /**
     * order process states from 'good' to 'bad'.
     */
    int valueState( char state ) {
      switch ( state ) {
        case 'S' : return 0;
        case 'R' : return 1;
        case 'D' : return 2;
        case 'Z' : return 3;
        default: return 4;
      }
    }

    int StatsSorter::operator()( ProcPidStatDelta d1, ProcPidStatDelta d2 ) {
      switch ( sortby_ ) {
        case utime:
          return
            (d1.utime > d2.utime) ||
            (d1.utime ==  d2.utime && d1.stime > d2.stime ) ||
            (d1.utime ==  d2.utime && d1.stime == d2.stime && d1.pid > d2.pid );
        case stime:
          return d1.stime > d2.stime;
        case cputime:
          return d1.utime+d1.stime > d2.utime + d2.stime;
        case minflt:
          return d1.minflt > d2.minflt;
        case majflt:
          return d1.majflt > d2.majflt;
        case rss:
          return d1.rss > d2.rss;
        case vsize:
          return d1.vsize > d2.vsize;
        case top:
        default:
          return
                 ( d1.utime+d1.stime+d1.delayacct_blkio_ticks > d2.utime+d2.stime+d2.delayacct_blkio_ticks ) ||
                 ( d1.utime+d1.stime+d1.delayacct_blkio_ticks == d2.utime+d2.stime+d2.delayacct_blkio_ticks && d1.majflt > d2.majflt ) ||
                 ( d1.utime+d1.stime+d1.delayacct_blkio_ticks == d2.utime+d2.stime+d2.delayacct_blkio_ticks && d1.majflt == d2.majflt && d1.minflt > d2.minflt ) ||
                 ( d1.utime+d1.stime+d1.delayacct_blkio_ticks == d2.utime+d2.stime+d2.delayacct_blkio_ticks && d1.majflt == d2.majflt && d1.minflt == d2.minflt && d1.rss > d2.rss);
      }
    }

    bool findProcByComm( const std::string& comm, ProcPidStatMap &stats ) {
      stats.clear();
      bool result = false;
      ProcPidStatMap all;
      getAllProcPidStat( all );
      for ( ProcPidStatMap::const_iterator i = all.begin(); i != all.end(); ++i ) {
        if ( i->second.comm == comm ) {
          result = true;
          stats[i->first] = i->second;
        }
      }
      return result;
    }

    bool getProcUid( pid_t pid, uid_t &uid ) {
      std::stringstream ss;
      ss << "/proc/" << pid << "/status";
      std::ifstream ifs( ss.str().c_str() );
      bool found = false;
      if ( ifs.good() ) {
        while ( ifs.good() && !ifs.eof() ) {
          std::string s;
          getline( ifs, s );
          if ( sscanf( s.c_str(), "Uid: %*u %u %*u %*u", &uid ) == 1 ) {
            found = true;
            break;
          }
        }
      } else return false;
      return found;
    }

    bool findProcByUid( uid_t uid, ProcPidStatMap &stats ) {
      stats.clear();
      bool result = false;
      ProcPidStatMap all;
      getAllProcPidStat( all );
      for ( ProcPidStatMap::const_iterator i = all.begin(); i != all.end(); ++i ) {
        uid_t l_uid;
        if ( getProcUid( i->first, l_uid ) ) {
          if ( l_uid == uid ) {
            result = true;
            stats[i->first] = i->second;
          }
        }
      }
      return result;
    }

  }
}
