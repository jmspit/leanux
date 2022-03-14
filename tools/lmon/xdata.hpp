//========================================================================
//
// This file is part of the leanux toolkit.
//
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

/**
 * @file
 * ncurses based real time linux performance monitoring tool - c++ header file.
 * presentation data definiton.
 */
#ifndef LEANUX_LMON_XDATA_HPP
#define LEANUX_LMON_XDATA_HPP

#include "block.hpp"
#include "cpu.hpp"
#include "net.hpp"
#include "process.hpp"
#include "vmem.hpp"

namespace leanux {

  namespace tools {

    namespace lmon {

      /** Maximum samples kept for CPU trail. */
      const unsigned int MAX_CPU_TRAIL=280;

      /**
       * Create a character string representing a stacked CPU bar.
       * The bar will not be longer than maxlines.
       * When the calue=maxvalue, the length of the bar equals maxlines.
       * @param stat the source data
       * @param maxvalue the max value (max possible number of CPU seconds in stat)
       * @param maxlines the maximum number of lines/characters to incorporate.
       */
      std::string makeCPUBar( const cpu::CPUStat &stat, double maxvalue, int maxlines );


      struct SnapRange {
        unsigned long snap_min;
        unsigned long snap_max;
      };

      inline bool operator==( const SnapRange &a, const SnapRange &b ) { return a.snap_min == b.snap_min && a.snap_max == b.snap_max; };
      inline bool operator<( const SnapRange &a, const SnapRange &b ) { return a.snap_min < b.snap_min || (a.snap_min == b.snap_min && a.snap_max < b.snap_max); };

      struct XProcView {

        /** start of sample interval */
        struct timeval t1;

        /** end of sample interval */
        struct timeval t2;

        /** number of samples taken */
        unsigned long sample_count;

        process::ProcPidStatDeltaVector delta;

        std::map<pid_t,std::string> pidargs;
        std::map<pid_t,uid_t> piduids;

        bool disabled;

      };

      /**
       * Data record for NetView display
       */
      struct XNetView {
        /** start of sample interval */
        struct timeval t1;

        /** end of sample interval */
        struct timeval t2;

        /** number of samples taken */
        unsigned long sample_count;

        /** net stat delta. */
        net::NetStatDeviceVector delta;

        /** TCP server connections */
        std::list<net::TCPKeyCounter> tcpserver;

        /** TCP client connections */
        std::list<net::TCPKeyCounter> tcpclient;
      };

      /**
       * I/O statistics in a form suitable for XIOView.
       */
      struct XIORec {
        std::string device;
        double util;
        double svctm;
        double rs;
        double ws;
        double rbs;
        double wbs;
        double artm;
        double awtm;
        double qsz;
        double growths;
        double iodone_cnt;
        double iorequest_cnt;
        double ioerr_cnt;
      };

      typedef std::map<std::string,XIORec> IORecMap;

      /**
       * Data record for IOView display
       */
      struct XIOView {
        /** start of sample interval */
        struct timeval t1;

        /** end of sample interval */
        struct timeval t2;

        /** number of samples taken */
        unsigned long sample_count;

        IORecMap iostats;
        std::vector<std::string> iosorted;

        IORecMap mountstats;
        std::vector<std::string> mountsorted;

        std::map<std::string,unsigned long> fsbytes1;
        std::map<std::string,unsigned long> fsbytes2;
      };

      /**
       * Data record for SysView display
       */
      struct XSysView {
        /** start of sample interval */
        struct timeval t1;

        /** end of sample interval */
        struct timeval t2;

        unsigned long sample_count;

        /** The CPUTopology. */
        cpu::CPUTopology cpu_topo;

        /** the per CPU, per cpumode delta, in seconds */
        cpu::CPUStatsMap cpu_delta;

        /** per cpumode, total over all CPU's, in seconds */
        cpu::CPUStat cpu_total;

        /** the total number of cpu seconds */
        double cpu_seconds;

        /** The CPU load averages (average run+block queue sizes) */
        cpu::LoadAvg loadavg;

        /** average time slice duration. */
        double time_slice;

        /** context switches per second. */
        double ctxsws;

        /** forks per second. */
        double forks;

        /** size of the run queue. */
        unsigned int runq;

        /** size of the block queue. */
        unsigned int blockq;

        /** the system page size */
        unsigned long pagesize_;

        /** unused memory in bytes. */
        unsigned long mem_total;

        /** unused memory in bytes. */
        unsigned long mem_unused;

        /** comitted address space (linux kernel estimate) in bytes. */
        unsigned long mem_commitas;

        /** unused memory in bytes. */
        unsigned long mem_anon;

        /** size of page cache for file data in bytes. */
        unsigned long mem_file;

        /** shared memory in bytes. */
        unsigned long mem_shmem;

        /** slab size in bytes. */
        unsigned long mem_slab;

        /** space used by page tables in bytes. */
        unsigned long mem_pagetbls;

        /** dirty memory in bytes. */
        unsigned long mem_dirty;

        /** pages paged in per second. */
        double mem_pageins;

        /** pages paged out per second. */
        double mem_pageouts;

        /** pages swapped in per second. */
        double mem_swapins;

        /** pages swapped out per second. */
        double mem_swapouts;

        /** hugepages total in bytes. */
        unsigned long mem_hptotal;

        /** hugepages reserved in bytes. */
        unsigned long mem_hprsvd;

        /** hugepages free in bytes. */
        unsigned long mem_hpfree;

        /** transparent hugepages anonymous in bytes. */
        unsigned long mem_thpanon;

        /** locked memory in bytes. */
        unsigned long mem_mlock;

        /** mapped (memory mapped files) memory in bytes. */
        unsigned long mem_mapped;

        /** swap used in bytes. */
        unsigned long mem_swpused;

        /** swap size in bytes. */
        unsigned long mem_swpsize;

        /** minor faults per second. */
        double mem_minflts;

        /** major faults per second. */
        double mem_majflts;

        /** memory allocations per second. */
        double mem_allocs;

        /** memory frees per second. */
        double mem_frees;

        /** number of open files. */
        unsigned long res_filesopen;

        /** max number of open files. */
        unsigned long res_filesmax;

        /** number of open inodes. */
        unsigned long res_inodesopen;

        /** number of free inodes. */
        unsigned long res_inodesfree;

        /** number of processes. */
        unsigned long res_processes;

        /** number of distinct users. */
        unsigned long res_users;

        /** number of logins. */
        unsigned long res_logins;

        /** mounted filesystem growth rate bytes per second. */
        double fs_growths;

        /**
         * Past trail of CPU bars (realtime mode).
         * @see makeCPUBar
         */
        std::list<std::string> cpurtpast;

        /**
         * Future trail of CPU bars.
         * @see makeCPUBar
         */
        std::list<cpu::CPUStat> cpupast;
        std::list<cpu::CPUStat> cpufuture;

        SnapRange snap_range;

      };

    }; //namespace lmon

  }; //namespace tools

}; //namespace leanux

#endif

