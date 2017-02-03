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
 * ncurses based real time linux performance monitoring tool - c++ source file.
 * RealtimeSampler implementation.
 */

#include "realtime.hpp"
#include "system.hpp"
#include "util.hpp"
#include <sys/time.h>
#include <algorithm>
#include <math.h>

namespace leanux {

  namespace tools {

    namespace lmon {

      std::map<std::string,block::MajorMinor> RealtimeSampler::devicefilecache_;

      RealtimeSampler::RealtimeSampler() : xioview_(), xsysview_(), xnetview_(), xprocview_() {
        xsysview_.pagesize_ = system::getPageSize();
        cpu::getCPUInfo( cpuinfo_ );
        mounted_bytes_1_ = 0;
        mounted_bytes_2_ = 0;

        xprocview_.disabled = false;
        sample(0);
      }

      void RealtimeSampler::sample( int cpubarheight ) {
        sampleXSysView( cpubarheight );
        sampleXIOView();
        sampleXNetView();
        sampleXProcView();
      }

      void RealtimeSampler::sampleXProcView() {
        xprocview_.t1 = xprocview_.t2;
        xprocview_.pidargs.clear();
        xprocview_.piduids.clear();
        if ( !xprocview_.disabled ) {
          procsnap1_ = procsnap2_;
          gettimeofday( &xprocview_.t2, 0 );
          double dt = util::deltaTime( xprocview_.t1, xprocview_.t2 );
          util::Stopwatch sw;
          process::getAllProcPidStat( procsnap2_ );
          double duration = sw.stop();
          if ( duration > 29.22 ) {
            xprocview_.disabled = true;
            system::getNumProcesses( &disabled_procs_ );
          } else {
            deltaProcPidStats( procsnap1_, procsnap2_, xprocview_.delta );
            process::StatsSorter top_sorter( process::StatsSorter::top );
            sort( xprocview_.delta.begin(), xprocview_.delta.end(), top_sorter );
            for ( process::ProcPidStatDeltaVector::iterator i = xprocview_.delta.begin(); i != xprocview_.delta.end(); i++ ) {
              (*i).delayacct_blkio_ticks /= dt;
              (*i).majflt /= dt;
              (*i).minflt /= dt;
              (*i).stime /= dt;
              (*i).utime /= dt;
              uid_t uid;
              process::getProcUid( (*i).pid, uid );
              xprocview_.piduids[(*i).pid] = uid;
              xprocview_.pidargs[(*i).pid] = process::getProcCmdLine( (*i).pid );
            }
          }
        } else {
          unsigned long procs_now;
          system::getNumProcesses( &procs_now );
          if ( (double)procs_now < (double)disabled_procs_ * 0.9 ) {
            disabled_procs_ = 0;
            xprocview_.disabled = false;
          }
        }
        xprocview_.sample_count++;
      }

      void RealtimeSampler::sampleXNetView() {
        xnetview_.t1 = xnetview_.t2;
        gettimeofday( &xnetview_.t2, 0 );
        netsnap1_ = netsnap2_;
        net::getNetStat( netsnap2_ );
        net::getNetStatDelta( netsnap1_, netsnap2_, xnetview_.delta );
        double dt = util::deltaTime( xnetview_.t1, xnetview_.t2 );
        for ( net::NetStatDeviceVector::iterator i = xnetview_.delta.begin(); i != xnetview_.delta.end(); i++ ) {
          (*i).rx_bytes /= dt;
          (*i).tx_bytes /= dt;
          (*i).rx_packets /= dt;
          (*i).tx_packets /= dt;
          (*i).rx_errors /= dt;
          (*i).tx_errors /= dt;
        }
        getTCPConnectionCounters( xnetview_.tcpserver, xnetview_.tcpclient );
        xnetview_.sample_count++;
      }

      void RealtimeSampler::sampleXIOView() {
        block::DeviceStatsMap devicestats;
        block::MajorMinorVector sorted;
        diskstats1_ = diskstats2_;
        xioview_.t1 = xioview_.t2;
        gettimeofday( &xioview_.t2, 0 );
        double dt = util::deltaTime( xioview_.t1, xioview_.t2 );
        block::getStats( diskstats2_ );
        xioview_.iosorted.clear();
        xioview_.iostats.clear();
        xioview_.mountsorted.clear();
        xioview_.mountstats.clear();
        block::deltaDeviceStats( diskstats1_, diskstats2_, devicestats, sorted );
        block::StatsSorter sorter(&devicestats);
        sort( sorted.begin(), sorted.end(), sorter );

        for ( block::MajorMinorVector::const_iterator s = sorted.begin(); s != sorted.end(); s++ ) {
          XIORec rec;
          rec.device = (*s).getName();
          rec.util  = devicestats[*s].io_ms / 1000.0 / dt;
          rec.rs    = devicestats[*s].reads / dt;
          rec.ws    = devicestats[*s].writes / dt;
          rec.rbs   = devicestats[*s].read_sectors * (*s).getSectorSize() / dt;
          rec.wbs   = devicestats[*s].write_sectors * (*s).getSectorSize() / dt;
          if ( devicestats[*s].reads != 0 )
            rec.artm  = devicestats[*s].read_ms / 1000.0 / devicestats[*s].reads;
          else
            rec.artm = 0;
          if ( devicestats[*s].writes != 0 )
            rec.awtm  = devicestats[*s].write_ms  / 1000.0 / devicestats[*s].writes;
          else
            rec.awtm = 0;
          if ( (devicestats[*s].reads+devicestats[*s].writes) != 0 )
            rec.svctm = devicestats[*s].io_ms / 1000.0 / (devicestats[*s].reads+devicestats[*s].writes);
          else
            rec.svctm = 0;
          if ( rec.svctm != 0 )
            rec.qsz   =  (rec.awtm+rec.awtm)/rec.svctm;
          else
            rec.qsz = 0;
          rec.iodone_cnt = devicestats[*s].iodone_cnt / dt;
          rec.iorequest_cnt = devicestats[*s].iorequest_cnt / dt;
          rec.ioerr_cnt = devicestats[*s].ioerr_cnt / dt;
          xioview_.iostats[ (*s).getName() ] = rec;
          if ( (*s).isWholeDisk() ) {
            xioview_.iosorted.push_back((*s).getName());
          }
        }
        vmem::getSwapInfo( swaps_ );
        xioview_.fsbytes1 = xioview_.fsbytes2;
        std::map<block::MajorMinor,block::MountInfo> mounts;
        enumMounts( mounts, devicefilecache_ );


        for ( std::map<block::MajorMinor,block::MountInfo>::const_iterator m = mounts.begin(); m != mounts.end(); ++m ) {
          xioview_.fsbytes2[m->second.mountpoint] = block::getMountUsedBytes( m->second.mountpoint );
        }
        // map MajorMinor to mountpoint
        for ( block::MajorMinorVector::const_iterator s = sorted.begin(); s != sorted.end(); s++ ) {
          XIORec rec;
          std::map<block::MajorMinor,block::MountInfo>::const_iterator m = mounts.find(*s);
          if ( m != mounts.end() ) {
            xioview_.mountstats[m->second.mountpoint].util  = xioview_.iostats[(*s).getName()].util;
            xioview_.mountstats[m->second.mountpoint].rs    = xioview_.iostats[(*s).getName()].rs;
            xioview_.mountstats[m->second.mountpoint].ws    = xioview_.iostats[(*s).getName()].ws;
            xioview_.mountstats[m->second.mountpoint].rbs   = xioview_.iostats[(*s).getName()].rbs;
            xioview_.mountstats[m->second.mountpoint].wbs   = xioview_.iostats[(*s).getName()].wbs;
            xioview_.mountstats[m->second.mountpoint].artm  = xioview_.iostats[(*s).getName()].artm;
            xioview_.mountstats[m->second.mountpoint].awtm  = xioview_.iostats[(*s).getName()].awtm;
            xioview_.mountstats[m->second.mountpoint].svctm = xioview_.iostats[(*s).getName()].svctm;
            xioview_.mountstats[m->second.mountpoint].svctm = xioview_.iostats[(*s).getName()].svctm;
            xioview_.mountstats[m->second.mountpoint].device = m->second.mountpoint;
            xioview_.mountstats[m->second.mountpoint].growths = ((double)xioview_.fsbytes2[m->second.mountpoint] - (double)xioview_.fsbytes1[m->second.mountpoint])/dt;
            xioview_.mountsorted.push_back(m->second.mountpoint);
          } else {
            //include swap filesystems
            for ( std::list<vmem::SwapInfo>::const_iterator i = swaps_.begin(); i != swaps_.end(); i++ ) {
              if ( block::getFileMajorMinor( (*i).devicefile ) == *s ) {
                block::MountInfo mi;
                mi.attrs = "";
                mi.device = "";
                mi.fstype = "swap";
                mi.mountpoint = "swap:" + (*i).devicefile;
                xioview_.mountstats[mi.mountpoint].util  = xioview_.iostats[(*s).getName()].util;
                xioview_.mountstats[mi.mountpoint].rs    = xioview_.iostats[(*s).getName()].rs;
                xioview_.mountstats[mi.mountpoint].ws    = xioview_.iostats[(*s).getName()].ws;
                xioview_.mountstats[mi.mountpoint].rbs   = xioview_.iostats[(*s).getName()].rbs;
                xioview_.mountstats[mi.mountpoint].wbs   = xioview_.iostats[(*s).getName()].wbs;
                xioview_.mountstats[mi.mountpoint].artm  = xioview_.iostats[(*s).getName()].artm;
                xioview_.mountstats[mi.mountpoint].awtm  = xioview_.iostats[(*s).getName()].awtm;
                xioview_.mountstats[mi.mountpoint].svctm = xioview_.iostats[(*s).getName()].svctm;
                xioview_.mountstats[mi.mountpoint].svctm = xioview_.iostats[(*s).getName()].svctm;
                xioview_.mountstats[mi.mountpoint].device = mi.mountpoint;
                xioview_.mountsorted.push_back(mi.mountpoint);
                break;
              }
            }
          }
        }
        xioview_.sample_count++;
      }

      void RealtimeSampler::sampleXSysView( int cpubarheight ) {
        cpustat1_ = cpustat2_;
        sched1_ = sched2_;
        vmstat1_ = vmstat2_;
        xsysview_.t1 = xsysview_.t2;
        gettimeofday( &xsysview_.t2, 0 );
        cpu::getCPUTopology( xsysview_.cpu_topo );
        cpu::getCPUStats( cpustat2_ );
        sched2_ = cpu::getSchedInfo();
        vmem::getVMStat( vmstat2_ );
        vmem::getSwapInfo( swaps_ );
        mounted_bytes_1_ = mounted_bytes_2_;
        mounted_bytes_2_ = leanux::block::getMountUsedBytes();
        xsysview_.cpu_delta.clear();
        leanux::cpu::deltaStats( cpustat1_, cpustat2_, xsysview_.cpu_delta );
        //normalize the cpu_delta to sample interval resulting in CPU seconds/clock second.
        double dt = util::deltaTime( xsysview_.t1, xsysview_.t2 );
        for ( cpu::CPUStatsMap::iterator c = xsysview_.cpu_delta.begin(); c != xsysview_.cpu_delta.end(); ++c ) {
          cpu::normalizeCPUstats( c->second, dt, 1 );
        }
        cpu::getCPUTotal( xsysview_.cpu_delta, xsysview_.cpu_total );

        if ( xsysview_.sample_count > 0 ) {

          xsysview_.cpu_seconds = xsysview_.cpu_total.steal +
                                  xsysview_.cpu_total.softirq +
                                  xsysview_.cpu_total.irq +
                                  xsysview_.cpu_total.iowait +
                                  xsysview_.cpu_total.system +
                                  xsysview_.cpu_total.nice +
                                  xsysview_.cpu_total.user;

          xsysview_.time_slice = xsysview_.cpu_seconds / (double)(sched2_.ctxt - sched1_.ctxt );
          xsysview_.ctxsws = (double)(sched2_.ctxt - sched1_.ctxt )/ dt;
          xsysview_.forks = (double)(sched2_.processes - sched1_.processes )/ dt;

          cpu::getLoadAvg( xsysview_.loadavg );

          xsysview_.runq = sched2_.running-1;
          xsysview_.blockq = sched2_.blocked;

          xsysview_.mem_total = vmstat2_.mem_total;
          xsysview_.mem_unused = vmstat2_.nr_free_pages * xsysview_.pagesize_;
          xsysview_.mem_commitas = vmstat2_.committed_as;
          xsysview_.mem_anon = vmstat2_.nr_anon_pages * xsysview_.pagesize_;
          xsysview_.mem_file = vmstat2_.nr_file_pages * xsysview_.pagesize_;
          xsysview_.mem_shmem = vmstat2_.nr_shmem * xsysview_.pagesize_;
          xsysview_.mem_slab = (vmstat2_.nr_slab_reclaimable + vmstat2_.nr_slab_unreclaimable ) * xsysview_.pagesize_;
          xsysview_.mem_pagetbls = vmstat2_.nr_page_table_pages * xsysview_.pagesize_;
          xsysview_.mem_dirty = vmstat2_.nr_dirty * xsysview_.pagesize_;
          xsysview_.mem_pageins = (double)(vmstat2_.pgpgin -  vmstat1_.pgpgin ) / dt;
          xsysview_.mem_pageouts = (double)(vmstat2_.pgpgout -  vmstat1_.pgpgout ) / dt;
          xsysview_.mem_swapins = (double)(vmstat2_.pswpin -  vmstat1_.pswpin ) / dt;
          xsysview_.mem_swapouts = (double)(vmstat2_.pswpout -  vmstat1_.pswpout ) / dt;
          xsysview_.mem_hptotal = vmstat2_.hugepages_total * vmstat2_.hugepagesize;
          xsysview_.mem_hprsvd = vmstat2_.hugepages_reserved * vmstat2_.hugepagesize;
          xsysview_.mem_hpfree = vmstat2_.hugepages_free * vmstat2_.hugepagesize;
          xsysview_.mem_thpanon = vmstat2_.nr_anon_transparent_hugepages * vmstat2_.hugepagesize;
          xsysview_.mem_mlock = vmstat2_.nr_mlock * xsysview_.pagesize_;
          xsysview_.mem_mapped = vmstat2_.nr_mapped * xsysview_.pagesize_;
          xsysview_.mem_swpused = 0;
          xsysview_.mem_swpsize = 0;
          for ( std::list<vmem::SwapInfo>::const_iterator i = swaps_.begin(); i != swaps_.end(); i++ ) {
            xsysview_.mem_swpused += (*i).used;
            xsysview_.mem_swpsize += (*i).size;
          }

          xsysview_.mem_minflts = (double)(vmstat2_.pgfault -  vmstat1_.pgfault ) / dt;
          xsysview_.mem_majflts = (double)(vmstat2_.pgmajfault -  vmstat1_.pgmajfault ) / dt;
          xsysview_.mem_allocs = (double)(vmstat2_.pgalloc_dma -  vmstat1_.pgalloc_dma +
                                          vmstat2_.pgalloc_dma32 -  vmstat1_.pgalloc_dma32 +
                                          vmstat2_.pgalloc_normal -  vmstat1_.pgalloc_normal +
                                          vmstat2_.pgalloc_movable -  vmstat1_.pgalloc_movable ) / dt;
          xsysview_.mem_frees = (double)(vmstat2_.pgfree -  vmstat1_.pgfree ) / dt;

          system::getOpenFiles( &xsysview_.res_filesopen, &xsysview_.res_filesmax );
          system::getOpenInodes( &xsysview_.res_inodesopen, &xsysview_.res_inodesfree );
          system::getNumProcesses( &xsysview_.res_processes );
          xsysview_.res_users = system::getNumLoginUsers();
          xsysview_.res_logins = system::getNumLogins();
          xsysview_.fs_growths =  ((double)mounted_bytes_2_ - (double)mounted_bytes_1_) / dt;

          std::string bar = makeCPUBar( xsysview_.cpu_total, xsysview_.cpu_topo.logical, cpubarheight - 2 );
          xsysview_.cpurtpast.push_back( bar );
          while ( xsysview_.cpurtpast.size() > MAX_CPU_TRAIL ) xsysview_.cpurtpast.pop_front();
        } else {
          xsysview_.cpu_seconds = xsysview_.cpu_total.steal +
                                  xsysview_.cpu_total.softirq +
                                  xsysview_.cpu_total.irq +
                                  xsysview_.cpu_total.iowait +
                                  xsysview_.cpu_total.system +
                                  xsysview_.cpu_total.nice +
                                  xsysview_.cpu_total.user;

          xsysview_.time_slice = 0;
          xsysview_.ctxsws = 0;
          xsysview_.forks = 0;
          xsysview_.runq = 0;
          xsysview_.blockq = 0;
          xsysview_.mem_total = vmstat2_.mem_total;
          xsysview_.mem_unused = vmstat2_.nr_free_pages * xsysview_.pagesize_;
          xsysview_.mem_commitas = vmstat2_.committed_as;
          xsysview_.mem_anon = vmstat2_.nr_anon_pages * xsysview_.pagesize_;
          xsysview_.mem_file = vmstat2_.nr_file_pages * xsysview_.pagesize_;
          xsysview_.mem_shmem = vmstat2_.nr_shmem * xsysview_.pagesize_;
          xsysview_.mem_slab = (vmstat2_.nr_slab_reclaimable + vmstat2_.nr_slab_unreclaimable ) * xsysview_.pagesize_;
          xsysview_.mem_pagetbls = vmstat2_.nr_page_table_pages * xsysview_.pagesize_;
          xsysview_.mem_dirty = vmstat2_.nr_dirty * xsysview_.pagesize_;
          xsysview_.mem_pageins = 0;
          xsysview_.mem_pageouts = 0;
          xsysview_.mem_swapins = 0;
          xsysview_.mem_swapouts = 0;
          xsysview_.mem_hptotal = vmstat2_.hugepages_total * vmstat2_.hugepagesize;
          xsysview_.mem_hprsvd = vmstat2_.hugepages_reserved * vmstat2_.hugepagesize;
          xsysview_.mem_hpfree = vmstat2_.hugepages_free * vmstat2_.hugepagesize;
          xsysview_.mem_thpanon = vmstat2_.nr_anon_transparent_hugepages * vmstat2_.hugepagesize;
          xsysview_.mem_mlock = vmstat2_.nr_mlock * xsysview_.pagesize_;
          xsysview_.mem_mapped = vmstat2_.nr_mapped * xsysview_.pagesize_;
          xsysview_.mem_swpused = 0;
          xsysview_.mem_swpsize = 0;
          for ( std::list<vmem::SwapInfo>::const_iterator i = swaps_.begin(); i != swaps_.end(); i++ ) {
            xsysview_.mem_swpused += (*i).used;
            xsysview_.mem_swpsize += (*i).size;
          }

          xsysview_.mem_minflts = 0;
          xsysview_.mem_majflts = 0;
          xsysview_.mem_allocs = 0;
          xsysview_.mem_frees = 0;
          system::getOpenFiles( &xsysview_.res_filesopen, &xsysview_.res_filesmax );
          system::getOpenInodes( &xsysview_.res_inodesopen, &xsysview_.res_inodesfree );
          system::getNumProcesses( &xsysview_.res_processes );
          xsysview_.res_users = system::getNumLoginUsers();
          xsysview_.res_logins = system::getNumLogins();
          xsysview_.fs_growths =  0;
        }
        xsysview_.sample_count++;
      }

    }; //namespace lmon

  }; //namespace tools

}; //namespace leanux
