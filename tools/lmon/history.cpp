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
 * LardHistory implementation.
 */

#include "history.hpp"
#include "math.h"

namespace leanux {

  namespace tools {

    namespace lmon {

      LardHistory::LardHistory( persist::Database *db, time_t zoom ) {
        db_ = db;
        updateMinMax();
        setZoom( zoom );
        snap_end_ = snap_max_;
        snap_start_ = snap_max_ - snap_range_;
        updateRangeTime();
      }

      time_t LardHistory::setZoom( time_t zoom ) {
        if ( snap_range_ == 0 && zoom < zoom_ ) return zoom_;
        if ( snap_range_ == snap_max_ - snap_min_ && zoom > zoom_ ) return zoom_;
        zoom_ = zoom;
        updateMinMax();
        if ( snap_start_ + snap_range_ <= snap_max_  ) {
          snap_end_ = snap_start_ + snap_range_;
        } else {
           snap_end_ = snap_max_;
           snap_start_ = snap_end_ - snap_range_;
        }
        updateRangeTime();
        cpumap_.clear(); // wipe the cache
        return zoom_;
      }

      time_t LardHistory::zoomOut() {
        //double oldzoom = zoom_;
        //double newzoom = oldzoom;
        //updateMinMax();
        //if ( oldzoom < 60 )
          //newzoom += snap_avg_time_;
        //else if ( oldzoom < 300 )
          //newzoom += 60;
        //else if ( oldzoom < 3600 )
          //newzoom += 300;
        //else if ( oldzoom < 24*3600 )
          //newzoom += 3600;
        //if ( newzoom <= oldzoom ) newzoom *= 2;
        return setZoom(zoom_*2);
      }

      time_t LardHistory::zoomIn() {
        //double oldzoom = zoom_;
        //double newzoom = oldzoom;
        //updateMinMax();
        //if ( oldzoom < 60 )
          //newzoom -= snap_avg_time_;
        //else if ( oldzoom < 300 )
          //newzoom -= 60;
        //else if ( oldzoom < 3600 )
          //newzoom -= 300;
        //else if ( oldzoom < 24*3600 )
          //newzoom -= 3600;
        //if ( newzoom >= oldzoom ) newzoom /= 2;
        return setZoom(zoom_/2);
      }

      void LardHistory::rangeUp() {
        updateMinMax();
        if ( snap_end_ + snap_range_ + 1 <= snap_max_ ) {
          snap_start_ += (snap_range_+1);
          snap_end_ += (snap_range_+1);
          updateRangeTime();
        }
      }

      void LardHistory::rangeDown() {
        updateMinMax();
        if ( snap_start_ - snap_range_ - 1 >= snap_min_ ) {
          snap_start_ -= (snap_range_+1);
          snap_end_ -= (snap_range_+1);
          updateRangeTime();
        }
      }

      void LardHistory::hourUp() {
        updateMinMax();
        long diff = 3600.0/(double)(snap_end_time_ - snap_start_time_)*(snap_range_+1);
        if ( snap_end_ + diff + 1 <= snap_max_ ) {
          snap_start_ += (diff);
          snap_end_ += (diff);
          updateRangeTime();
          cpumap_.clear();
        } else {
          snap_end_ = snap_max_;
          snap_start_   = snap_end_ - snap_range_;
          updateRangeTime();
          cpumap_.clear();
        }
      }

      void LardHistory::hourDown() {
        updateMinMax();
        long diff = 3600.0/(double)(snap_end_time_ - snap_start_time_)*(snap_range_+1);
        if ( snap_start_ - diff >= snap_min_ ) {
          snap_start_ -= (diff);
          snap_end_ -= (diff);
          updateRangeTime();
          cpumap_.clear();
        } else {
          snap_start_ = snap_min_;
          snap_end_   = snap_min_ + snap_range_;
          updateRangeTime();
          cpumap_.clear();
        }
      }

      void LardHistory::dayUp() {
        updateMinMax();
        long diff = 24.0*3600.0/(double)(snap_end_time_ - snap_start_time_)*(snap_range_+1);
        if ( snap_end_ + diff + 1 <= snap_max_ ) {
          snap_start_ += (diff);
          snap_end_ += (diff);
          updateRangeTime();
          cpumap_.clear();
        } else {
          snap_end_ = snap_max_;
          snap_start_   = snap_end_ - snap_range_;
          updateRangeTime();
          cpumap_.clear();
        }
      }

      void LardHistory::dayDown() {
        updateMinMax();
        long diff = 24.0*3600.0/(double)(snap_end_time_ - snap_start_time_)*(snap_range_+1);
        if ( snap_start_ - diff >= snap_min_ ) {
          snap_start_ -= (diff);
          snap_end_ -= (diff);
          updateRangeTime();
          cpumap_.clear();
        } else {
          snap_start_ = snap_min_;
          snap_end_   = snap_min_ + snap_range_;
          updateRangeTime();
          cpumap_.clear();
        }
      }

      void LardHistory::weekUp() {
        updateMinMax();
        long diff = 7.0*24.0*3600.0/(double)(snap_end_time_ - snap_start_time_)*(snap_range_+1);
        if ( snap_end_ + diff + 1 <= snap_max_ ) {
          snap_start_ += (diff);
          snap_end_ += (diff);
          updateRangeTime();
          cpumap_.clear();
        } else {
          snap_end_ = snap_max_;
          snap_start_   = snap_end_ - snap_range_;
          updateRangeTime();
          cpumap_.clear();
        }
      }

      void LardHistory::weekDown() {
        updateMinMax();
        long diff = 7.0*24.0*3600.0/(double)(snap_end_time_ - snap_start_time_)*(snap_range_+1);
        if ( snap_start_ - diff >= snap_min_ ) {
          snap_start_ -= (diff);
          snap_end_ -= (diff);
          updateRangeTime();
          cpumap_.clear();
        } else {
          snap_start_ = snap_min_;
          snap_end_   = snap_min_ + snap_range_;
          updateRangeTime();
          cpumap_.clear();
        }
      }

      void LardHistory::rangeStart() {
        updateMinMax();
        snap_start_ = snap_min_;
        snap_end_ = snap_start_ + snap_range_;
        updateRangeTime();
        cpumap_.clear();
      }

      void LardHistory::rangeEnd() {
        updateMinMax();
        snap_end_ = snap_max_;
        snap_start_ = snap_end_ - snap_range_;
        updateRangeTime();
        cpumap_.clear();
      }

      void LardHistory::updateRangeTime() {
        persist::Query qrange( *db_ );
        qrange.prepare( "select istart from snapshot where id=:min" );
        qrange.bind( 1, snap_start_ );
        if ( qrange.step() ) {
          snap_start_time_ = qrange.getLong(0);
        }
        qrange.prepare( "select istop from snapshot where id=:max" );
        qrange.bind( 1, snap_end_ );
        if ( qrange.step() ) {
          snap_end_time_ = qrange.getLong(0);
        }
      }

      void LardHistory::updateMinMax() {
        persist::Query qminmax( *db_ );
        qminmax.prepare( "select min(id), max(id),min(istart),max(istop), avg(istop-istart) from snapshot" );
        if ( qminmax.step() ) {
          snap_min_ = qminmax.getLong( 0 );
          snap_max_ = qminmax.getLong( 1 );
          snap_min_start_time_ = qminmax.getLong( 2 );
          snap_max_stop_time_ = qminmax.getLong( 3 );
          snap_avg_time_ = qminmax.getDouble( 4 );
          snap_range_ = zoom_ / qminmax.getDouble( 4 ) - 1;
          if ( snap_range_ < 0 ) snap_range_ = 0;
          if ( snap_range_ > snap_max_ - snap_min_ ) snap_range_ = snap_max_ - snap_min_;
        }
      }

      void LardHistory::fetchXProcView( XProcView &procview ) {
        procview.disabled = false;
        persist::Query qstate( *db_ );
        qstate.prepare( "SELECT"
                        "  s.state,"
                        "  w.wchan,"
                        "  count(1) "
                        "FROM"
                        "  procstat s,"
                        "  wchan w "
                        "WHERE"
                        "  s.pid=:pid"
                        "  AND"
                        "  s.state='D'"
                        "  AND"
                        "  s.snapshot>=:min"
                        "  AND"
                        "  s.snapshot<=:max"
                        "  AND"
                        "  s.wchan=w.id "
                        "GROUP BY s.state "
                        "ORDER BY count(1) DESC "
                        "LIMIT 1 " );



        persist::Query qprocstat( *db_ );
        qprocstat.prepare( "SELECT"
                          "  c.cmd,"
                          "  c.args,"
                          "  s.pid,"
                          "  s.pgrp,"
                          "  s.uid,"
                          "  sum(s.usercpu),"
                          "  sum(s.systemcpu),"
                          "  sum(s.iotime),"
                          "  sum(s.minflt),"
                          "  sum(s.majflt),"
                          "  avg(s.rss),"
                          "  avg(s.vsz) "
                          "FROM"
                          "  procstat s,"
                          "  cmd c "
                          "WHERE"
                          "  s.cmd=c.id"
                          "  AND"
                          "  s.snapshot>=:min"
                          "  AND"
                          "  s.snapshot<=:max "
                          "GROUP BY s.pid "
                          "ORDER BY sum(s.usercpu)+sum(s.systemcpu)+sum(s.iotime) DESC, sum(s.majflt) DESC, sum(s.minflt) DESC" );
        qprocstat.bind( 1, snap_start_ );
        qprocstat.bind( 2, snap_end_ );
        procview.delta.clear();
        procview.pidargs.clear();
        procview.piduids.clear();
        double dt = snap_end_ - snap_start_ + 1;
        while ( qprocstat.step() ) {
          process::ProcPidStatDelta stat;
          stat.comm = qprocstat.getText(0);
          stat.pid  = qprocstat.getLong(2);
          stat.pgrp  = qprocstat.getLong(3);
          stat.utime = qprocstat.getDouble(5)/dt;
          stat.stime = qprocstat.getDouble(6)/dt;
          stat.delayacct_blkio_ticks = qprocstat.getDouble(7)/dt;
          stat.minflt = qprocstat.getDouble(8)/dt;
          stat.majflt = qprocstat.getDouble(9)/dt;
          stat.rss = qprocstat.getDouble(10);
          stat.vsize = qprocstat.getDouble(11);
          procview.pidargs[stat.pid] = qprocstat.getText(1);
          procview.piduids[stat.pid] = qprocstat.getLong(4);

          stat.state='S';
          stat.wchan="";
          qstate.reset();
          qstate.bind( 1, stat.pid );
          qstate.bind( 2, snap_min_ );
          qstate.bind( 3, snap_max_ );
          if ( qstate.step() ) {
            if ( !qstate.isNull(0) ) {
              stat.state = qstate.getText(0)[0];
              stat.wchan = qstate.getText(1);
            }
          }

          procview.delta.push_back(stat);
        }
        procview.sample_count = 2;
      }

      void LardHistory::fetchXNetView( XNetView &netview ) {
        netview.sample_count = 2;
        persist::Query qnetstat( *db_ );
        qnetstat.prepare( "SELECT"
                          "  n.device,"
                          "  avg(s.rxbs),"
                          "  avg(s.txbs),"
                          "  avg(s.rxpkts),"
                          "  avg(s.txpkts),"
                          "  avg(s.rxerrs),"
                          "  avg(s.txerrs) "
                          "FROM"
                          "  netstat s,"
                          "  nic n "
                          "WHERE"
                          "  s.nic=n.id"
                          "  AND"
                          "  s.snapshot>=:min"
                          "  AND"
                          "  s.snapshot<=:max "
                          "GROUP BY n.device "
                          "ORDER BY avg(s.rxbs)+avg(s.txbs) DESC" );
        qnetstat.bind( 1, snap_start_ );
        qnetstat.bind( 2, snap_end_ );
        netview.delta.clear();
        while ( qnetstat.step() ) {
          net::NetDeviceStat stat;
          stat.device = qnetstat.getText(0);
          stat.rx_bytes = qnetstat.getDouble(1);
          stat.tx_bytes = qnetstat.getDouble(2);
          stat.rx_packets = qnetstat.getDouble(3);
          stat.tx_packets = qnetstat.getDouble(4);
          stat.rx_errors = qnetstat.getDouble(5);
          stat.tx_errors = qnetstat.getDouble(6);
          netview.delta.push_back(stat);
        }

        netview.tcpserver.clear();
        persist::Query qserver( *db_ );
        qserver.prepare( "SELECT"
                         "  k.ip,"
                         "  k.port,"
                         "  k.uid,"
                         "  avg(s.esta) "
                         "FROM"
                         " tcpkey k,"
                         " tcpserverstat s "
                         "WHERE"
                         "  s.tcpkey=k.id "
                         "  AND"
                         "  s.snapshot>=:min"
                         "  AND"
                         "  s.snapshot<=:max "
                         "GROUP BY k.ip,k.port,k.uid "
                         "ORDER BY avg(s.esta) DESC" );
        qserver.bind( 1, snap_start_ );
        qserver.bind( 2, snap_end_ );
        while ( qserver.step() ) {
          net::TCPKey key( qserver.getText(0), qserver.getLong(1), qserver.getLong(2) );
          net::TCPKeyCounter cnt( key, qserver.getLong(3) );
          netview.tcpserver.push_back( cnt );
        }

        netview.tcpclient.clear();
        persist::Query qclient( *db_ );
        qclient.prepare( "SELECT"
                         "  k.ip,"
                         "  k.port,"
                         "  k.uid,"
                         "  avg(s.esta) "
                         "FROM"
                         " tcpkey k,"
                         " tcpclientstat s "
                         "WHERE"
                         "  s.tcpkey=k.id "
                         "  AND"
                         "  s.snapshot>=:min"
                         "  AND"
                         "  s.snapshot<=:max "
                         "GROUP BY k.ip,k.port,k.uid "
                         "ORDER BY avg(s.esta) DESC" );
        qclient.bind( 1, snap_start_ );
        qclient.bind( 2, snap_end_ );
        while ( qclient.step() ) {
          net::TCPKey key( qclient.getText(0), qclient.getLong(1), qclient.getLong(2) );
          net::TCPKeyCounter cnt( key, qclient.getLong(3) );
          netview.tcpclient.push_back( cnt );
        }

      }

      void LardHistory::fetchXIOView( XIOView &ioview ) {
        persist::Query qiostat( *db_ );
        qiostat.prepare( "SELECT"
                         "  d.device,"
                         "  d.wwn,"
                         "  avg(i.util),"
                         "  avg(i.svctm),"
                         "  avg(i.rs),"
                         "  avg(i.ws),"
                         "  avg(i.rbs),"
                         "  avg(i.wbs),"
                         "  avg(i.artm),"
                         "  avg(i.awtm),"
                         "  avg(i.qsz), "
                         "  avg(i.iodones), "
                         "  avg(i.ioreqs), "
                         "  avg(i.ioerrs) "
                         "FROM "
                         "  iostat i,"
                         "  disk d "
                         "WHERE "
                         "  i.disk = d.id"
                         "  AND"
                         "  i.snapshot>=:min"
                         "  AND"
                         "  i.snapshot<=:max "
                         "GROUP BY d.device,d.wwn "
                         "ORDER BY avg(i.util) DESC" );
        qiostat.bind( 1, snap_start_ );
        qiostat.bind( 2, snap_end_ );
        ioview.iostats.clear();
        ioview.iosorted.clear();
        while ( qiostat.step() ) {
          XIORec stat;
          std::string device  = qiostat.getText(0);
          std::string wwn     = qiostat.getText(1);
          stat.util           = qiostat.getDouble(2);
          stat.svctm          = qiostat.getDouble(3);
          stat.rs             = qiostat.getDouble(4);
          stat.ws             = qiostat.getDouble(5);
          stat.rbs            = qiostat.getDouble(6);
          stat.wbs            = qiostat.getDouble(7);
          stat.artm           = qiostat.getDouble(8);
          stat.awtm           = qiostat.getDouble(9);
          stat.qsz            = qiostat.getDouble(10);
          stat.iodone_cnt     = qiostat.getDouble(11);
          stat.iorequest_cnt  = qiostat.getDouble(12);
          stat.ioerr_cnt      = qiostat.getDouble(13);
          ioview.iostats[device] = stat;
          ioview.iosorted.push_back(device);
        }

        persist::Query qmntstat( *db_ );
        qmntstat.prepare( "SELECT"
                         "  m.mountpoint,"
                         "  avg(i.util),"
                         "  avg(i.svctm),"
                         "  avg(i.rs),"
                         "  avg(i.ws),"
                         "  avg(i.rbs),"
                         "  avg(i.wbs),"
                         "  avg(i.artm),"
                         "  avg(i.awtm),"
                         "  avg(i.growth) "
                         "FROM "
                         "  mountstat i,"
                         "  mountpoint m "
                         "WHERE "
                         "  i.mountpoint = m.id"
                         "  AND"
                         "  i.snapshot>=:min"
                         "  AND"
                         "  i.snapshot<=:max "
                         "GROUP BY m.mountpoint "
                         "ORDER BY avg(i.util) DESC" );
        qmntstat.bind( 1, snap_start_ );
        qmntstat.bind( 2, snap_end_ );
        ioview.mountstats.clear();
        ioview.mountsorted.clear();
        while ( qmntstat.step() ) {
          XIORec stat;
          std::string mountpoint  = qmntstat.getText(0);
          stat.util           = qmntstat.getDouble(1);
          stat.svctm          = qmntstat.getDouble(2);
          stat.rs             = qmntstat.getDouble(3);
          stat.ws             = qmntstat.getDouble(4);
          stat.rbs            = qmntstat.getDouble(5);
          stat.wbs            = qmntstat.getDouble(6);
          stat.artm           = qmntstat.getDouble(7);
          stat.awtm           = qmntstat.getDouble(8);
          stat.growths            = qmntstat.getDouble(9);
          ioview.mountstats[mountpoint] = stat;
          ioview.mountsorted.push_back(mountpoint);
        }
      }

      void LardHistory::fetchXSysView( XSysView &sysview ) {
        persist::Query qcputopo( *db_ );
        qcputopo.prepare( "SELECT"
                          "  count(distinct phyid),"
                          "  count(distinct coreid),"
                          "  count(distinct logical) "
                          "FROM "
                          "   cpustat "
                          "WHERE "
                          "  snapshot>=:start"
                          "  AND"
                          "  snapshot<=:end" );
        qcputopo.bind( 1, (long)snap_start_ );
        qcputopo.bind( 2, (long)snap_end_ );
        if ( qcputopo.step() ) {
          sysview.cpu_topo.physical = qcputopo.getLong( 0 );
          sysview.cpu_topo.cores = qcputopo.getLong( 1 );
          sysview.cpu_topo.logical = qcputopo.getLong( 2 );
        }

        persist::Query qcpustat( *db_ );
        qcpustat.prepare( "SELECT"
                          "  logical,"
                          "  avg(user_mode),"
                          "  avg(system_mode),"
                          "  avg(iowait_mode),"
                          "  avg(nice_mode),"
                          "  avg(irq_mode),"
                          "  avg(softirq_mode),"
                          "  avg(steal_mode) "
                          "FROM "
                          "   cpustat "
                          "WHERE "
                          "  snapshot>=:start"
                          "  AND"
                          "  snapshot<=:end "
                          "GROUP BY logical" );
        qcpustat.bind( 1, (long)snap_start_ );
        qcpustat.bind( 2, (long)snap_end_ );
        while ( qcpustat.step() ) {
          if ( sysview.sample_count < 2 ) sysview.sample_count = 2;
          sysview.cpu_delta[qcpustat.getLong(0)].user = qcpustat.getDouble(1);
          sysview.cpu_delta[qcpustat.getLong(0)].system = qcpustat.getDouble(2);
          sysview.cpu_delta[qcpustat.getLong(0)].iowait = qcpustat.getDouble(3);
          sysview.cpu_delta[qcpustat.getLong(0)].nice = qcpustat.getDouble(4);
          sysview.cpu_delta[qcpustat.getLong(0)].irq = qcpustat.getDouble(5);
          sysview.cpu_delta[qcpustat.getLong(0)].softirq = qcpustat.getDouble(6);
          sysview.cpu_delta[qcpustat.getLong(0)].steal = qcpustat.getDouble(7);
          sysview.cpu_delta[qcpustat.getLong(0)].guest = 0;
          sysview.cpu_delta[qcpustat.getLong(0)].guest_nice = 0;
          sysview.cpu_delta[qcpustat.getLong(0)].idle = 1 - (qcpustat.getDouble(1)+
                                                             qcpustat.getDouble(2)+
                                                             qcpustat.getDouble(3)+
                                                             qcpustat.getDouble(4)+
                                                             qcpustat.getDouble(5)+
                                                             qcpustat.getDouble(6)+
                                                             qcpustat.getDouble(7));
        }
        cpu::getCPUTotal( sysview.cpu_delta, sysview.cpu_total );
        sysview.cpu_seconds = cpu::getCPUUsageTotal( sysview.cpu_total );

        persist::Query qschedstat( *db_ );
        qschedstat.prepare( "SELECT"
                            "  avg(forks),"
                            "  avg(ctxsw),"
                            "  avg(load5),"
                            "  avg(load10),"
                            "  avg(load15),"
                            "  avg(runq),"
                            "  avg(blockq) "
                            "FROM "
                            "   schedstat "
                            "WHERE "
                            "  snapshot>=:start"
                            "  AND"
                            "  snapshot<=:end" );
        qschedstat.bind( 1, (long)snap_start_ );
        qschedstat.bind( 2, (long)snap_end_ );
        if ( qschedstat.step() ) {
          sysview.forks          = qschedstat.getDouble(0);
          sysview.ctxsws         = qschedstat.getDouble(1);
          sysview.loadavg.avg5_  = qschedstat.getDouble(2);
          sysview.loadavg.avg10_ = qschedstat.getDouble(3);
          sysview.loadavg.avg15_ = qschedstat.getDouble(4);
          sysview.runq           = qschedstat.getDouble(5);
          sysview.blockq         = qschedstat.getDouble(6);
          sysview.time_slice = 1.0/sysview.ctxsws;
        }

        persist::Query qvmstat( *db_ );
        qvmstat.prepare( "SELECT"
                         "  avg(realmem),"
                         "  avg(unused),"
                         "  avg(commitas),"
                         "  avg(anon),"
                         "  avg(file),"
                         "  avg(shmem),"
                         "  avg(slab),"
                         "  avg(pagetbls),"
                         "  avg(dirty),"
                         "  avg(pgins),"
                         "  avg(pgouts),"
                         "  avg(swpins),"
                         "  avg(swpouts),"
                         "  avg(hptotal),"
                         "  avg(hprsvd),"
                         "  avg(hpfree),"
                         "  avg(thpanon),"
                         "  avg(mlock),"
                         "  avg(mapped),"
                         "  avg(swpused),"
                         "  avg(swpsize),"
                         "  avg(minflts),"
                         "  avg(majflts),"
                         "  avg(allocs),"
                         "  avg(frees) "
                         "FROM "
                         "   vmstat "
                         "WHERE "
                         "  snapshot>=:start"
                         "  AND"
                         "  snapshot<=:end" );
        qvmstat.bind( 1, (long)snap_start_ );
        qvmstat.bind( 2, (long)snap_end_ );
        if ( qvmstat.step() ) {
          sysview.mem_total      = qvmstat.getDouble(0);
          sysview.mem_unused     = qvmstat.getDouble(1);
          sysview.mem_commitas   = qvmstat.getDouble(2);
          sysview.mem_anon       = qvmstat.getDouble(3);
          sysview.mem_file       = qvmstat.getDouble(4);
          sysview.mem_shmem      = qvmstat.getDouble(5);
          sysview.mem_slab       = qvmstat.getDouble(6);
          sysview.mem_pagetbls   = qvmstat.getDouble(7);
          sysview.mem_dirty      = qvmstat.getDouble(8);
          sysview.mem_pageins    = qvmstat.getDouble(9);
          sysview.mem_pageouts   = qvmstat.getDouble(10);
          sysview.mem_swapins    = qvmstat.getDouble(11);
          sysview.mem_swapouts   = qvmstat.getDouble(12);
          sysview.mem_hptotal    = qvmstat.getDouble(13);
          sysview.mem_hprsvd     = qvmstat.getDouble(14);
          sysview.mem_hpfree     = qvmstat.getDouble(15);
          sysview.mem_thpanon    = qvmstat.getDouble(16);
          sysview.mem_mlock      = qvmstat.getDouble(17);
          sysview.mem_mapped     = qvmstat.getDouble(18);
          sysview.mem_swpused    = qvmstat.getDouble(19);
          sysview.mem_swpsize    = qvmstat.getDouble(20);
          sysview.mem_minflts    = qvmstat.getDouble(21);
          sysview.mem_majflts    = qvmstat.getDouble(22);
          sysview.mem_allocs     = qvmstat.getDouble(23);
          sysview.mem_frees      = qvmstat.getDouble(24);
        }

        persist::Query qresstat( *db_ );
        qresstat.prepare( "SELECT"
                            "  avg(processes),"
                            "  avg(users),"
                            "  avg(logins),"
                            "  avg(files_open),"
                            "  avg(files_max),"
                            "  avg(inodes_open), "
                            "  avg(inodes_free) "
                            "FROM "
                            "   resstat "
                            "WHERE "
                            "  snapshot>=:start"
                            "  AND"
                            "  snapshot<=:end" );
        qresstat.bind( 1, (long)snap_start_ );
        qresstat.bind( 2, (long)snap_end_ );
        if ( qresstat.step() ) {
          sysview.res_processes  = qresstat.getDouble(0);
          sysview.res_users      = qresstat.getDouble(1);
          sysview.res_logins     = qresstat.getDouble(2);
          sysview.res_filesopen  = qresstat.getDouble(3);
          sysview.res_filesmax   = qresstat.getDouble(4);
          sysview.res_inodesopen = qresstat.getDouble(5);
          sysview.res_inodesfree = qresstat.getDouble(6);
        }

        persist::Query qmountstat( *db_ );
        qmountstat.prepare( "SELECT"
                            "  avg(total.s_growth) "
                            "FROM "
                            "   (SELECT snapshot, sum(growth) s_growth from mountstat where snapshot>=:start and snapshot<=:end group by snapshot) total,"
                            "   mountstat m "
                            "WHERE "
                            "  m.snapshot=total.snapshot"
                            "  AND"
                            "  m.snapshot>=:start"
                            "  AND"
                            "  m.snapshot<=:end" );
        qmountstat.bind( 1, (long)snap_start_ );
        qmountstat.bind( 2, (long)snap_end_ );
        if ( qmountstat.step() ) {
          sysview.fs_growths = qmountstat.getDouble(0);
        }

        sysview.cpufuture.clear();
        //unsigned long snap_range = snap_end_ - snap_start_;
        SnapRange range;
        range.snap_min = snap_start_+ snap_range_+1;
        range.snap_max = snap_end_+ snap_range_+1;
        for ( int i = 0; i < 120; i++ ) {
          if ( getCPUStatRange(range) ) {
            sysview.cpufuture.push_back( cpumap_[range] );
            range.snap_min += (snap_range_+1);
            range.snap_max += (snap_range_+1);
          } else break;
        }
        sysview.cpupast.clear();
        range.snap_min = snap_start_- snap_range_-1;
        range.snap_max = snap_end_- snap_range_-1;
        for ( int i = 0; i < 120; i++ ) {
          if ( getCPUStatRange(range) ) {
            sysview.cpupast.push_back( cpumap_[range] );
            range.snap_min -= (snap_range_+1);
            range.snap_max -= (snap_range_+1);
          } else break;
        }

        sysview.t1.tv_sec  = snap_start_time_;
        sysview.t1.tv_usec = 0;
        sysview.t2.tv_sec  = snap_end_time_;
        sysview.t2.tv_usec = 0;


      }

      bool LardHistory::getCPUStatRange( SnapRange range ) {
        RangeMap::const_iterator i = cpumap_.find(range);
        if ( i == cpumap_.end() ) {
          persist::Query qrange( *db_ );
          qrange.prepare( "SELECT"
                         "  avg(user_mode),"
                         "  avg(system_mode),"
                         "  avg(iowait_mode),"
                         "  avg(nice_mode),"
                         "  avg(irq_mode),"
                         "  avg(softirq_mode),"
                         "  avg(steal_mode) "
                         "FROM "
                         "  cpustat "
                         "WHERE "
                         "  snapshot<=:max"
                         "  AND"
                         "  snapshot>=:min " );
          qrange.bind( 1, (long)range.snap_max );
          qrange.bind( 2, (long)range.snap_min );
          if ( qrange.step() ) {
            if ( !qrange.isNull(0) ) {
              cpu::CPUStat stat;
              stat.user       = qrange.getDouble(0);
              stat.system     = qrange.getDouble(1);
              stat.iowait     = qrange.getDouble(2);
              stat.nice       = qrange.getDouble(3);
              stat.irq        = qrange.getDouble(4);
              stat.softirq    = qrange.getDouble(5);
              stat.steal      = qrange.getDouble(6);
              stat.guest      = 0;
              stat.guest_nice = 0;
              cpumap_[range] = stat;
            } else return false;
          }
        } else return true;
        return true;
      }

    }; //namespace lmon

  }; //namespace tools

}; //namespace leanux
