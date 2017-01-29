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
 * lrep tool c++ source file.
 */

#include <list>
#include <string>
#include <iostream>
#include <iomanip>
#include <sys/time.h>
#include "block.hpp"
#include "cpu.hpp"
#include "leanux-config.hpp"
#include "lrep-config.hpp"
#include "net.hpp"
#include "persist.hpp"
#include "device.hpp"
#include "system.hpp"
#include "util.hpp"
#include "vmem.hpp"

#include <sys/resource.h>
#include <sys/types.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include <typeinfo>


using namespace std;

namespace leanux {
  namespace tools {
    namespace lrep {


      // Global constants
      const string body_font_size = "0.70em";
      const string body_background_color = "#F9F9F9";
      const string border_color = "#888888";
      const string th_background_color = "#DDDDDD";
      const string timeline_chart_height = "230px";
      const string timeline_chartarea = "chartArea: {left:70, height:'70%', right:10}";
      const string timeline_legend = "legend: {position: 'top', maxLines: 3 }";
      const string timeline_background_color = "backgroundColor: '#E0E0E0'";
      const string timeline_fontsize = "fontSize: 10";
      const long max_chart_pixels = 800;

      const double one_gib = 1024.0*1024.0*1024.0;

      /**
       * Snapshot range type.
       */
      struct SnapRange {
        long snap_min;
        long snap_max;
        long snap_count;
        long snaps_in_db;
        time_t time_min;
        time_t time_max;
        string local_time_min;
        string local_time_max;
        long timeline_bucket;
      };

      /**
       * Snapshot range.
       */
      SnapRange snaprange;

      /**
       * Snapshot totals type.
       */
      struct SnapTotals {
        double cpu_seconds;
        double user_cpu_seconds;
        double nice_cpu_seconds;
        double system_cpu_seconds;
        double iowait_cpu_seconds;
        double irq_cpu_seconds;
        double softirq_cpu_seconds;
        double steal_cpu_seconds;
        double io_seconds;
        double cmd_usercpu_seconds;
        double cmd_systemcpu_seconds;
        double cmd_iotime_seconds;
      };

      /**
       * Snapshot totals.
       */
      SnapTotals snaptotals;

      /**
       * Command line options type.
       */
      struct Options {
        string lardfile;    /**< the lard/sqlite database. */
        string htmlfile;    /**< the html output file. */
        unsigned int trail; /**< if nonzero, the last trail in minutes */
        time_t begin;       /**< if nonzero, start of interval */
        time_t end;         /**< if nonzero, end of interval */
        bool noresolv;      /**< if set, do not attempt to resolv IP adresses */
        bool version;       /**< if set, only display version */
        bool help;          /**< if set, only display help */
      };

      /**
       * Command line options.
       */
      Options options;

      /**
       * The output stream (html file).
       */
      ofstream doc;

      /**
       * The buffered HTML doc.
       */
      stringstream html;

      /**
       * The buffered charting javascript.
       */
      stringstream jschart;

      /**
       * map of ip's to resolved names to prevent repeated DNS requests
       */
      map<string,string> resolvemap;

      /**
       * deterministic transform of arbitray strings into valid javascript identifiers
       */
      string jsIdFromString( const string &s ) {
        std::stringstream ss;
        ss << "id";
        for ( std::string::const_iterator i = s.begin(); i != s.end(); i++ ) {
          if ( *i > 122 || *i < 97 ) {
            ss << (int)*i;
          } else ss << *i;
        }
        return ss.str();
      }

      /**
       * get from cache or DNS request on cache miss
       */
      string resolveCacheIP( const string &ip ) {
        string r = "";
        map<string,string>::const_iterator i = resolvemap.find( ip );
        if ( i == resolvemap.end() ) {
          r = net::resolveIP( ip );
          resolvemap[ip] = r;
        } else r = i->second;
        return r;
      }

      /**
       * disk menu items
       */
      list<pair <string,string> > menu_disks;

      /**
       * cmd menu items
       */
      list<pair <string,string> > menu_cmds;

      /**
       * compute totals into snaptotals.
       */
      void computeTotals( const persist::Database &db ) {
        persist::Query qry(db);
        qry.prepare( "select sum(user_mode),sum(system_mode),sum(iowait_mode),sum(nice_mode),sum(irq_mode),sum(softirq_mode),sum(steal_mode) "
                     " from v_cpuagstat where id>=:from and id <=:to" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        if ( qry.step() ) {
          snaptotals.cpu_seconds = qry.getDouble(0) + qry.getDouble(1) + qry.getDouble(2) + qry.getDouble(3) + qry.getDouble(4) + qry.getDouble(5) + qry.getDouble(6);
          snaptotals.user_cpu_seconds  = qry.getDouble(0);
          snaptotals.system_cpu_seconds  = qry.getDouble(1);
          snaptotals.iowait_cpu_seconds  = qry.getDouble(2);
          snaptotals.nice_cpu_seconds  = qry.getDouble(3);
          snaptotals.irq_cpu_seconds  = qry.getDouble(4);
          snaptotals.softirq_cpu_seconds  = qry.getDouble(5);
          snaptotals.steal_cpu_seconds  = qry.getDouble(6);
        }
        qry.reset();
        qry.prepare( "select sum(util) "
                     " from iostat where snapshot>=:from and snapshot <=:to" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        if ( qry.step() ) {
          snaptotals.io_seconds = qry.getDouble(0);
        }
        qry.reset();
        qry.prepare( "select sum(usercpu),sum(systemcpu),sum(iotime) "
                     " from procstat where snapshot>=:from and snapshot <=:to" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        if ( qry.step() ) {
          snaptotals.cmd_usercpu_seconds = qry.getDouble(0);
          snaptotals.cmd_systemcpu_seconds = qry.getDouble(1);
          snaptotals.cmd_iotime_seconds = qry.getDouble(2);
        }
      }

      void chartDiskRSZHisto( const persist::Database &db, const string &dom, long diskid ) {
        stringstream js;
        stringstream ss;
        ss << "disk" << diskid;
        persist::Query qry(db);
        qry.prepare( "select power(2,ceil(log2(rbs/rs/1024))),count(1) from iostat where disk=:diskid and snapshot>=:from and snapshot <=:to and rs>0 and rbs>0 group by ceil(log2(rbs/rs/1024)) order by 1" );
        qry.bind( 1, diskid );
        qry.bind( 2, snaprange.snap_min );
        qry.bind( 3, snaprange.snap_max );
        int iter = 0;
        while ( qry.step() ) {
          if ( iter == 0 ) {
            js << "var " << dom << "_data = google.visualization.arrayToDataTable([" << endl;
            js << "['read size', 'count' ]," << endl;

          } else {
            js << ",";
          }
          js << "[ '" << qry.getDouble(0) << "', " << qry.getInt(1) << " ]";
          iter++;
        }
        if ( iter ) {
          js << "]);" << endl;
          js << "var " << dom << "_options = {" << endl;
          js << "title: '" << ss.str() << " read size histogram'," << endl;
          js << timeline_background_color << ", " << endl;
          js << "legend: {position: 'none' }," << endl;
          js << "fontSize: 10," << endl;
          js << "hAxis: { title: 'read size (KiB)', baselineColor: 'transparent' }," << endl;
          js << "vAxis: { title: 'count' }" << endl;
          js << "};" << endl;
          js << "var " << dom << " = new google.visualization.ColumnChart(document.getElementById('" << dom << "'));" << endl;
          js << dom << ".draw(" << dom << "_data, " << dom << "_options);" << endl;

          jschart << js.str();
        }
      }

      void chartDiskWSZHisto( const persist::Database &db, const string &dom, long diskid ) {
        stringstream js;
        stringstream ss;
        ss << "disk" << diskid;
        persist::Query qry(db);
        qry.prepare( "select power(2,ceil(log2(wbs/ws/1024))),count(1) from iostat where disk=:diskid and snapshot>=:from and snapshot <=:to and ws>0 and wbs>0 group by ceil(log2(wbs/ws/1024)) order by 1" );
        qry.bind( 1, diskid );
        qry.bind( 2, snaprange.snap_min );
        qry.bind( 3, snaprange.snap_max );
        int iter = 0;
        while ( qry.step() ) {
          if ( iter == 0 ) {
            js << "var " << dom << "_data = google.visualization.arrayToDataTable([" << endl;
            js << "['write size', 'count' ]," << endl;

          } else {
            js << ",";
          }
          js << "[ '" << qry.getDouble(0) << "', " << qry.getDouble(1) << " ]";
          iter++;
        }
        if ( iter ) {
          js << "]);" << endl;
          js << "var " << dom << "_options = {" << endl;
          js << "title: '" << ss.str() << " write size histogram'," << endl;
          js << timeline_background_color << ", " << endl;
          js << "legend: {position: 'none' }," << endl;
          js << "fontSize: 10," << endl;
          js << "hAxis: { title: 'write size (KiB)', baselineColor: 'transparent' }," << endl;
          js << "vAxis: { title: 'count' }" << endl;
          js << "};" << endl;
          js << "var " << dom << " = new google.visualization.ColumnChart(document.getElementById('" << dom << "'));" << endl;
          js << dom << ".draw(" << dom << "_data, " << dom << "_options);" << endl;

          jschart << js.str();
        }
      }

      void chartDiskScvHisto( const persist::Database &db, const string &dom, long diskid, double bucket ) {
        stringstream js;
        stringstream ss;
        ss << "disk" << diskid;
        persist::Query qry(db);
        qry.prepare( "select round(round(svctm/:bucket))*:bucket,count(1) from iostat where disk=:disk and snapshot>=:from and snapshot <=:to group by round(round(svctm/:bucket))*:bucket having count(1)>1" );
        qry.bind( 1, bucket );
        qry.bind( 2, diskid );
        qry.bind( 3, snaprange.snap_min );
        qry.bind( 4, snaprange.snap_max );
        int iter = 0;
        while ( qry.step() ) {
          if ( iter == 0 ) {
            js << "var " << dom << "_data = google.visualization.arrayToDataTable([" << endl;
            js << "['svctm', 'count' ]," << endl;

          } else {
            js << ",";
          }
          js << "[ " << qry.getDouble(0)*1000.0 << ", " << qry.getInt(1) << " ]";
          iter++;
        }
        if ( iter ) {
          js << "]);" << endl;
          js << "var " << dom << "_options = {" << endl;
          js << "title: '" << ss.str() << " service time histogram'," << endl;
          js << timeline_background_color << ", " << endl;
          js << "legend: {position: 'none' }," << endl;
          js << "fontSize: 10," << endl;
          js << "hAxis: { title: 'service time (ms)', baselineColor: 'transparent' }," << endl;
          js << "vAxis: { title: 'count' }" << endl;
          js << "};" << endl;
          js << "var " << dom << " = new google.visualization.ColumnChart(document.getElementById('" << dom << "'));" << endl;
          js << dom << ".draw(" << dom << "_data, " << dom << "_options);" << endl;

          jschart << js.str();
        }
      }


      void htmlDiskHolderTreeLine( const persist::Database &db,
                                  const block::MajorMinor &mm, unsigned int level ) {
        std::list<std::string> holders;
        std::list<std::string> parts;
        std::string devname = block::MajorMinor::getNameByMajorMinor( mm );
        std::stringstream ss;
        for ( unsigned int i = 0; i < level; i++ ) ss << "&nbsp;";
        ss << mm.getName();
        html << "<tr><td>" << ss.str() << "</td>" << endl;
        ss.str("");
        ss << mm;
        html << "<td>" << ss.str() << "</td>" << endl;
        html << "<td>" << mm.getClassStr() << "</td>" << endl;
        html << "<td>" << util::ByteStr( mm.getSize(), 3 ) << "</td>" << endl;
        html << "<td>" << mm.getDescription() << "</td></tr>" << endl;


        mm.getPartitions( parts );
        mm.getHolders( holders );
        for ( std::list<std::string>::const_iterator d = holders.begin(); d != holders.end(); d++ ) {
          htmlDiskHolderTreeLine( db, block::MajorMinor::getMajorMinorByName( (*d) ), level+1 );
        }
        for ( std::list<std::string>::const_iterator d = parts.begin(); d != parts.end(); d++ ) {
          htmlDiskHolderTreeLine( db, block::MajorMinor::getMajorMinorByName( (*d) ), level+1 );
        }
      }

      void htmlDiskHolderTree( const persist::Database &db,
                               const block::MajorMinor &mm ) {
        html << "<table class=\"datatable\">" << endl;
        html << "<caption>" << mm.getName() << " holder tree</caption>" << endl;
        html << "<tr>" << endl;
        html << "<th>holder</th>" << endl;
        html << "<th>dev_t</th>" << endl;
        html << "<th>class</th>" << endl;
        html << "<th>size</th>" << endl;
        html << "<th>description</th></tr>" << endl;

        htmlDiskHolderTreeLine( db, mm, 0 );

        html << "</table>" << endl;


      }

      void htmlDiskDetail( const persist::Database &db,
                           long diskid ) {
        double svctm_min = 0;
        double svctm_max = 0;
        persist::Query qminmax(db);
        qminmax.prepare( "select min(svctm), max(svctm) from iostat where disk=:disk and snapshot>=:from and snapshot<=:to order by disk" );
        qminmax.bind( 1, diskid );
        qminmax.bind( 2, snaprange.snap_min );
        qminmax.bind( 3, snaprange.snap_max );
        if ( qminmax.step() ) {
          svctm_min = qminmax.getDouble(0);
          svctm_max = qminmax.getDouble(1);
        }
        double order = log10(svctm_max - svctm_min);
        if ( order < 0 ) order = floor( order - 1 ); else order = ceil( order + 1 );
        double bucket = pow(10,order);
        stringstream ss;
        ss << "disk" << diskid << "_svctm_histo";
        chartDiskScvHisto( db, ss.str(), diskid, bucket );
        html << "<br/><div class=\"chart\" id='" << ss.str() << "' style='width: " << 600 << "px; height: " << 200 << "px;'></div>" << endl;

        ss.str("");
        ss << "disk" << diskid << "_rsz_histo";
        chartDiskRSZHisto( db, ss.str(), diskid );
        html << "<br/><div class=\"chart\" id='" << ss.str() << "' style='width: " << 600 << "px; height: " << 200 << "px;'></div>" << endl;

        ss.str("");
        ss << "disk" << diskid << "_wsz_histo";
        chartDiskWSZHisto( db, ss.str(), diskid );
        html << "<br/><div class=\"chart\" id='" << ss.str() << "' style='width: " << 600 << "px; height: " << 200 << "px;'></div>" << endl;
      }

      void htmlDiskDetails( const persist::Database &db ) {
        // build a map of current diskid's to device names
        std::list<block::MajorMinor> disks;
        std::multimap<std::string,block::MajorMinor> diskmap;
        block::enumWholeDisks( disks );
        for ( std::list<block::MajorMinor>::const_iterator d = disks.begin(); d != disks.end(); d++ ) {
          diskmap.insert( std::pair<std::string,block::MajorMinor>( (*d).getDiskId(), *d ) );
        }
        html << "<a class=\"anchor\" id=\"diskdetails\"></a><h1>Disk details</h1>" << endl;
        persist::Query qry(db);
        qry.prepare( "select disk.id, disk.device, disk.wwn, disk.model, disk.syspath from disk "
                     "where disk.id in (select distinct iostat.disk from iostat where iostat.snapshot>=:from and iostat.snapshot <=:to) order by 1" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        while ( qry.step() ) {
          stringstream ss_link;
          stringstream ss_menu;
          ss_link << "diskdetails" << qry.getText(0);
          ss_menu << "disk" << qry.getText(0);
          html << "<a class=\"anchor\" id=\"" << ss_link.str() << "\"></a><h2>disk" << qry.getInt(0) << "</h2>" << endl;
          menu_disks.push_back( std::pair<string,string>( ss_link.str(), ss_menu.str() ) );
          std::map<std::string,block::MajorMinor>::const_iterator idisk = diskmap.find( qry.getText(2) );
          if ( idisk != diskmap.end() ) {
            html << "<table class=\"datatable\">" << endl;
            html << "<caption>disk" << qry.getText(0) << " configuration</caption>" << endl;
            html << "<tr><th class='right'>report disk id</th><td>" << qry.getText(2) << "</td></tr>" << endl;
            html << "<tr><th class='right'>report sysfs</th><td>" << qry.getText(4) << "</td></tr>" << endl;
            std::pair<std::multimap<std::string,block::MajorMinor>::const_iterator,std::multimap<std::string,block::MajorMinor>::const_iterator> range;
            range = diskmap.equal_range( qry.getText(2) );
            for ( std::multimap<std::string,block::MajorMinor>::const_iterator r = range.first; r != range.second; r++ ) {
              html << "<tr><th class='right'>current sysfs</th><td>" << r->second.getSysPath() << "</td></tr>" << endl;
            }
            html << "<tr><th class='right'>current device file</th><td>" << (idisk->second).getDeviceFile() << "</td></tr>" << endl;
            html << "<tr><th class='right'>wwn</th><td>" << idisk->second.getWWN() << "</td></tr>" << endl;
            html << "<tr><th class='right'>serial</th><td>" << idisk->second.getSerial() << "</td></tr>" << endl;
            html << "<tr><th class='right'>model</th><td>" << idisk->second.getModel() << "</td></tr>" << endl;
            html << "<tr><th class='right'>revision</th><td>" << idisk->second.getRevision() << "</td></tr>" << endl;
            html << "<tr><th class='right'>class</th><td>" << idisk->second.getClassStr() << "</td></tr>" << endl;
            html << "<tr><th class='right'>description</th><td>" << idisk->second.getRotationalStr() << "</td></tr>" << endl;
            html << "<tr><th class='right'>kernel module</th><td>" << idisk->second.getKernelModule() << "</td></tr>" << endl;
            html << "<tr><th class='right'>size</th><td>" << util::ByteStr( idisk->second.getSize(), 2 ) << "</td></tr>" << endl;
            html << "<tr><th class='right'>sector size</th><td>" << idisk->second.getSectorSize() << "</td></tr>" << endl;
            html << "<tr><th class='right'>max hw io size</th><td>" << util::ByteStr( idisk->second.getMaxHWIOSize(), 2 ) << "</td></tr>" << endl;
            html << "<tr><th class='right'>max io size</th><td>" << util::ByteStr( idisk->second.getMaxIOSize(), 2 ) << "</td></tr>" << endl;
            html << "<tr><th class='right'>min io size</th><td>" << util::ByteStr( idisk->second.getMinIOSize(), 2 ) << "</td></tr>" << endl;
            html << "<tr><th class='right'>read ahead</th><td>" << util::ByteStr( idisk->second.getReadAhead(), 2 ) << "</td></tr>" << endl;
            html << "<tr><th class='right'>IO scheduler</th><td>" << idisk->second.getIOScheduler() << "</td></tr>" << endl;
            html << "<tr><th class='right'>cache mode</th><td>" << idisk->second.getCacheMode() << "</td></tr>" << endl;
            html << "</table>" << endl;

            persist::Query stats(db);
            stats.prepare( "select min(util),min(rs),min(ws),min(rbs),min(wbs),min(artm),min(awtm),min(svctm),"
                           "max(util),max(rs),max(ws),max(rbs),max(wbs),max(artm),max(awtm),max(svctm),"
                           "sum(util)/cnt.num, sum(rs)/cnt.num,sum(ws)/cnt.num,sum(rbs)/cnt.num,sum(wbs)/cnt.num,avg(artm),avg(awtm),avg(svctm) "
                           "from iostat, (select count(1) num from snapshot where id>=:from and id <=:to) cnt where disk=:disk and iostat.snapshot>=:from and iostat.snapshot <=:to" );
            stats.bind( 1, snaprange.snap_min );
            stats.bind( 2, snaprange.snap_max );
            stats.bind( 3, qry.getInt(0) );
            html << "<table class=\"datatable\">" << endl;
            html << "<caption>disk" << qry.getText(0) << " performance</caption>" << endl;
            html << "<tr><th></th><th>util</th><th>read/s</th><th>write/s</th><th>read bytes/s</th><th>write bytes/s</th><th>read time (ms)</th><th>write time (ms)</th><th>svctm (ms)</th></tr>" << endl;
            if ( stats.step() ) {
              html << "<tr><th class='right'>min</th>" <<
                "<td>" << util::NumStr( stats.getDouble(0), 3 ) << "</td>" <<
                "<td>" << util::NumStr( stats.getDouble(1), 3 ) << "</td>" <<
                "<td>" << util::NumStr( stats.getDouble(2), 3 ) << "</td>" <<
                "<td>" << util::ByteStr( stats.getDouble(3), 3 ) << "</td>" <<
                "<td>" << util::ByteStr( stats.getDouble(4), 3 ) << "</td>" <<
                "<td>" << util::NumStr( stats.getDouble(5)*1000.0, 3 ) << "</td>" <<
                "<td>" << util::NumStr( stats.getDouble(6)*1000.0, 3 ) << "</td>" <<
                "<td>" << util::NumStr( stats.getDouble(7)*1000.0, 3 ) << "</td>" <<
                "</tr>" << endl;
              html << "<tr><th class='right'>max</th>" <<
                "<td>" << util::NumStr( stats.getDouble(8), 3 ) << "</td>" <<
                "<td>" << util::NumStr( stats.getDouble(9), 3 ) << "</td>" <<
                "<td>" << util::NumStr( stats.getDouble(10), 3 ) << "</td>" <<
                "<td>" << util::ByteStr( stats.getDouble(11), 3 ) << "</td>" <<
                "<td>" << util::ByteStr( stats.getDouble(12), 3 ) << "</td>" <<
                "<td>" << util::NumStr( stats.getDouble(13)*1000.0, 3 ) << "</td>" <<
                "<td>" << util::NumStr( stats.getDouble(14)*1000.0, 3 ) << "</td>" <<
                "<td>" << util::NumStr( stats.getDouble(15)*1000.0, 3 ) << "</td>" <<
                "</tr>" << endl;
              html << "<tr><th class='right'>avg</th>" <<
                "<td>" << util::NumStr( stats.getDouble(16), 3 ) << "</td>" <<
                "<td>" << util::NumStr( stats.getDouble(17), 3 ) << "</td>" <<
                "<td>" << util::NumStr( stats.getDouble(18), 3 ) << "</td>" <<
                "<td>" << util::ByteStr( stats.getDouble(19), 3 ) << "</td>" <<
                "<td>" << util::ByteStr( stats.getDouble(20), 3 ) << "</td>" <<
                "<td>" << util::NumStr( stats.getDouble(21)*1000.0, 3 ) << "</td>" <<
                "<td>" << util::NumStr( stats.getDouble(22)*1000.0, 3 ) << "</td>" <<
                "<td>" << util::NumStr( stats.getDouble(23)*1000.0, 3 ) << "</td>" <<
                "</tr>" << endl;
            }
            html << "</table>" << endl;

            for ( std::multimap<std::string,block::MajorMinor>::const_iterator r = range.first; r != range.second; r++ ) {
              html << "<table class=\"datatable\">" << endl;
              html << "<caption>disk" << qry.getText(0) << " sysfs</caption>" << endl;
              html << "<tr>" << endl;
              html << "<th>current sysfs device</th>" << endl;
              html << "<th>type</th>" << endl;
              html << "<th>driver</th>" << endl;
              html << "<th>class</th>" << endl;
              html << "<th>attributes</th>" << endl;
              html << "</tr>" << endl;
              std::list<leanux::sysdevice::SysDevice*> devices;
              size_t path_indent = 0;
              try {
                if ( leanux::sysdevice::treeDetect( r->second.getSysPath(), devices ) ) {
                  unsigned int depth = 0;
                  for ( std::list<leanux::sysdevice::SysDevice*>::const_iterator d = devices.begin(); d != devices.end(); d++, depth++ ) {
                    html << "<tr>" << endl;
                    std::string sdepth = "";
                    for ( size_t i = 0; i < depth; i++ ) {
                      sdepth += "&nbsp;";
                    }
                    html << "<td>" << sdepth + util::shortenString( (*d)->getPath().substr(path_indent), 160, '_' ) << "</td>" << endl;
                    html << "<td>" << leanux::sysdevice::SysDevice::getTypeStr( (*d)->getType() ) << "</td>" << endl;
                    html << "<td>" << (*d)->getDriver() << "</td>" << endl;
                    html << "<td>" << (*d)->getClass() << "</td>" << endl;
                    html << "<td>" << (*d)->getDescription() << "</td>" << endl;
                    path_indent += ((*d)->getPath().length() - path_indent) + 1;
                    html << "</tr>" << endl;
                  }
                }
              }
              catch ( leanux::Oops &oops ) {
                html << "<tr><td colspan=\"5\">" << oops.getMessage() << "</td></tr>" << endl;
              }
              html << "</table>" << endl;
              leanux::sysdevice::destroy( devices );
              htmlDiskHolderTree( db, r->second );
            }

            htmlDiskDetail( db, qry.getInt(0) );
          } else {
            html << "<p>The disk with id '" << qry.getText(2) << "' could not be found.</p>" << endl;
          }
        }
      }

      /**
       * generate system details html.
       */
      void htmlSystemDetails() {
        vmem::VMStat vmstat;
        vmem::getVMStat( vmstat );
        cpu::CPUInfo cpuinfo;
        cpu::getCPUInfo( cpuinfo );
        cpu::CPUTopology cputopo;
        cpu::getCPUTopology( cputopo );
        list<std::string> netdevices;
        net::enumDevices( netdevices );
        html << "<a class=\"anchor\" id=\"nodedetails\"></a><h1>Node details</h1>" << endl;
        html << "<table class=\"datatable\">" << endl;
        html << "<caption>Node details</caption>" << endl;
        html << "<tr><th class='right' title=\"the name of the node\">node name</th><td>" << system::getNodeName() << "</td></tr>" << endl;
        html << "<tr><th class='right' title=\"the type of the chassis\">chassis type</th><td>" << system::getChassisTypeString() << "</td></tr>" << endl;
        html << "<tr><th class='right' title=\"the system board name\">system board</th><td>" << system::getBoardVendor() << " " << system::getBoardName() << " " << system::getBoardVersion() << "</td></tr>" << endl;
        html << "<tr><th class='right'>Linux kernel</th><td>" << system::getKernelVersion() << "</td></tr>" << endl;
        html << "<tr><th class='right'>architecture</th><td>" << system::getArchitecture() << "</td></tr>" << endl;
        html << "<tr><th class='right'>distribution</th><td>" << system::getDistribution().release << "</td></tr>" << endl;
        html << "<tr><th class='right'>page size</th><td>" << system::getPageSize() << "</td></tr>" << endl;
        html << "<tr><th class='right'>CPU model</th><td>" << cpuinfo.model << "</td></tr>" << endl;
        html << "<tr><th class='right'>physical</th><td>" << cputopo.physical << "</td></tr>" << endl;
        html << "<tr><th class='right'>cores</th><td>" << cputopo.cores << "</td></tr>" << endl;
        html << "<tr><th class='right'>logical</th><td>" << cputopo.logical << "</td></tr>" << endl;
        html << "<tr><th class='right'>RAM</th><td>" << util::ByteStr( vmstat.mem_total, 3 ) << "</td></tr>" << endl;
        for ( list<std::string>::const_iterator n = netdevices.begin(); n != netdevices.end(); n++ ) {
          html << "<tr><th class='right'>" << *n << "</th><td>";
          list<std::string> ip4;
          net::getDeviceIP4Addresses( *n, ip4 );
          for ( list<std::string>::const_iterator i = ip4.begin(); i != ip4.end(); i++ ) {
            html << *i << " (" << (options.noresolv?*i:resolveCacheIP( *i )) << ")<br />" ;
          }
          html << "</td></tr>" << endl;
        }
        html << "<tr><th class='right'>storage</th><td>" << util::ByteStr( block::getAttachedStorageSize(), 3 ) << "</td></tr>" << endl;

        list<block::MajorMinor> disks;
        block::enumWholeDisks( disks );
        for ( list<block::MajorMinor>::const_iterator d = disks.begin(); d != disks.end(); d++ ) {
          if ( (*d).getSize() > 0 ) {
            html << "<tr><th class='right'>" << block::MajorMinor::getNameByMajorMinor( *d ) << "</th><td>" << endl;
            html << *d << "&nbsp;" << (*d).getDeviceFile() << "&nbsp;" << (*d).getClassStr() << "<br />" << endl;
            html << (*d).getModel() << "<br />" << endl;
            html << util::ByteStr( (*d).getSize(), 3 ) << "<br />" << endl;
            html << "</td></tr>" << endl;
          }
        }

        html << "</table>" << endl;
      }

      void htmlSnapDetails( const persist::Database &db ) {
        html << "<a class=\"anchor\" id=\"reportdetails\"></a><h1>Report details</h1>" << endl;

        html << "<table class=\"datatable\">" << endl;
        html << "<caption>Report details</caption>" << endl;
        html << "<tr><th class='right'>report generated at</th><td>" << util::localStrISOTime() << "</td></tr>" << endl;
        char* resolvedpath = realpath( options.lardfile.c_str(), NULL );
        html << "<tr><th class='right'>source database</th><td>" << resolvedpath << "</td></tr>" << endl;
        struct stat st;
        stat( resolvedpath, &st );
        html << "<tr><th class='right'>database size</th><td>" << util::ByteStr( st.st_blocks * 512, 2 ) << "</td></tr>" << endl;
        html << "<tr><th class='right'>snapshots in database</th><td>" << snaprange.snaps_in_db << "</td></tr>" << endl;
        html << "<tr><th class='right'>snapshot min id</th><td>" << snaprange.snap_min << "</td></tr>" << endl;
        html << "<tr><th class='right'>snapshot max id</th><td>" << snaprange.snap_max << "</td></tr>" << endl;
        html << "<tr><th class='right'>snapshot count</th><td>" << snaprange.snap_count << "</td></tr>" << endl;
        html << "<tr><th class='right'>snapshot min local time</th><td>" << snaprange.local_time_min << "</td></tr>" << endl;
        html << "<tr><th class='right'>snapshot max local time</th><td>" << snaprange.local_time_max << "</td></tr>" << endl;
        html << "<tr><th class='right'>report time range</th><td>" <<  util::TimeStrSec( snaprange.time_max-snaprange.time_min ) << "</td></tr>" << endl;
        html << "<tr><th class='right'>interval average</th><td>" <<  util::TimeStrSec( (double)(snaprange.time_max-snaprange.time_min)/(double)(snaprange.snap_count) ) << "</td></tr>" << endl;
        html << "<tr><th class='right'>timeline bucket</th><td>" <<  util::TimeStrSec(snaprange.timeline_bucket) << "</td></tr>" << endl;
        html << "<tr><th class='right'>average snapshot size</th><td>" <<  util::ByteStr( st.st_blocks * 512 / snaprange.snaps_in_db, 2 ) << "</td></tr>" << endl;
        html << "</table>" << endl;
        free( resolvedpath );
      }

      void htmlCSS( ofstream &doc ) {
        doc << "<style type=\"text/css\">" << endl;
        doc << "body { font-family: sans-serif; font-size: " << body_font_size << "; background-color: " << body_background_color << "; }" << endl;
        doc << "h1 { font-size: 150%; font-weight: bold; border-bottom: 1px solid " << border_color << "; }" << endl;
        doc << "h2 { font-size: 130%; font-weight: bold; }" << endl;
        doc << "table { border-collapse: collapse; }" << endl;
        doc << "caption { display: table-caption; text-align: left; font-weight:bold;}" << endl;
        doc << "table.datatable { margin-top: 8px; margin-bottom: 4px; }" << endl;
        doc << "table, th, td { font-size: 100%; border: 0px solid " << border_color << "; }" << endl;
        doc << "table.datatable th, table.datatable td { font-size: 100%; vertical-align:middle; border: 1px solid " << border_color << "; }" << endl;
        doc << "table.datatable td { font-family: \"Courier New\", Courier, monospace; font-size: 100%; vertical-align:middle; }" << endl;
        doc << "table.datatable th { text-align: left; vertical-align:middle; }" << endl;
        doc << "th { background-color: " << th_background_color << "; }" << endl;
        doc << "table.datatable th.right { text-align: right; }" << endl;
        doc << "div.chart { border: 1px solid #cccccc; margin-top: 4px; margin-bottom: 6px; }" << endl;

        doc << "ul.menu {list-style-type: none; margin: 0; padding: 0; position: fixed; top:0; width: 100%; height:32px; background-color: #333;}" << endl;
        doc << "li.menu, li.menu-dropdown {float: left; border-right:0px solid #bbb; font-size:10px; font-weight: bold;}" << endl;
        doc << "li.menu a, .menu-dropbtn  {display: inline-block; color: white; text-align:center; padding: 10px 8px; text-decoration: none; }" << endl;
        doc << "li.menu a:hover, .menu-dropdown:hover, .dropdown-content a:hover { background-color: #555; color: white; }" << endl;
        doc << "li.menu-dropdown {display: inline-block; }" << endl;
        doc << ".dropdown-content { display: none; position: absolute; background-color: #333; min-width: 160px; box-shadow: 0px 8px 16px 0px rgba(0,0,0,0.2);}" << endl;
        doc << ".dropdown-content a { color: white; padding: 10px 8px; text-decoration: none; display: block; text-align: left; }" << endl;
        doc << ".menu-dropdown:hover div.dropdown-content { display: block; }" << endl;

        doc << "a.anchor { display: block; position: relative; top: -32px; }" << endl;
        doc << "a.toggle { display: block; cursor: pointer; color: #333; font-size: 95%; font-weight: bold; border-bottom: 1px solid " << border_color << "; padding-bottom: 1px; margin-top:2px; }" << endl;
        doc << "div.toggle { margin-bottom: 8px;}" << endl;
        doc << "body {margin:0;}" << endl;


        doc << "p.foot {font-style: italic; margin-top:20px;}" << endl;

        doc << "table.heatmap td {border: 1px solid " << border_color << "; width: 1.1em; height: 1.1em; line-height: 1}" << endl;
        doc << "table.heatmap td.level0 {background-color: #003000}" << endl;
        doc << "table.heatmap td.level1 {background-color: #003800}" << endl;
        doc << "table.heatmap td.level2 {background-color: #004000}" << endl;
        doc << "table.heatmap td.level3 {background-color: #004800}" << endl;
        doc << "table.heatmap td.level4 {background-color: #005000}" << endl;
        doc << "table.heatmap td.level5 {background-color: #005800}" << endl;
        doc << "table.heatmap td.level6 {background-color: #006000}" << endl;
        doc << "table.heatmap td.level7 {background-color: #006800}" << endl;
        doc << "table.heatmap td.level8 {background-color: #007000}" << endl;
        doc << "table.heatmap td.level9 {background-color: #007800}" << endl;
        doc << "table.heatmap td.level10 {background-color: #008000}" << endl;
        doc << "table.heatmap td.level11 {background-color: #008800}" << endl;
        doc << "table.heatmap td.level12 {background-color: #009000}" << endl;
        doc << "table.heatmap td.level13 {background-color: #009800}" << endl;
        doc << "table.heatmap td.level14 {background-color: #00A000}" << endl;
        doc << "table.heatmap td.level15 {background-color: #00A800}" << endl;
        doc << "table.heatmap td.level16 {background-color: #00B000}" << endl;
        doc << "table.heatmap td.level17 {background-color: #00B800}" << endl;
        doc << "table.heatmap td.level18 {background-color: #00C000}" << endl;
        doc << "table.heatmap td.level19 {background-color: #00C800}" << endl;
        doc << "table.heatmap td.level20 {background-color: #00D000}" << endl;
        doc << "table.heatmap td.level21 {background-color: #00D800}" << endl;
        doc << "table.heatmap td.level22 {background-color: #00E000}" << endl;
        doc << "table.heatmap td.level23 {background-color: #00E800}" << endl;
        doc << "table.heatmap td.level24 {background-color: #00F000}" << endl;
        doc << "table.heatmap td.level25 {background-color: #00FF00}" << endl;
        doc << "table.heatmap td.min {background-color: #003000}" << endl;
        doc << "table.heatmap td.max {background-color: #00FF00}" << endl;

        doc << "table.heatmap th {border: 1px solid " << border_color << ";}" << endl;

        doc << "</style>" << endl;
      }

      void jsConst( ofstream &doc ) {
        doc << "<script type='text/javascript'>" << endl;
        doc << "color_user_cpu = '#3366cc';" << endl;
        doc << "color_system_cpu = '#dc3912';" << endl;
        doc << "color_iowait_cpu = '#ff9900';" << endl;
        doc << "color_nice_cpu = '#109618';" << endl;
        doc << "color_irq_cpu = '#95139d';" << endl;
        doc << "color_softirq_cpu = '#2b1464';" << endl;
        doc << "color_steal_cpu = '#5cdcde';" << endl;
        doc << "color_idle_cpu = '#AAAAAA';" << endl;
        doc << "</script>" << endl;
      }

      void jsScript( ofstream &doc ) {
        doc << "<script type='text/javascript'>" << endl;
        doc << "function toggleVisible( idstr, title ) {" << endl;
        doc << "  if ( document.getElementById( idstr + '_togglediv' ).style.display == 'none'  ) {" << endl;
        doc << "    document.getElementById( idstr + '_togglediv').style.display = 'block';" << endl;
        doc << "    document.getElementById( idstr + '_toggle' ).innerHTML = '&#xab; ' + title;" << endl;
        doc << "  } else {" << endl;
        doc << "    document.getElementById( idstr + '_togglediv' ).style.display = 'none';" << endl;
        doc << "    document.getElementById( idstr + '_toggle' ).innerHTML = '&#xbb; ' + title;" << endl;
        doc << "  }" << endl;
        doc << "}" << endl;
        doc << "</script>" << endl;
      }

      /**
       * setup the chart in jschart and return the html div element
       */
      string chartTotalCPUAverage( const persist::Database &db, const string &domname ) {
        persist::Query qry(db);
        qry.prepare( "select avg(user_mode),avg(system_mode),avg(iowait_mode),avg(nice_mode),avg(irq_mode),avg(softirq_mode),avg(steal_mode), "
                     "max((user_mode+system_mode+iowait_mode+nice_mode+irq_mode+softirq_mode+steal_mode)),"
                     "min((user_mode+system_mode+iowait_mode+nice_mode+irq_mode+softirq_mode+steal_mode)),"
                     "avg((user_mode+system_mode+iowait_mode+nice_mode+irq_mode+softirq_mode+steal_mode)),"
                     "avg(numcpu) "
                     " from v_cpuagstat where id>=:from and id <=:to" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        stringstream ss;
        if ( qry.step() ) {
          size_t cleft = 5;
          size_t ctop = 22;
          size_t cright = 5;
          size_t cbottom = 5;
          size_t cwidth = 200;
          size_t cheight = 160;
          size_t dheight = cheight + ctop + cbottom;
          size_t dwidth = cleft + cwidth + cright;
          jschart << "var " << domname << "_data = google.visualization.arrayToDataTable([" << endl;
          jschart << "['CPU utilization', 'seconds per second']," << endl;
          jschart << "['User'," << qry.getDouble(0) << " ]," << endl;
          jschart << "['System'," << qry.getDouble(1) << " ]," << endl;
          jschart << "['IO wait'," << qry.getDouble(2) << " ]," << endl;
          jschart << "['Nice'," << qry.getDouble(3) << " ]," << endl;
          jschart << "['IRQ'," << qry.getDouble(4) << " ]," << endl;
          jschart << "['Soft IRQ'," << qry.getDouble(5) << " ]," << endl;
          jschart << "['Steal'," << qry.getDouble(6) << " ]," << endl;
          jschart << "['Idle'," << qry.getDouble(10)-(qry.getDouble(1)+qry.getDouble(2)+qry.getDouble(3)+qry.getDouble(4)+qry.getDouble(5)+qry.getDouble(6)) << " ]" << endl;
          jschart << "]);" << endl;
          jschart << "var " << domname << "_options = {" << endl;
          jschart << "title: 'CPU utilization'," << endl;
          jschart << "fontSize: 10," << endl;
          jschart << timeline_background_color << ", " << endl;
          jschart << "sliceVisibilityThreshold: 0," << endl;
          jschart << "chartArea: { left: " << cleft << ",top: " << ctop << ", width: " << cwidth << ",height: " << cheight << "}," << endl;
          jschart << "slices: { 0: {color:color_user_cpu}, 1: {color:color_system_cpu}, 2:{color:color_iowait_cpu},3: {color:color_nice_cpu},4: {color:color_irq_cpu},5: {color:color_softirq_cpu},6: {color:color_steal_cpu},7: {color:color_idle_cpu} }" << endl;
          jschart << "};" << endl;
          jschart << "var " << domname << "_chart = new google.visualization.PieChart(document.getElementById('" << domname << "'));" << endl;
          jschart << domname << "_chart.draw(" << domname << "_data, " << domname << "_options);" << endl;
          ss << "<div class=\"chart\" id='" << domname << "' style='width: " << dwidth << "px; height: " << dheight << "px;'></div>";
        }
        return ss.str();
      }

      string chartPerCPUAverage( const persist::Database &db, const string &domname ) {
      persist::Query qry(db);
        qry.prepare( "select logical, avg(user_mode), avg(nice_mode), avg(system_mode), avg(iowait_mode), avg(irq_mode), avg(softirq_mode), avg(steal_mode) from v_cpustat where id>=:from and id <=:to group by logical order by logical" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        int iter = 0;
        while ( qry.step() ) {
          if ( iter == 0 ) {
            jschart << "var " << domname << "_data = google.visualization.arrayToDataTable([" << endl;
            jschart << "['CPU mode', 'User', 'Nice', 'System', 'IO wait', 'IRQ', 'Soft IRQ', 'Steal', { role: 'annotation' } ]," << endl;
          } else {
            jschart << ",";
          }
          jschart << "[ 'CPU " << qry.getInt(0) << "', " << qry.getDouble(1) << ", " << qry.getDouble(2) << ", " << qry.getDouble(3) << ", "
                               << qry.getDouble(4) << ", "  << qry.getDouble(5) << ", "  << qry.getDouble(6) << ", " << qry.getDouble(7) << ", '']" << endl;
          iter++;
        }
        jschart << "]);" << endl;
        size_t cleft = 40;
        size_t cright = 20;
        size_t ctop = 50;
        size_t cbottom = 28;
        size_t cwidth = 460;
        size_t cheight = iter * 18;
        size_t dheight = cheight + ctop + cbottom;
        size_t dwidth = cleft + cwidth + cright;
        jschart << "var " << domname << "_options = {" << endl;
        jschart << "title: 'per CPU average, CPU seconds per second'," << endl;
        jschart << "isStacked: true," << endl;
        jschart << timeline_background_color << ", " << endl;
        jschart << "fontSize: 10," << endl;
        jschart << "chartArea: {left:" << cleft << ",top:" << ctop << ",width:" << cwidth << ",height:" << cheight << "}," << endl;
        jschart << "colors: [color_user_cpu, color_nice_cpu, color_system_cpu, color_iowait_cpu, color_irq_cpu,color_softirq_cpu,color_steal_cpu,color_idle_cpu]," << endl;
        jschart << "legend: {position: 'top', maxLines: 3 }" << endl;
        jschart << "};" << endl;
        jschart << "var " << domname << " = new google.visualization.BarChart(document.getElementById('" << domname << "'));" << endl;
        jschart << domname << ".draw(" << domname << "_data, " << domname << "_options);" << endl;
        stringstream ss;
        ss << "<div class=\"chart\" id='" + domname + "' style='width: " << dwidth << "px; height: " << dheight << "px;'></div>";
        return ss.str();
      }

      string chartVMAverage( const persist::Database &db, const string &domname ) {
        persist::Query qry(db);
        qry.prepare( "select avg(unused),avg(anon),avg(file),avg(slab), avg(pagetbls) from vmstat where snapshot>=:from and snapshot <=:to" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        stringstream ss;
        if ( qry.step() ) {
          size_t cleft = 5;
          size_t ctop = 22;
          size_t cright = 5;
          size_t cbottom = 5;
          size_t cwidth = 200;
          size_t cheight = 160;
          size_t dheight = cheight + ctop + cbottom;
          size_t dwidth = cleft + cwidth + cright;
          jschart << "var " << domname << "_data = google.visualization.arrayToDataTable([" << endl;
          jschart << "['Memory utilization (GiB)', '% real memory']," << endl;
          jschart << "['unused'," << qry.getDouble(0)/one_gib << " ]," << endl;
          jschart << "['anon'," << qry.getDouble(1)/one_gib << " ]," << endl;
          jschart << "['file'," << qry.getDouble(2)/one_gib << " ]," << endl;
          jschart << "['slab'," << qry.getDouble(3)/one_gib << " ]," << endl;
          jschart << "['page tables'," << qry.getDouble(4)/one_gib << " ]" << endl;
          jschart << "]);" << endl;
          jschart << "var global_vm_pie_options = {" << endl;
          jschart << "title: 'Memory utilization (GiB)'," << endl;
          jschart << "fontSize: 10," << endl;
          jschart << timeline_background_color << ", " << endl;
          jschart << "chartArea: {left:" << cleft << ",top:" << ctop << ",width:" << cwidth << ",height:" << cheight << "}," << endl;
          jschart << "legend: {right: 'top' }," << endl;
          jschart << "sliceVisibilityThreshold: 0," << endl;
          jschart << "};" << endl;
          jschart << "var " << domname << "_chart = new google.visualization.PieChart(document.getElementById('" << domname << "'));" << endl;
          jschart << domname << "_chart.draw(global_vm_pie_data, global_vm_pie_options);" << endl;

          ss << "<div class=\"chart\" id='" + domname + "' style='width: " << dwidth << "px; height: " << dheight << "px;'></div>";
        }
        return ss.str();
      }

      string chartUserAverage( const persist::Database &db, const string &domcmd ) {
        persist::Query qry(db);
        qry.prepare( "select uid, sum(usercpu)/cnt.num a_usercpu, sum(systemcpu)/cnt.num a_systemcpu, sum(iotime)/cnt.num a_iotime from procstat, "
                     "(select count(1) num from snapshot where id>=:from and id <=:to) cnt where snapshot>=:from and snapshot<=:to "
                      "group by uid order by a_usercpu+a_systemcpu+a_iotime desc limit 15;" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        int iter = 0;
        while ( qry.step() ) {
          if ( iter == 0 ) {
            jschart << "var " << domcmd << "_barchart_data = google.visualization.arrayToDataTable([" << endl;
            jschart << "['user', 'User', 'System', 'IO wait', { role: 'annotation' } ]," << endl;
          } else {
            jschart << ",";
          }
          jschart << "[ '" << system::getUserName( qry.getInt(0) ) << "', " << qry.getDouble(1) << ", " << qry.getDouble(2) << ", " << qry.getDouble(3) << ", '']" << endl;
          iter++;
        }
        size_t cleft = 70;
        size_t cright = 20;
        size_t ctop = 42;
        size_t cbottom = 28;
        size_t cwidth = 460;
        size_t cheight = iter * 18;
        size_t dheight = cheight + ctop + cbottom;
        size_t dwidth = cleft + cwidth + cright;

        jschart << "]);" << endl;
        jschart << "var " << domcmd << "_barchart_options = {" << endl;
        jschart << "title: 'top users, CPU seconds per second'," << endl;
        jschart << "isStacked: true," << endl;
        jschart << timeline_background_color << ", " << endl;
        jschart << "fontSize: 10," << endl;
        jschart << "colors: [color_user_cpu, color_system_cpu, color_iowait_cpu]," << endl;
        jschart << "chartArea: {left:" << cleft << ",top:" << ctop << ",width:" << cwidth << ",height:" << cheight << "}," << endl;
        jschart << "legend: {position: 'top' }" << endl;
        jschart << "};" << endl;
        jschart << "var " << domcmd << "_barchart = new google.visualization.BarChart(document.getElementById('" << domcmd << "'));" << endl;
        jschart << domcmd << "_barchart.draw(" << domcmd << "_barchart_data, " << domcmd << "_barchart_options);" << endl;

        stringstream ss;
        ss << "<div class=\"chart\" id='" << domcmd << "' style='width: " << dwidth << "px; height: " << dheight << "px;'></div>" << endl;
        return ss.str();

      }

      string chartCmdAverage( const persist::Database &db, const string &domcmd ) {
        persist::Query qry(db);
        qry.prepare( "select * from "
                     "(select cmd.cmd, sum(usercpu)/cnt.num usercpu,sum(systemcpu)/cnt.num systemcpu, sum(iotime)/cnt.num iotime "
                     " from procstat,cmd,(select count(1) num from snapshot where id>=:from and id <=:to) cnt where procstat.cmd=cmd.id "
                     " and snapshot>=:from and snapshot <=:to group by cmd.cmd "
                     " union "
                     "select 'unknown', :usercpu,:systemcpu,:iotime ) "
                     "order by usercpu+systemcpu+iotime desc limit 15;" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        qry.bind( 3, std::max(0.0, (snaptotals.user_cpu_seconds+snaptotals.nice_cpu_seconds-snaptotals.cmd_usercpu_seconds) )/(snaprange.snap_max-snaprange.snap_min) );
        qry.bind( 4, std::max(0.0, (snaptotals.system_cpu_seconds+snaptotals.irq_cpu_seconds+snaptotals.softirq_cpu_seconds-snaptotals.cmd_systemcpu_seconds) )/(snaprange.snap_max-snaprange.snap_min) );
        qry.bind( 5, std::max(0.0, (double)(snaptotals.io_seconds-snaptotals.cmd_iotime_seconds) )/(snaprange.snap_max-snaprange.snap_min) );
        int iter = 0;
        size_t sw = 0;
        while ( qry.step() ) {
          if ( iter == 0 ) {
            jschart << "var " << domcmd << "_barchart_data = google.visualization.arrayToDataTable([" << endl;
            jschart << "['cmd', 'User', 'System', 'IO wait', { role: 'annotation' } ]," << endl;
          } else {
            jschart << ",";
          }
          jschart << "[ '" << qry.getText(0) << "', " << qry.getDouble(1) << ", " << qry.getDouble(2) << ", " << qry.getDouble(3) << ", '']" << endl;
          iter++;
          if ( qry.getText(0).length() > sw ) sw = qry.getText(0).length();
        }

        size_t cleft = sw*5+10;
        size_t cright = 20;
        size_t ctop = 42;
        size_t cbottom = 28;
        size_t cwidth = 460;
        size_t cheight = iter * 18;
        size_t dheight = cheight + ctop + cbottom;
        size_t dwidth = cleft + cwidth + cright;

        jschart << "]);" << endl;
        jschart << "var " << domcmd << "_barchart_options = {" << endl;
        jschart << "title: 'top commands, CPU seconds per second'," << endl;
        jschart << "isStacked: true," << endl;
        jschart << timeline_background_color << ", " << endl;
        jschart << "fontSize: 10," << endl;
        jschart << "colors: [color_user_cpu, color_system_cpu, color_iowait_cpu]," << endl;
        jschart << "chartArea: {left:" << cleft << ",top:" << ctop << ",width:" << cwidth << ",height:" << cheight << "}," << endl;
        jschart << "legend: {position: 'top' }" << endl;
        jschart << "};" << endl;
        jschart << "var " << domcmd << "_barchart = new google.visualization.BarChart(document.getElementById('" << domcmd << "'));" << endl;
        jschart << domcmd << "_barchart.draw(" << domcmd << "_barchart_data, " << domcmd << "_barchart_options);" << endl;

        stringstream ss;
        ss << "<div class=\"chart\" id='" << domcmd << "' style='width: " << dwidth << "px; height: " << dheight << "px;'></div>" << endl;
        return ss.str();

      }

      list<string> chartNetAverage( const persist::Database &db, const string &dombw, const string &dompktsz ) {
        stringstream jsbw;
        stringstream jspktsz;
        persist::Query qry(db);
        qry.prepare( "select nic.device, sum(rxbs)/cnt.num, sum(txbs)/cnt.num, sum(rxbs)/sum(rxpkts), sum(txbs)/sum(txpkts) "
                     "from netstat, nic, (select count(1) num from snapshot where id>=:from and id <=:to ) cnt where netstat.nic=nic.id and snapshot>=:from and snapshot <=:to group by nic.device;" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        int iter = 0;
        size_t sw = 0;
        while ( qry.step() ) {
          if ( iter == 0 ) {
            jsbw << "var " << dombw << "_data = google.visualization.arrayToDataTable([" << endl;
            jsbw << "['nic', 'received', 'transmitted' ]," << endl;
            jspktsz << "var " << dompktsz << "_data = google.visualization.arrayToDataTable([" << endl;
            jspktsz << "['nic', 'received', 'transmitted' ]," << endl;
          } else {
            jsbw << ",";
            jspktsz << ",";
          }
          jsbw << "[ '" << qry.getText(0) << "', " << qry.getDouble(1) << ", " << qry.getDouble(2) << " ]" << endl;
          jspktsz << "[ '" << qry.getText(0) << "', " << qry.getDouble(3) << ", " << qry.getDouble(4) << " ]" << endl;
          iter++;
          if ( qry.getText(0).length() > sw ) sw = qry.getText(0).length();
        }
        jsbw << "]);" << endl;
        jspktsz << "]);" << endl;

        list<string> result;
        stringstream ss;

        size_t cleft = sw*5+10;
        size_t cright = 20;
        size_t ctop = 42;
        size_t cbottom = 28;
        size_t cwidth = 460;
        size_t cheight = iter * 18;
        size_t dheight = cheight + ctop + cbottom;
        size_t dwidth = cleft + cwidth + cright;

        jsbw << "var " << dombw << "_options = {" << endl;
        jsbw << "title: 'nic bandwidth, bytes per second'," << endl;
        jsbw << "isStacked: true," << endl;
        jsbw << timeline_background_color << ", " << endl;
        jsbw << "legend: {position: 'top' }," << endl;
        jsbw << "chartArea: {left:" << cleft << ",top:" << ctop << ",width:" << cwidth << ",height:" << cheight << "}," << endl;
        jsbw << "fontSize: 10" << endl;
        jsbw << "};" << endl;
        jsbw << "var " << dombw << " = new google.visualization.BarChart(document.getElementById('" << dombw << "'));" << endl;
        jsbw << dombw << ".draw(" << dombw << "_data, " << dombw << "_options);" << endl;

        ss << "<div class=\"chart\" id='" << dombw << "' style='width: " << dwidth << "px; height: " << dheight << "px;'></div>" << endl;
        result.push_back( ss.str() );
        ss.str("");

        jspktsz << "var " << dompktsz << "_options = {" << endl;
        jspktsz << "title: 'nic packet size, bytes'," << endl;
        jspktsz << "isStacked: true," << endl;
        jspktsz << timeline_background_color << ", " << endl;
        jspktsz << "legend: {position: 'top' }," << endl;
        jspktsz << "chartArea: {left:" << cleft << ",top:" << ctop << ",width:" << cwidth << ",height:" << cheight << "}," << endl;
        jspktsz << "fontSize: 10" << endl;
        jspktsz << "};" << endl;
        jspktsz << "var " << dompktsz << " = new google.visualization.BarChart(document.getElementById('" << dompktsz << "'));" << endl;
        jspktsz << dompktsz << ".draw(" << dompktsz << "_data, " << dompktsz << "_options);" << endl;

        ss << "<div class=\"chart\" id='" << dompktsz << "' style='width: " << dwidth << "px; height: " << dheight << "px;'></div>" << endl;
        result.push_back( ss.str() );
        ss.str("");

        jschart << jsbw.str();
        jschart << jspktsz.str();

        return result;
      }

      string chartTCPServerAverage( const persist::Database &db, const string &dom ) {
        stringstream js;
        persist::Query qry(db);
        qry.prepare( "select tcpkey.ip, tcpkey.port, sum(esta)*1.0/cnt.num esta from tcpserverstat, tcpkey, (select count(1) num "
                     " from snapshot where id>=:from and id <=:to) cnt where tcpserverstat.tcpkey=tcpkey.id and snapshot>=:from and snapshot <=:to group by tcpkey.ip, tcpkey.port order by esta desc limit 10;" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        int iter = 0;
        size_t sw = 0;
        while ( qry.step() ) {
          if ( iter == 0 ) {
            js << "var " << dom << "_data = google.visualization.arrayToDataTable([" << endl;
            js << "['ip', 'connections', {role:'annotation'} ]," << endl;

          } else {
            js << ",";
          }
          string netservicename = net::getServiceName( qry.getInt(1) );
          string dns  = options.noresolv?qry.getText(0):resolveCacheIP( qry.getText(0) );
          string skey = qry.getText(0) + " " + qry.getText(1);
          js << "[ '" << skey << "', " << qry.getDouble(2);
          if ( dns != qry.getText(0) )
            js << ", '" << dns;
          else
            js << ", '";
          if ( netservicename != "" )
            js << " (" << netservicename <<  ") ']" << endl;
          else
            js << "' ]" << endl;
          iter++;
          if ( skey.length() > sw ) sw = skey.length();
        }

        size_t cleft = sw*5+10;
        size_t cright = 20;
        size_t ctop = 22;
        size_t cbottom = 28;
        size_t cwidth = 460;
        size_t cheight = iter * 18;
        size_t dheight = cheight + ctop + cbottom;
        size_t dwidth = cleft + cwidth + cright;

        stringstream ss;

        if ( iter ) {
          js << "]);" << endl;
          js << "var " << dom << "_options = {" << endl;
          js << "title: 'average TCP server #connections'," << endl;
          js << timeline_background_color << ", " << endl;
          js << "legend: {position: 'none' }," << endl;
          js << "fontSize: 10," << endl;
          js << "chartArea: {left:" << cleft << ",top:" << ctop << ",width:" << cwidth << ",height:" << cheight << "}," << endl;
          js << "};" << endl;
          js << "var " << dom << " = new google.visualization.BarChart(document.getElementById('" << dom << "'));" << endl;
          js << dom << ".draw(" << dom << "_data, " << dom << "_options);" << endl;

          jschart << js.str();

          ss << "<div class=\"chart\" id='" << dom << "' style='width: " << dwidth << "px; height: " << dheight << "px;'></div>" << endl;
        }
        return ss.str();
      }

      string chartTCPClientAverage( const persist::Database &db, const string &dom ) {
        stringstream js;
        persist::Query qry(db);
        qry.prepare( "select tcpkey.ip, tcpkey.port, sum(esta)*1.0/cnt.num esta from tcpclientstat, tcpkey, (select count(1) num "
                     " from snapshot where id>=:from and id <=:to) cnt where tcpclientstat.tcpkey=tcpkey.id and snapshot>=:from and snapshot <=:to group by tcpkey.ip, tcpkey.port order by esta desc limit 10;" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        int iter = 0;
        size_t sw = 0;
        while ( qry.step() ) {
          if ( iter == 0 ) {
            js << "var " << dom << "_data = google.visualization.arrayToDataTable([" << endl;
            js << "['ip', 'connections', {role:'annotation'} ]," << endl;

          } else {
            js << ",";
          }
          string netservicename = net::getServiceName( qry.getInt(1) );
          string dns  = options.noresolv?qry.getText(0):resolveCacheIP( qry.getText(0) );
          string skey = qry.getText(0) + " " + qry.getText(1);
          js << "[ '" << skey << "', " << qry.getDouble(2);
          if ( dns != qry.getText(0) )
            js << ", '" << dns;
          else
            js << ", '";
          if ( netservicename != "" )
            js << " (" << netservicename <<  ") ']" << endl;
          else
            js << "' ]" << endl;
          iter++;
          if ( skey.length() > sw ) sw = skey.length();
        }

        size_t cleft = sw*5+10;
        size_t cright = 20;
        size_t ctop = 22;
        size_t cbottom = 28;
        size_t cwidth = 460;
        size_t cheight = iter * 18;
        size_t dheight = cheight + ctop + cbottom;
        size_t dwidth = cleft + cwidth + cright;

        stringstream ss;

        if ( iter ) {
          js << "]);" << endl;
          js << "var " << dom << "_options = {" << endl;
          js << "title: 'average TCP client #connections'," << endl;
          js << timeline_background_color << ", " << endl;
          js << "legend: {position: 'none' }," << endl;
          js << "fontSize: 10," << endl;
          js << "chartArea: {left:" << cleft << ",top:" << ctop << ",width:" << cwidth << ",height:" << cheight << "}," << endl;
          js << "};" << endl;
          js << "var " << dom << " = new google.visualization.BarChart(document.getElementById('" << dom << "'));" << endl;
          js << dom << ".draw(" << dom << "_data, " << dom << "_options);" << endl;

          jschart << js.str();

          ss << "<div class=\"chart\" id='" << dom << "' style='width: " << dwidth << "px; height: " << dheight << "px;'></div>" << endl;
        }
        return ss.str();
      }

      list<string> chartMountAverage( const persist::Database &db, const string &domutil, const string &domsvctm, const string &domrsws, const string &domgrowth ) {
        stringstream jsutil;
        stringstream jssvctm;
        stringstream jsrsws;
        stringstream jsgrowth;
        persist::Query qry(db);
        qry.prepare( "select mountpoint.mountpoint, sum(util)/cnt.num, avg(svctm), sum(rs)/cnt.num, sum(ws)/cnt.num, sum(rbs)/cnt.num, sum(wbs)/cnt.num, avg(artm), avg(awtm), avg(growth) "
                     " from mountpoint,mountstat, (select count(1) num from snapshot where id>=:from and id <=:to ) cnt  "
                     " where mountpoint.id=mountstat.mountpoint and snapshot>=:from and snapshot <=:to group by mountpoint.mountpoint order by sum(util)/cnt.num desc" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        int iter = 0;
        size_t sw = 0;
        //double dt = snaprange.time_max - snaprange.time_min;
        while ( qry.step() ) {
          if ( iter == 0 ) {
            jsutil << "var " << domutil << "_data = google.visualization.arrayToDataTable([" << endl;
            jsutil << "['filesystem', 'utilization' ]," << endl;
            jssvctm << "var " << domsvctm << "_data = google.visualization.arrayToDataTable([" << endl;
            jssvctm << "['filesystem', 'service time' ]," << endl;
            jsrsws << "var " << domrsws << "_data = google.visualization.arrayToDataTable([" << endl;
            jsrsws << "[ 'filesystem', 'reads', 'writes' ]," << endl;
            jsgrowth << "var " << domgrowth << "_data = google.visualization.arrayToDataTable([" << endl;
            jsgrowth << "[ 'filesystem', 'growth' ]," << endl;

          } else {
            jsutil << ",";
            jssvctm << ",";
            jsrsws << ",";
            jsgrowth << ",";
          }
          jsutil << "[ '" << qry.getText(0) << "', " << qry.getDouble(1) << " ]" << endl;
          jssvctm << "[ '" << qry.getText(0) << "', " << qry.getDouble(2)*1000.0 << " ]" << endl;
          jsrsws << "[ '" << qry.getText(0) << "', " << qry.getDouble(3) << ", " << qry.getDouble(4) << " ]" << endl;
          jsgrowth << "[ '" << qry.getText(0) << "', " << qry.getDouble(9)/1024.0/1024.0*3600.0 << " ]" << endl;
          iter++;
          if ( qry.getText(0).length() > sw ) sw = qry.getText(0).length();
        }

        size_t cleft = sw*5+10;
        size_t cright = 20;
        size_t ctop = 22;
        size_t cbottom = 28;
        size_t cwidth = 460;
        size_t cheight = iter * 18;
        size_t dheight = cheight + ctop + cbottom;
        size_t dwidth = cleft + cwidth + cright;

        list<string> result;
        stringstream ss;

        jsutil << "]);" << endl;
        jsutil << "var " << domutil << "_options = {" << endl;
        jsutil << "title: 'filesystem utilization, IO time per second'," << endl;
        jsutil << timeline_background_color << ", " << endl;
        jsutil << "legend: {position: 'none' }," << endl;
        jsutil << "chartArea: {left:" << cleft << ",top:" << ctop << ",width:" << cwidth << ",height:" << cheight << "}," << endl;
        jsutil << "fontSize: 10" << endl;
        jsutil << "};" << endl;
        jsutil << "var " << domutil << " = new google.visualization.BarChart(document.getElementById('" << domutil << "'));" << endl;
        jsutil << domutil << ".draw(" << domutil << "_data, " << domutil << "_options);" << endl;

        ss << "<div class=\"chart\" id='" << domutil << "' style='width: " << dwidth << "px; height: " << dheight << "px;'></div>" << endl;
        result.push_back( ss.str() );
        ss.str("");

        jssvctm << "]);" << endl;
        jssvctm << "var " << domsvctm << "_options = {" << endl;
        jssvctm << "title: 'filesystem service time, milliseconds'," << endl;
        jssvctm << timeline_background_color << ", " << endl;
        jssvctm << "legend: {position: 'none' }," << endl;
        jssvctm << "chartArea: {left:" << cleft << ",top:" << ctop << ",width:" << cwidth << ",height:" << cheight << "}," << endl;
        jssvctm << "fontSize: 10" << endl;
        jssvctm << "};" << endl;
        jssvctm << "var " << domsvctm << " = new google.visualization.BarChart(document.getElementById('" << domsvctm << "'));" << endl;
        jssvctm << domsvctm << ".draw(" << domsvctm << "_data, " << domsvctm << "_options);" << endl;

        ss << "<div class=\"chart\" id='" << domsvctm << "' style='width: " << dwidth << "px; height: " << dheight << "px;'></div>" << endl;
        result.push_back( ss.str() );
        ss.str("");

        ctop = 42;
        dheight = cheight + ctop + cbottom;

        jsrsws << "]);" << endl;
        jsrsws << "var " << domrsws << "_options = {" << endl;
        jsrsws << "title: 'filesystem IO operations, per second'," << endl;
        jsrsws << timeline_background_color << ", " << endl;
        jsrsws << "isStacked: true," << endl;
        jsrsws << "legend: {position: 'top', maxLines: 3 }," << endl;
        jsrsws << "chartArea: {left:" << cleft << ",top:" << ctop << ",width:" << cwidth << ",height:" << cheight << "}," << endl;
        jsrsws << "fontSize: 10" << endl;
        jsrsws << "};" << endl;
        jsrsws << "var " << domrsws << " = new google.visualization.BarChart(document.getElementById('" << domrsws << "'));" << endl;
        jsrsws << domrsws << ".draw(" << domrsws << "_data, " << domrsws << "_options);" << endl;

        ss << "<div class=\"chart\" id='" << domrsws << "' style='width: " << dwidth << "px; height: " << dheight << "px;'></div>" << endl;
        result.push_back( ss.str() );
        ss.str("");

        jsgrowth << "]);" << endl;
        jsgrowth << "var " << domgrowth << "_options = {" << endl;
        jsgrowth << "title: 'filesystem growth, MiB per hour'," << endl;
        jsgrowth << timeline_background_color << ", " << endl;
        jsgrowth << "legend: {position: 'none' }," << endl;
        jsgrowth << "chartArea: {left:" << cleft << ",top:" << ctop << ",width:" << cwidth << ",height:" << cheight << "}," << endl;
        jsgrowth << "fontSize: 10" << endl;
        jsgrowth << "};" << endl;
        jsgrowth << "var " << domgrowth << " = new google.visualization.BarChart(document.getElementById('" << domgrowth << "'));" << endl;
        jsgrowth << domgrowth << ".draw(" << domgrowth << "_data, " << domgrowth << "_options);" << endl;

        ss << "<div class=\"chart\" id='" << domgrowth << "' style='width: " << dwidth << "px; height: " << dheight << "px;'></div>" << endl;
        result.push_back( ss.str() );
        ss.str("");


        jschart << jsutil.str();
        jschart << jssvctm.str();
        jschart << jsrsws.str();
        jschart << jsgrowth.str();

        return result;
      }

      list<string> chartDiskAverage( const persist::Database &db, const string &domutil, const string &domsvctm, const string &domrsws ) {
        stringstream jsutil;
        stringstream jssvctm;
        stringstream jsrsws;
        persist::Query qry(db);
        qry.prepare( "select 'disk'||disk.id,sum(iostat.util)/cnt.num,avg(iostat.svctm),sum(iostat.rs)/cnt.num,sum(iostat.ws)/cnt.num "
                     "from iostat,disk,(select count(1) num from snapshot where id>=:from and id <=:to ) cnt  where iostat.disk=disk.id and snapshot>=:from and snapshot <=:to group by disk.id" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        int iter = 0;
        while ( qry.step() ) {
          if ( iter == 0 ) {
            jsutil << "var " << domutil << "_data = google.visualization.arrayToDataTable([" << endl;
            jsutil << "['disk', 'utilization' ]," << endl;
            jssvctm << "var " << domsvctm << "_data = google.visualization.arrayToDataTable([" << endl;
            jssvctm << "['disk', 'service time' ]," << endl;
            jsrsws << "var " << domrsws << "_data = google.visualization.arrayToDataTable([" << endl;
            jsrsws << "[ 'disk', 'reads', 'writes' ]," << endl;

          } else {
            jsutil << ",";
            jssvctm << ",";
            jsrsws << ",";
          }
          jsutil << "[ '" << qry.getText(0) << "', " << qry.getDouble(1) << " ]" << endl;
          jssvctm << "[ '" << qry.getText(0) << "', " << qry.getDouble(2)*1000.0 << " ]" << endl;
          jsrsws << "[ '" << qry.getText(0) << "', " << qry.getDouble(3) << ", " << qry.getDouble(4) << " ]" << endl;
          iter++;
        }

        size_t cleft = 70;
        size_t cright = 20;
        size_t ctop = 22;
        size_t cbottom = 28;
        size_t cwidth = 460;
        size_t cheight = iter * 18;
        size_t dheight = cheight + ctop + cbottom;
        size_t dwidth = cleft + cwidth + cright;

        list<string> result;
        stringstream ss;

        jsutil << "]);" << endl;
        jsutil << "var " << domutil << "_options = {" << endl;
        jsutil << "title: 'disk utilization, IO time per second'," << endl;
        jsutil << timeline_background_color << ", " << endl;
        jsutil << "legend: {position: 'none' }," << endl;
        jsutil << "chartArea: {left:" << cleft << ",top:" << ctop << ",width:" << cwidth << ",height:" << cheight << "}," << endl;
        jsutil << "fontSize: 10" << endl;
        jsutil << "};" << endl;
        jsutil << "var " << domutil << " = new google.visualization.BarChart(document.getElementById('" << domutil << "'));" << endl;
        jsutil << domutil << ".draw(" << domutil << "_data, " << domutil << "_options);" << endl;

        ss << "<div class=\"chart\" id='" << domutil << "' style='width: " << dwidth << "px; height: " << dheight << "px;'></div>" << endl;
        result.push_back( ss.str() );
        ss.str("");

        jssvctm << "]);" << endl;
        jssvctm << "var " << domsvctm << "_options = {" << endl;
        jssvctm << "title: 'disk service time, milliseconds'," << endl;
        jssvctm << timeline_background_color << ", " << endl;
        jssvctm << "legend: {position: 'none' }," << endl;
        jssvctm << "chartArea: {left:" << cleft << ",top:" << ctop << ",width:" << cwidth << ",height:" << cheight << "}," << endl;
        jssvctm << "fontSize: 10" << endl;
        jssvctm << "};" << endl;
        jssvctm << "var " << domsvctm << " = new google.visualization.BarChart(document.getElementById('" << domsvctm << "'));" << endl;
        jssvctm << domsvctm << ".draw(" << domsvctm << "_data, " << domsvctm << "_options);" << endl;

        ss << "<div class=\"chart\" id='" << domsvctm << "' style='width: " << dwidth << "px; height: " << dheight << "px;'></div>" << endl;
        result.push_back( ss.str() );
        ss.str("");

        ctop = 50;
        dheight = cheight + ctop + cbottom;
        jsrsws << "]);" << endl;
        jsrsws << "var " << domrsws << "_options = {" << endl;
        jsrsws << "title: 'disk IO operations, per second'," << endl;
        jsrsws << timeline_background_color << ", " << endl;
        jsrsws << "isStacked: true," << endl;
        jsrsws << "legend: {position: 'top', maxLines: 3 }," << endl;
        jsrsws << "chartArea: {left:" << cleft << ",top:" << ctop << ",width:" << cwidth << ",height:" << cheight << "}," << endl;
        jsrsws << "fontSize: 10" << endl;
        jsrsws << "};" << endl;
        jsrsws << "var " << domrsws << " = new google.visualization.BarChart(document.getElementById('" << domrsws << "'));" << endl;
        jsrsws << domrsws << ".draw(" << domrsws << "_data, " << domrsws << "_options);" << endl;

        ss << "<div class=\"chart\" id='" << domrsws << "' style='width: " << dwidth << "px; height: " << dheight << "px;'></div>" << endl;
        result.push_back( ss.str() );


        jschart << jsutil.str();
        jschart << jssvctm.str();
        jschart << jsrsws.str();

        return result;
      }

      void htmlReportAverages( const persist::Database &db ) {
        html << "<a class=\"anchor\" id=\"reportaverage\"></a><h1>Report averages</h1>" << endl;
        html << "<p>Averages over the report time range.</p>" << endl;

        html << "<a class=\"anchor\" id=\"reportaverage_cpu\"></a><h2>CPU</h2>" << endl;
        html << chartTotalCPUAverage( db, "global_cpu_pie" ) << endl;
        html << chartPerCPUAverage( db, "global_cpu_barchart" ) << endl;

        html << "<a class=\"anchor\" id=\"reportaverage_mem\"></a><h2>Memory</h2>" << endl;
        html << chartVMAverage( db, "global_vm_pie" );

        html << "<a class=\"anchor\" id=\"reportaverage_disk\"></a><h2>Disks</h2>" << endl;
        list<string> divs = chartDiskAverage( db, "global_disk_util", "global_disk_svctm", "global_disk_rsws" );
        for ( list<string>::const_iterator s = divs.begin(); s != divs.end(); s++ ) {
          html << *s << endl;
        }

        divs.clear();
        html << "<a class=\"anchor\" id=\"reportaverage_mount\"></a><h2>Mountpoints</h2>" << endl;
        divs = chartMountAverage( db, "global_mount_util", "global_mount_svctm", "global_mount_rsws", "global_mount_growth" );
        for ( list<string>::const_iterator s = divs.begin(); s != divs.end(); s++ ) {
          html << *s << endl;
        }

        divs.clear();
        divs = chartNetAverage( db, "global_nic_bw", "global_nic_pktsz" );
        html << "<a class=\"anchor\" id=\"reportaverage_nic\"></a><h2>NICs</h2>" << endl;
        for ( list<string>::const_iterator s = divs.begin(); s != divs.end(); s++ ) {
          html << *s << endl;
        }

        html << "<a class=\"anchor\" id=\"reportaverage_cmd\"></a><h2>Commands</h2>" << endl;
        html << chartCmdAverage( db, "global_cmd" );

        html << "<a class=\"anchor\" id=\"reportaverage_user\"></a><h2>Users</h2>" << endl;
        html << chartUserAverage( db, "global_user" );

        html << "<a class=\"anchor\" id=\"reportaverage_tcpserver\"></a><h2>TCP server</h2>" << endl;
        html << chartTCPServerAverage( db, "global_tcpserver" );

        html << "<a class=\"anchor\" id=\"reportaverage_tcpclient\"></a><h2>TCP client</h2>" << endl;
        html << chartTCPClientAverage( db, "global_tcpclient" );
      }

      void chartCPUTimeLine( const persist::Database &db, const string &dom ) {
        stringstream js;
        persist::Query qry(db);
        qry.prepare( "select avg(istop), avg(user_mode),avg(system_mode), "
                     "avg(iowait_mode), avg(nice_mode), avg(irq_mode), avg(softirq_mode), avg(steal_mode) from v_cpuagstat where id>=:from and id <=:to group by istop/:bucket order by 1;" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        qry.bind( 3, snaprange.timeline_bucket );
        int iter = 0;
        while ( qry.step() ) {
          if ( iter == 0 ) {
            js << "var " << dom << "_data = google.visualization.arrayToDataTable([" << endl;
            js << "['datetime', 'user', 'system', 'iowait', 'nice', 'irq', 'softirq', 'steal' ]," << endl;

          } else {
            js << ",";
          }
          time_t istop = qry.getDouble(0);
          struct tm *lt = localtime( &istop );
          js << "[ new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ), ";
          js << qry.getDouble(1) << ", " << qry.getDouble(2) << ", " <<
             qry.getDouble(3) << ", " << qry.getDouble(4) << ", " << qry.getDouble(5) << ", " << qry.getDouble(6) << ", " << qry.getDouble(7) << " ]" << endl;
          iter++;
        }
        js << "]);" << endl;
        js << "var " << dom << "_options = {" << endl;
        js << "title: 'CPU timeline'," << endl;
        js << timeline_background_color << ", " << endl;
        js << "isStacked: true," << endl;
        js << "lineWidth: 0.2," << endl;
        js << "areaOpacity: 1.0," << endl;
        js << "colors: [color_user_cpu, color_system_cpu, color_iowait_cpu, color_nice_cpu, color_irq_cpu,color_softirq_cpu,color_steal_cpu]," << endl;
        js << timeline_legend << "," << endl;
        js << timeline_fontsize << "," << endl;
        js << timeline_chartarea << ", " << endl;
        js << "};" << endl;
        js << "var " << dom << " = new google.visualization.AreaChart(document.getElementById('" << dom << "'));" << endl;
        js << dom << ".draw(" << dom << "_data, " << dom << "_options);" << endl;

        jschart << js.str();
      }

      void chartPagingTimeLine( const persist::Database &db, const string &dommajflt, const string &dompagein, const string &dompageout ) {
        stringstream jsmajflt;
        stringstream jspagein;
        stringstream jspageout;
        persist::Query qry(db);
        qry.prepare( "select avg(snapshot.istop), avg(majflts), avg(pgins), avg(pgouts) from snapshot, vmstat where snapshot.id=vmstat.snapshot and snapshot.id>=:from and snapshot.id <=:to group by snapshot.istop/:bucket order by 1;" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        qry.bind( 3, snaprange.timeline_bucket );
        int iter = 0;
        while ( qry.step() ) {
          if ( iter == 0 ) {
            jsmajflt << "var " << dommajflt << "_data = google.visualization.arrayToDataTable([" << endl;
            jsmajflt << "['datetime', 'majflt' ]," << endl;

            jspagein << "var " << dompagein << "_data = google.visualization.arrayToDataTable([" << endl;
            jspagein << "['datetime', 'pagein' ]," << endl;

            jspageout << "var " << dompageout << "_data = google.visualization.arrayToDataTable([" << endl;
            jspageout << "['datetime', 'pageout' ]," << endl;

          } else {
            jsmajflt << ",";
            jspagein << ",";
            jspageout << ",";
          }
          time_t istop = qry.getDouble(0);
          struct tm *lt = localtime( &istop );
          jsmajflt << "[ new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ), ";
          jsmajflt << qry.getDouble(1) << " ]" << endl;

          jspagein << "[ new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ), ";
          jspagein << qry.getDouble(2) << " ]" << endl;

          jspageout << "[ new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ), ";
          jspageout << qry.getDouble(3) << " ]" << endl;
          iter++;
        }
        jsmajflt << "]);" << endl;
        jsmajflt << "var " << dommajflt << "_options = {" << endl;
        jsmajflt << "title: 'Major faults per second timeline'," << endl;
        jsmajflt << timeline_background_color << ", " << endl;
        jsmajflt << "colors: [color_user_cpu, color_system_cpu, color_iowait_cpu, color_nice_cpu, color_irq_cpu,color_softirq_cpu]," << endl;
        jsmajflt << "lineWidth: 1," << endl;
        jsmajflt << "legend: 'none'," << endl;
        jsmajflt << timeline_fontsize << "," << endl;
        jsmajflt << timeline_chartarea << ", " << endl;
        jsmajflt << "};" << endl;
        jsmajflt << "var " << dommajflt << " = new google.visualization.LineChart(document.getElementById('" << dommajflt << "'));" << endl;
        jsmajflt << dommajflt << ".draw(" << dommajflt << "_data, " << dommajflt << "_options);" << endl;

        jspagein << "]);" << endl;
        jspagein << "var " << dompagein << "_options = {" << endl;
        jspagein << "title: 'page-ins per second timeline'," << endl;
        jspagein << timeline_background_color << ", " << endl;
        jspagein << "colors: [color_user_cpu, color_system_cpu, color_iowait_cpu, color_nice_cpu, color_irq_cpu,color_softirq_cpu]," << endl;
        jspagein << "lineWidth: 1," << endl;
        jspagein << "legend: 'none'," << endl;
        jspagein << timeline_fontsize << "," << endl;
        jspagein << timeline_chartarea << ", " << endl;
        jspagein << "};" << endl;
        jspagein << "var " << dompagein << " = new google.visualization.LineChart(document.getElementById('" << dompagein << "'));" << endl;
        jspagein << dompagein << ".draw(" << dompagein << "_data, " << dompagein << "_options);" << endl;

        jspageout << "]);" << endl;
        jspageout << "var " << dompageout << "_options = {" << endl;
        jspageout << "title: 'page-outs per second timeline'," << endl;
        jspageout << timeline_background_color << ", " << endl;
        jspageout << "colors: [color_user_cpu, color_system_cpu, color_iowait_cpu, color_nice_cpu, color_irq_cpu,color_softirq_cpu]," << endl;
        jspageout << "lineWidth: 1," << endl;
        jspageout << "legend: 'none'," << endl;
        jspageout << timeline_fontsize << "," << endl;
        jspageout << timeline_chartarea << ", " << endl;
        jspageout << "};" << endl;
        jspageout << "var " << dompageout << " = new google.visualization.LineChart(document.getElementById('" << dompageout << "'));" << endl;
        jspageout << dompageout << ".draw(" << dompageout << "_data, " << dompageout << "_options);" << endl;

        jschart << jsmajflt.str();
        jschart << jspagein.str();
        jschart << jspageout.str();
      }

      void chartSchedTimeLine( const persist::Database &db, const string &domfork, const string &domctxsw, const string &domload5 ) {
        stringstream jsfork;
        stringstream jsctxsw;
        stringstream jsload5;
        persist::Query qry(db);
        qry.prepare( "select avg(snapshot.istop), avg(forks), avg(ctxsw), avg(load5) from snapshot, schedstat where snapshot.id=schedstat.snapshot and snapshot.id>=:from and snapshot.id <=:to group by snapshot.istop/:bucket order by 1;" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        qry.bind( 3, snaprange.timeline_bucket );
        int iter = 0;
        while ( qry.step() ) {
          if ( iter == 0 ) {
            jsfork << "var " << domfork << "_data = google.visualization.arrayToDataTable([" << endl;
            jsfork << "['datetime', 'forks' ]," << endl;

            jsctxsw << "var " << domctxsw << "_data = google.visualization.arrayToDataTable([" << endl;
            jsctxsw << "['datetime', 'context switches' ]," << endl;

            jsload5 << "var " << domload5 << "_data = google.visualization.arrayToDataTable([" << endl;
            jsload5 << "['datetime', '5 minute load average' ]," << endl;

          } else {
            jsfork << ",";
            jsctxsw << ",";
            jsload5 << ",";
          }
          time_t istop = qry.getDouble(0);
          struct tm *lt = localtime( &istop );
          jsfork << "[ new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ), ";
          jsfork << qry.getDouble(1) << " ]" << endl;

          jsctxsw << "[ new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ), ";
          jsctxsw << qry.getDouble(2) << " ]" << endl;

          jsload5 << "[ new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ), ";
          jsload5 << qry.getDouble(3) << " ]" << endl;

          iter++;
        }
        jsfork << "]);" << endl;
        jsfork << "var " << domfork << "_options = {" << endl;
        jsfork << "title: 'Forks per seond timeline'," << endl;
        jsfork << timeline_background_color << ", " << endl;
        jsfork << "colors: [color_user_cpu, color_system_cpu, color_iowait_cpu, color_nice_cpu, color_irq_cpu,color_softirq_cpu]," << endl;
        jsfork << "lineWidth: 1," << endl;
        jsfork << "legend: 'none'," << endl;
        jsfork << timeline_fontsize << "," << endl;
        jsfork << timeline_chartarea << ", " << endl;
        jsfork << "};" << endl;
        jsfork << "var " << domfork << " = new google.visualization.LineChart(document.getElementById('" << domfork << "'));" << endl;
        jsfork << domfork << ".draw(" << domfork << "_data, " << domfork << "_options);" << endl;

        jsctxsw << "]);" << endl;
        jsctxsw << "var " << domctxsw << "_options = {" << endl;
        jsctxsw << "title: 'Context switches per seond timeline'," << endl;
        jsctxsw << timeline_background_color << ", " << endl;
        jsctxsw << "colors: [color_user_cpu, color_system_cpu, color_iowait_cpu, color_nice_cpu, color_irq_cpu,color_softirq_cpu]," << endl;
        jsctxsw << "lineWidth: 1," << endl;
        jsctxsw << "legend: 'none'," << endl;
        jsctxsw << timeline_fontsize << "," << endl;
        jsctxsw << timeline_chartarea << ", " << endl;
        jsctxsw << "};" << endl;
        jsctxsw << "var " << domctxsw << " = new google.visualization.LineChart(document.getElementById('" << domctxsw << "'));" << endl;
        jsctxsw << domctxsw << ".draw(" << domctxsw << "_data, " << domctxsw << "_options);" << endl;

        jsload5 << "]);" << endl;
        jsload5 << "var " << domload5 << "_options = {" << endl;
        jsload5 << "title: 'Load5 average timeline'," << endl;
        jsload5 << timeline_background_color << ", " << endl;
        jsload5 << "colors: [color_user_cpu, color_system_cpu, color_iowait_cpu, color_nice_cpu, color_irq_cpu,color_softirq_cpu]," << endl;
        jsload5 << "lineWidth: 1," << endl;
        jsload5 << "legend: 'none'," << endl;
        jsload5 << timeline_fontsize << "," << endl;
        jsload5 << timeline_chartarea << ", " << endl;
        jsload5 << "};" << endl;
        jsload5 << "var " << domload5 << " = new google.visualization.LineChart(document.getElementById('" << domload5 << "'));" << endl;
        jsload5 << domload5 << ".draw(" << domload5 << "_data, " << domload5 << "_options);" << endl;

        jschart << jsfork.str();
        jschart << jsctxsw.str();
        jschart << jsload5.str();
      }

      void chartKernelTimeLine( const persist::Database &db, const string &domprocs, const string &domusers, const string &domfiles, const string &dominodes ) {
        stringstream jsprocs;
        stringstream jsusers;
        stringstream jsfiles;
        stringstream jsinodes;
        persist::Query qry(db);
        qry.prepare( "select avg(snapshot.istop), avg(processes), avg(users), avg(logins), avg(files_open), avg(inodes_open) from"
                     " snapshot, resstat where snapshot.id=resstat.snapshot and snapshot.id>=:from and snapshot.id <=:to group by snapshot.istop/:bucket order by 1;" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        qry.bind( 3, snaprange.timeline_bucket );
        int iter = 0;
        while ( qry.step() ) {
          if ( iter == 0 ) {
            jsprocs << "var " << domprocs << "_data = google.visualization.arrayToDataTable([" << endl;
            jsprocs << "['datetime', 'processes' ]," << endl;

            jsusers << "var " << domusers << "_data = google.visualization.arrayToDataTable([" << endl;
            jsusers << "['datetime', 'users', 'logins' ]," << endl;

            jsfiles << "var " << domfiles << "_data = google.visualization.arrayToDataTable([" << endl;
            jsfiles << "['datetime', 'open files' ]," << endl;

            jsinodes << "var " << dominodes << "_data = google.visualization.arrayToDataTable([" << endl;
            jsinodes << "['datetime', 'open inodes' ]," << endl;

          } else {
            jsprocs << ",";
            jsusers << ",";
            jsfiles << ",";
            jsinodes << ",";
          }
          time_t istop = qry.getDouble(0);
          struct tm *lt = localtime( &istop );
          jsprocs << "[ new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ), ";
          jsprocs << qry.getDouble(1) << " ]" << endl;

          jsusers << "[ new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ), ";
          jsusers << qry.getDouble(2) << ", " << qry.getDouble(3) << " ]" << endl;

          jsfiles << "[ new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ), ";
          jsfiles << qry.getDouble(4) << " ]" << endl;

          jsinodes << "[ new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ), ";
          jsinodes << qry.getDouble(5)/1000 << " ]" << endl;

          iter++;
        }
        jsprocs << "]);" << endl;
        jsprocs << "var " << domprocs << "_options = {" << endl;
        jsprocs << "title: '#processes timeline'," << endl;
        jsprocs << timeline_background_color << ", " << endl;
        jsprocs << "colors: [color_user_cpu, color_system_cpu, color_iowait_cpu, color_nice_cpu, color_irq_cpu,color_softirq_cpu]," << endl;
        jsprocs << "lineWidth: 1," << endl;
        jsprocs << "legend: 'none'," << endl;
        jsprocs << timeline_fontsize << "," << endl;
        jsprocs << timeline_chartarea << ", " << endl;
        jsprocs << "};" << endl;
        jsprocs << "var " << domprocs << " = new google.visualization.LineChart(document.getElementById('" << domprocs << "'));" << endl;
        jsprocs << domprocs << ".draw(" << domprocs << "_data, " << domprocs << "_options);" << endl;

        jsusers << "]);" << endl;
        jsusers << "var " << domusers << "_options = {" << endl;
        jsusers << "title: 'users and logins timeline'," << endl;
        jsusers << timeline_background_color << ", " << endl;
        jsusers << "colors: [color_user_cpu, color_system_cpu, color_iowait_cpu, color_nice_cpu, color_irq_cpu,color_softirq_cpu]," << endl;
        jsusers << "lineWidth: 1," << endl;
        jsusers << "legend: {position: 'top', maxLines: 3 }," << endl;
        jsusers << timeline_fontsize << "," << endl;
        jsusers << timeline_chartarea << ", " << endl;
        jsusers << "};" << endl;
        jsusers << "var " << domusers << " = new google.visualization.LineChart(document.getElementById('" << domusers << "'));" << endl;
        jsusers << domusers << ".draw(" << domusers << "_data, " << domusers << "_options);" << endl;

        jsfiles << "]);" << endl;
        jsfiles << "var " << domfiles << "_options = {" << endl;
        jsfiles << "title: 'open files timeline'," << endl;
        jsfiles << timeline_background_color << ", " << endl;
        jsfiles << "colors: [color_user_cpu, color_system_cpu, color_iowait_cpu, color_nice_cpu, color_irq_cpu,color_softirq_cpu]," << endl;
        jsfiles << "lineWidth: 1," << endl;
        jsfiles << "legend: 'none'," << endl;
        jsfiles << timeline_fontsize << "," << endl;
        jsfiles << timeline_chartarea << ", " << endl;
        jsfiles << "};" << endl;
        jsfiles << "var " << domfiles << " = new google.visualization.LineChart(document.getElementById('" << domfiles << "'));" << endl;
        jsfiles << domfiles << ".draw(" << domfiles << "_data, " << domfiles << "_options);" << endl;

        jsinodes << "]);" << endl;
        jsinodes << "var " << dominodes << "_options = {" << endl;
        jsinodes << "title: 'open inodes timeline (x1000)'," << endl;
        jsinodes << timeline_background_color << ", " << endl;
        jsinodes << "colors: [color_user_cpu, color_system_cpu, color_iowait_cpu, color_nice_cpu, color_irq_cpu,color_softirq_cpu]," << endl;
        jsinodes << "lineWidth: 1," << endl;
        jsinodes << "legend: 'none'," << endl;
        jsinodes << timeline_fontsize << "," << endl;
        jsinodes << timeline_chartarea << ", " << endl;
        jsinodes << "};" << endl;
        jsinodes << "var " << dominodes << " = new google.visualization.LineChart(document.getElementById('" << dominodes << "'));" << endl;
        jsinodes << dominodes << ".draw(" << dominodes << "_data, " << dominodes << "_options);" << endl;

        jschart << jsprocs.str();
        jschart << jsusers.str();
        jschart << jsfiles.str();
        jschart << jsinodes.str();
      }

      void chartVMTimeLine( const persist::Database &db, const string &domram, const string &domswap ) {
        stringstream jsram;
        stringstream jsswp;
        persist::Query qry(db);
        qry.prepare( "select avg(snapshot.istop), avg(unused),avg(anon),avg(file),avg(slab),avg(pagetbls), avg(mapped), avg(hptotal), avg(hprsvd), avg(hpfree), "
                     "avg(swpused),avg(swpsize) from vmstat, snapshot where vmstat.snapshot=snapshot.id and snapshot>=:from and snapshot <=:to group by snapshot.istop/:bucket;" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        qry.bind( 3, snaprange.timeline_bucket );
        int iter = 0;
        while ( qry.step() ) {
          if ( iter == 0 ) {
            jsram << "var " << domram << "_data = google.visualization.arrayToDataTable([" << endl;
            jsram << "['datetime', 'unused', 'anonymous', 'file', 'slab', 'page tables', 'mapped', 'hp used', 'hp reserved', 'hp free' ]," << endl;

            jsswp << "var " << domswap << "_data = google.visualization.arrayToDataTable([" << endl;
            jsswp << "['datetime', 'swap used', 'swap free', ]," << endl;

          } else {
            jsram << ",";
            jsswp << ",";
          }
          time_t istop = qry.getDouble(0);
          struct tm *lt = localtime( &istop );
          jsram << "[ new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ), ";
          jsram << qry.getDouble(1)/one_gib << ", " << qry.getDouble(2)/one_gib << ", " <<
             qry.getDouble(3)/one_gib << ", " << qry.getDouble(4)/one_gib << ", " << qry.getDouble(5)/one_gib <<  ", " << qry.getDouble(6)/one_gib <<  ", " <<
             (qry.getDouble(7)-qry.getDouble(8)-qry.getDouble(9))/one_gib << ", " << qry.getDouble(8)/one_gib << ", " << qry.getDouble(9)/one_gib << " ]" << endl;

          jsswp << "[ new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ), ";
          jsswp << qry.getDouble(10)/one_gib << ", " << (qry.getDouble(11)-qry.getDouble(10))/one_gib << " ]" << endl;
          iter++;
        }
        jsram << "]);" << endl;
        jsram << "var " << domram << "_options = {" << endl;
        jsram << "title: 'Memory timeline (GiB)'," << endl;
        jsram << timeline_background_color << ", " << endl;
        jsram << "isStacked: true," << endl;
        jsram << "lineWidth: 0.2," << endl;
        jsram << "areaOpacity: 1.0," << endl;
        jsram << timeline_legend << ", " << endl;
        jsram << timeline_fontsize << "," << endl;
        jsram << timeline_chartarea << endl;
        jsram << "};" << endl;
        jsram << "var " << domram << " = new google.visualization.AreaChart(document.getElementById('" << domram << "'));" << endl;
        jsram << domram << ".draw(" << domram << "_data, " << domram << "_options);" << endl;

        jsswp << "]);" << endl;
        jsswp << "var " << domswap << "_options = {" << endl;
        jsswp << "title: 'Swap timeline (GiB)'," << endl;
        jsswp << timeline_background_color << ", " << endl;
        jsswp << "isStacked: true," << endl;
        jsswp << "lineWidth: 0.2," << endl;
        jsswp << "areaOpacity: 1.0," << endl;
        jsswp << timeline_legend << ", " << endl;
        jsswp << timeline_fontsize << "," << endl;
        jsswp << timeline_chartarea << endl;
        jsswp << "};" << endl;
        jsswp << "var " << domswap << " = new google.visualization.AreaChart(document.getElementById('" << domswap << "'));" << endl;
        jsswp << domswap << ".draw(" << domswap << "_data, " << domswap << "_options);" << endl;

        jschart << jsram.str();
        jschart << jsswp.str();
      }

      typedef struct { double util; double bwr; double bww; double iopsr; double iopsw; double artm; double awtm; double svctm; double used; } IORec;

      void chartDiskTimeLine( const persist::Database &db, const string &domutil, const string &dombwr, const string &dombww,
                              const string &domiopsr, const string &domiopsw, const string &domartm, const string & domawtm, const string &domsvctm ) {
        stringstream jsutil;
        stringstream jsutilcolumns;
        stringstream jsutildata;

        stringstream jsbwr;
        stringstream jsbwrcolumns;
        stringstream jsbwrdata;

        stringstream jsbww;
        stringstream jsbwwcolumns;
        stringstream jsbwwdata;

        stringstream jsiopsr;
        stringstream jsiopsrcolumns;
        stringstream jsiopsrdata;

        stringstream jsiopsw;
        stringstream jsiopswcolumns;
        stringstream jsiopswdata;

        stringstream jsartm;
        stringstream jsartmcolumns;
        stringstream jsartmdata;

        stringstream jsawtm;
        stringstream jsawtmcolumns;
        stringstream jsawtmdata;

        stringstream jssvctm;
        stringstream jssvctmcolumns;
        stringstream jssvctmdata;

        persist::Query qry(db);
        qry.prepare( "select 'disk'||disk.id, bat.a_istop, avg(util), avg(rbs)/1024.0/1024.0, avg(wbs)/1024.0/1024.0, avg(rs), avg(ws), avg(artm), avg(awtm), avg(svctm) from iostat, disk, "
                     "(select min(id) minid, max(id) maxid, avg(istop) a_istop from snapshot where id>=:from and id <=:to group by istop/:bucket) bat where "
                     "iostat.disk=disk.id and iostat.snapshot>=bat.minid and iostat.snapshot<=bat.maxid group by bat.a_istop, disk.id" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        qry.bind( 3, snaprange.timeline_bucket );

        typedef map<double,map<string,IORec> > DData; // datetime (time_t), to pair ( device to utilization)
        DData data;
        size_t maxsz = 0;
        set<string> devices;
        while ( qry.step() ) {
          (data[qry.getDouble(1)][ qry.getText(0) ]).util = qry.getDouble(2);
          (data[qry.getDouble(1)][ qry.getText(0) ]).bwr = qry.getDouble(3);
          (data[qry.getDouble(1)][ qry.getText(0) ]).bww = qry.getDouble(4);
          (data[qry.getDouble(1)][ qry.getText(0) ]).iopsr = qry.getDouble(5);
          (data[qry.getDouble(1)][ qry.getText(0) ]).iopsw = qry.getDouble(6);
          (data[qry.getDouble(1)][ qry.getText(0) ]).artm = qry.getDouble(7);
          (data[qry.getDouble(1)][ qry.getText(0) ]).awtm = qry.getDouble(8);
          (data[qry.getDouble(1)][ qry.getText(0) ]).svctm = qry.getDouble(9);
          devices.insert(qry.getText(0));
        }
        for ( set<string>::const_iterator d = devices.begin(); d != devices.end(); d++ ) {
          jsutilcolumns << domutil << "_data.addColumn( 'number', '" << *d << "' );" << endl;
          jsbwrcolumns << dombwr << "_data.addColumn( 'number', '" << *d << "' );" << endl;
          jsbwwcolumns << dombww << "_data.addColumn( 'number', '" << *d << "' );" << endl;
          jsiopsrcolumns << domiopsr << "_data.addColumn( 'number', '" << *d << "' );" << endl;
          jsiopswcolumns << domiopsw << "_data.addColumn( 'number', '" << *d << "' );" << endl;
          jsartmcolumns << domartm << "_data.addColumn( 'number', '" << *d << "' );" << endl;
          jsawtmcolumns << domawtm << "_data.addColumn( 'number', '" << *d << "' );" << endl;
          jssvctmcolumns << domsvctm << "_data.addColumn( 'number', '" << *d << "' );" << endl;
        }
        maxsz = data.size();
        if ( maxsz > 0 ) {
          jsutil << "var " << domutil << "_data = new google.visualization.DataTable();" << endl;
          jsutil << domutil << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jsutil << jsutilcolumns.str();
          jsutil << domutil << "_data.addRows(" << maxsz << ");" << endl;

          jsbwr << "var " << dombwr << "_data = new google.visualization.DataTable();" << endl;
          jsbwr << dombwr << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jsbwr << jsbwrcolumns.str();
          jsbwr << dombwr << "_data.addRows(" << maxsz << ");" << endl;

          jsbww << "var " << dombww << "_data = new google.visualization.DataTable();" << endl;
          jsbww << dombww << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jsbww << jsbwwcolumns.str();
          jsbww << dombww << "_data.addRows(" << maxsz << ");" << endl;

          jsiopsr << "var " << domiopsr << "_data = new google.visualization.DataTable();" << endl;
          jsiopsr << domiopsr << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jsiopsr << jsiopsrcolumns.str();
          jsiopsr << domiopsr << "_data.addRows(" << maxsz << ");" << endl;

          jsiopsw << "var " << domiopsw << "_data = new google.visualization.DataTable();" << endl;
          jsiopsw << domiopsw << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jsiopsw << jsiopswcolumns.str();
          jsiopsw << domiopsw << "_data.addRows(" << maxsz << ");" << endl;

          jsartm << "var " << domartm << "_data = new google.visualization.DataTable();" << endl;
          jsartm << domartm << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jsartm << jsartmcolumns.str();
          jsartm << domartm << "_data.addRows(" << maxsz << ");" << endl;

          jsawtm << "var " << domawtm << "_data = new google.visualization.DataTable();" << endl;
          jsawtm << domawtm << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jsawtm << jsawtmcolumns.str();
          jsawtm << domawtm << "_data.addRows(" << maxsz << ");" << endl;

          jssvctm << "var " << domsvctm << "_data = new google.visualization.DataTable();" << endl;
          jssvctm << domsvctm << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jssvctm << jssvctmcolumns.str();
          jssvctm << domsvctm << "_data.addRows(" << maxsz << ");" << endl;

          size_t iter = 0;
          for ( DData::const_iterator i = data.begin(); i != data.end(); ++i ) {
            time_t istop = i->first;
            struct tm *lt = localtime( &istop );
            jsutildata << domutil << "_data.setCell( " << iter << ", " << "0, ";
            jsutildata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            jsbwrdata << dombwr << "_data.setCell( " << iter << ", " << "0, ";
            jsbwrdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            jsbwwdata << dombww << "_data.setCell( " << iter << ", " << "0, ";
            jsbwwdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            jsiopsrdata << domiopsr << "_data.setCell( " << iter << ", " << "0, ";
            jsiopsrdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            jsiopswdata << domiopsw << "_data.setCell( " << iter << ", " << "0, ";
            jsiopswdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            jsartmdata << domartm << "_data.setCell( " << iter << ", " << "0, ";
            jsartmdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            jsawtmdata << domawtm << "_data.setCell( " << iter << ", " << "0, ";
            jsawtmdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            jssvctmdata << domsvctm << "_data.setCell( " << iter << ", " << "0, ";
            jssvctmdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            size_t colidx = 1;
            for ( set<string>::const_iterator d = devices.begin(); d != devices.end(); d++ ) {
              map<string,IORec>::const_iterator found = i->second.find( *d );
              if ( found != i->second.end() ) {
                jsutildata << domutil << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.util << ");" << endl;
                jsbwrdata << dombwr << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.bwr << ");" << endl;
                jsbwwdata << dombww << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.bww << ");" << endl;
                jsiopsrdata << domiopsr << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.iopsr << ");" << endl;
                jsiopswdata << domiopsw << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.iopsw << ");" << endl;
                jsartmdata << domartm << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.artm*1000.0 << ");" << endl;
                jsawtmdata << domawtm << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.awtm*1000.0 << ");" << endl;
                jssvctmdata << domsvctm << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.svctm*1000.0 << ");" << endl;
              } else {
                jsutildata << domutil << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
                jsbwrdata << dombwr << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
                jsbwwdata << dombww << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
                jsiopsrdata << domiopsr << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
                jsiopswdata << domiopsw << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
                jsartmdata << domartm << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
                jsawtmdata << domawtm << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
                jssvctmdata << domsvctm << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
              }
              colidx++;
            }
            iter++;
          }
          jsutil << jsutildata.str();
          jsutil << "var " << domutil << "_options = {" << endl;
          jsutil << "title: 'Disk utilization timeline (IO seconds per second, stacked)'," << endl;
          jsutil << timeline_background_color << ", " << endl;
          jsutil << "legend: {position: 'none' }," << endl;
          jsutil << "isStacked: true," << endl;
          jsutil << "lineWidth: 0.2," << endl;
          jsutil << "areaOpacity: 1.0," << endl;
          jsutil << "aggregationTarget: 'category'," << endl;
          jsutil << "legend: {position: 'top', maxLines: 3 }," << endl;
          jsutil << timeline_fontsize << "," << endl;
          jsutil << timeline_chartarea << endl;
          jsutil << "};" << endl;
          jsutil << "var " << domutil << " = new google.visualization.AreaChart(document.getElementById('" << domutil << "'));" << endl;
          jsutil << domutil << ".draw(" << domutil << "_data, " << domutil << "_options);" << endl;

          jsbwr << jsbwrdata.str();
          jsbwr << "var " << dombwr << "_options = {" << endl;
          jsbwr << "title: 'Disk read bandwidth timeline (MiB per second, stacked)'," << endl;
          jsbwr << timeline_background_color << ", " << endl;
          jsbwr << "legend: {position: 'none' }," << endl;
          jsbwr << "isStacked: true," << endl;
          jsbwr << "lineWidth: 0.2," << endl;
          jsbwr << "areaOpacity: 1.0," << endl;
          jsbwr << "aggregationTarget: 'category'," << endl;
          jsbwr << "legend: {position: 'top', maxLines: 3 }," << endl;
          jsbwr << timeline_fontsize << "," << endl;
          jsbwr << timeline_chartarea << endl;
          jsbwr << "};" << endl;
          jsbwr << "var " << dombwr << " = new google.visualization.AreaChart(document.getElementById('" << dombwr << "'));" << endl;
          jsbwr << dombwr << ".draw(" << dombwr << "_data, " << dombwr << "_options);" << endl;

          jsbww << jsbwwdata.str();
          jsbww << "var " << dombww << "_options = {" << endl;
          jsbww << "title: 'Disk write bandwidth timeline (MiB per second, stacked)'," << endl;
          jsbww << timeline_background_color << ", " << endl;
          jsbww << "isStacked: true," << endl;
          jsbww << "lineWidth: 0.2," << endl;
          jsbww << "areaOpacity: 1.0," << endl;
          jsbww << timeline_legend << ", " << endl;
          jsbww << timeline_fontsize << "," << endl;
          jsbww << timeline_chartarea << endl;
          jsbww << "};" << endl;
          jsbww << "var " << dombww << " = new google.visualization.AreaChart(document.getElementById('" << dombww << "'));" << endl;
          jsbww << dombww << ".draw(" << dombww << "_data, " << dombww << "_options);" << endl;

          jsiopsr << jsiopsrdata.str();
          jsiopsr << "var " << domiopsr << "_options = {" << endl;
          jsiopsr << "title: 'Disk read IOPS timeline (IO per second, stacked)'," << endl;
          jsiopsr << timeline_background_color << ", " << endl;
          jsiopsr << "isStacked: true," << endl;
          jsiopsr << "lineWidth: 0.2," << endl;
          jsiopsr << "areaOpacity: 1.0," << endl;
          jsiopsr << timeline_legend << ", " << endl;
          jsiopsr << timeline_fontsize << "," << endl;
          jsiopsr << "chartArea: {left:70, height:'70%', right:10}" << endl;
          jsiopsr << "};" << endl;
          jsiopsr << "var " << domiopsr << " = new google.visualization.AreaChart(document.getElementById('" << domiopsr << "'));" << endl;
          jsiopsr << domiopsr << ".draw(" << domiopsr << "_data, " << domiopsr << "_options);" << endl;

          jsiopsw << jsiopswdata.str();
          jsiopsw << "var " << domiopsw << "_options = {" << endl;
          jsiopsw << "title: 'Disk write IOPS timeline (IO per second, stacked)'," << endl;
          jsiopsw << timeline_background_color << ", " << endl;
          jsiopsw << "isStacked: true," << endl;
          jsiopsw << "lineWidth: 0.2," << endl;
          jsiopsw << "areaOpacity: 1.0," << endl;
          jsiopsw << timeline_legend << ", " << endl;
          jsiopsw << timeline_fontsize << "," << endl;
          jsiopsw << timeline_chartarea << endl;
          jsiopsw << "};" << endl;
          jsiopsw << "var " << domiopsw << " = new google.visualization.AreaChart(document.getElementById('" << domiopsw << "'));" << endl;
          jsiopsw << domiopsw << ".draw(" << domiopsw << "_data, " << domiopsw << "_options);" << endl;

          jsartm << jsartmdata.str();
          jsartm << "var " << domartm << "_options = {" << endl;
          jsartm << "title: 'Disk average read time (ms)'," << endl;
          jsartm << timeline_background_color << ", " << endl;
          jsartm << "lineWidth: 1," << endl;
          jsartm << timeline_legend << ", " << endl;
          jsartm << timeline_fontsize << "," << endl;
          jsartm << timeline_chartarea << endl;
          jsartm << "};" << endl;
          jsartm << "var " << domartm << " = new google.visualization.LineChart(document.getElementById('" << domartm << "'));" << endl;
          jsartm << domartm << ".draw(" << domartm << "_data, " << domartm << "_options);" << endl;

          jsawtm << jsawtmdata.str();
          jsawtm << "var " << domawtm << "_options = {" << endl;
          jsawtm << "title: 'Disk average write time (ms)'," << endl;
          jsawtm << timeline_background_color << ", " << endl;
          jsawtm << "lineWidth: 1," << endl;
          jsawtm << timeline_legend << ", " << endl;
          jsawtm << timeline_fontsize << "," << endl;
          jsawtm << timeline_chartarea << endl;
          jsawtm << "};" << endl;
          jsawtm << "var " << domawtm << " = new google.visualization.LineChart(document.getElementById('" << domawtm << "'));" << endl;
          jsawtm << domawtm << ".draw(" << domawtm << "_data, " << domawtm << "_options);" << endl;

          jssvctm << jssvctmdata.str();
          jssvctm << "var " << domsvctm << "_options = {" << endl;
          jssvctm << "title: 'Disk average service time (ms)'," << endl;
          jssvctm << timeline_background_color << ", " << endl;
          jssvctm << "lineWidth: 1," << endl;
          jssvctm << timeline_legend << ", " << endl;
          jssvctm << timeline_fontsize << "," << endl;
          jssvctm << timeline_chartarea << endl;
          jssvctm << "};" << endl;
          jssvctm << "var " << domsvctm << " = new google.visualization.LineChart(document.getElementById('" << domsvctm << "'));" << endl;
          jssvctm << domsvctm << ".draw(" << domsvctm << "_data, " << domsvctm << "_options);" << endl;

          jschart << jsutil.str();
          jschart << jsbwr.str();
          jschart << jsbww.str();
          jschart << jsiopsr.str();
          jschart << jsiopsw.str();
          jschart << jsartm.str();
          jschart << jsawtm.str();
          jschart << jssvctm.str();
        }
      }

      void chartMountTimeLine( const persist::Database &db, const string &domutil, const string &dombwr, const string &dombww,
                              const string &domiopsr, const string &domiopsw, const string &domartm, const string & domawtm, const string &domsvctm, const string &domused ) {
        stringstream jsutil;
        stringstream jsutilcolumns;
        stringstream jsutildata;

        stringstream jsbwr;
        stringstream jsbwrcolumns;
        stringstream jsbwrdata;

        stringstream jsbww;
        stringstream jsbwwcolumns;
        stringstream jsbwwdata;

        stringstream jsiopsr;
        stringstream jsiopsrcolumns;
        stringstream jsiopsrdata;

        stringstream jsiopsw;
        stringstream jsiopswcolumns;
        stringstream jsiopswdata;

        stringstream jsartm;
        stringstream jsartmcolumns;
        stringstream jsartmdata;

        stringstream jsawtm;
        stringstream jsawtmcolumns;
        stringstream jsawtmdata;

        stringstream jssvctm;
        stringstream jssvctmcolumns;
        stringstream jssvctmdata;

        stringstream jsused;
        stringstream jsusedcolumns;
        stringstream jsuseddata;

        persist::Query qry(db);
        qry.prepare( "select mountpoint.mountpoint, bat.a_istop, avg(util), avg(rbs)/1024.0/1024.0, avg(wbs)/1024.0/1024.0, avg(rs), avg(ws), avg(artm), avg(awtm), avg(svctm), max(used) from mountstat, mountpoint, "
                     "(select min(id) minid, max(id) maxid, avg(istop) a_istop from snapshot where id>=:from and id <=:to group by istop/:bucket) bat where "
                     "mountstat.mountpoint=mountpoint.id and mountstat.snapshot>=bat.minid and mountstat.snapshot<=bat.maxid group by bat.a_istop, mountpoint.id" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        qry.bind( 3, snaprange.timeline_bucket );

        typedef map<double,map<string,IORec> > DData; // datetime (time_t), to pair ( device to utilization)
        DData data;
        size_t maxsz = 0;
        set<string> devices;
        while ( qry.step() ) {
          (data[qry.getDouble(1)][ qry.getText(0) ]).util = qry.getDouble(2);
          (data[qry.getDouble(1)][ qry.getText(0) ]).bwr = qry.getDouble(3);
          (data[qry.getDouble(1)][ qry.getText(0) ]).bww = qry.getDouble(4);
          (data[qry.getDouble(1)][ qry.getText(0) ]).iopsr = qry.getDouble(5);
          (data[qry.getDouble(1)][ qry.getText(0) ]).iopsw = qry.getDouble(6);
          (data[qry.getDouble(1)][ qry.getText(0) ]).artm = qry.getDouble(7);
          (data[qry.getDouble(1)][ qry.getText(0) ]).awtm = qry.getDouble(8);
          (data[qry.getDouble(1)][ qry.getText(0) ]).svctm = qry.getDouble(9);
          (data[qry.getDouble(1)][ qry.getText(0) ]).used = qry.getDouble(10);
          devices.insert(qry.getText(0));
        }
        for ( set<string>::const_iterator d = devices.begin(); d != devices.end(); d++ ) {
          jsutilcolumns << domutil << "_data.addColumn( 'number', '" << *d << "' );" << endl;
          jsbwrcolumns << dombwr << "_data.addColumn( 'number', '" << *d << "' );" << endl;
          jsbwwcolumns << dombww << "_data.addColumn( 'number', '" << *d << "' );" << endl;
          jsiopsrcolumns << domiopsr << "_data.addColumn( 'number', '" << *d << "' );" << endl;
          jsiopswcolumns << domiopsw << "_data.addColumn( 'number', '" << *d << "' );" << endl;
          jsartmcolumns << domartm << "_data.addColumn( 'number', '" << *d << "' );" << endl;
          jsawtmcolumns << domawtm << "_data.addColumn( 'number', '" << *d << "' );" << endl;
          jssvctmcolumns << domsvctm << "_data.addColumn( 'number', '" << *d << "' );" << endl;
          jsusedcolumns << domused << "_data.addColumn( 'number', '" << *d << "' );" << endl;
        }
        maxsz = data.size();
        if ( maxsz > 0 ) {
          jsutil << "var " << domutil << "_data = new google.visualization.DataTable();" << endl;
          jsutil << domutil << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jsutil << jsutilcolumns.str();
          jsutil << domutil << "_data.addRows(" << maxsz << ");" << endl;

          jsbwr << "var " << dombwr << "_data = new google.visualization.DataTable();" << endl;
          jsbwr << dombwr << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jsbwr << jsbwrcolumns.str();
          jsbwr << dombwr << "_data.addRows(" << maxsz << ");" << endl;

          jsbww << "var " << dombww << "_data = new google.visualization.DataTable();" << endl;
          jsbww << dombww << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jsbww << jsbwwcolumns.str();
          jsbww << dombww << "_data.addRows(" << maxsz << ");" << endl;

          jsiopsr << "var " << domiopsr << "_data = new google.visualization.DataTable();" << endl;
          jsiopsr << domiopsr << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jsiopsr << jsiopsrcolumns.str();
          jsiopsr << domiopsr << "_data.addRows(" << maxsz << ");" << endl;

          jsiopsw << "var " << domiopsw << "_data = new google.visualization.DataTable();" << endl;
          jsiopsw << domiopsw << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jsiopsw << jsiopswcolumns.str();
          jsiopsw << domiopsw << "_data.addRows(" << maxsz << ");" << endl;

          jsartm << "var " << domartm << "_data = new google.visualization.DataTable();" << endl;
          jsartm << domartm << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jsartm << jsartmcolumns.str();
          jsartm << domartm << "_data.addRows(" << maxsz << ");" << endl;

          jsawtm << "var " << domawtm << "_data = new google.visualization.DataTable();" << endl;
          jsawtm << domawtm << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jsawtm << jsawtmcolumns.str();
          jsawtm << domawtm << "_data.addRows(" << maxsz << ");" << endl;

          jssvctm << "var " << domsvctm << "_data = new google.visualization.DataTable();" << endl;
          jssvctm << domsvctm << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jssvctm << jssvctmcolumns.str();
          jssvctm << domsvctm << "_data.addRows(" << maxsz << ");" << endl;

          jsused << "var " << domused << "_data = new google.visualization.DataTable();" << endl;
          jsused << domused << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jsused << jsusedcolumns.str();
          jsused << domused << "_data.addRows(" << maxsz << ");" << endl;

          size_t iter = 0;
          for ( DData::const_iterator i = data.begin(); i != data.end(); ++i ) {
            time_t istop = i->first;
            struct tm *lt = localtime( &istop );
            jsutildata << domutil << "_data.setCell( " << iter << ", " << "0, ";
            jsutildata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            jsbwrdata << dombwr << "_data.setCell( " << iter << ", " << "0, ";
            jsbwrdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            jsbwwdata << dombww << "_data.setCell( " << iter << ", " << "0, ";
            jsbwwdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            jsiopsrdata << domiopsr << "_data.setCell( " << iter << ", " << "0, ";
            jsiopsrdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            jsiopswdata << domiopsw << "_data.setCell( " << iter << ", " << "0, ";
            jsiopswdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            jsartmdata << domartm << "_data.setCell( " << iter << ", " << "0, ";
            jsartmdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            jsawtmdata << domawtm << "_data.setCell( " << iter << ", " << "0, ";
            jsawtmdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            jssvctmdata << domsvctm << "_data.setCell( " << iter << ", " << "0, ";
            jssvctmdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            jsuseddata << domused << "_data.setCell( " << iter << ", " << "0, ";
            jsuseddata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            size_t colidx = 1;
            for ( set<string>::const_iterator d = devices.begin(); d != devices.end(); d++ ) {
              map<string,IORec>::const_iterator found = i->second.find( *d );
              if ( found != i->second.end() ) {
                jsutildata << domutil << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.util << ");" << endl;
                jsbwrdata << dombwr << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.bwr << ");" << endl;
                jsbwwdata << dombww << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.bww << ");" << endl;
                jsiopsrdata << domiopsr << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.iopsr << ");" << endl;
                jsiopswdata << domiopsw << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.iopsw << ");" << endl;
                jsartmdata << domartm << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.artm*1000.0 << ");" << endl;
                jsawtmdata << domawtm << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.awtm*1000.0 << ");" << endl;
                jssvctmdata << domsvctm << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.svctm*1000.0 << ");" << endl;
                jsuseddata << domused << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.used/1024.0/1024.0/1024.0 << ");" << endl;
              } else {
                jsutildata << domutil << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
                jsbwrdata << dombwr << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
                jsbwwdata << dombww << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
                jsiopsrdata << domiopsr << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
                jsiopswdata << domiopsw << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
                jsartmdata << domartm << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
                jsawtmdata << domawtm << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
                jssvctmdata << domsvctm << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
                jsuseddata << domused << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
              }
              colidx++;
            }
            iter++;
          }
          jsutil << jsutildata.str();
          jsutil << "var " << domutil << "_options = {" << endl;
          jsutil << "title: 'Mountpoint utilization timeline (IO seconds per second, stacked)'," << endl;
          jsutil << timeline_background_color << ", " << endl;
          jsutil << "legend: {position: 'none' }," << endl;
          jsutil << "isStacked: true," << endl;
          jsutil << "lineWidth: 0.2," << endl;
          jsutil << "areaOpacity: 1.0," << endl;
          jsutil << "aggregationTarget: 'category'," << endl;
          jsutil << "legend: {position: 'top', maxLines: 3 }," << endl;
          jsutil << timeline_fontsize << "," << endl;
          jsutil << timeline_chartarea << endl;
          jsutil << "};" << endl;
          jsutil << "var " << domutil << " = new google.visualization.AreaChart(document.getElementById('" << domutil << "'));" << endl;
          jsutil << domutil << ".draw(" << domutil << "_data, " << domutil << "_options);" << endl;

          jsbwr << jsbwrdata.str();
          jsbwr << "var " << dombwr << "_options = {" << endl;
          jsbwr << "title: 'Mountpoint read bandwidth timeline (MiB per second, stacked)'," << endl;
          jsbwr << timeline_background_color << ", " << endl;
          jsbwr << "legend: {position: 'none' }," << endl;
          jsbwr << "isStacked: true," << endl;
          jsbwr << "lineWidth: 0.2," << endl;
          jsbwr << "areaOpacity: 1.0," << endl;
          jsbwr << "aggregationTarget: 'category'," << endl;
          jsbwr << "legend: {position: 'top', maxLines: 3 }," << endl;
          jsbwr << timeline_fontsize << "," << endl;
          jsbwr << timeline_chartarea << endl;
          jsbwr << "};" << endl;
          jsbwr << "var " << dombwr << " = new google.visualization.AreaChart(document.getElementById('" << dombwr << "'));" << endl;
          jsbwr << dombwr << ".draw(" << dombwr << "_data, " << dombwr << "_options);" << endl;

          jsbww << jsbwwdata.str();
          jsbww << "var " << dombww << "_options = {" << endl;
          jsbww << "title: 'Mountpoint write bandwidth timeline (MiB per second, stacked)'," << endl;
          jsbww << timeline_background_color << ", " << endl;
          jsbww << "isStacked: true," << endl;
          jsbww << "lineWidth: 0.2," << endl;
          jsbww << "areaOpacity: 1.0," << endl;
          jsbww << timeline_legend << ", " << endl;
          jsbww << timeline_fontsize << "," << endl;
          jsbww << timeline_chartarea << endl;
          jsbww << "};" << endl;
          jsbww << "var " << dombww << " = new google.visualization.AreaChart(document.getElementById('" << dombww << "'));" << endl;
          jsbww << dombww << ".draw(" << dombww << "_data, " << dombww << "_options);" << endl;

          jsiopsr << jsiopsrdata.str();
          jsiopsr << "var " << domiopsr << "_options = {" << endl;
          jsiopsr << "title: 'Mountpoint read IOPS timeline (IO per second, stacked)'," << endl;
          jsiopsr << timeline_background_color << ", " << endl;
          jsiopsr << "isStacked: true," << endl;
          jsiopsr << "lineWidth: 0.2," << endl;
          jsiopsr << "areaOpacity: 1.0," << endl;
          jsiopsr << timeline_legend << ", " << endl;
          jsiopsr << timeline_fontsize << "," << endl;
          jsiopsr << "chartArea: {left:70, height:'70%', right:10}" << endl;
          jsiopsr << "};" << endl;
          jsiopsr << "var " << domiopsr << " = new google.visualization.AreaChart(document.getElementById('" << domiopsr << "'));" << endl;
          jsiopsr << domiopsr << ".draw(" << domiopsr << "_data, " << domiopsr << "_options);" << endl;

          jsiopsw << jsiopswdata.str();
          jsiopsw << "var " << domiopsw << "_options = {" << endl;
          jsiopsw << "title: 'Mountpoint write IOPS timeline (IO per second, stacked)'," << endl;
          jsiopsw << timeline_background_color << ", " << endl;
          jsiopsw << "isStacked: true," << endl;
          jsiopsw << "lineWidth: 0.2," << endl;
          jsiopsw << "areaOpacity: 1.0," << endl;
          jsiopsw << timeline_legend << ", " << endl;
          jsiopsw << timeline_fontsize << "," << endl;
          jsiopsw << timeline_chartarea << endl;
          jsiopsw << "};" << endl;
          jsiopsw << "var " << domiopsw << " = new google.visualization.AreaChart(document.getElementById('" << domiopsw << "'));" << endl;
          jsiopsw << domiopsw << ".draw(" << domiopsw << "_data, " << domiopsw << "_options);" << endl;

          jsartm << jsartmdata.str();
          jsartm << "var " << domartm << "_options = {" << endl;
          jsartm << "title: 'Mountpoint average read time (ms)'," << endl;
          jsartm << timeline_background_color << ", " << endl;
          jsartm << "lineWidth: 1," << endl;
          jsartm << timeline_legend << ", " << endl;
          jsartm << timeline_fontsize << "," << endl;
          jsartm << timeline_chartarea << endl;
          jsartm << "};" << endl;
          jsartm << "var " << domartm << " = new google.visualization.LineChart(document.getElementById('" << domartm << "'));" << endl;
          jsartm << domartm << ".draw(" << domartm << "_data, " << domartm << "_options);" << endl;

          jsawtm << jsawtmdata.str();
          jsawtm << "var " << domawtm << "_options = {" << endl;
          jsawtm << "title: 'Mountpoint average write time (ms)'," << endl;
          jsawtm << timeline_background_color << ", " << endl;
          jsawtm << "lineWidth: 1," << endl;
          jsawtm << timeline_legend << ", " << endl;
          jsawtm << timeline_fontsize << "," << endl;
          jsawtm << timeline_chartarea << endl;
          jsawtm << "};" << endl;
          jsawtm << "var " << domawtm << " = new google.visualization.LineChart(document.getElementById('" << domawtm << "'));" << endl;
          jsawtm << domawtm << ".draw(" << domawtm << "_data, " << domawtm << "_options);" << endl;

          jssvctm << jssvctmdata.str();
          jssvctm << "var " << domsvctm << "_options = {" << endl;
          jssvctm << "title: 'Mountpoint average service time (ms)'," << endl;
          jssvctm << timeline_background_color << ", " << endl;
          jssvctm << "lineWidth: 1," << endl;
          jssvctm << timeline_legend << ", " << endl;
          jssvctm << timeline_fontsize << "," << endl;
          jssvctm << timeline_chartarea << endl;
          jssvctm << "};" << endl;
          jssvctm << "var " << domsvctm << " = new google.visualization.LineChart(document.getElementById('" << domsvctm << "'));" << endl;
          jssvctm << domsvctm << ".draw(" << domsvctm << "_data, " << domsvctm << "_options);" << endl;

          jsused << jsuseddata.str();
          jsused << "var " << domused << "_options = {" << endl;
          jsused << "title: 'Mountpoint used (GiB)'," << endl;
          jsused << timeline_background_color << ", " << endl;
          jsused << "isStacked: true," << endl;
          jsused << "lineWidth: 0.2," << endl;
          jsused << "areaOpacity: 1.0," << endl;
          jsused << timeline_legend << ", " << endl;
          jsused << timeline_fontsize << "," << endl;
          jsused << timeline_chartarea << endl;
          jsused << "};" << endl;
          jsused << "var " << domused << " = new google.visualization.AreaChart(document.getElementById('" << domused << "'));" << endl;
          jsused << domused << ".draw(" << domused << "_data, " << domused << "_options);" << endl;

          jschart << jsutil.str();
          jschart << jsbwr.str();
          jschart << jsbww.str();
          jschart << jsiopsr.str();
          jschart << jsiopsw.str();
          jschart << jsartm.str();
          jschart << jsawtm.str();
          jschart << jssvctm.str();
          jschart << jsused.str();
        }
      }

      typedef struct {
        double rx;
        double tx;
      } NicBWRec;

      void chartNICPacketTimeLine( const persist::Database &db, const string &domrx, const string &domtx ) {
        stringstream jsrx;
        stringstream jsrxcolumns;
        stringstream jsrxdata;
        stringstream jstx;
        stringstream jstxcolumns;
        stringstream jstxdata;
        persist::Query qry(db);
        qry.prepare( "select nic.device, avg(snapshot.istop), avg(rxpkts)/1000.0, avg(txpkts)/1000.0 from nic,netstat,snapshot "
                     "where nic.id=netstat.nic and netstat.snapshot=snapshot.id and snapshot>=:from and snapshot <=:to group by nic.device, snapshot.istop/:bucket" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        qry.bind( 3, snaprange.timeline_bucket );
        typedef map<double,map<string,NicBWRec> > DData; // datetime (time_t), to pair ( device to rxpkts+txpkts)
        DData data;
        size_t maxsz = 0;
        set<string> devices;
        while ( qry.step() ) {
          data[qry.getDouble(1)][ qry.getText(0) ].rx = qry.getDouble(2);
          data[qry.getDouble(1)][ qry.getText(0) ].tx = qry.getDouble(3);
          devices.insert(qry.getText(0));
        }
        for ( set<string>::const_iterator d = devices.begin(); d != devices.end(); d++ ) {
          jsrxcolumns << domrx << "_data.addColumn( 'number', '" << *d << "' );" << endl;
          jstxcolumns << domtx << "_data.addColumn( 'number', '" << *d << "' );" << endl;
        }
        maxsz = data.size();
        if ( maxsz > 0 ) {
          jsrx << "var " << domrx << "_data = new google.visualization.DataTable();" << endl;
          jsrx << domrx << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jsrx << jsrxcolumns.str();
          jsrx << domrx << "_data.addRows(" << maxsz << ");" << endl;

          jstx << "var " << domtx << "_data = new google.visualization.DataTable();" << endl;
          jstx << domtx << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jstx << jstxcolumns.str();
          jstx << domtx << "_data.addRows(" << maxsz << ");" << endl;

          size_t iter = 0;
          for ( DData::const_iterator i = data.begin(); i != data.end(); ++i ) {
            time_t istop = i->first;
            struct tm *lt = localtime( &istop );
            jsrxdata << domrx << "_data.setCell( " << iter << ", " << "0, ";
            jsrxdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            jstxdata << domtx << "_data.setCell( " << iter << ", " << "0, ";
            jstxdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            size_t colidx = 1;
            for ( set<string>::const_iterator d = devices.begin(); d != devices.end(); d++ ) {
              map<string,NicBWRec>::const_iterator found = i->second.find( *d );
              if ( found != i->second.end() ) {
                jsrxdata << domrx << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.rx << ");" << endl;
                jstxdata << domtx << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.tx << ");" << endl;
              } else {
                jsrxdata << domrx << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
                jstxdata << domtx << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
              }
              colidx++;
            }
            iter++;
          }
          jsrx << jsrxdata.str();
          jsrx << "var " << domrx << "_options = {" << endl;
          jsrx << "title: 'NIC receive packet rate (x1000/s)'," << endl;
          jsrx << timeline_background_color << ", " << endl;
          //jsrx << "isStacked: true," << endl;
          jsrx << "lineWidth: 1," << endl;
          //jsrx << "areaOpacity: 1.0," << endl;
          jsrx << timeline_legend << ", " << endl;
          jsrx << timeline_fontsize << "," << endl;
          jsrx << timeline_chartarea << endl;
          jsrx << "};" << endl;
          jsrx << "var " << domrx << " = new google.visualization.LineChart(document.getElementById('" << domrx << "'));" << endl;
          jsrx << domrx << ".draw(" << domrx << "_data, " << domrx << "_options);" << endl;

          jstx << jstxdata.str();
          jstx << "var " << domtx << "_options = {" << endl;
          jstx << "title: 'NIC transmit packet rate (x1000/s)'," << endl;
          jstx << timeline_background_color << ", " << endl;
          //jstx << "isStacked: true," << endl;
          jstx << "lineWidth: 1," << endl;
          //jstx << "areaOpacity: 1.0," << endl;
          jstx << timeline_legend << ", " << endl;
          jstx << timeline_fontsize << "," << endl;
          jstx << timeline_chartarea << endl;
          jstx << "};" << endl;
          jstx << "var " << domtx << " = new google.visualization.LineChart(document.getElementById('" << domtx << "'));" << endl;
          jstx << domtx << ".draw(" << domtx << "_data, " << domtx << "_options);" << endl;

          jschart << jsrx.str();
          jschart << jstx.str();
        }
      }

      void chartNicBWTimeLine( const persist::Database &db, const string &domrx, const string &domtx ) {
        stringstream jsrx;
        stringstream jsrxcolumns;
        stringstream jsrxdata;
        stringstream jstx;
        stringstream jstxcolumns;
        stringstream jstxdata;
        persist::Query qry(db);
        qry.prepare( "select nic.device, avg(snapshot.istop), avg(rxbs)/1024.0/1024.0, avg(txbs)/1024.0/1024.0 from nic,netstat,snapshot "
                     "where nic.id=netstat.nic and netstat.snapshot=snapshot.id and snapshot>=:from and snapshot <=:to group by nic.device, snapshot.istop/:bucket" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        qry.bind( 3, snaprange.timeline_bucket );
        typedef map<double,map<string,NicBWRec> > DData; // datetime (time_t), to pair ( device to rxpkts+txpkts)
        DData data;
        size_t maxsz = 0;
        set<string> devices;
        while ( qry.step() ) {
          data[qry.getDouble(1)][ qry.getText(0) ].rx = qry.getDouble(2);
          data[qry.getDouble(1)][ qry.getText(0) ].tx = qry.getDouble(3);
          devices.insert(qry.getText(0));
        }
        for ( set<string>::const_iterator d = devices.begin(); d != devices.end(); d++ ) {
          jsrxcolumns << domrx << "_data.addColumn( 'number', '" << *d << "' );" << endl;
          jstxcolumns << domtx << "_data.addColumn( 'number', '" << *d << "' );" << endl;
        }
        maxsz = data.size();
        if ( maxsz > 0 ) {
          jsrx << "var " << domrx << "_data = new google.visualization.DataTable();" << endl;
          jsrx << domrx << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jsrx << jsrxcolumns.str();
          jsrx << domrx << "_data.addRows(" << maxsz << ");" << endl;

          jstx << "var " << domtx << "_data = new google.visualization.DataTable();" << endl;
          jstx << domtx << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          jstx << jstxcolumns.str();
          jstx << domtx << "_data.addRows(" << maxsz << ");" << endl;

          size_t iter = 0;
          for ( DData::const_iterator i = data.begin(); i != data.end(); ++i ) {
            time_t istop = i->first;
            struct tm *lt = localtime( &istop );
            jsrxdata << domrx << "_data.setCell( " << iter << ", " << "0, ";
            jsrxdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            jstxdata << domtx << "_data.setCell( " << iter << ", " << "0, ";
            jstxdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;

            size_t colidx = 1;
            for ( set<string>::const_iterator d = devices.begin(); d != devices.end(); d++ ) {
              map<string,NicBWRec>::const_iterator found = i->second.find( *d );
              if ( found != i->second.end() ) {
                jsrxdata << domrx << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.rx << ");" << endl;
                jstxdata << domtx << "_data.setCell( " << iter << ", " << colidx << ", " << found->second.tx << ");" << endl;
              } else {
                jsrxdata << domrx << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
                jstxdata << domtx << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
              }
              colidx++;
            }
            iter++;
          }
          jsrx << jsrxdata.str();
          jsrx << "var " << domrx << "_options = {" << endl;
          jsrx << "title: 'NIC receive bandwidth (MiB per second)'," << endl;
          jsrx << timeline_background_color << ", " << endl;
          jsrx << "lineWidth: 1," << endl;
          jsrx << timeline_legend << ", " << endl;
          jsrx << timeline_fontsize << "," << endl;
          jsrx << timeline_chartarea << endl;
          jsrx << "};" << endl;
          jsrx << "var " << domrx << " = new google.visualization.LineChart(document.getElementById('" << domrx << "'));" << endl;
          jsrx << domrx << ".draw(" << domrx << "_data, " << domrx << "_options);" << endl;

          jstx << jstxdata.str();
          jstx << "var " << domtx << "_options = {" << endl;
          jstx << "title: 'NIC transmit bandwidth (MiB per second)'," << endl;
          jstx << timeline_background_color << ", " << endl;
          jstx << "lineWidth: 1," << endl;
          jstx << timeline_legend << ", " << endl;
          jstx << timeline_fontsize << "," << endl;
          jstx << timeline_chartarea << endl;
          jstx << "};" << endl;
          jstx << "var " << domtx << " = new google.visualization.LineChart(document.getElementById('" << domtx << "'));" << endl;
          jstx << domtx << ".draw(" << domtx << "_data, " << domtx << "_options);" << endl;

          jschart << jsrx.str();
          jschart << jstx.str();
        }
      }

      void chartTCPServerTimeLine( const persist::Database &db, const string &dom ) {
        stringstream js;
        stringstream jscolumns;
        stringstream jsdata;
        persist::Query qry(db);
        qry.prepare( "select driver.a_istop, tcpkey.uid, tcpkey.port, avg(case when esta is null then 0.0 else esta end) esta from "
                     " ( select bat.minid, bat.maxid, bat.a_istop, lim.tcpkey from "
                     "     (select min(id) minid, max(id) maxid, avg(istop) a_istop from snapshot where id>=:from and id <=:to group by istop/:bucket) bat, "
                     "     (select tcpkey from tcpserverstat s where s.snapshot>=:from and s.snapshot <=:to group by s.tcpkey order by sum(s.esta) desc limit 6) lim "
                     "   order by 3 "
                     " ) driver left outer join tcpserverstat tcs on tcs.snapshot>=driver.minid and tcs.snapshot<=driver.maxid and tcs.tcpkey=driver.tcpkey, "
                     " tcpkey where tcpkey.id=driver.tcpkey group by driver.a_istop, tcpkey.uid, tcpkey.port order by 1" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        qry.bind( 3, snaprange.timeline_bucket );
        typedef map<double,map<string,double> > DData; // datetime (time_t), to pair ( device to utilization)
        DData data;
        size_t maxsz = 0;
        set<string> devices;
        while ( qry.step() ) {
          stringstream label;
          label << system::getUserName( qry.getInt( 1 ) ) << " " << net::getServiceName( qry.getInt( 2 ) );
          data[qry.getDouble(0)][ label.str() ] = qry.getDouble(3);
          devices.insert(label.str());
        }
        for ( set<string>::const_iterator d = devices.begin(); d != devices.end(); d++ ) {
          jscolumns << dom << "_data.addColumn( 'number', '" << *d << "' );" << endl;
        }
        maxsz = data.size();
        if ( maxsz > 0 ) {
          js << "var " << dom << "_data = new google.visualization.DataTable();" << endl;
          js << dom << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          js << jscolumns.str();
          js << dom << "_data.addRows(" << maxsz << ");" << endl;

          size_t iter = 0;
          for ( DData::const_iterator i = data.begin(); i != data.end(); ++i ) {
            time_t istop = i->first;
            struct tm *lt = localtime( &istop );
            jsdata << dom << "_data.setCell( " << iter << ", " << "0, ";
            jsdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;
            size_t colidx = 1;
            for ( set<string>::const_iterator d = devices.begin(); d != devices.end(); d++ ) {
              map<string,double>::const_iterator found = i->second.find( *d );
              if ( found != i->second.end() ) {
                jsdata << dom << "_data.setCell( " << iter << ", " << colidx << ", " << found->second << ");" << endl;
              } else {
                jsdata << dom << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
              }
              colidx++;
            }
            iter++;
          }
          js << jsdata.str();
          js << "var " << dom << "_options = {" << endl;
          js << "title: 'TCP server connection timeline (average number of connections)'," << endl;
          js << timeline_background_color << ", " << endl;
          js << "isStacked: true," << endl;
          js << "lineWidth: 1," << endl;
          js << "areaOpacity: 1.0," << endl;
          js << timeline_legend << ", " << endl;
          js << timeline_fontsize << "," << endl;
          js << timeline_chartarea << endl;
          js << "};" << endl;
          js << "var " << dom << " = new google.visualization.AreaChart(document.getElementById('" << dom << "'));" << endl;
          js << dom << ".draw(" << dom << "_data, " << dom << "_options);" << endl;

          jschart << js.str();
        }
      }

      void chartTCPClientTimeLine( const persist::Database &db, const string &dom ) {
        stringstream js;
        stringstream jscolumns;
        stringstream jsdata;
        persist::Query qry(db);
        qry.prepare( "select driver.a_istop, tcpkey.uid, tcpkey.ip, tcpkey.port, avg(case when esta is null then 0.0 else esta end) esta from "
                     " ( select bat.minid, bat.maxid, bat.a_istop, lim.tcpkey from "
                     "     (select min(id) minid, max(id) maxid, avg(istop) a_istop from snapshot where id>=:from and id <=:to group by istop/:bucket) bat, "
                     "     (select tcpkey from tcpclientstat s where s.snapshot>=:from and s.snapshot <=:to group by s.tcpkey order by sum(s.esta) desc limit 6) lim "
                     "   order by 3 "
                     " ) driver left outer join tcpclientstat tcs on tcs.snapshot>=driver.minid and tcs.snapshot<=driver.maxid and tcs.tcpkey=driver.tcpkey, "
                     " tcpkey where tcpkey.id=driver.tcpkey group by driver.a_istop, tcpkey.uid, tcpkey.ip, tcpkey.port order by 1" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        qry.bind( 3, snaprange.timeline_bucket );
        typedef map<double,map<string,double> > DData; // datetime (time_t), to pair ( device to utilization)
        DData data;
        size_t maxsz = 0;
        set<string> devices;
        while ( qry.step() ) {
          stringstream label;
          label << system::getUserName( qry.getInt( 1 ) ) << " " << (options.noresolv?qry.getText(2):resolveCacheIP(qry.getText(2))) << " " << net::getServiceName( qry.getInt( 3 ) );
          data[qry.getDouble(0)][ label.str() ] = qry.getDouble(4);
          devices.insert(label.str());
        }
        for ( set<string>::const_iterator d = devices.begin(); d != devices.end(); d++ ) {
          jscolumns << dom << "_data.addColumn( 'number', '" << *d << "' );" << endl;
        }
        maxsz = data.size();
        if ( maxsz > 0 ) {
          js << "var " << dom << "_data = new google.visualization.DataTable();" << endl;
          js << dom << "_data.addColumn( 'datetime', 'datetime' );" << endl;
          js << jscolumns.str();
          js << dom << "_data.addRows(" << maxsz << ");" << endl;

          size_t iter = 0;
          for ( DData::const_iterator i = data.begin(); i != data.end(); ++i ) {
            time_t istop = i->first;
            struct tm *lt = localtime( &istop );
            jsdata << dom << "_data.setCell( " << iter << ", " << "0, ";
            jsdata << "new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ) );" << endl;
            size_t colidx = 1;
            for ( set<string>::const_iterator d = devices.begin(); d != devices.end(); d++ ) {
              map<string,double>::const_iterator found = i->second.find( *d );
              if ( found != i->second.end() ) {
                jsdata << dom << "_data.setCell( " << iter << ", " << colidx << ", " << found->second << ");" << endl;
              } else {
                jsdata << dom << "_data.setCell( " << iter << ", " << colidx << ", " << 0.0 << ");" << endl;
              }
              colidx++;
            }
            iter++;
          }
          js << jsdata.str();
          js << "var " << dom << "_options = {" << endl;
          js << "title: 'TCP client connection timeline (average number of connections, stacked)'," << endl;
          js << timeline_background_color << ", " << endl;
          js << "isStacked: true," << endl;
          js << "lineWidth: 1," << endl;
          js << "areaOpacity: 1.0," << endl;
          js << timeline_legend << ", " << endl;
          js << timeline_fontsize << "," << endl;
          js << timeline_chartarea << endl;
          js << "};" << endl;
          js << "var " << dom << " = new google.visualization.AreaChart(document.getElementById('" << dom << "'));" << endl;
          js << dom << ".draw(" << dom << "_data, " << dom << "_options);" << endl;

          jschart << js.str();
        }
      }

      void htmlTimeLine( ostream &os, const string &id, const string &title ) {
        os << "<a class=\"toggle\" id=\"" << id << "_toggle\" onclick=\"toggleVisible( '" << id << "', '" << title << "' );\">&#xab; " << title << "</a>" << endl;
        os << "<div class=\"toggle\" id=\"" << id << "_togglediv\">" << endl;
        os << "<div class=\"chart\" id='" << id << "' style='width: " << max_chart_pixels + 100 << "px; height: " << timeline_chart_height << ";'></div>" << endl;
        os << "</div>" << endl;
      }

      void htmlTimeLines( const persist::Database &db ) {
        html << "<a class=\"anchor\" id=\"timelines\"></a><h1>Timelines</h1>" << endl;
        html << "<p>Each datapoint in the below charts is an average over " << util::TimeStrSec( snaprange.timeline_bucket ) << ".</p>" << endl;

        html << "<a class=\"anchor\" id=\"timeline_cpu\"></a><h2>CPU</h2>" << endl;
        chartCPUTimeLine( db, "cputimeline" );
        htmlTimeLine( html, "cputimeline", "CPU timeline" );

        html << "<a class=\"anchor\" id=\"timeline_sched\"></a><h2>Scheduler</h2>" << endl;
        chartSchedTimeLine( db, "forktimeline", "ctxswtimeline", "load5timeline" );
        htmlTimeLine( html, "load5timeline", "Load5 average timeline" );
        htmlTimeLine( html, "forktimeline", "Forks per second timeline" );
        htmlTimeLine( html, "ctxswtimeline", "Context switches per second timeline" );

        html << "<a class=\"anchor\" id=\"timeline_mem\"></a><h2>Memory</h2>" << endl;
        chartVMTimeLine( db, "vmtimeline", "swaptimeline" );
        htmlTimeLine( html, "vmtimeline", "Memory timeline" );
        htmlTimeLine( html, "swaptimeline", "Swap timeline" );
        chartPagingTimeLine( db, "majflttimeline", "pageintimeline", "pageouttimeline" );
        htmlTimeLine( html, "majflttimeline", "Major faults per second timeline" );
        htmlTimeLine( html, "pageintimeline", "page-ins per second timeline" );
        htmlTimeLine( html, "pageouttimeline", "page-outs per second timeline" );

        html << "<a class=\"anchor\" id=\"timeline_kernel\"></a><h2>Kernel resources</h2>" << endl;
        chartKernelTimeLine( db, "procstimeline", "usertimeline", "filestimeline", "inodestimeline" );
        htmlTimeLine( html, "procstimeline", "#processes timeline" );
        htmlTimeLine( html, "usertimeline", "users and logins timeline" );
        htmlTimeLine( html, "filestimeline", "open files timeline" );
        htmlTimeLine( html, "inodestimeline", "inodes timeline" );

        html << "<a class=\"anchor\" id=\"timeline_disk\"></a><h2>Disk</h2>" << endl;
        chartDiskTimeLine( db, "diskutiltimeline",
                               "diskbwrtimeline",
                              "diskbwwtimeline",
                              "diskiopsrtimeline",
                              "diskiopswtimeline",
                              "diskartmtimeline",
                              "diskawtmtimeline",
                              "disksvctmtimeline" );
        htmlTimeLine( html, "diskutiltimeline", "Disk utilization timeline" );
        htmlTimeLine( html, "diskbwrtimeline", "Disk read bandwidth timeline" );
        htmlTimeLine( html, "diskbwwtimeline", "Disk write bandwidth timeline" );
        htmlTimeLine( html, "diskiopsrtimeline", "Disk read IOPS timeline" );
        htmlTimeLine( html, "diskiopswtimeline", "Disk write IOPS timeline" );
        htmlTimeLine( html, "diskartmtimeline", "Disk average read time timeline" );
        htmlTimeLine( html, "diskawtmtimeline", "Disk average write time timeline" );
        htmlTimeLine( html, "disksvctmtimeline", "Disk average service time timeline" );

        html << "<a class=\"anchor\" id=\"timeline_mount\"></a><h2>Mountpoints</h2>" << endl;
        chartMountTimeLine( db, "mountpointutiltimeline",
                                "mountpointbwrtimeline",
                                "mountpointbwwtimeline",
                                "mountpointiopsrtimeline",
                                "mountpointiopswtimeline",
                                "mountpointartmtimeline",
                                "mountpointawtmtimeline",
                                "mountpointsvctmtimeline",
                                "mountpointusedtimeline" );
        htmlTimeLine( html, "mountpointutiltimeline", "Mountpoint utilization timeline" );
        htmlTimeLine( html, "mountpointbwrtimeline", "Mountpoint read bandwidth timeline" );
        htmlTimeLine( html, "mountpointbwwtimeline", "Mountpoint write bandwidth timeline" );
        htmlTimeLine( html, "mountpointiopsrtimeline", "Mountpoint read IOPS timeline" );
        htmlTimeLine( html, "mountpointiopswtimeline", "Mountpoint write IOPS timeline" );
        htmlTimeLine( html, "mountpointartmtimeline", "Mountpoint average read time timeline" );
        htmlTimeLine( html, "mountpointawtmtimeline", "Mountpoint average write time timeline" );
        htmlTimeLine( html, "mountpointsvctmtimeline", "Mountpoint average service time timeline" );
        htmlTimeLine( html, "mountpointusedtimeline", "Mountpoint used bytes timeline" );

        html << "<a class=\"anchor\" id=\"timeline_nic\"></a><h2>NICs</h2>" << endl;
        chartNICPacketTimeLine( db, "nicrxpkttimeline", "nictxpkttimeline" );
        htmlTimeLine( html, "nicrxpkttimeline", "NIC receive packet rate timeline" );
        htmlTimeLine( html, "nictxpkttimeline", "NIC transmit packet rate timeline" );

        chartNicBWTimeLine( db, "nicrxbwtimeline", "nictxbwtimeline" );
        htmlTimeLine( html, "nicrxbwtimeline", "NIC receive bandwidth timeline" );
        htmlTimeLine( html, "nictxbwtimeline", "NIC transmit bandwidth timeline" );

        html << "<a class=\"anchor\" id=\"timeline_tcpserver\"></a><h2>TCP server</h2>" << endl;
        chartTCPServerTimeLine( db, "tcpservertimeline" );
        htmlTimeLine( html, "tcpservertimeline", "TCP server timeline" );

        html << "<a class=\"anchor\" id=\"timeline_tcpclient\"></a><h2>TCP client</h2>" << endl;
        chartTCPClientTimeLine( db, "tcpclienttimeline" );
        htmlTimeLine( html, "tcpclienttimeline", "TCP client timeline" );
      }

      /**
       * map of weekday (Mon=0) to a (map of hour to value)
       */
      typedef map<int,double> HourValueMap;
      typedef HourValueMap::const_iterator HourValueMapCI;
      typedef map< int,HourValueMap> WeekHourMap;
      typedef WeekHourMap::const_iterator WeekHourMapCI;

      void htmlHeatMap( ostream &os, WeekHourMap &whmap, const string &title, const std::string &unit, double min, double max ) {
        os << "<br/>" << endl;
        os << "<table class=\"heatmap\">" << endl;
        os << "<tr><th colspan=\"25\">" << title;
        if ( unit != "" )
          os << " (" << unit << ")";
        os << "</th></tr>" << endl;
        os << "<tr><th></th><td class=\"min\" title=\"minimum value\"></td><td colspan=\"11\">" << util::NumStr( min, 3 ) <<
          "</td><td class=\"max\" title=\"maximum value\"></td><td colspan=\"11\">" << util::NumStr( max, 3 ) << "</td></tr>" << endl;
        for ( unsigned int dow = 0; dow < 7; dow++ ) {
          os << "<tr><th>";
          switch ( dow ) {
            case 0: os << "Mon" << endl; break;
            case 1: os << "Tue" << endl; break;
            case 2: os << "Wed" << endl; break;
            case 3: os << "Thu" << endl; break;
            case 4: os << "Fri" << endl; break;
            case 5: os << "Sat" << endl; break;
            case 6: os << "Sun" << endl; break;
          }
          os << "</th>" << endl;
          WeekHourMapCI w = whmap.find(dow);
          for ( unsigned int hod = 0; hod < 24; hod++ ) {
            HourValueMapCI h;
            if ( w != whmap.end() )  h = w->second.find(hod);
            if ( w != whmap.end() && h != w->second.end() ) {
              os <<  "<td class=\"level" << int(( (h->second-min)/ (max-min) )*25) << "\" title=\"" << util::NumStr( h->second, 3 ) << "\"></td>" << endl;
            } else os <<  "<td class=\"levelnodata\"></td>" << endl;
          }
          os << "</tr>";
        }
        os << "<tr><th></th>";
        for ( unsigned int hod = 0; hod < 24; hod++ ) {
          os << "<td>" << setw(2) << setfill('0') << hod << "</td>";
        }
        os << "</tr>" << endl;
        os << "</table>" << endl;
        os << "<br/>" << endl;
      }

      void CPUHeatMap( const persist::Database &db ) {
        WeekHourMap cpumap;
        double min = 1E99, max = -1E99;
        persist::Query qry(db);
        qry.prepare( "select strftime('%w',istop,'unixepoch','localtime'), strftime('%H',istop,'unixepoch','localtime'),avg(user_mode+system_mode+iowait_mode+nice_mode+irq_mode+softirq_mode+steal_mode) from v_cpuagstat where id>=:from and id <=:to group by strftime('%w',istop,'unixepoch','localtime'), strftime('%H',istop,'unixepoch','localtime') order by 1,2" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        while ( qry.step() ) {
          int dow = qry.getInt(0) + 6;
          if ( dow > 6 ) dow -= 7;
          (cpumap[dow])[qry.getInt(1)] = qry.getDouble(2);
          if ( qry.getDouble(2) > max ) max = qry.getDouble(2);
          if ( qry.getDouble(2) < min ) min = qry.getDouble(2);
        }

        htmlHeatMap( html, cpumap, "CPU utlization heatmap", "CPU time/real time", min, max);
      }

      void DiskHeatMap( const persist::Database &db ) {
        WeekHourMap diskmap;
        double min = 1E99, max = -1E99;
        persist::Query qry(db);
        qry.prepare( "select strftime('%w',snapshot.istop,'unixepoch','localtime'), strftime('%H',snapshot.istop,'unixepoch','localtime'),avg(iostat.util) from iostat, snapshot where snapshot.id = iostat.snapshot and snapshot.id>=:from and snapshot.id <=:to group by strftime('%w',snapshot.istop,'unixepoch','localtime'), strftime('%H',snapshot.istop,'unixepoch','localtime') order by 1,2" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        while ( qry.step() ) {
          int dow = qry.getInt(0) + 6;
          if ( dow > 6 ) dow -= 7;
          (diskmap[dow])[qry.getInt(1)] = qry.getDouble(2);
          if ( qry.getDouble(2) > max ) max = qry.getDouble(2);
          if ( qry.getDouble(2) < min ) min = qry.getDouble(2);
        }

        htmlHeatMap( html, diskmap, "Disk utlization heatmap", "disk IO time/real time", min, max);
      }

      void load5HeatMap( const persist::Database &db ) {
        WeekHourMap load5map;
        double min = 1E99, max = -1E99;
        persist::Query qry(db);
        qry.prepare( "select strftime('%w',snapshot.istop,'unixepoch','localtime'), strftime('%H',snapshot.istop,'unixepoch','localtime'),avg(load5) from snapshot,schedstat where snapshot.id=schedstat.snapshot and snapshot.id>=:from and snapshot.id <=:to group by strftime('%w',snapshot.istop,'unixepoch'), strftime('%H',snapshot.istop,'unixepoch') order by 1,2" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        while ( qry.step() ) {
          int dow = qry.getInt(0) + 6;
          if ( dow > 6 ) dow -= 7;
          (load5map[dow])[qry.getInt(1)] = qry.getDouble(2);
          if ( qry.getDouble(2) > max ) max = qry.getDouble(2);
          if ( qry.getDouble(2) < min ) min = qry.getDouble(2);
        }

        htmlHeatMap( html, load5map, "5 minute load average heatmap", "", min, max);
      }

      void NetPktHeatMap( const persist::Database &db ) {
        WeekHourMap netpktmap;
        double min = 1E99, max = -1E99;
        persist::Query qry(db);
        qry.prepare( "select strftime('%w',snapshot.istop,'unixepoch','localtime'), strftime('%H',snapshot.istop,'unixepoch','localtime'),avg(rxpkts+txpkts) from snapshot,netstat where snapshot.id=netstat.snapshot and snapshot.id>=:from and snapshot.id <=:to group by strftime('%w',snapshot.istop,'unixepoch'), strftime('%H',snapshot.istop,'unixepoch') order by 1,2" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        while ( qry.step() ) {
          int dow = qry.getInt(0) + 6;
          if ( dow > 6 ) dow -= 7;
          (netpktmap[dow])[qry.getInt(1)] = qry.getDouble(2);
          if ( qry.getDouble(2) > max ) max = qry.getDouble(2);
          if ( qry.getDouble(2) < min ) min = qry.getDouble(2);
        }

        htmlHeatMap( html, netpktmap, "network rx+tx packet rate heatmap", "packets/s", min, max);
      }

      void htmlHeatMaps( const persist::Database &db ) {
        html << "<a class=\"anchor\" id=\"heatmaps\"></a><h1>Heat maps</h1>" << endl;
        if ( snaprange.time_max - snaprange.time_min < 2*60*60 ) {
          html << "<p>Heatmaps require at least 2 hours of data.</p>" << endl;
        } else {
          html << "<p>CPU, disk, load and network activicty grouped by (day of week,hour of day).</p>" << endl;
          CPUHeatMap( db );
          DiskHeatMap( db );
          load5HeatMap( db );
          NetPktHeatMap( db );
        }
      }

      void htmlCmdCPUDetail( const string& cmdname, const persist::Database &db ) {
        stringstream js;
        stringstream ssdom;
        ssdom << "cmd" << jsIdFromString(cmdname) << "_cpu_timeline";
        persist::Query qry(db);
        qry.prepare( "select sub.istop, avg(sub.usercpu), avg(systemcpu), avg(iotime) from ( "
           "select istop, ifnull(usercpu,0) usercpu, ifnull(systemcpu,0) systemcpu, ifnull(iotime,0) iotime from snapshot "
           "left outer join (select snapshot,cmd.cmd,sum(usercpu) usercpu,sum(systemcpu) systemcpu, sum(iotime) iotime from procstat, cmd where procstat.cmd=cmd.id and cmd.cmd=:cmdname "
           "                  and procstat.snapshot>=:from and procstat.snapshot<=:to group by snapshot,cmd.cmd) procstat on snapshot.id=procstat.snapshot "
         ") sub "
         "group by sub.istop/:bucket "
         "order by 1" );
        qry.bind( 1, cmdname );
        qry.bind( 2, snaprange.snap_min );
        qry.bind( 3, snaprange.snap_max );
        qry.bind( 4, snaprange.timeline_bucket );
        int iter = 0;
        while ( qry.step() ) {
          if ( iter == 0 ) {
            js << "var " << ssdom.str() << "_data = google.visualization.arrayToDataTable([" << endl;
            js << "['datetime', 'usercpu', 'systemcpu', 'iotime' ]," << endl;

          } else {
            js << ",";
          }
          time_t istop = qry.getDouble(0);
          struct tm *lt = localtime( &istop );
          js << "[ new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ), ";
          js << qry.getDouble(1) << ", " << qry.getDouble(2) << ", " <<
             qry.getDouble(3) << " ]" << endl;
          iter++;
        }
        if ( iter ) {
          js << "]);" << endl;
          js << "var " << ssdom.str() << "_options = {" << endl;
          js << "title: '" << cmdname << " CPU timeline'," << endl;
          js << timeline_background_color << ", " << endl;
          js << "legend: {position: 'top' }," << endl;
          js << "fontSize: 10," << endl;
          js << "isStacked: true," << endl;
          js << "lineWidth: 0.2," << endl;
          js << "areaOpacity: 1.0," << endl;
          js << "hAxis: { title: 'datetime ', baselineColor: 'transparent' }," << endl;
          js << "vAxis: { title: 'time' }" << endl;
          js << "};" << endl;
          js << "var " << ssdom.str() << " = new google.visualization.AreaChart(document.getElementById('" << ssdom.str() << "'));" << endl;
          js << ssdom.str() << ".draw(" << ssdom.str() << "_data, " << ssdom.str() << "_options);" << endl;

          jschart << js.str();
          htmlTimeLine( html, ssdom.str(), cmdname + " CPU timeline" );
        }
      }

      void htmlCmdRSSDetail( const string& cmdname, const persist::Database &db ) {
        stringstream js;
        stringstream ssdom;
        ssdom << "cmd" << jsIdFromString(cmdname) << "_rss_timeline";
        persist::Query qry(db);
        qry.prepare( "select sub.istop, avg(sub.srss) from ( "
           "select istop, ifnull(srss,0) srss from snapshot "
           " left outer join (select snapshot,cmd.cmd,sum(rss) srss from procstat, cmd where procstat.cmd=cmd.id and cmd.cmd=:cmdname "
           "                  and procstat.snapshot>=:from and procstat.snapshot<=:to group by snapshot,cmd.cmd) procstat on snapshot.id=procstat.snapshot "
         ") sub "
         "group by sub.istop/:bucket "
         "order by 1" );

        qry.bind( 1, cmdname );
        qry.bind( 2, snaprange.snap_min );
        qry.bind( 3, snaprange.snap_max );
        qry.bind( 4, snaprange.timeline_bucket );
        int iter = 0;
        while ( qry.step() ) {
          if ( iter == 0 ) {
            js << "var " << ssdom.str() << "_data = google.visualization.arrayToDataTable([" << endl;
            js << "['datetime', 'rss' ]," << endl;

          } else {
            js << ",";
          }
          time_t istop = qry.getDouble(0);
          struct tm *lt = localtime( &istop );
          js << "[ new Date( " << lt->tm_year + 1900 << ", " << lt->tm_mon << ", " << lt->tm_mday << ", " << lt->tm_hour << ", " << lt->tm_min << ", " << lt->tm_sec << ", 0.0 ), ";
          js << qry.getDouble(1) * leanux::system::getPageSize() / 1024.0 << " ]" << endl;
          iter++;
        }
        if ( iter ) {
          js << "]);" << endl;
          js << "var " << ssdom.str() << "_options = {" << endl;
          js << "title: '" << cmdname << " RSS timeline'," << endl;
          js << timeline_background_color << ", " << endl;
          js << "legend: {position: 'none' }," << endl;
          js << "fontSize: 10," << endl;
          js << "lineWidth: 0.6," << endl;
          js << "hAxis: { title: 'datetime ', baselineColor: 'transparent' }," << endl;
          js << "vAxis: { title: 'KiB' }" << endl;
          js << "};" << endl;
          js << "var " << ssdom.str() << " = new google.visualization.LineChart(document.getElementById('" << ssdom.str() << "'));" << endl;
          js << ssdom.str() << ".draw(" << ssdom.str() << "_data, " << ssdom.str() << "_options);" << endl;

          jschart << js.str();
          htmlTimeLine( html, ssdom.str(), cmdname + " RSS timeline" );
        }
      }

      void htmlCmdUidDetail( const string& cmdname, const persist::Database &db ) {
        persist::Query qry(db);
        qry.prepare( "select uid, avg(usercpu),avg(systemcpu),avg(iotime) from procstat, cmd "
                     "where procstat.cmd=cmd.id and cmd.cmd=:cmdname "
                     "and snapshot>=:from and snapshot<=:to group by uid order by sum(usercpu),sum(systemcpu),sum(iotime) desc limit 10;" );
        qry.bind( 1, cmdname );
        qry.bind( 2, snaprange.snap_min );
        qry.bind( 3, snaprange.snap_max );
        html << "<table class=\"datatable\">" << endl;
        html << "<tr><th>user</th><th>usercpu</th><th>systemcpu</th><th>iotime</th></tr>" << endl;
        while ( qry.step() ) {
          html << "<tr><td>" << system::getUserName( qry.getInt(0) ) << "</td><td>" << util::NumStr(qry.getDouble(1), 4 ) << "</td><td>" <<
            util::NumStr(qry.getDouble(2), 4 ) << "</td><td>" << util::NumStr(qry.getDouble(3), 4 ) << "</td></tr>" << endl;
        }
        html << "</table>" << endl;
      }

      void htmlCmdDetails( const persist::Database &db ) {
        html << "<a class=\"anchor\" id=\"cmddetails\"></a><h1>Command details</h1>" << endl;
        persist::Query qry(db);
        qry.prepare( "select id, cmd from "
                     "(select cmd.id, cmd.cmd, sum(usercpu)/cnt.num usercpu,sum(systemcpu)/cnt.num systemcpu, sum(iotime)/cnt.num iotime "
                     " from procstat,cmd,(select count(1) num from snapshot where id>=:from and id <=:to) cnt where procstat.cmd=cmd.id "
                     " and snapshot>=:from and snapshot <=:to group by cmd.cmd) order by usercpu+systemcpu+iotime desc limit 15;" );
        qry.bind( 1, snaprange.snap_min );
        qry.bind( 2, snaprange.snap_max );
        while ( qry.step() ) {
          stringstream ss_link;
          stringstream ss_menu;
          ss_link << "cmddetail_" << qry.getText(1);
          ss_menu << qry.getText(1);
          menu_cmds.push_back( std::pair<string,string>( ss_link.str(), ss_menu.str() ) );
          html << "<a class=\"anchor\" id=\"" << ss_link.str() << "\"></a><h2>" << qry.getText(1) << "</h2>" << endl;
          htmlCmdUidDetail( qry.getText(1), db );
          htmlCmdCPUDetail( qry.getText(1), db );
          htmlCmdRSSDetail( qry.getText(1), db );
        }
      }


      bool parseArgs( int argc, char* argv[] ) {
        options.lardfile = "";
        options.htmlfile = "lard.html";
        options.trail = 0;
        options.begin = 0;
        options.end = 0;
        options.noresolv = false;
        options.version = false;
        options.help = false;
        int opt;
        string tmpstr;
        struct tm tmptm;
        while ( (opt = getopt( argc, argv, "f:o:m:b:e:nvh" ) ) != -1 ) {
          switch ( opt ) {
            case 'f':
              options.lardfile = optarg;
              break;
            case 'o':
              options.htmlfile = optarg;
              break;
            case 'm':
              options.trail = atoi(optarg);
              if ( options.trail == 0 ) {
                cerr << "invalid minutes trail value" << endl;
                return false;
              }
              break;
            case 'b':
              tmpstr = optarg;
              if ( strptime( tmpstr.c_str(), "%Y-%m-%d %H:%M:%S", &tmptm ) ) {
                options.begin = mktime( &tmptm );
              }
              if ( options.begin == 0 ) {
                cerr << "invalid begin value" << endl;
                return false;
              }
              break;
            case 'e':
              tmpstr = optarg;
              if ( strptime( tmpstr.c_str(), "%Y-%m-%d %H:%M:%S", &tmptm ) ) {
                options.end = mktime( &tmptm );
              }
              if ( options.end == 0 ) {
                cerr << "invalid end value" << endl;
                return false;
              }
              break;
            case 'n':
              options.noresolv = true;
              break;
            case 'v':
              options.version = true;
              break;
            case 'h':
              options.help = true;
              break;
            default:
              return false;
          };
        }
        if ( !options.help && !options.version && options.trail == 0 && options.end == 0 && options.begin == 0 ) {
          cerr << "specify at least one of -t, -b or -e to mark a time range" << endl;
          return false;
        }
        if ( !options.help && !options.version && options.lardfile == "" ) {
          cerr << "specify -f [lardfile]" << endl;
          return false;
        }
        return true;
      }

      void printVersion() {
         std::cout << LEANUX_VERSION << std::endl;
      }

      void printHelp() {
        std::cout << "lrep - " << LREP_DESCR << std::endl;
        std::cout << "  usage:" << std::endl;
        std::cout << "    lrep (-m minutes | -b begin -e end) [-n] -f database -o outfile" << std::endl;
      }

      int main( int argc, char* argv[] ) {
        try {
          leanux::init();
          if ( parseArgs( argc, argv ) ) {
            if ( options.version )
              printVersion();
            else if ( options.help )
              printHelp();
            else {
              persist::Database db( options.lardfile );
              cout << "using data from sqlite file '" << options.lardfile << "'" << endl;
              persist::Query qry(db);
              if ( options.trail > 0 ) {
                cout << "trail of last " << options.trail << " minutes" << endl;
                stringstream ss;
                ss << "'-" << options.trail << " minutes'";
                qry.prepare( "select min(id), datetime(min(istart),'unixepoch','localtime'), max(id), datetime(max(istop),'unixepoch','localtime'),min(istart),max(istop), count(id) "
                            "from snapshot where datetime(snapshot.istart,'unixepoch') >= datetime('now', " + ss.str() + " )" );

              } else if ( options.begin > 0 && options.end > 0 ) {
                cout << "interval from " << options.begin << " to " << options.end << endl;
                stringstream ss;
                qry.prepare( "select min(id), datetime(min(istart),'unixepoch','localtime'), max(id), datetime(max(istop),'unixepoch','localtime'),min(istart),max(istop), count(id) "
                            "from snapshot where snapshot.istart>=:begin and snapshot.istop<=:end" );
                qry.bind( 1, (long)options.begin );
                qry.bind( 2, (long)options.end );
              }
              if ( qry.step() ) {

                snaprange.snap_min = qry.getInt(0);
                snaprange.snap_max = qry.getInt(2);
                if ( snaprange.snap_max - snaprange.snap_min == 0 ) {
                  cerr << "no data found for the specified range " << snaprange.snap_max << endl;
                  return 1;
                }
                snaprange.local_time_min = qry.getText(1);
                snaprange.local_time_max = qry.getText(3);
                snaprange.time_min = qry.getInt(4);
                snaprange.time_max = qry.getInt(5);
                snaprange.snap_count = qry.getInt(6);
                snaprange.timeline_bucket = (snaprange.time_max - snaprange.time_min) / max_chart_pixels;
                if ( snaprange.timeline_bucket == 0 ) snaprange.timeline_bucket = 1;

                persist::Query qry2(db);
                qry2.prepare( "select count(1) from snapshot" );
                if ( qry2.step() ) {
                  snaprange.snaps_in_db = qry2.getInt(0);
                }
                qry2.reset();
                cout << "min snapshot=" << snaprange.snap_min << " " << snaprange.local_time_min << endl;
                cout << "max snapshot=" << snaprange.snap_max << " " << snaprange.local_time_max << endl;
                cout << "snapshot range spans " << snaprange.time_max-snaprange.time_min << " seconds, " << util::TimeStrSec( snaprange.time_max-snaprange.time_min ) << endl;

                util::Stopwatch sw;

                //if ( setpriority( PRIO_PROCESS, 0, 2 ) ) {
                //  cerr << "failed to lower scheduling priority: " << strerror( errno ) << endl;
                //  return 1;
                //}

                cout << "compute totals ..." << flush;
                computeTotals( db );
                cout << " done!" << endl << flush;

                cout << "snapshot details ..." << flush;
                htmlSnapDetails( db );
                cout << " done!" << endl << flush;

                cout << "system details ..." << flush;
                htmlSystemDetails();
                cout << " done!" << endl << flush;

                cout << "heat maps ..." << flush;
                htmlHeatMaps( db );
                cout << " done!" << endl << flush;

                cout << "report averages ..." << flush;
                htmlReportAverages( db );
                cout << " done!" << endl << flush;

                cout << "timelines ..." << flush;
                htmlTimeLines( db );
                cout << " done!" << endl << flush;

                cout << "disk details ..." << flush;
                htmlDiskDetails( db );
                cout << " done!" << endl << flush;

                cout << "cmd details ..." << flush;
                htmlCmdDetails( db );
                cout << " done!" << endl << flush;

                html << "<p class=\"foot\">This report is generated with lrep, part of the <a href=\"https://www.o-rho.com/leanux\">leanux</a> toolkit.</p>" << endl;

                // write the html document
                cout << "writing doc ..." << flush;
                doc.open( options.htmlfile.c_str() );
                doc << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" "
                        "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" << endl;
                doc << "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">" << endl;
                doc << "<head>" << endl;
                doc << "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />" << endl;
                doc << "<title>" << system::getNodeName() << " snapshot report</title>" << endl;
                doc << "<script type='text/javascript' src='https://www.gstatic.com/charts/loader.js'></script>" << endl;
                htmlCSS(doc);
                jsConst(doc);
                jsScript(doc);
                doc << "<script type='text/javascript'>" << endl;
                doc << "google.charts.load('current', {'packages':['corechart','bar']});" << endl;
                doc << "google.charts.setOnLoadCallback(drawChart);" << endl;
                doc << "function drawChart() {" << endl;
                doc << jschart.str();
                doc << "}" << endl;
                doc << "</script>" << endl;
                doc << "</head>" << endl;
                doc << "<body>" << endl;

                doc << "<ul class=\"menu\" style=\"z-index:9999;\">" << endl;
                doc << "<li class=\"menu\"><a href=\"#reportdetails\">Report details</a></li>" << endl;
                doc << "<li class=\"menu\"><a href=\"#nodedetails\">Node details</a></li>" << endl;
                doc << "<li class=\"menu\"><a href=\"#heatmaps\">Heat maps</a></li>" << endl;

                doc << "<li class=\"menu-dropdown\">" << endl;
                doc << "<a class=\"menu-dropbtn\" href=\"#reportaverage\">Report averages</a>" << endl;
                doc << "<div class=\"dropdown-content\">" << endl;
                doc << "<a href=\"#reportaverage_cpu\">CPU</a>" << endl;
                doc << "<a href=\"#reportaverage_mem\">Memory</a>" << endl;
                doc << "<a href=\"#reportaverage_disk\">Disks</a>" << endl;
                doc << "<a href=\"#reportaverage_mount\">Mountpoints</a>" << endl;
                doc << "<a href=\"#reportaverage_nic\">NICs</a>" << endl;
                doc << "<a href=\"#reportaverage_cmd\">Commands</a>" << endl;
                doc << "<a href=\"#reportaverage_user\">Users</a>" << endl;
                doc << "<a href=\"#reportaverage_tcpserver\">TCP server</a>" << endl;
                doc << "<a href=\"#reportaverage_tcpclient\">TCP client</a>" << endl;
                doc << "</div>" << endl;
                doc << "</li>" << endl;

                doc << "<li class=\"menu-dropdown\">" << endl;
                doc << "<a class=\"menu-dropbtn\" href=\"#timelines\">Time lines</a>" << endl;
                doc << "<div class=\"dropdown-content\">" << endl;
                doc << "<a href=\"#timeline_cpu\">CPU</a>" << endl;
                doc << "<a href=\"#timeline_sched\">Scheduler</a>" << endl;
                doc << "<a href=\"#timeline_mem\">Memory</a>" << endl;
                doc << "<a href=\"#timeline_kernel\">Kernel resources</a>" << endl;
                doc << "<a href=\"#timeline_disk\">Disks</a>" << endl;
                doc << "<a href=\"#timeline_mount\">Mountpoints</a>" << endl;
                doc << "<a href=\"#timeline_nic\">NICs</a>" << endl;
                doc << "<a href=\"#timeline_tcpserver\">TCP server</a>" << endl;
                doc << "<a href=\"#timeline_tcpclient\">TCP client</a>" << endl;
                doc << "</div>" << endl;
                doc << "</li>" << endl;

                doc << "<li class=\"menu-dropdown\">" << endl;
                doc << "<a class=\"menu-dropbtn\" href=\"#diskdetails\">Disk details</a>" << endl;
                doc << "<div class=\"dropdown-content\">" << endl;
                for ( list<pair<string,string> >::const_iterator i = menu_disks.begin(); i != menu_disks.end(); i++ ) {
                  doc << "<a href=\"#" << i->first << "\">" << i->second << "</a>" << endl;
                }
                doc << "</div>" << endl;
                doc << "</li>" << endl;

                doc << "<li class=\"menu-dropdown\">" << endl;
                doc << "<a class=\"menu-dropbtn\" href=\"#cmddetails\">Command details</a>" << endl;
                doc << "<div class=\"dropdown-content\">" << endl;
                for ( list<pair<string,string> >::const_iterator i = menu_cmds.begin(); i != menu_cmds.end(); i++ ) {
                  doc << "<a href=\"#" << i->first << "\">" << i->second << "</a>" << endl;
                }
                doc << "</div>" << endl;
                doc << "</li>" << endl;

                doc << "</ul>" << endl;

                doc << "<div style=\"padding:20px;margin-top:30px;\">" << endl;

                doc << html.str();
                doc << "</div>" << endl;
                doc << "</body>" << endl;
                doc << "</html>" << endl;
                doc.close();
                cout << " done!" << endl << flush;
                cout << fixed << setprecision(2) << sw.getElapsedSeconds() << " seconds" << endl;
              }
            }
          }
        }
        catch ( const Oops &oops ) {
          cerr << oops.getMessage() << endl;
          return 2;
        }
        catch ( std::exception &e ) {
          cerr << "exception " << e.what() << " (" << typeid(e).name() << ")" << endl;
          return 3;
        }
        catch ( ... ) {
          cerr << "undetermined exception" << endl;
          return 4;
        }
        return 0;
      }

    }; // namespace lrep
  }; // namespace tools
}; // namespace leanux

int main( int argc, char* argv[] ) {
  return leanux::tools::lrep::main( argc, argv );
}
