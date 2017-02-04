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
 * leanux::vmem c++ source file.
 */
#include "system.hpp"
#include "util.hpp"
#include "vmem.hpp"

#include <string>

#include <process.hpp>

#include <fstream>
#include <limits>
#include <iostream>

namespace leanux {

  namespace vmem {

    void getVMStat( VMStat &stat ) {
      {
        std::ifstream procvmstat( "/proc/vmstat" );
        stat.nr_free_pages = 0;
        stat.nr_inactive_anon = 0;
        stat.nr_inactive_file = 0;
        stat.nr_active_file = 0;
        stat.nr_unevictable = 0;
        stat.nr_mlock = 0;
        stat.nr_anon_pages = 0;
        stat.nr_mapped = 0;
        stat.nr_file_pages = 0;
        stat.nr_dirty = 0;
        stat.nr_writeback = 0;
        stat.nr_slab_reclaimable = 0;
        stat.nr_slab_unreclaimable = 0;
        stat.nr_page_table_pages = 0;
        stat.nr_kernel_stack = 0;
        stat.pgpgin = 0;
        stat.pgpgout = 0;
        stat.pswpin = 0;
        stat.pswpout = 0;
        stat.pgfault = 0;
        stat.pgmajfault = 0;
        stat.pgalloc_dma = 0;
        stat.pgalloc_dma32 = 0;
        stat.pgalloc_normal = 0;
        stat.pgalloc_movable = 0;
        stat.pgfree = 0;
        stat.committed_as = 0;
        stat.hugepages_total = 0;
        stat.hugepages_free = 0;
        stat.hugepages_reserved = 0;
        stat.hugepagesize = 0;
        while ( procvmstat.good() ) {
          std::string name;
          unsigned long value;
          procvmstat >> name;
          procvmstat >> value;
          if ( name == "nr_free_pages" ) stat.nr_free_pages = value;
          else if ( name == "nr_inactive_anon" ) stat.nr_inactive_anon = value;
          else if ( name == "nr_active_anon" ) stat.nr_active_anon = value;
          else if ( name == "nr_inactive_file" ) stat.nr_inactive_file = value;
          else if ( name == "nr_active_file" ) stat.nr_active_file = value;
          else if ( name == "nr_unevictable" ) stat.nr_unevictable = value;
          else if ( name == "nr_mlock" ) stat.nr_mlock = value;
          else if ( name == "nr_anon_pages" ) stat.nr_anon_pages = value;
          else if ( name == "nr_mapped" ) stat.nr_mapped = value;
          else if ( name == "nr_file_pages" ) stat.nr_file_pages = value;
          else if ( name == "nr_dirty" ) stat.nr_dirty = value;
          else if ( name == "nr_writeback" ) stat.nr_writeback = value;
          else if ( name == "nr_slab_reclaimable" ) stat.nr_slab_reclaimable = value;
          else if ( name == "nr_slab_unreclaimable" ) stat.nr_slab_unreclaimable = value;
          else if ( name == "nr_page_table_pages" ) stat.nr_page_table_pages = value;
          else if ( name == "nr_kernel_stack" ) stat.nr_kernel_stack = value;
          else if ( name == "nr_anon_transparent_hugepages" ) stat.nr_anon_transparent_hugepages = value;
          else if ( name == "nr_shmem" ) stat.nr_shmem = value;
          else if ( name == "pgpgin" ) stat.pgpgin = value;
          else if ( name == "pgpgout" ) stat.pgpgout = value;
          else if ( name == "pswpin" ) stat.pswpin = value;
          else if ( name == "pswpout" ) stat.pswpout = value;
          else if ( name == "pgfault" ) stat.pgfault = value;
          else if ( name == "pgmajfault" ) stat.pgmajfault = value;
          else if ( name == "pgalloc_dma" ) stat.pgalloc_dma = value;
          else if ( name == "pgalloc_dma32" ) stat.pgalloc_dma32 = value;
          else if ( name == "pgalloc_normal" ) stat.pgalloc_normal = value;
          else if ( name == "pgalloc_movable" ) stat.pgalloc_movable = value;
          else if ( name == "pgfree" ) stat.pgfree = value;
        }
      }
      {
        std::ifstream procmeminfo( "/proc/meminfo" );
        stat.mem_total = 0;
        stat.committed_as = 0;
        while ( procmeminfo.good() ) {
          std::string name;
          unsigned long value;
          procmeminfo >> name;
          procmeminfo >> value;
          procmeminfo.ignore ( std::numeric_limits<std::streamsize>::max(), '\n' );
          if ( name == "MemTotal:" ) stat.mem_total = 1024 * value;
          else if ( name == "Committed_AS:" ) stat.committed_as = 1024 * value;
          else if ( name == "HugePages_Total:" ) stat.hugepages_total = value;
          else if ( name == "HugePages_Free:" ) stat.hugepages_free = value;
          else if ( name == "HugePages_Rsvd:" ) stat.hugepages_reserved = value;
          else if ( name == "Hugepagesize:" ) stat.hugepagesize = 1024 * value;
        }
      }

    }

    void getSwapInfo( std::list<SwapInfo> &swaps ) {
      swaps.clear();
      std::ifstream fs( "/proc/swaps" );
      std::string s;
      getline( fs, s );
      SwapInfo inf;
      while ( fs.good() ) {
        fs >> inf.devicefile;
        fs >> inf.devicetype;
        fs >> inf.size;
        fs >> inf.used;
        fs >> inf.priority;
        inf.size *= 1024;
        inf.used *= 1024;
        if ( fs.good() ) swaps.push_back( inf );
      }
    }

    ssize_t getDirtyBytes() {
      return util::fileReadUL( "/proc/sys/vm/dirty_bytes" );
    }

    ssize_t getDirtyRatio() {
      return util::fileReadUL( "/proc/sys/vm/dirty_ratio" );
    }

    ssize_t getDirtyBackgroundBytes() {
      return util::fileReadUL( "/proc/sys/vm/dirty_background_bytes" );
    }

    ssize_t getDirtyBackgroundRatio() {
      return util::fileReadUL( "/proc/sys/vm/dirty_background_ratio" );
    }

    double getDirtyPressure( ssize_t ram, ssize_t dirtied ) {
      ssize_t dirty_limit = getDirtyBytes();
      if ( dirty_limit == 0 ) dirty_limit = (double)ram / 100.0 * (double)getDirtyRatio();
      return (double)dirtied/(double)dirty_limit;
    }

  }

}
