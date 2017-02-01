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

#include "oops.hpp"
#include "configfile.hpp"
#include "device.hpp"
#include "net.hpp"
#include "system.hpp"
#include "pci.hpp"
#include "tabular.hpp"
#include "usb.hpp"
#include "util.hpp"
#include "leanux-config.hpp"
#include "lard-config.hpp"
#include "natsort.hpp"
#include <algorithm>
#include <iostream>
#include <list>
#include <set>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <string.h>
#include "lar_snap.hpp"
#include "lar_schema.hpp"

#include  <stdio.h>
#include  <signal.h>

/**
 * @file
 * lar tool c++ source file.
 */


namespace leanux {
  namespace tools {
    namespace lard {

      /**
       * when this becomes true, lard stops.
       * @see interrupt_handler( int sig ) and mainLoop()
       */
      bool stopped = false;

      bool was_busy = false;

      const std::string pid_name = "lard_pid";

      struct Options {
        /** sqlite database file */
        std::string database;
        /** configuration file */
        std::string config;
      };

      Options options;

      int wait_log_handler( void* p, int count ) {
        was_busy = true;
        if ( count % 30 == 0 ) {
          std::stringstream ss;
          ss << "sampling blocked by SQLITE_BUSY (" << count << " seconds)";
          sysLog( LOG_WARN, util::ConfigFile::getConfig()->getIntValue("LOG_LEVEL"), ss.str() );
        }
        util::Sleep( 1, 0 );
        return 1;
      }

      void interrupt_handler( int sig ) {
        switch ( sig ) {
          case SIGINT:
          case SIGQUIT:
          case SIGTERM:
            stopped = true;
            break;
        }
        std::stringstream ss;
        ss << "received signal " << sig;
        sysLog( LOG_STAT, util::ConfigFile::getConfig()->getIntValue("LOG_LEVEL"), ss.str() );
      };

      /**
       * set page_size and enable WAL mode.
       */
      void initDB() {
        persist::Database enablewal(  options.database );
        {
          persist::DDL ddl( enablewal );
          std::stringstream ss;
          ss << "PRAGMA main.page_size=" << util::ConfigFile::getConfig()->getValue("DATABASE_PAGE_SIZE");
          ddl.prepare( ss.str() );
          ddl.execute();
          ddl.close();
          persist::Query query( enablewal );
          query.prepare( "PRAGMA journal_mode=wal" );
          query.step();
          query.close();
        }
      }

      long ensureSinglePid( persist::Database &db ) {
        try {
          persist::Query qry(db);
          qry.prepare( "SELECT count(name) FROM sqlite_master WHERE type='table' AND name='status'" );
          if ( qry.step() ) {
            if ( qry.getInt(0) == 0 ) {
              createTableStatus( db );
            }
          }
          qry.reset();

          qry.prepare( "SELECT value FROM status WHERE name=:name" );
          qry.bind(1,pid_name);
          if ( qry.step() ) {
            long db_pid = qry.getInt(0);
            process::ProcPidStat stat;
            if ( process::getProcPidStat( db_pid, stat ) ) {
              if ( stat.comm == "lard" ) {
                return db_pid;
              }
            }
            persist::DML dml(db);
            dml.prepare( "UPDATE status SET value=:value WHERE name=:name" );
            dml.bind(1,getpid());
            dml.bind(2,pid_name);
            dml.execute();
            dml.close();
          } else {
            persist::DML dml(db);
            dml.prepare( "INSERT INTO status (name,value) VALUES (:name,:value)" );
            dml.bind( 1, pid_name );
            dml.bind( 2, getpid() );
            dml.execute();
            dml.close();
          }
        }
        catch ( const Oops &oops ) {
          sysLog( LOG_ERR, util::ConfigFile::getConfig()->getIntValue("LOG_LEVEL"), oops.getMessage() );
        }
        return 0;
      }

      void mainLoop() {

        persist::Database::softHeapLimit(util::ConfigFile::getConfig()->getIntValue("SQLITE_SOFT_HEAPLIMIT"));

        initDB();
        leanux::persist::Database db( options.database, wait_log_handler );
        long pid = ensureSinglePid( db );
        if ( pid != 0 ) {
          std::stringstream ss;
          ss << "pid " << pid << " is already controlling '" << options.database << "'";
          sysLog( LOG_ERR, util::ConfigFile::getConfig()->getIntValue("LOG_LEVEL"), ss.str() );
          return;
        }
        {
          persist::Query query( db );
          query.prepare( "PRAGMA journal_size_limit=0" );
          query.step();
          query.close();
        }
        db.enableForeignKeys();
        createSchema( db );
        updateSchema( db );
        std::stringstream ss;
        ss << "sampling to " << options.database << " version " << db.getUserVersion();
        sysLog( LOG_STAT, util::ConfigFile::getConfig()->getIntValue("LOG_LEVEL"), ss.str() );
        double sample_interval = util::ConfigFile::getConfig()->getIntValue("SNAPSHOT_INTERVAL");
        double maintenance_interval = util::ConfigFile::getConfig()->getIntValue("MAINTENANCE_INTERVAL") * 60;
        long snapshot_checkpoint = util::ConfigFile::getConfig()->getIntValue("SNAPSHOT_CHECKPOINT");

        util::Stopwatch sample_timer;
        util::Stopwatch maintenance_timer;
        maintenance_timer.start();
        long checkpoint_counter = 0;

        TimeSnap timesnap;
        CPUSnap cpusnap;
        IOSnap iosnap;
        SchedSnap schedsnap;
        NetSnap netsnap;
        VMSnap vmsnap;
        ProcSnap procsnap;
        ResSnap ressnap;
        MountSnap mountsnap;
        TCPEstaSnap tcpestasnap;
        timesnap.startSnap();
        cpusnap.startSnap();
        iosnap.startSnap();
        schedsnap.startSnap();
        netsnap.startSnap();
        vmsnap.startSnap();
        procsnap.startSnap();
        ressnap.startSnap();
        mountsnap.startSnap();
        tcpestasnap.startSnap();

        while ( !stopped ) {

          sample_timer.start();

          if ( maintenance_timer.getElapsedSeconds() > maintenance_interval ) {
            sysLog( LOG_DEBUG, util::ConfigFile::getConfig()->getIntValue("LOG_LEVEL"), "maintenance: cleanup, checkpoint, vacuum, analyze" );
            db.begin();
            applyRetention( db );
            db.commit();
            shrinkDB(db, options.database );
            vacuumAnalyze( db );
            db.checkPointPassive();
            std::stringstream ss;
            ss << "sqlite memory used: " << util::ByteStr( persist::Database::memUsed(), 2 );
            ss << ", highwater: " << util::ByteStr( persist::Database::memHighWater(), 2 );
            ss << ", heap softlimit: " << util::ByteStr( persist::Database::softHeapLimit(-1), 2 );
            sysLog( LOG_DEBUG, util::ConfigFile::getConfig()->getIntValue("LOG_LEVEL"), ss.str() );
            maintenance_timer.start();
          }

          while ( sample_timer.getElapsedSeconds() < sample_interval && !stopped ) {
            leanux::util::Sleep( 0, 400 * 1000 * 1000 );
          }

          timesnap.stopSnap();

          if ( timesnap.getSeconds() > 0.0 ) {
            db.begin();
            long snapid = timesnap.storeSnap( db, 0, 0 );
            double timesnap_seconds = timesnap.getSeconds();
            timesnap.startSnap();

            schedsnap.stopSnap();
            schedsnap.storeSnap( db, snapid, timesnap_seconds );
            schedsnap.startSnap();

            netsnap.stopSnap();
            netsnap.storeSnap( db, snapid, timesnap_seconds );
            netsnap.startSnap();

            vmsnap.stopSnap();
            vmsnap.storeSnap( db, snapid, timesnap_seconds );
            vmsnap.startSnap();

            ressnap.stopSnap();
            ressnap.storeSnap( db, snapid, timesnap_seconds );
            ressnap.startSnap();

            mountsnap.stopSnap();
            mountsnap.storeSnap( db, snapid, timesnap_seconds );
            mountsnap.startSnap();

            tcpestasnap.stopSnap();
            tcpestasnap.storeSnap( db, snapid, timesnap_seconds );
            tcpestasnap.startSnap();

            procsnap.stopSnap();
            procsnap.storeSnap( db, snapid, timesnap_seconds );
            procsnap.startSnap();

            iosnap.stopSnap();
            iosnap.storeSnap( db, snapid, timesnap_seconds );
            iosnap.startSnap();

            cpusnap.stopSnap();
            cpusnap.storeSnap( db, snapid, timesnap_seconds );
            cpusnap.startSnap();

            db.commit();
            checkpoint_counter++;
            std::stringstream ss;
            ss << "committed snapshot " << snapid;
            sysLog( LOG_DEBUG, util::ConfigFile::getConfig()->getIntValue("LOG_LEVEL"), ss.str() );
          }
          if ( was_busy ) {
            sysLog( LOG_STAT, util::ConfigFile::getConfig()->getIntValue("LOG_LEVEL"), "sampling resumed" );
            was_busy = false;
          }

          if ( checkpoint_counter >= snapshot_checkpoint ) {
            sysLog( LOG_DEBUG, util::ConfigFile::getConfig()->getIntValue("LOG_LEVEL"), "issuing checkpoint" );
            db.checkPointPassive();
            checkpoint_counter = 0;
          }
        }
        try {
          persist::DML dml(db);
          dml.prepare( "DELETE FROM status WHERE name=:name" );
          dml.bind( 1, pid_name );
          dml.execute();
        }
        catch ( const Oops &oops ) {
        }
        db.checkPointTruncate();
        sysLog( LOG_STAT, util::ConfigFile::getConfig()->getIntValue("LOG_LEVEL"), "stopped" );
      }

      bool parseArgs( int argc, char* argv[] ) {
        options.database = "";
        options.config = "";
        int opt;
        while ( (opt = getopt( argc, argv, "f:c:" ) ) != -1 ) {
          switch ( opt ) {
            case 'f':
              options.database = optarg;
              break;
            case 'c':
              options.config = optarg;
              break;
            default:
              return false;
          };
        }
        if ( options.database == "" && options.config == "" ) {
          std::cerr << "specify both -f databasefile -c configfile" << std::endl;
          return false;
        }
        return true;
      }

      /**
       * Entry point.
       * @param argc number of command line arguments.
       * @param argv command line arguments.
       * @return 0 on succes, 1 on Oops, 2 on other errors.
       */
      int main( int argc, char* argv[] ) {
        if ( parseArgs( argc, argv ) ) {

          try {
            signal(SIGINT, interrupt_handler);
            signal(SIGQUIT, interrupt_handler);
            signal(SIGTERM, interrupt_handler);
            if ( daemon(1,1) ) {
              sysLog( LOG_ERR, 0, "daemon call failed" );
              return 2;
            }
            leanux::init();

            util::ConfigFile::declareParameter( "DATABASE_PAGE_SIZE", LARD_CONF_DATABASE_PAGE_SIZE_DEFAULT, LARD_CONF_DATABASE_PAGE_SIZE_DESCR, LARD_CONF_DATABASE_PAGE_SIZE_COMMENT );
            util::ConfigFile::declareParameter( "MAX_DISKS", LARD_CONF_MAX_DISKS_DEFAULT, LARD_CONF_MAX_DISKS_DESCR, LARD_CONF_MAX_DISKS_COMMENT );
            util::ConfigFile::declareParameter( "MAX_MOUNTS", LARD_CONF_MAX_MOUNTS_DEFAULT, LARD_CONF_MAX_MOUNTS_DESCR, LARD_CONF_MAX_MOUNTS_COMMENT );
            util::ConfigFile::declareParameter( "MAX_PROCESSES", LARD_CONF_MAX_PROCESSES_DEFAULT, LARD_CONF_MAX_PROCESSES_DESCR, LARD_CONF_MAX_PROCESSES_COMMENT );
            util::ConfigFile::declareParameter( "RETAIN_DAYS", LARD_CONF_RETAIN_DAYS_DEFAULT, LARD_CONF_RETAIN_DAYS_DESCR, LARD_CONF_RETAIN_DAYS_COMMENT );
            util::ConfigFile::declareParameter( "MAX_DB_SIZE", LARD_CONF_MAX_DB_SIZE_DEFAULT, LARD_CONF_MAX_DB_SIZE_DESCR, LARD_CONF_MAX_DB_SIZE_COMMENT );
            util::ConfigFile::declareParameter( "SNAPSHOT_CHECKPOINT", LARD_CONF_SNAPSHOT_CHECKPOINT_DEFAULT, LARD_CONF_SNAPSHOT_CHECKPOINT_DESCR, LARD_CONF_SNAPSHOT_CHECKPOINT_COMMENT );
            util::ConfigFile::declareParameter( "SNAPSHOT_INTERVAL", LARD_CONF_SNAPSHOT_INTERVAL_DEFAULT, LARD_CONF_SNAPSHOT_INTERVAL_DESCR, LARD_CONF_SNAPSHOT_INTERVAL_COMMENT );
            util::ConfigFile::declareParameter( "MAINTENANCE_INTERVAL", LARD_CONF_MAINTENANCE_INTERVAL_DEFAULT, LARD_CONF_MAINTENANCE_INTERVAL_DESCR, LARD_CONF_MAINTENANCE_INTERVAL_COMMENT );
            util::ConfigFile::declareParameter( "LOG_LEVEL", LARD_CONF_LOG_LEVEL_DEFAULT, LARD_CONF_LOG_LEVEL_DESCR, LARD_CONF_LOG_LEVEL_COMMENT );
            util::ConfigFile::declareParameter( "SQLITE_SOFT_HEAPLIMIT", LARD_CONF_SQLITE_SOFT_HEAPLIMIT_DEFAULT, LARD_CONF_SQLITE_SOFT_HEAPLIMIT_DESCR, LARD_CONF_SQLITE_SOFT_HEAPLIMIT_COMMENT );
            util::ConfigFile::declareParameter( "COMMAND_ARGS_IGNORE", LARD_CONF_COMMAND_ARGS_IGNORE_DEFAULT, LARD_CONF_COMMAND_ARGS_IGNORE_DESCR, LARD_CONF_COMMAND_ARGS_IGNORE_COMMENT );
            util::ConfigFile::setConfig( "lard", options.config );
            util::ConfigFile::getConfig()->write();

            mainLoop();
          }
          catch ( const leanux::Oops &oops ) {
            sysLog( LOG_ERR, 0, oops.getMessage() + " - terminated" );
            return 1;
          }
          catch ( ... ) {
            sysLog( LOG_ERR, 0, "unhandled exception - terminated" );
            return 2;
          }
          util::ConfigFile::releaseConfig();
          return 0;
        } else return 1;
      }

    }; // namespace lard
  }; // namespace tools
}; // namespace leanux


int main( int argc, char* argv[] ) {
  return leanux::tools::lard::main( argc, argv );
}
