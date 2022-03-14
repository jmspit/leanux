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
 * RealtimeSampler definition.
 */
#ifndef LEANUX_LMON_REALTIME_HPP
#define LEANUX_LMON_REALTIME_HPP

#include "xdata.hpp"

namespace leanux {

  namespace tools {

    namespace lmon {

      /**
       * Provides data by real time sampling.
       */
      class RealtimeSampler {
        public:
          /**
           * Constructor
           */
          RealtimeSampler();

          /**
           * Sample a snapshot.
           */
          void sample( int cpubarheight );

          /**
           * Return snapshot XSysView data.
           */
          const XSysView& getXSysView() const { return xsysview_; };
          const XIOView& getXIOView() const { return xioview_; };
          const XNetView& getXNetView() const { return xnetview_; };
          const XProcView& getXProcView() const { return xprocview_; };

          void resetCPUTrail() { xsysview_.cpurtpast.clear(); };

        protected:
          void sampleXSysView( int cpubarheight );
          void sampleXIOView();
          void sampleXNetView();
          void sampleXProcView();
          XIOView xioview_;
          XSysView xsysview_;
          XNetView xnetview_;
          XProcView xprocview_;

          /** Earlier CPUStatsMap snapshot. */
          cpu::CPUStatsMap cpustat1_;

          /** Later CPUStatsMap snapshot. */
          cpu::CPUStatsMap cpustat2_;

          /** Earlier SchedInfo snapshot. */
          cpu::SchedInfo   sched1_;

          /** Later SchedInfo snapshot. */
          cpu::SchedInfo   sched2_;

          /** Earlier VMStat snapshot. */
          vmem::VMStat vmstat1_;

          /** Later VMStat snapshot. */
          vmem::VMStat vmstat2_;

          /** List of SwapInfo. */
          std::list<vmem::SwapInfo> swaps_;

          /** Earlier DeviceStatsMap snapshot. */
          block::DeviceStatsMap diskstats1_;

          /** Later DeviceStatsMap snapshot. */
          block::DeviceStatsMap diskstats2_;


          unsigned long mounted_bytes_1_;

          unsigned long mounted_bytes_2_;

          /** Delta of stat1_ and stat2_. */
          leanux::cpu::CPUStatsMap delta_;

          /** cpu model info */
          cpu::CPUInfo cpuinfo_;

          /** cache of device special files to MajorMinor */
          static std::map<std::string,block::MajorMinor> devicefilecache_;

          /** earlier snap. */
          net::NetStatDeviceMap netsnap1_;

          /** later snap. */
          net::NetStatDeviceMap netsnap2_;

          /** earlier snap. */
          process::ProcPidStatMap procsnap1_;

          /** later snap. */
          process::ProcPidStatMap procsnap2_;

          /** number of procs on the system when sampling disabled. */
          unsigned long disabled_procs_;

          /** maximum sample duration in seconds before being disabled. */
          static const double max_proc_sample_time;

      };

    }; //namespace lmon

  }; //namespace tools

}; //namespace leanux

#endif
