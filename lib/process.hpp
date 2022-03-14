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
 * leanux::process c++ header file.
 */
#ifndef LEANUX_PROCESS_HPP
#define LEANUX_PROCESS_HPP

#include <string>
#include <ostream>
#include <map>
#include <list>
#include <vector>

#include <unistd.h>
#include <sys/types.h>


namespace leanux {

  /**
   * \example example_process.cpp
   * leanux::process example 1.
   * \example example_process2.cpp
   * leanux::process example 2.
   */

  /**
   * Process configuration and performance API.
   */
  namespace process {

    /**
     * Process details and statistics as in /proc/[pid]/stat.
     */
    struct ProcPidStat {
      /** the process id. */
      pid_t pid;
      /** the executable name. */
      std::string comm;
      /** the current process state. */
      char state;
      /** the parent process id. */
      pid_t ppid;
      /** the process group id of the process. */
      pid_t pgrp;
      /** the session id of the process. */
      pid_t session;
      /** the controlling terminal of the process. */
      int tty_nr;
      /** the id of the foreground process group of the controlling terminal of the process. */
      pid_t tpgid;
      /** the number of minor faults the process has made which have not required loading a memory page from disk. */
      unsigned long minflt;
      /** the number of minor faults that the process's waited-for children have made. */
      unsigned long cminflt;
      /** the number of major faults. */
      unsigned long majflt;
      /** the number of major faults that the process's waited-for children have made. */
      unsigned long cmajflt;
      /** amount of time that this process has been scheduled in user mode, measured in seconds. */
      double utime;
      /** amount of time that this process has been scheduled in kernel mode, measured in seconds.*/
      double stime;
      /** amount of time that this process's waited-for children have been scheduled in user mode, measured in seconds. */
      double cutime;
      /** amount of time that this process's waited-for children have been scheduled in kernel mode, measured in seconds. */
      double cstime;
      /** the negated scheduling priority, minus one; that is, a number in the range -2 to -100. */
      unsigned long priority;
      /** the nice value */
      long nice;
      /** number of threads in this process. */
      unsigned long num_threads;
      /** the time the process started after system boot. */
      unsigned long long starttime;
      /** virtual memory size in bytes. */
      unsigned long vsize;
      /** resident set size: number of pages the process has in real memory. multiply by leanux::system::getPageSize to get the number of bytes. */
      unsigned long rss;
      /** current soft limit in bytes on the rss of the process; see the description of RLIMIT_RSS in getrlimit(2) */
      unsigned long rsslim;
      /** processsor the process is on */
      unsigned int processor;
      /** Aggregated block I/O delays, measured in seconds */
      double delayacct_blkio_ticks;
      /** kernel wait channel (or empty string) */
      std::string wchan;
    };

    /**
     * Get the ProcPidStat for the pid.
     * @param pid the process id to get the stats for.
     * @param stat the ProcPidStat struct in which to set the results.
     * @return false if the pid is not found.
     */
    bool getProcPidStat( pid_t pid, ProcPidStat &stat );

    /**
     * get the current kernel channel waited on by the process. the value "0" means the process is not waiting.
     * @param pid the process to return the wchan for.
     * @return the wchan for the pid, or an empty std::string if the pid does not exist.
     */
    std::string getWChan( pid_t pid );

    /**
     * Process IO details from /proc/pid/io. Note that the IO counters encompass all IO, not only to disk, but also pipes and sockets.
     * Root access is required to read /prod/pid/io (one may detect the size of a password of another process by examining this file),
     */
    struct ProcPidIO {
      /**
       * characters read - The number of bytes which this task has caused to be read from storage. This is simply the sum of bytes which this process passed to
       * read(2) and similar system calls. It includes things such as terminal I/O and is unaffected by whether or not actual physical disk I/O
       * was required (the read might have been satisfied from pagecache).
       */
      unsigned long rchar;
      /**
       * characters written - The number of bytes which this task has caused, or shall cause to be written to disk.  Similar caveats apply here as with rchar.
       */
      unsigned long wchar;
      /**
       * read syscalls - Attempt to count the number of read I/O operations—that is, system calls such as read(2) and pread(2).
       */
      unsigned long syscr;
      /**
       * write syscalls - Attempt to count the number of write I/O operations—that is, system calls such as write(2) and pwrite(2).
       */
      unsigned long syscw;
      /**
       * bytes read - Attempt to count the number of bytes which this process really did cause to be fetched from the  storage  layer.   This  is  accurate  for
       * block-backed filesystems.
       */
      unsigned long read_bytes;
      /**
       * bytes written - Attempt to count the number of bytes which this process caused to be sent to the storage layer.
       */
      unsigned long write_bytes;
      /**
       * The  big  inaccuracy  here is truncate.  If a process writes 1MB to a file and then deletes the file, it will in fact perform no writeout.
       * But it will have been accounted as having caused 1MB of write.  In other words: this field represents  the  number  of  bytes  which  this
       * process caused to not happen, by truncating pagecache.  A task can cause "negative" I/O too.  If this task truncates some dirty pagecache,
       * some I/O which another task has been accounted for (in its write_bytes) will not be happening.
       */
      unsigned long cancelled_write_bytes;
    };

    /**
     * Get process IO details, root access required.
     * @param pid the process id.
     * @param io the ProcPidIO struct to fill.
     * @return false if the operation failed (no permission, pid does not exist).
     * @root root access is required even for processes the caller owns (as the information in this file
     * might be used to deduce the size of an entered password).
     */
    bool getProcPidIO( pid_t pid, ProcPidIO &io );

    /**
     * Get the pid's command line.
     */
    std::string getProcCmdLine( pid_t pid );

    /**
     * Convienence typedef  for a std::map keyed by pid_t to ProcPidStat.
     */
    typedef std::map<pid_t, ProcPidStat> ProcPidStatMap;

    /**
     * Get a snapshot of all pids (seen as /proc/PID) into a std::map keyed by pid_t.
     * Note that the snapshot is not 'consistent', the ProcPidStat info
     * is filled sequentially over a lttle amount of time - an amount that depends on
     * the number of processes. Also note that this call scales O(n), n being the
     * number of processes on the system.
     * @param stats the ProcPidStatMap to fill.
     */
    void getAllProcPidStat( ProcPidStatMap &stats );

    /**
     * Get all direct children of a parent pid from a ProcPidStatMap snapshot.
     */
    void getAllDirectChildren( pid_t parent, const ProcPidStatMap &snap, std::list<pid_t> &children );

    /**
     * A std::map of open files (file names) keyed by pid_t.
     */
    typedef std::map<unsigned long,std::string> OpenFileMap;

    /**
     * Get a process's open files - if the caller has the privilege to see them.
     * @param pid the process id for which to get the open files.
     * @param files the OpenFileMap of the pid's open files.
     * @root only root can see the open files of any process,
     * other users can only see open files of their own processes.
     */
    void getOpenFiles( pid_t pid, OpenFileMap &files );


    /** Delta of a pid's stats. */
    struct ProcPidStatDelta {
      pid_t pid;
      pid_t pgrp;
      char state;
      /** user time delta */
      double utime;
      double stime;
      unsigned long minflt;
      unsigned long majflt;
      unsigned long rss;
      unsigned long vsize;
      double delayacct_blkio_ticks;
      std::string comm;
      std::string wchan;
    };

    /**
     * A std::vector of ProcPidStatDelta elements.
     */
    typedef std::vector<ProcPidStatDelta> ProcPidStatDeltaVector;

    /**
     * Get a delta of two ProcPidStatMap std::maps into the delta std::map.
     * @param snap1 first snapshot.
     * @param snap2 second snapshot.
     * @param delta resulting delta.
     */
    void deltaProcPidStats( const ProcPidStatMap &snap1, const ProcPidStatMap &snap2, ProcPidStatDeltaVector &delta );

    /**
     * Functor class for parametrized sorting with std::sort.
     * Specify one of the StatsSorter::SortBy enums in the constructor
     * to create a functor with the desired sorting behavior.
     * @code
     * StatsSorter mysorter( StatsSorter::top );
     * sort( snap1, snap2, mysorter );
     * @endcode
     */
    class StatsSorter {
      public:
        /**
         * Sort criteria.
         */
        enum SortBy {
          /** sort by user mode cpu */
          utime,
          /** sort by system mode cpu*/
          stime,
          /** sort by system+user mode cpu*/
          cputime,
          /** sort by minor faults */
          minflt,
          /** sort by major faults */
          majflt,
          /** sort by rss difference */
          rss,
          /** sort by vsize difference */
          vsize,
          /** sort by rss absolute */
          rss_abs,
          /** sort by vsize absolute */
          vsize_abs,
          /** sort by delayacct_blkio_ticks */
          delayacct_blkio_ticks,
          /** sort on utime+stime+delayacct_blkio_ticks, majflt, minflt. */
          top
        };

        /**
         * Constructor.
         */
        StatsSorter( SortBy sortby ) { sortby_ = sortby; };

        /**
         * Sort functor.
         */
        int operator()( ProcPidStatDelta d1, ProcPidStatDelta d2 );

      private:
        /**
         * The sort criterium.
         */
        SortBy sortby_;
    };

    /**
     * return all pids with specified comm (executable image) into stats
     * @param comm the command (executable image) to filter on.
     * @param stats the ProcPidStatMap to fill.
     * @return true when at least one pid is returned (aka stats.size()>0).
     */
    bool findProcByComm( const std::string& comm, ProcPidStatMap &stats );

    /**
     * get the effective uid of a running process.
     * @param pid the pid to get the uid for.
     * @param uid is set to the uid of the pid if returning true.
     * @return true if pid was found and uid detrmined.
     */
    bool getProcUid( pid_t pid, uid_t &uid );

    /**
     * return all pids with specified uid.
     * @param uid the uid to filter on.
     * @param stats the ProcPidStatMap to fill.
     * @return true when at least one pid is returned (aka stats.size()>0).
     */
    bool findProcByUid( uid_t uid, ProcPidStatMap &stats );

  }

}

#endif
