LMON(1)                     General Commands Manual                    LMON(1)



NAME
       lmon - ncurses based linux performance viewer

SYNOPSIS
       lmon   [OPTION]

OPTIONS
       -b     browse    the    history    in    the   system   lard   database
              /var/lib/lard/lard.db (if present)

       -f database
              browse the history in the lard database

       -g     regenerate the configuration file with defaults

       -h     basic help.

       -v     show lmon version.

       without arguments, lmon operates as a realtime performance viewer.

       lmon likes terminals with 256 colors (such as TERM=xterm-256color)  and
       will adapt the screen layout to terminal resizing.

   Exit status:
       0      if OK,

       1      error.

       2      more serious error.

DESCRIPTION
       lmon  shows  kernel, CPU, memory, disk, filesystem, network and process
       activity drawn from the /proc and /sys pseudo filesystems  as  well  as
       udev.

       lmon  either  operates in realtime mode, taking snapshots from the live
       system, or in browse mode (-f, -b), displaying snapshots  stored  in  a
       lard database.

       lmon recognizes these key commands in realtime mode:

       q      quits lmon (as does ^C).

       +      increases sample interval by 1 second.

       -      decreases sample interval by 1 second.

       Note that changing the sample interval clears the CPU trail.

       lmon recognizes these key commands in browse mode:

       q      quits lmon (as does ^C).

       leftarrow or ,
              move back

       rightarrow or .
              move forward

       +      zoom in

       -      zoom out

       h      one hour back

       H      one hour forward

       d      one day back

       D      one day forward

       w      one week back

       W      one week forward

       HOME   home key - start of history

       END    end key - end of history

VIEWS
       Some  views  are  only  visible,  or  partly visible, when the terminal
       screen is large enough.

       Sizes expressed in bytes are given in base 1024, '1.0G'  is  a  screen-
       space saving abbreviation for 1.0GiB, one Gibibyte.

   CPU
       CPU  and schedulling details over the last sample interval, title shows
       number of sockets/cores/logical CPU's. The character  chart  shows  CPU
       mode usage per logical CPU. The per-logical CPU chart is not shown when
       the number of processors exceeds 32.

       CPU utilization is given as the ratio CPU time  /  wallclock  time,  so
       full utilization equals 1.

       user(u)
              user mode CPU (CPU seconds/clock second).

       nice(n)
              nice mode CPU (CPU seconds/clock second).

       system(s)
              system mode CPU (CPU seconds/clock second).

       iowait(w)
              iowait mode CPU (CPU seconds/clock second).

       irq(i) irq mode CPU (CPU seconds/clock second).

       softirq(o)
              softirq mode (CPU seconds/clock second).

       total  total of the above CPU modes.

       slice  total CPU time divided by number of context switches.

       ctxsw/s
              the number of context switches per second.

       forks/s
              the number of forks per second.

       loadavg
              5 and 10 minute load average (average number of processes on the
              run or block queue).

       runq   the number of processes on the run/block queue.

   Memory
       Memory details, title shows the amount of real memory available to  the
       kernel.

       Anonymous memory is memory not associated with file data.

       unused real memory not in use.

       commitas
              total virtual memory allocated.

       anon   anonymous virtual memory.

       file   real memory used as page cache.

       shmem  shared memory.

       slab   real memory used for the kernel slab.

       pagetbls
              real memory used for virtual memory paging tables.

       dirty  memory used by dirty file pages.

       pgin/s number of pages/s read into memory from persistent storage.

       pgout/s
              number of pages/s written from memory to persistent storage.

       swpin/s
              number of virtual memory pages/s read into memory from swap.

       swpout/s
              number of virtual memory pages/s written from memory to swap.

       hp total
              total number of hugepages.

       hp rsvd
              number of reserved hugepages.

       hp free
              number of free hugepages.

       thp anon
              anonymous memory used by transparent hugepages.

       mlock  (m)locked memory.

       mapped memory used by memory-mapped files.

       swp used
              swap used.

       swp size
              swap size.

       minflt/s
              minor page faults per second.

       majflt/s
              major page faults per second.

       alloc/s
              memory allocations per second.

       free/s memory freed per second.


   System resources
       Miscellaneous system resources.

       files open
              the number of allocated file handles on the system.

       files max
              the  maximum number of file handles that can be allocated on the
              system.

       inodes open
              the number of allocated inode handles.

       inodes free
              the number of free inode handles.

       processes
              the number of processes.

       users  the number of distinct users logged in.

       logins the number of user logins.

       fs growth/s
              total filesystem growth per second.

   CPU trail
       A time trail of CPU usage, aggregated over  all  CPUs.  CPU  modes  are
       coded by character and color. On the 'x-axis, a '+' marks 10 ticks.

   Disk IO
       Disk  IO statistics of top 'true' disks ordered by utilization. Derived
       block devices such as LVM or MetaDisk are excluded. Not all  disks  may
       be  shown  due  to lack of terminal space, but the totals aggregate all
       disks nevertheless. The header shows total attached storage size, aver-
       age  IOPS (r+w) and average bandwidth (r+w) over the last sample inter-
       val.

        device
              the device name.

        util  utilization as busy time / wallclock time.

        svct  average service time - not including queuing time.

        r/s   read operations per second.

        w/s   write operations per second.

        rb/s  bytes read per second.

        wb/s  bytes written per second.

        artm  average read time - includes queuing time.

        awtm  average write time - includes queuing time.

        rsz   average read size.

        wsz   average write size.

        qsz   number of IO's on the device queue at time of last sample.

   Filesystem IO
       Shows statistics on block devices with mounted filesystems  ordered  by
       utilization. Same columns as 'Disk IO'.

   Network
       Shows network device statistics.

        device
              the device name.

        rxb/s bytes received per second.

        txb/s bytes transmitted per second.

        rxpkt/s
              packets received per second.

        txpkt/s
              packets transmitted per second.

        rxsz  received packet average size.

        txsz  transmitted packet average size.

        rxerr/s
              average receive error rate.

        txerr/s
              average transmit error rate.

   TCP server
       Shows  TCP  (v4  and  v6) server connection statistics. Connections are
       counted and grouped by  (server  address,  server  port,  user  running
       server process) and sorted on number of connections.

        address
              the address of the server.

        port  the port of the server.

        user  the user running the server process.

        #conn the number of established tcp connections on this server.

   TCP client
       Shows  TCP (v4 and v6) client connection statistics. Client connections
       are counted and grouped by (server address, server port,  user  running
       client process) and sorted on number of connections.

        address
              the address of the server.

        port  the port of the server.

        user  the user running the server process.

        #conn the number of established tcp connections to the server.

   Process
       Shows  top  processes ordered by time. In order to be 'seen', a process
       must exist during at least two consecutive samples. Consequently,  pro-
       cesses  that are created and destroyed within a sample interval are in-
       visible and not aggregated.  The reported totals can therefore be lower
       than the system-wide reality shown in the CPU and Memory views.

       The  Process  view  stops sampling when the sample time exceeds 120ms -
       the sampling time scales linear with the number  of  processes  on  the
       system.  On an Intel core i7 this would disable the Process view output
       above roughly 10000 processes. If the number of processes drops by  10%
       since disable, a new sample is tried.

        pid   process id.

        pgrp  process group id.

        S     process  status.  In  realtime  mode,  this is the status of the
              process at the time of last sample. In browse mode, this is  set
              to 'D' if the pid ever had 'D' in the snapshot range.

        user  user owning the process.

        comm  command or 'process image'.

        time  total of utime, stime and iotime for the process.

        utime user  mode  CPU  time over the last interval divided by interval
              duration.

        stime system mode CPU time over the last interval divided by  interval
              duration.

        minflt
              the minor faults per second caused by the process.

        majflt
              the major faults per second caused by the process.

        rss   the process resident set size - real memory used by the process,
              some of which may be shared.

        vsz   the process virtual memory size - virtual  memory  used  by  the
              process, some of which may be shared.

        args  the process arguments.

        wchan for  blocked processes (status D), the kernel channel waited on.
              In browse mode, this is set to the most frequent whcan for  pids
              having state D in the snapshot range at least once.

CONFIG FILE
       Configuration file is '.leanux-lmon' located in the first match among
        $XDG_CONFIG_HOME, HOME and getpwuid->pw_dir.

BUGS
       Report     bugs,    documentation    errors    and    suggestions    at
       https://github.com/jmspit/leanux/issues.

AUTHOR
       Jan-Marten Spit <spitjm@xs4all.nl>

COPYRIGHT
       copyright  GPL  v3,  Jan-Marten   Spit   2015-2021   <spitjm@xs4all.nl>
       https://jmspit.github.io/leanux

       License   GPLv3+:  GNU  GPL  version  3  or  later  <http://gnu.org/li-
       censes/gpl.html>.
       This is free software: you are free  to  change  and  redistribute  it.
       There is NO WARRANTY, to the extent permitted by law.

SEE ALSO
       lard(1), lblk(1), lrep(1), lsys(1)
