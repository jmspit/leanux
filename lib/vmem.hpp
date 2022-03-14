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
 * @file vmem.hpp
 * leanux::vmem c++ header file.
 */
#ifndef LEANUX_VMEM_HPP
#define LEANUX_VMEM_HPP

#include <string>
#include <list>

namespace leanux {

  /** virtual memory API */
  namespace vmem {

    /**
     * information from /proc/vmstat.
     */
    struct VMStat {
      /** number of pages not used for anything. */
      unsigned long nr_free_pages;

      /** number of inactive anonymous pages. */
      unsigned long nr_inactive_anon;

      /** number of active anonymous pages. */
      unsigned long nr_active_anon;

      /** number of inactive file pages. */
      unsigned long nr_inactive_file;

      /** number of active file pages. */
      unsigned long nr_active_file;

      /** number of unevictable pages */
      unsigned long nr_unevictable;

      /** number of mlocked pages */
      unsigned long nr_mlock;

      /** number of anonymous pages */
      unsigned long nr_anon_pages;

      /** number of mapped pages */
      unsigned long nr_mapped;

      /** number of file pages */
      unsigned long nr_file_pages;

      /** number of dirty file pages */
      unsigned long nr_dirty;

      /** number of dirty file pages written out now */
      unsigned long nr_writeback;

      /** number of reclaimable pages from the slab */
      unsigned long nr_slab_reclaimable;

      /** number of unreclaimable pages from the slab */
      unsigned long nr_slab_unreclaimable;

      /** number of paging table pages */
      unsigned long nr_page_table_pages;

      /** number of kernel pages */
      unsigned long nr_kernel_stack;

      /** number of shared memory pages */
      unsigned long nr_shmem;

      /** number of anonymous transparent pages */
      unsigned long nr_anon_transparent_hugepages;

      /** number of pages paged in */
      unsigned long pgpgin;

      /** number of pages paged out */
      unsigned long pgpgout;

      /** number of pages swapped in */
      unsigned long pswpin;

      /** number of pages swapped out */
      unsigned long pswpout;

      /** number of minor page faults */
      unsigned long pgfault;

      /** number of major page faults */
      unsigned long pgmajfault;

      /** number of dma pages */
      unsigned long pgalloc_dma;

      /** number of dma 32 pages */
      unsigned long pgalloc_dma32;

      /** number of allocated normal pages */
      unsigned long pgalloc_normal;

      /** number of allocated moveable pages */
      unsigned long pgalloc_movable;

      /** number of pages freed */
      unsigned long pgfree;

      /** total memory bytes */
      unsigned long mem_total;

      /** committed address space bytes */
      unsigned long committed_as;

      /** total number of (fixed) hugepages */
      unsigned long hugepages_total;

      /** total number of (fixed) hugepages unused */
      unsigned long hugepages_free;

      /** total number of (fixed) hugepages unused but reserved */
      unsigned long hugepages_reserved;

      /** hugepagesize */
      unsigned long hugepagesize;

    };

    /**
     * get virtual memory statistics.
     * @param stat the VMStat structure to fill.
     */
    void getVMStat( VMStat &stat );

    /**
     * Information on a swap container.
     */
    struct SwapInfo {
      /** the device file used as swap. */
      std::string devicefile;

      /** the device type used as swap. */
      std::string devicetype;

      /** the size of the device in bytes. */
      unsigned long size;

      /** the number of used bytes. */
      unsigned long used;

      /** the swap priority. */
      int priority;
    };

    /**
     * Get a std::list of SwapInfo swap spaces.
     * @param swaps the std::list to fill with the systems swaps.
     */
    void getSwapInfo( std::list<SwapInfo> &swaps );

    /**
     * get /proc/sys/vm/dirty_bytes
     * @return /proc/sys/vm/dirty_bytes
     */
    ssize_t getDirtyBytes();

    /**
     * get /proc/sys/vm/dirty_ratio
     * @return /proc/sys/vm/dirty_ratio
     */
    ssize_t getDirtyRatio();

    /**
     * get /proc/sys/vm/dirty_background_bytes
     * @return /proc/sys/vm/dirty_background_bytes
     */
    ssize_t getDirtyBackgroundBytes();

    /**
     * get /proc/sys/vm/dirty_background_ratio
     * @return /proc/sys/vm/dirty_background_ratio
     */
    ssize_t getDirtyBackgroundRatio();

  }

}

#endif
