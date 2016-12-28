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
 * leanux::cpu c++ source file.
 */
#include "cpu.hpp"
#include "oops.hpp"
#include "system.hpp"
#include "util.hpp"
#include "device.hpp"

#include <fstream>
#include <set>

#include <string.h>
#include <dirent.h>
#include <stdio.h>

#include <iostream>

namespace leanux {

  /**
   * CPU configuration and performance API.
   */
  namespace cpu {

    /**
     * Utility struct to aid in identifying unique cores.
     * @see getCPUTopology.
     */
    struct CoreId {
      /** the physical id of a (logical) CPU. */
      unsigned long phy_id;
      /** the core id of a (logical) CPU. */
      unsigned long core_id;
      /**
       * Compare CoreId.
       * @param cid the COreId to compare to.
       * @return true when *this < cid.
       */
      bool operator<( const CoreId& cid ) const { return phy_id < cid.phy_id || (phy_id == cid.phy_id && core_id < cid.core_id);  };
    };

    void getCPUTopology( CPUTopology &topology ) {
      topology.logical = 0;
      topology.physical = 0;
      topology.cores = 0;
      DIR *d;
      struct dirent *dir;
      std::set<CoreId> cores;
      std::set<unsigned long> physical;
      d = opendir( (sysdevice::sysdevice_root + "/system/cpu").c_str() );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          unsigned int cpunum = 0;
          if ( sscanf( dir->d_name, "cpu%u", &cpunum ) == 1 ) {
            topology.logical++;
            CoreId cid;
            cid.phy_id = util::fileReadUL( (std::string)sysdevice::sysdevice_root + "/system/cpu/" + dir->d_name + "/topology/physical_package_id" );
            cid.core_id = util::fileReadUL( (std::string)sysdevice::sysdevice_root + "/system/cpu/" + dir->d_name + "/topology/core_id" );
            cores.insert( cid );
            physical.insert( cid.phy_id);
          }
        }
        closedir( d );
      } else throw Oops( __FILE__, __LINE__, "failed to read directory " + sysdevice::sysdevice_root + "/system/cpu" );
      topology.physical = physical.size();
      topology.cores = cores.size();
      if ( topology.logical < 1 ||
           topology.physical < 1 ||
           topology.cores < 1 ||
           topology.physical > topology.cores ||
           topology.cores > topology.logical ) throw Oops( __FILE__, __LINE__, "the detected CPU topology is erratic" );

    };

    unsigned long getCPUPhysicalId( unsigned long logicalcpu ) {
      std::stringstream ss;
      ss << sysdevice::sysdevice_root <<  "/system/cpu/cpu" << std::fixed << logicalcpu << "/topology/physical_package_id";
      return util::fileReadUL( ss.str() );
    }

    unsigned long getCPUCoreId( unsigned long logicalcpu ) {
      std::stringstream ss;
      ss << sysdevice::sysdevice_root <<  "/system/cpu/cpu" << std::fixed << logicalcpu << "/topology/core_id";
      return util::fileReadUL( ss.str() );
    }

    bool getCPUInfo( CPUInfo &info ) {
      std::ifstream ifs( "/proc/cpuinfo" );
      std::string s;
      int num_found = 0;
      while ( ifs.good() && !ifs.eof() && num_found < 3 ) {
        getline( ifs, s );

        //Intel
        double dv = 0;
        if ( strncmp( s.c_str(), "model name", 10 ) == 0 ) {
          info.model = s.substr(13);
          num_found++;
        } else if ( sscanf( s.c_str(), "cpu MHz   : %lf", &dv ) == 1 ) {
          info.cpu_mhz = dv;
          num_found++;
        } else if ( sscanf( s.c_str(), "bogomips  : %lf", &dv ) == 1 ) {
          info.bogomips = dv;
          num_found++;
        }
        //PPC
        else if ( strncmp( s.c_str(), "cpu : ", 6 ) == 0 ) {
          info.model = s.substr(6);
          num_found++;
        } else if ( sscanf( s.c_str(), "clock : %lfMHz", &dv ) == 1 ) {
          info.cpu_mhz = dv;
          num_found++;
        } else if ( sscanf( s.c_str(), "clock : %lf MHz", &dv ) == 1 ) {
          info.cpu_mhz = dv;
          num_found++;
        } else if ( sscanf( s.c_str(), "bogomips : %lf", &dv ) == 1 ) {
          info.bogomips = dv;
          num_found++;
        }
      }
      return (num_found == 3 );
    }

    void getCPUStats( CPUStatsMap &stats ) {
      std::ifstream ifs( "/proc/stat" );
      std::string s;
      stats.clear();
      while ( ifs.good() && !ifs.eof() ) {
        std::getline( ifs, s );
        if ( strncmp( s.c_str(), "cpu ", 4 ) != 0 ) { //skip total cpu line, as 'cpu 123' matches "cpu%u" in sscanf!
          unsigned int cpunum = 0;
          CPUStat stat;
          memset( &stat, 0, sizeof(stat) );
          unsigned long user = 0;
          unsigned long nice = 0;
          unsigned long system = 0;
          unsigned long idle = 0;
          unsigned long iowait = 0;
          unsigned long irq = 0;
          unsigned long softirq = 0;
          unsigned long steal = 0;
          unsigned long guest = 0;
          unsigned long guest_nice = 0;
          if ( sscanf( s.c_str(), "cpu%u %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                                  &cpunum,
                                  &user,
                                  &nice,
                                  &system,
                                  &idle,
                                  &iowait,
                                  &irq,
                                  &softirq,
                                  &steal,
                                  &guest,
                                  &guest_nice
                                  ) == 11 ) {
            stat.user = (double)user / leanux::system::getUserHz();
            stat.nice = (double)nice / leanux::system::getUserHz();
            stat.system = (double)system / leanux::system::getUserHz();
            stat.idle = (double)idle / leanux::system::getUserHz();
            stat.iowait = (double)iowait / leanux::system::getUserHz();
            stat.irq = (double)irq / leanux::system::getUserHz();
            stat.softirq = (double)softirq / leanux::system::getUserHz();
            stat.steal = (double)steal / leanux::system::getUserHz();
            stat.guest = (double)guest / leanux::system::getUserHz();
            stat.guest_nice = (double)guest_nice / leanux::system::getUserHz();
            stats[cpunum] = stat;
          } else if ( sscanf( s.c_str(), "cpu%u %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                                  &cpunum,
                                  &user,
                                  &nice,
                                  &system,
                                  &idle,
                                  &iowait,
                                  &irq,
                                  &softirq,
                                  &steal,
                                  &guest
                                  ) == 10 ) {
            stat.user = (double)user / leanux::system::getUserHz();
            stat.nice = (double)nice / leanux::system::getUserHz();
            stat.system = (double)system / leanux::system::getUserHz();
            stat.idle = (double)idle / leanux::system::getUserHz();
            stat.iowait = (double)iowait / leanux::system::getUserHz();
            stat.irq = (double)irq / leanux::system::getUserHz();
            stat.softirq = (double)softirq / leanux::system::getUserHz();
            stat.steal = (double)steal / leanux::system::getUserHz();
            stat.guest = (double)guest / leanux::system::getUserHz();
            stat.guest_nice = 0;
            stats[cpunum] = stat;
          }
        }
      }
    }

    bool deltaStats( const CPUStatsMap &earlier, const CPUStatsMap &later, CPUStatsMap &delta ) {
      bool result = true;
      delta.clear();
      if ( earlier.size() != later.size() ) result = false; else {
        for ( CPUStatsMap::const_iterator e = earlier.begin(); e != earlier.end(); ++e ) {
          CPUStatsMap::const_iterator l = later.find( e->first );

          if ( l == later.end() ) {
            result = false;
            break;
          } else {
            if ( l->second.user >= e->second.user ) delta[e->first].user = l->second.user - e->second.user; else { result = false; break; };
            if ( l->second.nice >= e->second.nice ) delta[e->first].nice = l->second.nice - e->second.nice; else { result = false; break; };
            if ( l->second.system >= e->second.system ) delta[e->first].system = l->second.system - e->second.system; else { result = false; break; };
            if ( l->second.idle >= e->second.idle ) delta[e->first].idle = l->second.idle - e->second.idle; else { result = false; break; };
            if ( l->second.iowait >= e->second.iowait ) delta[e->first].iowait = l->second.iowait - e->second.iowait; else { result = false; break; };
            if ( l->second.irq >= e->second.irq ) delta[e->first].irq = l->second.irq - e->second.irq; else { result = false; break; };
            if ( l->second.softirq >= e->second.softirq ) delta[e->first].softirq = l->second.softirq - e->second.softirq; else { result = false; break; };
            if ( l->second.steal >= e->second.steal ) delta[e->first].steal = l->second.steal - e->second.steal; else { result = false; break; };
            if ( l->second.guest >= e->second.guest ) delta[e->first].guest = l->second.guest - e->second.guest; else { result = false; break; };
            if ( l->second.guest_nice >= e->second.guest_nice ) delta[e->first].guest_nice = l->second.guest_nice - e->second.guest_nice; else { result = false; break; };
          }
        }
      }
      if (!result) delta.clear();
      return result;
    }

    std::ostream& operator<<( std::ostream &os, CPUInfo &info ) {
      os << "cpu model  : " << info.model << std::endl;
      os << "cpu_mhz    : " << info.cpu_mhz << std::endl;
      os << "bogomips   : " << info.bogomips << std::endl;
      return os;
    }


    void getLoadAvg( LoadAvg &avg ) {
      std::ifstream ifs( "/proc/loadavg" );
      std::string s;
      getline( ifs, s );
      if ( ifs.good() ) {
        if ( sscanf( s.c_str(), "%lf %lf %lf", &avg.avg5_, &avg.avg10_, &avg.avg15_ ) != 3 )
          throw Oops( __FILE__, __LINE__, "failed to parse /proc/loadavg" );
      } else throw Oops( __FILE__, __LINE__, "unable ro read /proc/loadavg" );
    }

    SchedInfo getSchedInfo() {
      SchedInfo rq;
      std::ifstream ifs( "/proc/stat" );
      std::string s;
      rq.running = 0;
      rq.blocked = 0;
      int match = 0;
      while ( ifs.good() && !ifs.eof() && match < 4 ) {
        getline( ifs, s );
        if ( sscanf( s.c_str(), "procs_running %u", &rq.running ) == 1 ) match++;
        else if ( sscanf( s.c_str(), "procs_blocked %u", &rq.blocked ) == 1 ) match++;
        else if ( sscanf( s.c_str(), "processes %lu", &rq.processes ) == 1 ) match++;
        else if ( sscanf( s.c_str(), "ctxt %lu", &rq.ctxt ) == 1 ) match++;
      }
      if ( match != 4 ) throw Oops( __FILE__, __LINE__, "/proc/stat parse failure" );
      return rq;
    }

    void getCPUTotal( const CPUStatsMap &all, CPUStat &total ) {
      total.user = 0;
      total.nice = 0;
      total.system = 0;
      total.idle = 0;
      total.iowait = 0;
      total.irq = 0;
      total.softirq = 0;
      total.steal = 0;
      total.guest = 0;
      total.guest_nice = 0;
      for ( CPUStatsMap::const_iterator i = all.begin(); i != all.end(); ++i ) {
        total.user += i->second.user;
        total.nice += i->second.nice;
        total.system += i->second.system;
        total.idle += i->second.idle;
        total.iowait += i->second.iowait;
        total.irq += i->second.irq;
        total.softirq += i->second.softirq;
        total.steal += i->second.steal;
        total.guest += i->second.guest;
        total.guest_nice += i->second.guest_nice;
      }
    }

    void normalizeCPUstats( CPUStat &stat, double dt, unsigned int processors ) {
      double factor = dt * (double)processors;
      stat.user /= factor;
      stat.nice /= factor;
      stat.system /= factor;
      stat.idle /= factor;
      stat.iowait /= factor;
      stat.irq /= factor;
      stat.softirq /= factor;
      stat.steal /= factor;
      stat.guest /= factor;
      stat.guest_nice /= factor;
    }

  }

}
