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
 * ncurses based real time linux performance monitoring tool - c++ header file.
 * LardHistory definition.
 */
#ifndef LEANUX_LMON_HISTORY_HPP
#define LEANUX_LMON_HISTORY_HPP

#include "persist.hpp"
#include "xdata.hpp"

namespace leanux {

  namespace tools {

    namespace lmon {

      class LardHistory {

        public:
          LardHistory( persist::Database *db, time_t zoom );

          /**
           * Sets the current time selection into the lard data.
           * If requested data is not available, setSel will do it's best
           * to provide a sane alternative.
           * @param start will select the maximum of alls snapshots that are < start
           * @param interval will select the minimum of all snapshots that are > start+interval
           */
          time_t setZoom( time_t zoom );

          void fetchXSysView( XSysView &sysview );
          void fetchXIOView( XIOView &ioview );
          void fetchXNetView( XNetView &netview );
          void fetchXProcView( XProcView &procview );

          time_t zoomOut();
          time_t zoomIn();

          void rangeUp();
          void rangeDown();

          void hourDown();
          void hourUp();

          void dayDown();
          void dayUp();

          void weekDown();
          void weekUp();

          void rangeStart();
          void rangeEnd();

          long getStartSnap() const { return snap_start_; };

          long getEndSnap() const { return snap_end_; };

          time_t getStartTime() const { return snap_start_time_; };

          time_t getEndTime() const { return snap_end_time_; };

          time_t getRangeDuration() const { return snap_end_time_ - snap_start_time_; };

        protected:
          void updateMinMax();
          void updateRangeTime();

          bool getCPUStatRange( SnapRange range );

          persist::Database *db_;

          long snap_start_;
          long snap_start_time_;
          long snap_end_;
          long snap_end_time_;
          long snap_min_;
          long snap_max_;
          time_t snap_min_start_time_;
          time_t snap_max_stop_time_;
          time_t snap_avg_time_;

          time_t zoom_;
          long snap_range_;

          typedef std::map<SnapRange,cpu::CPUStat> RangeMap;
          std::map<SnapRange,cpu::CPUStat> cpumap_;

      };

    }; //namespace lmon

  }; //namespace tools

}; //namespace leanux

#endif
