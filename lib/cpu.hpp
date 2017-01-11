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
 * leanux::cpu c++ header file.
 */
#ifndef LEANUX_CPU_HPP
#define LEANUX_CPU_HPP

#include <string>
#include <ostream>
#include <map>

namespace leanux {

  /**
   * \example example_cpu.cpp
   * leanux::cpu example.
   */

  /**
   * CPU configuration and performance API.
   */
  namespace cpu {

    /**
     * CPU topology.
     */
    struct CPUTopology {
      /** number of physical CPU's aka CPU sockets aka CPU packages. */
      unsigned int physical;
      /** number of CPU cores */
      unsigned int cores;
      /** number of logical CPUs */
      unsigned int logical;
    };

    /**
     * get the CPU topology. Should work on diffrent architectures as it does not rely
     * on architecture dependent /proc/cpuinfo but inspects /sys/devices/cpu/cpu?/topology
     * instead.
     * @throw Oops when directory tree /sys/devices/system/cpu cannot be interpreted as expected.
     * @param topology the CPUTopology to fill.
     */
    void getCPUTopology( CPUTopology &topology );

    /**
     * get the physical id of the logical CPU from
     * /sys/devices/system/cpu/cpuX/physical_package_id.
     * @param logicalcpu the logical CPU number (starting from zero).
     * @return the physical id (starting from zero).
     */
    unsigned long getCPUPhysicalId( unsigned long logicalcpu );

    /**
     * get the core id of the logical CPU from
     * /sys/devices/system/cpu/cpuX/core_id.
     * @param logicalcpu the logical CPU number (starting from zero).
     * @return the core id (starting from zero).
     */
    unsigned long getCPUCoreId( unsigned long logicalcpu );

    /**
     * CPU information. As the information in /proc/cpuinfo varies wildy between
     * architectures, the CPUInfo is limited to these common attributes.
     *
     */
    struct CPUInfo {
      /** a descriptive std::string for the CPU model. */
      std::string model;
      /** the (current) clock speed of the CPU */
      double cpu_mhz;
      /** the bogomips value for the CPU (bogomips is a bogus value, not usable to compare processor speeds) */
      double bogomips;
    };

    /**
     * Get CPU info.
     * @param info the CPUInfo to fill.
     * @return false when the cpu is not found.
     */
    bool getCPUInfo( CPUInfo &info );

    /**
     * Dump info to a stream.
     */
    std::ostream& operator<<( std::ostream &os, CPUInfo &info );

    /**
     * CPU usage statistics from /proc/stat.
     * value are in seconds, leanux::system::getUserHz is silently applied.
     */
    struct CPUStat {
      /** user mode CPU */
      double user;
      /** nice mode CPU */
      double nice;
      /** system mode CPU */
      double system;
      /** idle mode CPU */
      double idle;
      /** iowait mode CPU */
      double iowait;
      /** irq mode CPU */
      double irq;
      /** softirq mode CPU */
      double softirq;
      /** steal mode CPU */
      double steal;
      /** guest mode CPU */
      double guest;
      /** guest nice mode CPU */
      double guest_nice;
    };

    /**
     * Map of processor id to CPUStat
     */
    typedef std::map<unsigned int,CPUStat> CPUStatsMap;

    /**
     * Get CPU usage statistics from /proc/stat.
     * @param stats std::map filled, keyed by CPU number, starts with 0.
     */
    void getCPUStats( CPUStatsMap &stats );

    /**
     * Compute the deltas for two CPUStatMap std::maps into delta.
     * There are conditions that could lead to an invalid delta
     * - (at least) one of the numbers overflows.
     * - the number of CPUs is changed between the stats asked to delta.
     * In both cases, the function returns false indicating the results are invalid.
     * @param earlier the earlier set of stats.
     * @param later the later set of stats.
     * @param delta the std::map that will hold the resulting delta.
     * @return false when a delta would show incorrect data
     */
    bool deltaStats( const CPUStatsMap &earlier, const CPUStatsMap &later, CPUStatsMap &delta );

    /**
     * System load average - the average number of processes on the run queue.
     */
    struct LoadAvg {
      /** 5 minute load average */
      double avg5_;
      /** 10 minute load average */
      double avg10_;
      /** 15 minute load average */
      double avg15_;
    };

    /**
     *  Get the system load averages
     */
    void getLoadAvg( LoadAvg &avg );

    /**
     * CPU scheduler info.
     * @see getRunQueue
     */
    struct SchedInfo {
      /** the number of running processes (processes in state R). */
      unsigned int running;
      /** the number of blocked processes (processes in state D). */
      unsigned int blocked;
      /** the total number of processes created since boot. */
      unsigned long processes;
      /** the total number of context switches since boot. */
      unsigned long ctxt;
    };

    /**
     * Get the number of running and blocked processes.
     */
    SchedInfo getSchedInfo();

    /**
     * Sum the entries in all to derive the total.
     */
    void getCPUTotal( const CPUStatsMap &all, CPUStat &total );

    /**
     * Scales the numbers in stat to the time interval dt times the number of
     * processors, which is equivalent to the maximum amount of CPU time that can
     * be consumed in interval dt.
     * @param stat the CPUStat to scale.
     * @param dt   the time interval to scale to.
     * @param processors the number of processors to scale to.
     */
    void normalizeCPUstats( CPUStat &stat, double dt, unsigned int processors );

    /**
     * Compute the total amount of CPU time in stat.
     * @param stat the CPUStat to compute over.
     * @return the total amount of cputime in stat.
     */
    inline double getCPUUsageTotal( const CPUStat &stat ) {
      return stat.user + stat.nice + stat.system + stat.iowait + stat.irq + stat.softirq;
    }


  }

}

#endif
