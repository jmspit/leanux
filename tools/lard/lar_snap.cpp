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
 * lar tool snapshot c++ source file.
 */

#include "lar_snap.hpp"
#include "configfile.hpp"
#include "system.hpp"
#include <syslog.h>
#include <algorithm>
#include <sstream>
#include <string.h>

namespace leanux {
  namespace tools {
    namespace lard {

      /**
       * maximum size of a command name as tracked by the kernel.
       * should be set to TASK_COMM_LEN which seems to be unavailable
       * outside kernel sources (/usr/src/linux/include/linux/sched.h)
       */
      const size_t task_comm_len = 16;

      void sysLog( unsigned short level, unsigned short limit, const std::string &msg ) {
        if ( level <= limit ) {
          short severity = LOG_USER;
          std::stringstream ss;
          switch ( level ) {
            case 0 :
              ss << "[ERR]";
              severity = LOG_ERR;
              break;
            case 1 :
              ss << "[WARN]";
              severity = LOG_WARNING;
              break;
            case 2 :
              ss << "[STAT]";
              severity = LOG_NOTICE;
              break;
            case 3 :
              ss << "[INFO]";
              severity = LOG_INFO;
              break;
            case 4 :
              ss << "[DEBUG]";
              severity = LOG_DEBUG;
              break;
            default :
              ss << "[?]";
              break;
          }
          ss << " " << msg;
          openlog( "lard", LOG_PID, LOG_DAEMON );
          syslog( LOG_MAKEPRI(LOG_DAEMON, severity), "%s", ss.str().c_str() );
          closelog();
        }
      }

      long TimeSnap::storeSnap( const persist::Database &db, long snapid, double seconds ) {
        persist::DML dml( db );
        dml.prepare( "INSERT INTO snapshot (istart,istop) VALUES (:istart,:istop)" );
        dml.bind( 1, istart_ );
        dml.bind( 2, istop_ );
        dml.execute();
        return db.lastInsertRowid();
      }

      void IOSnap::startSnap() {
        block::getStats( stat1_ );
      }

      void IOSnap::stopSnap() {
        block::getStats( stat2_ );
      }

      long IOSnap::storeSnap( const persist::Database &db, long snapid, double seconds ) {
        int stored_disks = 0;
        block::DeviceStatsMap delta;
        block::MajorMinorVector vec;
        block::deltaDeviceStats( stat1_, stat2_, delta, vec );
        block::StatsSorter sorter( &delta );
        sort( vec.begin(), vec.end(), sorter );
        for ( block::MajorMinorVector::const_iterator d = vec.begin(); d != vec.end(); d++ ) {
          //if ( (*d).isWholeDisk() && (delta[*d].reads + delta[*d].writes + delta[*d].iorequest_cnt) > 0 ) {
          if ( (*d).isWholeDisk() ) {
            std::string sdiskid = (*d).getDiskId();
            std::string ssyspath = (*d).getSysPath();
            size_t p = ssyspath.rfind("/");
            ssyspath  = ssyspath.substr(0,p);
            long diskid = 0;
            persist::Query qry(db);
            qry.prepare( "SELECT id FROM disk WHERE syspath=:syspath and wwn=:wwn and not (syspath='' and wwn='')" );
            qry.bind( 1, ssyspath );
            qry.bind( 2, sdiskid );
            if ( qry.step() ) {
              diskid = qry.getLong(0);
              persist::DML dml(db);
              dml.prepare( "UPDATE disk set device=:device WHERE id=:id" );
              dml.bind( 1, diskid );
              dml.execute();
            } else {
              persist::DML dml(db);
              dml.prepare( "INSERT INTO disk (device,syspath,wwn,model) VALUES (:device,:syspath,:wwn,:model)" );
              dml.bind( 1, (*d).getName() );
              dml.bind( 2, ssyspath );
              dml.bind( 3, sdiskid );
              dml.bind( 4, (*d).getModel() );
              dml.execute();
              diskid = db.lastInsertRowid();
            }
            persist::DML dml(db);
            dml.prepare( "INSERT INTO iostat (snapshot,disk,util,svctm,rs,ws,rbs,wbs,artm,awtm,qsz,iodones,ioreqs,ioerrs) VALUES ( \
              :snapid, \
              :disk, \
              :util, \
              :svctm, \
              :rs, \
              :ws, \
              :rbs, \
              :wbs, \
              :artm, \
              :awtm, \
              :qsz, \
              :iodones, \
              :ioreqs, \
              :ioerrs \
              )" );
            dml.bind( 1, snapid );
            dml.bind( 2, diskid );
            dml.bind( 3, delta[(*d)].io_ms/1000.0/seconds  );
            if ( delta[*d].reads + delta[*d].writes > 0 ) dml.bind( 4, delta[*d].io_ms/1000.0/(delta[*d].reads + delta[*d].writes)  ); else dml.bind(4, 0 );
            dml.bind( 5, delta[*d].reads/seconds );
            dml.bind( 6, delta[*d].writes/seconds );
            dml.bind( 7, delta[*d].read_sectors*(*d).getSectorSize()/seconds );
            dml.bind( 8, delta[*d].write_sectors*(*d).getSectorSize()/seconds );
            if ( delta[*d].reads > 0 ) dml.bind( 9, delta[*d].read_ms/1000.0/delta[*d].reads ); else dml.bind( 9, 0 );
            if ( delta[*d].writes > 0 ) dml.bind( 10, delta[*d].write_ms/1000.0/delta[*d].writes ); else dml.bind( 10, 0 );
            dml.bind( 11, (double)delta[*d].io_in_progress );
            dml.bind( 12, (double)delta[*d].iodone_cnt/seconds );
            dml.bind( 13, (double)delta[*d].iorequest_cnt/seconds );
            dml.bind( 14, (double)delta[*d].ioerr_cnt/seconds );
            dml.execute();
            stored_disks++;
          }
          if ( stored_disks >= util::ConfigFile::getConfig()->getIntValue("MAX_DISKS") ) break;
        }
        return 0;
      }



      void CPUSnap::startSnap() {
        cpu::getCPUStats( stat1_ );
      }

      void CPUSnap::stopSnap() {
        cpu::getCPUStats( stat2_ );
      }

      long CPUSnap::storeSnap( const persist::Database &db, long snapid, double seconds ) {
        for ( cpu::CPUStatsMap::const_iterator s2 = stat2_.begin(); s2 != stat2_.end(); ++s2 ) {
          cpu::CPUStatsMap::const_iterator s1 = stat1_.find(s2->first);
          if ( s1 != stat1_.end() ) {
            persist::DML dml(db);
            dml.prepare( "INSERT INTO cpustat (snapshot,phyid,coreid,logical,user_mode,system_mode,iowait_mode,nice_mode,irq_mode,softirq_mode,steal_mode) VALUES ( \
              :snapid, \
              :phyid, \
              :coreid, \
              :logical, \
              :user_mode, \
              :system_mode, \
              :iowait_mode, \
              :nice_mode, \
              :irq_mode, \
              :softirq_mode, \
              :steal_mode )" );
            dml.bind( 1, snapid );
            dml.bind( 2, (int)cpu::getCPUPhysicalId( (long)s2->first ) );
            dml.bind( 3, (int)cpu::getCPUCoreId( (long)s2->first ) );
            dml.bind( 4, (long)s2->first );
            dml.bind( 5, (s2->second.user - s1->second.user)/seconds );
            dml.bind( 6, (s2->second.system - s1->second.system)/seconds );
            dml.bind( 7, (s2->second.iowait - s1->second.iowait)/seconds );
            dml.bind( 8, (s2->second.nice - s1->second.nice)/seconds );
            dml.bind( 9, (s2->second.irq - s1->second.irq)/seconds );
            dml.bind( 10, (s2->second.softirq - s1->second.softirq)/seconds );
            dml.bind( 11, (s2->second.steal - s1->second.steal)/seconds );
            dml.execute();
          }
        }
        return 0;
      }


      void SchedSnap::startSnap() {
        sched1_ = cpu::getSchedInfo();
        cpu::getLoadAvg( load1_ );
      }

      void SchedSnap::stopSnap() {
        sched2_ = cpu::getSchedInfo();
        cpu::getLoadAvg( load2_ );
      }

      long SchedSnap::storeSnap( const persist::Database &db, long snapid, double seconds ) {
        persist::DML dml(db);
        dml.prepare( "INSERT INTO schedstat (snapshot,forks,ctxsw,load5,load10,load15,runq,blockq) VALUES ( \
                     :snapid, \
                     :forks, \
                     :ctxsw, \
                     :load5, \
                     :load10, \
                     :load15, \
                     :runq, \
                     :blockq )" );
        dml.bind( 1, snapid );
        dml.bind( 2, (sched2_.processes-sched1_.processes)/seconds );
        dml.bind( 3, (sched2_.ctxt-sched1_.ctxt)/seconds );
        dml.bind( 4, load2_.avg5_ );
        dml.bind( 5, load2_.avg10_ );
        dml.bind( 6, load2_.avg15_ );
        dml.bind( 7, (long)sched2_.running );
        dml.bind( 8, (long)sched2_.blocked );
        dml.execute();
        return 0;
      }



      void NetSnap::startSnap() {
        net::getNetDeviceStat( stat1_ );
      }

      void NetSnap::stopSnap() {
        net::getNetDeviceStat( stat2_ );
      }

      long NetSnap::storeSnap( const persist::Database &db, long snapid, double seconds ) {
        for ( net::NetDeviceStatDeviceMap::const_iterator s2 = stat2_.begin(); s2 != stat2_.end(); ++s2 ) {
          net::NetDeviceStatDeviceMap::const_iterator s1 = stat1_.find(s2->first);
          if ( s1 != stat1_.end() ) {
            std::string mac = net::getDeviceMACAddress( s1->first );
            std::string syspath = net::getSysPath( s1->first );
            long nicid = 0;
            persist::Query qry(db);
            qry.prepare( "SELECT id FROM nic WHERE mac=:mac and syspath=:syspath" );
            qry.bind( 1, mac );
            qry.bind( 2, syspath );
            if ( qry.step() ) {
              nicid = qry.getLong(0);
            } else {
              persist::DML dml(db);
              dml.prepare( "INSERT INTO nic (device,mac,syspath) VALUES (:device,:mac,:syspath)" );
              dml.bind( 1, s1->first );
              dml.bind( 2, mac );
              dml.bind( 3, syspath );
              dml.execute();
              nicid = db.lastInsertRowid();
            }
            persist::DML dml(db);
            dml.prepare( "INSERT INTO netstat (snapshot,nic,rxbs,txbs,rxpkts,txpkts,rxerrs,txerrs) VALUES ( \
              :snapid, \
              :nic, \
              :rxbs, \
              :txbs, \
              :rxpkts, \
              :txpkts, \
              :rxerrs, \
              :txerrs \
              )" );
            double d_rx_bytes = s2->second.rx_bytes - s1->second.rx_bytes;
            double d_tx_bytes = s2->second.tx_bytes - s1->second.tx_bytes;
            double d_rx_packets = s2->second.rx_packets - s1->second.rx_packets;
            double d_tx_packets = s2->second.tx_packets - s1->second.tx_packets;
            double d_rx_errors = s2->second.rx_errors - s1->second.rx_errors;
            double d_tx_errors = s2->second.tx_errors - s1->second.tx_errors;
            dml.bind( 1, snapid );
            dml.bind( 2, nicid );
            dml.bind( 3, d_rx_bytes/seconds );
            dml.bind( 4, d_tx_bytes/seconds );
            dml.bind( 5, d_rx_packets/seconds );
            dml.bind( 6, d_tx_packets/seconds );
            dml.bind( 7, d_rx_errors/seconds );
            dml.bind( 8, d_tx_errors/seconds );
            dml.execute();
          }
        }
        return 0;
      }





      void VMSnap::startSnap() {
        vmem::getVMStat( stat1_ );
      }

      void VMSnap::stopSnap() {
        vmem::getVMStat( stat2_ );
      }

      long VMSnap::storeSnap( const persist::Database &db, long snapid, double seconds ) {
        long pagesize = system::getPageSize();
        std::list<vmem::SwapInfo> swaps;
        vmem::getSwapInfo( swaps );
        double swpused = 0;
        double swpsize = 0;
        for ( std::list<vmem::SwapInfo>::const_iterator i = swaps.begin(); i != swaps.end(); i++ ) {
          swpused += (*i).used;
          swpsize+= (*i).size;
        }
        persist::DML dml(db);
        dml.prepare( "INSERT INTO vmstat (snapshot,realmem,unused,commitas,anon,file,shmem,slab,pagetbls,dirty,pgins,pgouts,swpins,swpouts, \
                      hptotal,hprsvd,hpfree,thpanon,mlock,mapped,swpused,swpsize,minflts,majflts,allocs,frees) VALUES ( \
                     :snapid, \
                     :realmem, \
                     :unused, \
                     :commitas, \
                     :anon, \
                     :file, \
                     :shmem, \
                     :slab, \
                     :pagetbls, \
                     :dirty, \
                     :pgins, \
                     :pgouts, \
                     :swpins, \
                     :swpouts, \
                     :hptotal, \
                     :hprsvd, \
                     :hpfree, \
                     :thpanon, \
                     :mlock, \
                     :mapped, \
                     :swpused, \
                     :swpsize, \
                     :minflts, \
                     :majflts, \
                     :allocs, \
                     :frees )" );
        dml.bind( 1, snapid );
        dml.bind( 2, (long)stat2_.mem_total );
        dml.bind( 3, (long)stat2_.nr_free_pages*pagesize );
        dml.bind( 4, (long)stat2_.committed_as );
        dml.bind( 5, (long)stat2_.nr_anon_pages*pagesize );
        dml.bind( 6, (long)stat2_.nr_file_pages*pagesize );
        dml.bind( 7, (long)stat2_.nr_shmem*pagesize );
        dml.bind( 8, (long)stat2_.nr_slab_reclaimable*pagesize+(long)stat2_.nr_slab_unreclaimable*pagesize );
        dml.bind( 9, (long)stat2_.nr_page_table_pages*pagesize );
        dml.bind( 10, (long)stat2_.nr_dirty*pagesize );
        dml.bind( 11, (stat2_.pgpgin - stat1_.pgpgin) / seconds );
        dml.bind( 12, (stat2_.pgpgout - stat1_.pgpgout) / seconds );
        dml.bind( 13, (stat2_.pswpin - stat1_.pswpin) / seconds );
        dml.bind( 14, (stat2_.pswpout - stat1_.pswpout) / seconds );
        dml.bind( 15, (long)stat2_.hugepages_total*(long)stat2_.hugepagesize );
        dml.bind( 16, (long)stat2_.hugepages_reserved*(long)stat2_.hugepagesize );
        dml.bind( 17, (long)stat2_.hugepages_free*(long)stat2_.hugepagesize );
        dml.bind( 18, (long)stat2_.nr_anon_transparent_hugepages*(long)stat2_.hugepagesize );
        dml.bind( 19, (long)stat2_.nr_mlock*pagesize );
        dml.bind( 20, (long)stat2_.nr_mapped*pagesize );
        dml.bind( 21, swpused );
        dml.bind( 22, swpsize );
        dml.bind( 23, (stat2_.pgfault - stat1_.pgfault) / seconds );
        dml.bind( 24, (stat2_.pgmajfault - stat1_.pgmajfault) / seconds );
        dml.bind( 25, (stat2_.pgalloc_normal + stat2_.pgalloc_dma + stat2_.pgalloc_dma32 + stat2_.pgalloc_movable -
                       stat1_.pgalloc_normal - stat1_.pgalloc_dma - stat1_.pgalloc_dma32 - stat1_.pgalloc_movable) / seconds );
        dml.bind( 26, (stat2_.pgfree - stat1_.pgfree) / seconds );
        dml.execute();
        return 0;
      }



      void ProcSnap::startSnap() {
        process::getAllProcPidStat( snap1_ );
      }

      void ProcSnap::stopSnap() {
        process::getAllProcPidStat( snap2_ );
      }

      long ProcSnap::storeSnap( const persist::Database &db, long snapid, double seconds ) {
        process::ProcPidStatDeltaVector delta;
        process::deltaProcPidStats( snap1_, snap2_, delta );
        process::StatsSorter my_sorter( leanux::process::StatsSorter::top );
        std::sort( delta.begin(), delta.end(), my_sorter );

        persist::Query qry_cmd(db);
        qry_cmd.prepare( "SELECT id FROM cmd WHERE cmd=:cmd and args=:args" );

        persist::Query qry_wchan(db);
        qry_wchan.prepare( "SELECT id FROM wchan WHERE wchan=:wchan" );

        int maxrow = 0;
        long cmdid = 0;
        long wchanid = 0;
        long max_proc = util::ConfigFile::getConfig()->getIntValue("MAX_PROCESSES");
        for ( leanux::process::ProcPidStatDeltaVector::const_iterator i = delta.begin(); i != delta.end() && maxrow < max_proc; i++, maxrow++ ) {
          qry_cmd.reset();
          std::list<std::string> excludecmdargs = util::ConfigFile::getConfig()->getStringListValue("COMMAND_ARGS_IGNORE");
          std::string args = process::getProcCmdLine( (*i).pid );
          for ( std::list<std::string>::const_iterator e = excludecmdargs.begin(); e != excludecmdargs.end(); e++ ) {
            if ( strncmp( (*e).c_str(), snap2_[(*i).pid].comm.c_str(), task_comm_len ) == 0 ) {
              args = "";
              break;
            }
          }

          qry_cmd.bind( 1, snap2_[(*i).pid].comm );
          qry_cmd.bind( 2, args );
          if ( qry_cmd.step() ) {
            cmdid = qry_cmd.getLong(0);
          } else {
            persist::DML dml(db);
            dml.prepare( "INSERT INTO cmd (cmd, args) VALUES (:cmd, :args)" );
            dml.bind( 1, snap2_[(*i).pid].comm );
            dml.bind( 2, args );
            dml.execute();
            cmdid = db.lastInsertRowid();
          }

          if ( (*i).state == 'D' ) {
            qry_wchan.reset();
            qry_wchan.bind( 1, (*i).wchan );
            if ( qry_wchan.step() ) {
              wchanid = qry_wchan.getLong(0);
            } else {
              persist::DML dml(db);
              dml.prepare( "INSERT INTO wchan (wchan) VALUES (:wchan)" );
              dml.bind( 1, (*i).wchan );
              dml.execute();
              wchanid = db.lastInsertRowid();
            }
          }

          persist::DML dml(db);
          dml.prepare( "INSERT INTO procstat (snapshot,pid,pgrp,state,uid,cmd,usercpu,systemcpu,iotime,minflt,majflt,rss,vsz,wchan) VALUES ( \
            :snapid, \
            :pid, \
            :pgrp, \
            :state, \
            :uid, \
            :cmd, \
            :usercpu, \
            :systemcpu, \
            :iotime, \
            :minflt, \
            :majflt, \
            :rss, \
            :vsz, \
            :wchan \
            )" );
          dml.bind( 1, snapid );
          dml.bind( 2, (*i).pid );
          dml.bind( 3, (*i).pgrp );
          std::string sstate = "";
          sstate += (*i).state;
          dml.bind( 4, sstate );
          uid_t uid = 0;
          if ( !process::getProcUid( (*i).pid, uid ) ) uid = 20041968;
          dml.bind( 5,  (long)uid );
          dml.bind( 6, cmdid );
          dml.bind( 7, (*i).utime/seconds );
          dml.bind( 8, (*i).stime/seconds );
          dml.bind( 9, (*i).delayacct_blkio_ticks/seconds );
          dml.bind( 10, (*i).minflt/seconds );
          dml.bind( 11, (*i).majflt/seconds );
          dml.bind( 12, (long)(*i).rss );
          dml.bind( 13, (long)(*i).vsize );
          if ( (*i).state == 'D' ) dml.bind( 14, wchanid ); else dml.bind( 14, 0 );
          dml.execute();
        }
        return 0;
      }



      void ResSnap::startSnap() {
      }

      void ResSnap::stopSnap() {
      }

      long ResSnap::storeSnap( const persist::Database &db, long snapid, double seconds ) {
        persist::DML dml(db);
        dml.prepare( "INSERT INTO resstat (snapshot,processes,users,logins,files_open,files_max,inodes_open,inodes_free) VALUES ( \
                     :snapid, \
                     :processes, \
                     :users, \
                     :logins, \
                     :files_open, \
                     :files_max, \
                     :inodes_open, \
                     :inodes_free)" );
        dml.bind( 1, snapid );
        unsigned long processes = 0;
        system::getNumProcesses( &processes );
        dml.bind( 2, (long)processes );
        dml.bind( 3, (long)system::getNumLoginUsers() );
        dml.bind( 4, (long)system::getNumLogins() );
        unsigned long used,max,free;
        system::getOpenFiles( &used, &max );
        dml.bind( 5, (long)used );
        dml.bind( 6, (long)max );
        system::getOpenInodes( &used, &free );
        dml.bind( 7, (long)used );
        dml.bind( 8, (long)free );
        dml.execute();
        return 0;
        return 0;
      }




      std::map<std::string,block::MajorMinor> MountSnap::devicefilecache_;

      void MountSnap::startSnap() {
        block::getStats( stat1_ );
      }

      void MountSnap::stopSnap() {
        block::getStats( stat2_ );

        fsbytes1_ = fsbytes2_;

        std::map<block::MajorMinor,block::MountInfo> mounts;
        enumMounts( mounts, devicefilecache_ );
        for ( std::map<block::MajorMinor,block::MountInfo>::const_iterator m = mounts.begin(); m != mounts.end(); ++m ) {
          fsbytes2_[m->second.mountpoint] = block::getMountUsedBytes( m->second.mountpoint );
        }
      }

      long MountSnap::storeSnap( const persist::Database &db, long snapid, double seconds ) {
        std::map<block::MajorMinor,block::MountInfo> mounts;
        block::enumMounts( mounts, devicefilecache_ );
        block::DeviceStatsMap delta;
        block::MajorMinorVector vec;
        block::deltaDeviceStats( stat1_, stat2_, delta, vec );
        block::StatsSorter sorter( &delta );
        sort( vec.begin(), vec.end(), sorter );
        long stored_mounts = 0;
        persist::Query qry(db);
        qry.prepare( "SELECT id FROM mountpoint WHERE mountpoint=:mountpoint" );
        for ( block::MajorMinorVector::const_iterator d = vec.begin(); d != vec.end(); d++ ) {
          bool isswap = false;
          std::string mp = "";
          std::map<block::MajorMinor,block::MountInfo>::const_iterator m = mounts.find( *d );
          if ( m == mounts.end() ) {
            std::list<vmem::SwapInfo> swaps;
            vmem::getSwapInfo( swaps );
            for ( std::list<vmem::SwapInfo>::const_iterator i = swaps.begin(); i != swaps.end(); i++ ) {
              if ( block::getFileMajorMinor( (*i).devicefile ) == *d ) {
                isswap = true;
                std::stringstream sswap;
                sswap << "swap:" << (*i).devicefile;
                mp = sswap.str();
                break;
              }
            }
          } else mp = mounts[*d].mountpoint;
          if ( m != mounts.end() || isswap ) {
            qry.reset();
            qry.bind( 1, mp );
            long mpid = 0;
            if ( qry.step() ) {
              mpid = qry.getLong(0);
            } else {
              persist::DML dml(db);
              dml.prepare( "INSERT INTO mountpoint (mountpoint) VALUES (:mountpoint)" );
              dml.bind( 1, mp );
              dml.execute();
              mpid = db.lastInsertRowid();
            }

            persist::DML dml(db);
            dml.prepare( "INSERT INTO mountstat VALUES ( \
              :snapid, \
              :mountpoint, \
              :util, \
              :svctm, \
              :rs, \
              :ws, \
              :rbs, \
              :wbs, \
              :artm, \
              :awtm, \
              :growth, \
              :used \
              )" );
            dml.bind( 1, snapid );
            dml.bind( 2, mpid );
            dml.bind( 3, delta[*d].io_ms/1000.0/seconds  );
            if ( delta[*d].reads+delta[*d].writes > 0 )
              dml.bind( 4, delta[*d].io_ms/1000.0/(delta[*d].reads+delta[*d].writes)  );
            else
              dml.bind( 4, 0.0  );
            dml.bind( 5, delta[*d].reads/seconds  );
            dml.bind( 6, delta[*d].writes/seconds  );
            dml.bind( 7, delta[*d].read_sectors*(*d).getSectorSize()/seconds );
            dml.bind( 8, delta[*d].write_sectors*(*d).getSectorSize()/seconds );
            if ( delta[*d].reads > 0 )
              dml.bind( 9, delta[*d].read_ms/1000.0/delta[*d].reads );
            else
              dml.bind( 9, 0 );
            if ( delta[*d].writes > 0 )
              dml.bind( 10, delta[*d].write_ms/1000.0/delta[*d].writes );
            else
              dml.bind( 10, 0 );
            if ( isswap ) {
              dml.bind( 11, 0 );
              dml.bind( 12, 0 );
            } else {
              std::map<std::string,unsigned long>::const_iterator m1,m2;
              m1 = fsbytes1_.find( m->second.mountpoint );
              m2 = fsbytes2_.find( m->second.mountpoint );
              if ( m1 != fsbytes1_.end() && m2 != fsbytes2_.end() ) {
                dml.bind( 11, ((double)m2->second-(double)m1->second)/seconds );
                dml.bind( 12, (double)m2->second );
              } else {
                dml.bind( 11, 0 );
                dml.bind( 12, 0 );
              }
            }
            dml.execute();
            stored_mounts++;
            qry.reset();
          }
          if ( stored_mounts >= util::ConfigFile::getConfig()->getIntValue("MAX_MOUNTS") ) break;
        }
        return 0;
      }



      long TCPEstaSnap::storeSnap( const persist::Database &db, long snapid, double seconds ) {

        std::list<net::TCPKeyCounter> server;
        std::list<net::TCPKeyCounter> client;
        getTCPConnectionCounters( server, client );

        persist::Query qry(db);
        qry.prepare( "SELECT id FROM tcpkey WHERE ip=:ip AND port=:port AND uid=:uid" );
        for ( std::list<net::TCPKeyCounter>::const_iterator i = server.begin(); i!= server.end(); i++ ) {
          qry.reset();
          qry.bind( 1, (*i).getKey().getIP() );
          qry.bind( 2, (long)(*i).getKey().getPort() );
          qry.bind( 3, (long)(*i).getKey().getUID() );
          long tcpkeyid = 0;
          if ( qry.step() ) {
            tcpkeyid = qry.getLong(0);
          } else {
            persist::DML dml(db);
            dml.prepare( "INSERT INTO tcpkey (ip,port,uid) VALUES (:ip,:port,:uid)" );
            dml.bind( 1, (*i).getKey().getIP() );
            dml.bind( 2, (long)(*i).getKey().getPort() );
            dml.bind( 3, (long)(*i).getKey().getUID() );
            dml.execute();
            tcpkeyid = db.lastInsertRowid();
          }

          persist::DML dml(db);
          dml.prepare( "INSERT INTO tcpserverstat VALUES ( \
            :snapid, \
            :tcpkey, \
            :esta \
            )" );
          dml.bind( 1, snapid );
          dml.bind( 2, tcpkeyid );
          dml.bind( 3, (long)(*i).getEsta() );
          dml.execute();
          qry.reset();
        }


        for ( std::list<net::TCPKeyCounter>::const_iterator i = client.begin(); i!= client.end(); i++ ) {
          qry.prepare( "SELECT id FROM tcpkey WHERE ip=:ip AND port=:port AND uid=:uid" );
          qry.bind( 1, (*i).getKey().getIP() );
          qry.bind( 2, (long)(*i).getKey().getPort() );
          qry.bind( 3, (long)(*i).getKey().getUID() );
          long tcpkeyid = 0;
          if ( qry.step() ) {
            tcpkeyid = qry.getLong(0);
          } else {
            persist::DML dml(db);
            dml.prepare( "INSERT INTO tcpkey (ip,port,uid) VALUES (:ip,:port,:uid)" );
            dml.bind( 1, (*i).getKey().getIP() );
            dml.bind( 2, (long)(*i).getKey().getPort() );
            dml.bind( 3, (long)(*i).getKey().getUID() );
            dml.execute();
            tcpkeyid = db.lastInsertRowid();
          }

          persist::DML dml(db);
          dml.prepare( "INSERT INTO tcpclientstat VALUES ( \
            :snapid, \
            :tcpkey, \
            :esta \
            )" );
          dml.bind( 1, snapid );
          dml.bind( 2, tcpkeyid );
          dml.bind( 3, (long)(*i).getEsta() );
          dml.execute();
          qry.reset();
        }
        return 0;
      }
      
      void TCPSnap::startSnap() {
        getTCPStatNetstat( stat1_ );
        getTCPStatSnmp( stat1_ );
      }
      
      void TCPSnap::stopSnap() {
        getTCPStatNetstat( stat2_ );
        getTCPStatSnmp( stat2_ );
        diffSnap();
      }
      
      #define DIFFDO(x) diff_.x = stat2_.x - stat1_.x
      
      void TCPSnap::diffSnap() {
        memset(&diff_,0, sizeof(diff_));
        DIFFDO(RtoAlgorithm);
        DIFFDO(RtoMin);
        DIFFDO(RtoMax);
        DIFFDO(MaxConn);
        DIFFDO(ActiveOpens);
        DIFFDO(PassiveOpens);
        DIFFDO(AttemptFails);
        DIFFDO(EstabResets);
        DIFFDO(CurrEstab);
        DIFFDO(InSegs);
        DIFFDO(OutSegs);
        DIFFDO(RetransSegs);
        DIFFDO(InErrs);
        DIFFDO(OutRsts);
        DIFFDO(InCsumErrors);        
        DIFFDO(SyncookiesSent);
        DIFFDO(SyncookiesRecv);
        DIFFDO(SyncookiesFailed);
        DIFFDO(EmbryonicRsts);
        DIFFDO(PruneCalled);
        DIFFDO(RcvPruned);
        DIFFDO(OfoPruned);
        DIFFDO(OutOfWindowIcmps);
        DIFFDO(LockDroppedIcmps);
        DIFFDO(ArpFilter);
        DIFFDO(TW);
        DIFFDO(TWRecycled);
        DIFFDO(TWKilled);
        DIFFDO(PAWSActive);
        DIFFDO(PAWSEstab);
        DIFFDO(DelayedACKs);
        DIFFDO(DelayedACKLocked);
        DIFFDO(DelayedACKLost);
        DIFFDO(ListenOverflows);
        DIFFDO(ListenDrops);
        DIFFDO(TCPHPHits);
        DIFFDO(TCPPureAcks);
        DIFFDO(TCPHPAcks);
        DIFFDO(TCPRenoRecovery);
        DIFFDO(TCPSackRecovery);
        DIFFDO(TCPSACKReneging);
        DIFFDO(TCPSACKReorder);
        DIFFDO(TCPRenoReorder);
        DIFFDO(TCPTSReorder);
        DIFFDO(TCPFullUndo);
        DIFFDO(TCPPartialUndo);
        DIFFDO(TCPDSACKUndo);
        DIFFDO(TCPLossUndo);
        DIFFDO(TCPLostRetransmit);
        DIFFDO(TCPRenoFailures);
        DIFFDO(TCPSackFailures);
        DIFFDO(TCPLossFailures);
        DIFFDO(TCPFastRetrans);
        DIFFDO(TCPSlowStartRetrans);
        DIFFDO(TCPTimeouts);
        DIFFDO(TCPLossProbes);
        DIFFDO(TCPLossProbeRecovery);
        DIFFDO(TCPRenoRecoveryFail);
        DIFFDO(TCPSackRecoveryFail);
        DIFFDO(TCPRcvCollapsed);
        DIFFDO(TCPBacklogCoalesce);
        DIFFDO(TCPDSACKOldSent);
        DIFFDO(TCPDSACKOfoSent);
        DIFFDO(TCPDSACKRecv);
        DIFFDO(TCPDSACKOfoRecv);
        DIFFDO(TCPAbortOnData);
        DIFFDO(TCPAbortOnClose);
        DIFFDO(TCPAbortOnMemory);
        DIFFDO(TCPAbortOnTimeout);
        DIFFDO(TCPAbortOnLinger);
        DIFFDO(TCPAbortFailed);
        DIFFDO(TCPMemoryPressures);
        DIFFDO(TCPMemoryPressuresChrono);
        DIFFDO(TCPSACKDiscard);
        DIFFDO(TCPDSACKIgnoredOld);
        DIFFDO(TCPDSACKIgnoredNoUndo);
        DIFFDO(TCPSpuriousRTOs);
        DIFFDO(TCPMD5NotFound);
        DIFFDO(TCPMD5Unexpected);
        DIFFDO(TCPMD5Failure);
        DIFFDO(TCPSackShifted);
        DIFFDO(TCPSackMerged);
        DIFFDO(TCPSackShiftFallback);
        DIFFDO(TCPBacklogDrop);
        DIFFDO(PFMemallocDrop);
        DIFFDO(TCPMinTTLDrop);
        DIFFDO(TCPDeferAcceptDrop);
        DIFFDO(IPReversePathFilter);
        DIFFDO(TCPTimeWaitOverflow);
        DIFFDO(TCPReqQFullDoCookies);
        DIFFDO(TCPReqQFullDrop);
        DIFFDO(TCPRetransFail);
        DIFFDO(TCPRcvCoalesce);
        DIFFDO(TCPOFOQueue);
        DIFFDO(TCPOFODrop);
        DIFFDO(TCPOFOMerge);
        DIFFDO(TCPChallengeACK);
        DIFFDO(TCPSYNChallenge);
        DIFFDO(TCPFastOpenActive);
        DIFFDO(TCPFastOpenActiveFail);
        DIFFDO(TCPFastOpenPassive);
        DIFFDO(TCPFastOpenPassiveFail);
        DIFFDO(TCPFastOpenListenOverflow);
        DIFFDO(TCPFastOpenCookieReqd);
        DIFFDO(TCPFastOpenBlackhole);
        DIFFDO(TCPSpuriousRtxHostQueues);
        DIFFDO(BusyPollRxPackets);
        DIFFDO(TCPAutoCorking);
        DIFFDO(TCPFromZeroWindowAdv);
        DIFFDO(TCPToZeroWindowAdv);
        DIFFDO(TCPWantZeroWindowAdv);
        DIFFDO(TCPSynRetrans);
        DIFFDO(TCPOrigDataSent);
        DIFFDO(TCPHystartTrainDetect);
        DIFFDO(TCPHystartTrainCwnd);
        DIFFDO(TCPHystartDelayDetect);
        DIFFDO(TCPHystartDelayCwnd);
        DIFFDO(TCPACKSkippedSynRecv);
        DIFFDO(TCPACKSkippedPAWS);
        DIFFDO(TCPACKSkippedSeq);
        DIFFDO(TCPACKSkippedFinWait2);
        DIFFDO(TCPACKSkippedTimeWait);
        DIFFDO(TCPACKSkippedChallenge);
        DIFFDO(TCPWinProbe);
        DIFFDO(TCPKeepAlive);
        DIFFDO(TCPMTUPFail);
        DIFFDO(TCPMTUPSuccess);
        DIFFDO(TCPDelivered);
        DIFFDO(TCPDeliveredCE);
        DIFFDO(TCPAckCompressed);
        DIFFDO(TCPZeroWindowDrop);
        DIFFDO(TCPRcvQDrop);
        DIFFDO(TCPWqueueTooBig);
        DIFFDO(TCPFastOpenPassiveAltKey);
        DIFFDO(TcpTimeoutRehash);
        DIFFDO(TcpDuplicateDataRehash);
        DIFFDO(TCPDSACKRecvSegs);
        DIFFDO(TCPDSACKIgnoredDubious);
        DIFFDO(TCPMigrateReqSuccess);
        DIFFDO(TCPMigrateReqFailure);
      }      
      
      long TCPSnap::storeSnap( const persist::Database &db, long snapid, double seconds ) {
        persist::DML dml(db);
        dml.prepare( 
          "INSERT INTO tcpstat values("
          ":snapid,"
          ":RtoAlgorithm,"
          ":RtoMin,"
          ":RtoMax,"
          ":MaxConn,"
          ":ActiveOpens,"
          ":PassiveOpens,"
          ":AttemptFails,"
          ":EstabResets,"
          ":CurrEstab,"
          ":InSegs,"
          ":OutSegs,"
          ":RetransSegs,"
          ":InErrs,"
          ":OutRsts,"
          ":InCsumErrors,"
          ":SyncookiesSent,"
          ":SyncookiesRecv,"
          ":SyncookiesFailed,"
          ":EmbryonicRsts,"
          ":PruneCalled,"
          ":RcvPruned,"
          ":OfoPruned,"
          ":OutOfWindowIcmps,"
          ":LockDroppedIcmps,"
          ":ArpFilter,"
          ":TW,"
          ":TWRecycled,"
          ":TWKilled,"
          ":PAWSActive,"
          ":PAWSEstab,"
          ":DelayedACKs,"
          ":DelayedACKLocked,"
          ":DelayedACKLost,"
          ":ListenOverflows,"
          ":ListenDrops,"
          ":TCPHPHits,"
          ":TCPPureAcks,"
          ":TCPHPAcks,"
          ":TCPRenoRecovery,"
          ":TCPSackRecovery,"
          ":TCPSACKReneging,"
          ":TCPSACKReorder,"
          ":TCPRenoReorder,"
          ":TCPTSReorder,"
          ":TCPFullUndo,"
          ":TCPPartialUndo,"
          ":TCPDSACKUndo,"
          ":TCPLossUndo,"
          ":TCPLostRetransmit,"
          ":TCPRenoFailures,"
          ":TCPSackFailures,"
          ":TCPLossFailures,"
          ":TCPFastRetrans,"
          ":TCPSlowStartRetrans,"
          ":TCPTimeouts,"
          ":TCPLossProbes,"
          ":TCPLossProbeRecovery,"
          ":TCPRenoRecoveryFail,"
          ":TCPSackRecoveryFail,"
          ":TCPRcvCollapsed,"
          ":TCPBacklogCoalesce,"
          ":TCPDSACKOldSent,"
          ":TCPDSACKOfoSent,"
          ":TCPDSACKRecv,"
          ":TCPDSACKOfoRecv,"
          ":TCPAbortOnData,"
          ":TCPAbortOnClose,"
          ":TCPAbortOnMemory,"
          ":TCPAbortOnTimeout,"
          ":TCPAbortOnLinger,"
          ":TCPAbortFailed,"
          ":TCPMemoryPressures,"
          ":TCPMemoryPressuresChrono,"
          ":TCPSACKDiscard,"
          ":TCPDSACKIgnoredOld,"
          ":TCPDSACKIgnoredNoUndo,"
          ":TCPSpuriousRTOs,"
          ":TCPMD5NotFound,"
          ":TCPMD5Unexpected,"
          ":TCPMD5Failure,"
          ":TCPSackShifted,"
          ":TCPSackMerged,"
          ":TCPSackShiftFallback,"
          ":TCPBacklogDrop,"
          ":PFMemallocDrop,"
          ":TCPMinTTLDrop,"
          ":TCPDeferAcceptDrop,"
          ":IPReversePathFilter,"
          ":TCPTimeWaitOverflow,"
          ":TCPReqQFullDoCookies,"
          ":TCPReqQFullDrop,"
          ":TCPRetransFail,"
          ":TCPRcvCoalesce,"
          ":TCPOFOQueue,"
          ":TCPOFODrop,"
          ":TCPOFOMerge,"
          ":TCPChallengeACK,"
          ":TCPSYNChallenge,"
          ":TCPFastOpenActive,"
          ":TCPFastOpenActiveFail,"
          ":TCPFastOpenPassive,"
          ":TCPFastOpenPassiveFail,"
          ":TCPFastOpenListenOverflow,"
          ":TCPFastOpenCookieReqd,"
          ":TCPFastOpenBlackhole,"
          ":TCPSpuriousRtxHostQueues,"
          ":BusyPollRxPackets,"
          ":TCPAutoCorking,"
          ":TCPFromZeroWindowAdv,"
          ":TCPToZeroWindowAdv,"
          ":TCPWantZeroWindowAdv,"
          ":TCPSynRetrans,"
          ":TCPOrigDataSent,"
          ":TCPHystartTrainDetect,"
          ":TCPHystartTrainCwnd,"
          ":TCPHystartDelayDetect,"
          ":TCPHystartDelayCwnd,"
          ":TCPACKSkippedSynRecv,"
          ":TCPACKSkippedPAWS,"
          ":TCPACKSkippedSeq,"
          ":TCPACKSkippedFinWait2,"
          ":TCPACKSkippedTimeWait,"
          ":TCPACKSkippedChallenge,"
          ":TCPWinProbe,"
          ":TCPKeepAlive,"
          ":TCPMTUPFail,"
          ":TCPMTUPSuccess,"
          ":TCPDelivered,"
          ":TCPDeliveredCE,"
          ":TCPAckCompressed,"
          ":TCPZeroWindowDrop,"
          ":TCPRcvQDrop,"
          ":TCPWqueueTooBig,"
          ":TCPFastOpenPassiveAltKey,"
          ":TcpTimeoutRehash,"
          ":TcpDuplicateDataRehash,"
          ":TCPDSACKRecvSegs,"
          ":TCPDSACKIgnoredDubious,"
          ":TCPMigrateReqSuccess,"
          ":TCPMigrateReqFailure)"
        );
        int c= 1;
        dml.bind( c++, snapid );
        dml.bind( c++, diff_.RtoAlgorithm );
        dml.bind( c++, diff_.RtoMin );
        dml.bind( c++, diff_.RtoMax );
        dml.bind( c++, diff_.MaxConn );
        dml.bind( c++, diff_.ActiveOpens );
        dml.bind( c++, diff_.PassiveOpens );
        dml.bind( c++, diff_.AttemptFails );
        dml.bind( c++, diff_.EstabResets );
        dml.bind( c++, diff_.CurrEstab );
        dml.bind( c++, diff_.InSegs );
        dml.bind( c++, diff_.OutSegs );
        dml.bind( c++, diff_.RetransSegs );
        dml.bind( c++, diff_.InErrs );
        dml.bind( c++, diff_.OutRsts );
        dml.bind( c++, diff_.InCsumErrors );
        dml.bind( c++, diff_.SyncookiesSent );
        dml.bind( c++, diff_.SyncookiesRecv );
        dml.bind( c++, diff_.SyncookiesFailed );
        dml.bind( c++, diff_.EmbryonicRsts );
        dml.bind( c++, diff_.PruneCalled );
        dml.bind( c++, diff_.RcvPruned );
        dml.bind( c++, diff_.OfoPruned );
        dml.bind( c++, diff_.OutOfWindowIcmps );
        dml.bind( c++, diff_.LockDroppedIcmps );
        dml.bind( c++, diff_.ArpFilter );
        dml.bind( c++, diff_.TW );
        dml.bind( c++, diff_.TWRecycled );
        dml.bind( c++, diff_.TWKilled );
        dml.bind( c++, diff_.PAWSActive );
        dml.bind( c++, diff_.PAWSEstab );
        dml.bind( c++, diff_.DelayedACKs );
        dml.bind( c++, diff_.DelayedACKLocked );
        dml.bind( c++, diff_.DelayedACKLost );
        dml.bind( c++, diff_.ListenOverflows );
        dml.bind( c++, diff_.ListenDrops );
        dml.bind( c++, diff_.TCPHPHits );
        dml.bind( c++, diff_.TCPPureAcks );
        dml.bind( c++, diff_.TCPHPAcks );
        dml.bind( c++, diff_.TCPRenoRecovery );
        dml.bind( c++, diff_.TCPSackRecovery );
        dml.bind( c++, diff_.TCPSACKReneging );
        dml.bind( c++, diff_.TCPSACKReorder );
        dml.bind( c++, diff_.TCPRenoReorder );
        dml.bind( c++, diff_.TCPTSReorder );
        dml.bind( c++, diff_.TCPFullUndo );
        dml.bind( c++, diff_.TCPPartialUndo );
        dml.bind( c++, diff_.TCPDSACKUndo );
        dml.bind( c++, diff_.TCPLossUndo );
        dml.bind( c++, diff_.TCPLostRetransmit );
        dml.bind( c++, diff_.TCPRenoFailures );
        dml.bind( c++, diff_.TCPSackFailures );
        dml.bind( c++, diff_.TCPLossFailures );
        dml.bind( c++, diff_.TCPFastRetrans );
        dml.bind( c++, diff_.TCPSlowStartRetrans );
        dml.bind( c++, diff_.TCPTimeouts );
        dml.bind( c++, diff_.TCPLossProbes );
        dml.bind( c++, diff_.TCPLossProbeRecovery );
        dml.bind( c++, diff_.TCPRenoRecoveryFail );
        dml.bind( c++, diff_.TCPSackRecoveryFail );
        dml.bind( c++, diff_.TCPRcvCollapsed );
        dml.bind( c++, diff_.TCPBacklogCoalesce );
        dml.bind( c++, diff_.TCPDSACKOldSent );
        dml.bind( c++, diff_.TCPDSACKOfoSent );
        dml.bind( c++, diff_.TCPDSACKRecv );
        dml.bind( c++, diff_.TCPDSACKOfoRecv );
        dml.bind( c++, diff_.TCPAbortOnData );
        dml.bind( c++, diff_.TCPAbortOnClose );
        dml.bind( c++, diff_.TCPAbortOnMemory );
        dml.bind( c++, diff_.TCPAbortOnTimeout );
        dml.bind( c++, diff_.TCPAbortOnLinger );
        dml.bind( c++, diff_.TCPAbortFailed );
        dml.bind( c++, diff_.TCPMemoryPressures );
        dml.bind( c++, diff_.TCPMemoryPressuresChrono );
        dml.bind( c++, diff_.TCPSACKDiscard );
        dml.bind( c++, diff_.TCPDSACKIgnoredOld );
        dml.bind( c++, diff_.TCPDSACKIgnoredNoUndo );
        dml.bind( c++, diff_.TCPSpuriousRTOs );
        dml.bind( c++, diff_.TCPMD5NotFound );
        dml.bind( c++, diff_.TCPMD5Unexpected );
        dml.bind( c++, diff_.TCPMD5Failure );
        dml.bind( c++, diff_.TCPSackShifted );
        dml.bind( c++, diff_.TCPSackMerged );
        dml.bind( c++, diff_.TCPSackShiftFallback );
        dml.bind( c++, diff_.TCPBacklogDrop );
        dml.bind( c++, diff_.PFMemallocDrop );
        dml.bind( c++, diff_.TCPMinTTLDrop );
        dml.bind( c++, diff_.TCPDeferAcceptDrop );
        dml.bind( c++, diff_.IPReversePathFilter );
        dml.bind( c++, diff_.TCPTimeWaitOverflow );
        dml.bind( c++, diff_.TCPReqQFullDoCookies );
        dml.bind( c++, diff_.TCPReqQFullDrop );
        dml.bind( c++, diff_.TCPRetransFail );
        dml.bind( c++, diff_.TCPRcvCoalesce );
        dml.bind( c++, diff_.TCPOFOQueue );
        dml.bind( c++, diff_.TCPOFODrop );
        dml.bind( c++, diff_.TCPOFOMerge );
        dml.bind( c++, diff_.TCPChallengeACK );
        dml.bind( c++, diff_.TCPSYNChallenge );
        dml.bind( c++, diff_.TCPFastOpenActive );
        dml.bind( c++, diff_.TCPFastOpenActiveFail );
        dml.bind( c++, diff_.TCPFastOpenPassive );
        dml.bind( c++, diff_.TCPFastOpenPassiveFail );
        dml.bind( c++, diff_.TCPFastOpenListenOverflow );
        dml.bind( c++, diff_.TCPFastOpenCookieReqd );
        dml.bind( c++, diff_.TCPFastOpenBlackhole );
        dml.bind( c++, diff_.TCPSpuriousRtxHostQueues );
        dml.bind( c++, diff_.BusyPollRxPackets );
        dml.bind( c++, diff_.TCPAutoCorking );
        dml.bind( c++, diff_.TCPFromZeroWindowAdv );
        dml.bind( c++, diff_.TCPToZeroWindowAdv );
        dml.bind( c++, diff_.TCPWantZeroWindowAdv );
        dml.bind( c++, diff_.TCPSynRetrans );
        dml.bind( c++, diff_.TCPOrigDataSent );
        dml.bind( c++, diff_.TCPHystartTrainDetect );
        dml.bind( c++, diff_.TCPHystartTrainCwnd );
        dml.bind( c++, diff_.TCPHystartDelayDetect );
        dml.bind( c++, diff_.TCPHystartDelayCwnd );
        dml.bind( c++, diff_.TCPACKSkippedSynRecv );
        dml.bind( c++, diff_.TCPACKSkippedPAWS );
        dml.bind( c++, diff_.TCPACKSkippedSeq );
        dml.bind( c++, diff_.TCPACKSkippedFinWait2 );
        dml.bind( c++, diff_.TCPACKSkippedTimeWait );
        dml.bind( c++, diff_.TCPACKSkippedChallenge );
        dml.bind( c++, diff_.TCPWinProbe );
        dml.bind( c++, diff_.TCPKeepAlive );
        dml.bind( c++, diff_.TCPMTUPFail );
        dml.bind( c++, diff_.TCPMTUPSuccess );
        dml.bind( c++, diff_.TCPDelivered );
        dml.bind( c++, diff_.TCPDeliveredCE );
        dml.bind( c++, diff_.TCPAckCompressed );
        dml.bind( c++, diff_.TCPZeroWindowDrop );
        dml.bind( c++, diff_.TCPRcvQDrop );
        dml.bind( c++, diff_.TCPWqueueTooBig );
        dml.bind( c++, diff_.TCPFastOpenPassiveAltKey );
        dml.bind( c++, diff_.TcpTimeoutRehash );
        dml.bind( c++, diff_.TcpDuplicateDataRehash );
        dml.bind( c++, diff_.TCPDSACKRecvSegs );
        dml.bind( c++, diff_.TCPDSACKIgnoredDubious );
        dml.bind( c++, diff_.TCPMigrateReqSuccess );
        dml.bind( c++, diff_.TCPMigrateReqFailure );
        dml.execute();
        return 0;
      }

    }; // namespace lard
  }; // namespace tools
}; // namespace leanux

