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
 * lar tool sqlite3 database schema c++ source file.
 */

#include "lar_schema.hpp"
#include "lar_snap.hpp"
#include "oops.hpp"
#include "persist.hpp"
#include "configfile.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sstream>

namespace leanux {
  namespace tools {
    namespace lard {

      int schema_version = 1976;

      void createStatusTable( persist::Database &db ) {
        persist::DDL ddl( db );
        ddl.prepare( "CREATE TABLE IF NOT EXISTS status ( name TEXT PRIMARY KEY, value TEXT NOT NULL)" );
        ddl.execute();
      }

      void createSchema( persist::Database &db ) {
        persist::DDL ddl( db );
        createStatusTable( db );
        ddl.prepare( "CREATE TABLE IF NOT EXISTS snapshot (\n"
                     "  id     INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, -- monotonically increasing\n"
                     "  istart INTEGER NOT NULL,  -- start time in seconds since unixepoch\n"
                     "  istop  INTEGER NOT NULL,  -- stop time in seconds since unixepoch\n"
                     "  label  TEXT DEFAULT NULL, -- snapshot label (or NULL)\n"
                     "  UNIQUE(label)\n"
                     ")" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE INDEX IF NOT EXISTS i_stop_id ON snapshot( istop, id )" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE TABLE IF NOT EXISTS cpustat (\n"
                     "  snapshot     INTEGER NOT NULL, -- snapshot id\n"
                     "  phyid        INTEGER NOT NULL, -- physical id\n"
                     "  coreid       INTEGER NOT NULL, -- core id\n"
                     "  logical      INTEGER NOT NULL, -- logical cpu number\n"
                     "  user_mode    REAL NOT NULL,    -- average user mode cpu seconds/second\n"
                     "  system_mode  REAL NOT NULL,    -- average system mode cpu seconds/second\n"
                     "  iowait_mode  REAL NOT NULL,    -- average iowait mode cpu seconds/second\n"
                     "  nice_mode    REAL NOT NULL,    -- average nice mode cpu seconds/second\n"
                     "  irq_mode     REAL NOT NULL,    -- average irq mode cpu seconds/second\n"
                     "  softirq_mode REAL NOT NULL,    -- average softirq mode cpu seconds/second\n"
                     "  steal_mode   REAL NOT NULL,    -- average steal mode cpu seconds/second\n"
                     "  PRIMARY KEY (snapshot,phyid,coreid,logical),\n"
                     "  FOREIGN KEY (snapshot) REFERENCES snapshot(id)\n"
                     ")" );
        ddl.execute();
        ddl.reset();
        ddl.prepare( "CREATE VIEW IF NOT EXISTS v_cpustat AS \n"
                     "SELECT\n"
                     "  snapshot.id id,\n"
                     "  datetime(snapshot.istart,'unixepoch') istart,\n"
                     "  datetime(snapshot.istop,'unixepoch') istop,\n"
                     "  cpustat.phyid,\n"
                     "  cpustat.coreid,\n"
                     "  cpustat.logical,\n"
                     "  cpustat.user_mode,\n"
                     "  cpustat.system_mode,\n"
                     "  cpustat.iowait_mode,\n"
                     "  cpustat.nice_mode,\n"
                     "  cpustat.irq_mode,\n"
                     "  cpustat.softirq_mode,\n"
                     "  cpustat.steal_mode \n"
                     "FROM\n"
                     "  snapshot,\n"
                     "  cpustat\n"
                     "WHERE\n"
                     "  cpustat.snapshot=snapshot.id\n" );
        ddl.execute();
        ddl.reset();
        ddl.prepare( "CREATE VIEW IF NOT EXISTS v_cpuagstat AS -- aggregated sums over logical cpu's\n"
                     "SELECT\n"
                     "  snapshot.id id,\n"
                     "  snapshot.istart istart,\n"
                     "  snapshot.istop istop,\n"
                     "  count(distinct cpustat.phyid) numphy,\n"
                     "  count(distinct cpustat.coreid) numcore,\n"
                     "  count(distinct cpustat.logical) numcpu,\n"
                     "  sum(cpustat.user_mode) user_mode,\n"
                     "  sum(cpustat.system_mode) system_mode,\n"
                     "  sum(cpustat.iowait_mode) iowait_mode,\n"
                     "  sum(cpustat.nice_mode) nice_mode,\n"
                     "  sum(cpustat.irq_mode) irq_mode,\n"
                     "  sum(cpustat.softirq_mode) softirq_mode,\n"
                     "  sum(cpustat.steal_mode) steal_mode \n"
                     "FROM\n"
                     "  snapshot,\n"
                     "  cpustat\n"
                     "WHERE\n"
                     "  cpustat.snapshot=snapshot.id\n"
                     "GROUP BY id,istart,istop" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE TABLE IF NOT EXISTS schedstat (\n"
                     "  snapshot INTEGER PRIMARY KEY NOT NULL, -- snapshot id\n"
                     "  forks    REAL NOT NULL,                -- average forks/second\n"
                     "  ctxsw    REAL NOT NULL,                -- average context switches/second\n"
                     "  load5    REAL NOT NULL,                -- 5 minute load average at snapshot end\n"
                     "  load10   REAL NOT NULL,                -- 10 minute load average at snapshot end\n"
                     "  load15   REAL NOT NULL,                -- 15 minute load average at snapshot end\n"
                     "  runq     REAL NOT NULL,                -- run queue at snapshot end\n"
                     "  blockq   REAL NOT NULL,                -- block queue at snapshot end\n"
                     "  FOREIGN KEY (snapshot) REFERENCES snapshot(id)\n"
                     ")" );
        ddl.execute();
        ddl.reset();
        ddl.prepare( "CREATE VIEW IF NOT EXISTS v_schedstat AS \n"
                     "SELECT\n"
                     "  snapshot.id,\n"
                     "  datetime(snapshot.istart,'unixepoch') istart,\n"
                     "  datetime(snapshot.istop,'unixepoch') istop,\n"
                     "  schedstat.forks,\n"
                     "  schedstat.ctxsw,\n"
                     "  schedstat.load5,\n"
                     "  schedstat.load10,\n"
                     "  schedstat.load15,\n"
                     "  schedstat.runq,\n"
                     "  schedstat.blockq \n"
                     "FROM\n"
                     "  snapshot,\n"
                     "  schedstat\n"
                     "WHERE\n"
                     "  schedstat.snapshot=snapshot.id\n" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE TABLE IF NOT EXISTS disk (\n"
                     "  id        INTEGER PRIMARY KEY NOT NULL,\n"
                     "  device    TEXT NOT NULL, -- disk device name\n"
                     "  wwn       TEXT NOT NULL, -- disk wwn\n"
                     "  model     TEXT NOT NULL, -- disk model\n"
                     "  syspath   TEXT NOT NULL  -- disk sysfs (hardware) path\n"
                     ")" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE TABLE IF NOT EXISTS iostat (\n"
                     "  snapshot INTEGER NOT NULL, -- snapshot id\n"
                     "  disk     INTEGER NOT NULL, -- disk id\n"
                     "  util     REAL NOT NULL,    -- util seconds/second\n"
                     "  svctm    REAL NOT NULL,    -- service time in seconds\n"
                     "  rs       REAL NOT NULL,    -- average reads per second\n"
                     "  ws       REAL NOT NULL,    -- average writes per second\n"
                     "  rbs      REAL NOT NULL,    -- average read bytes per second\n"
                     "  wbs      REAL NOT NULL,    -- average write bytes per second\n"
                     "  artm     REAL NOT NULL,    -- average read time in seconds (svctm + queue time)\n"
                     "  awtm     REAL NOT NULL,    -- averate write time in seconds (svctm + queue time)\n"
                     "  qsz      REAL NOT NULL,    -- disk queue size at snapshot end\n"
                     "  PRIMARY KEY (snapshot,disk),\n"
                     "  FOREIGN KEY (disk) REFERENCES disk(id),\n"
                     "  FOREIGN KEY (snapshot) REFERENCES snapshot(id)\n"
                     ")" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE INDEX IF NOT EXISTS i_iostat_disk ON iostat( disk )" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE VIEW IF NOT EXISTS v_iostat AS \n"
                     "SELECT\n"
                     "  snapshot.id,\n"
                     "  datetime(snapshot.istart,'unixepoch') istart,\n"
                     "  datetime(snapshot.istop,'unixepoch') istop,\n"
                     "  iostat.disk,\n"
                     "  iostat.util,\n"
                     "  iostat.svctm,\n"
                     "  iostat.rs,\n"
                     "  iostat.ws,\n"
                     "  iostat.rbs,\n"
                     "  iostat.wbs,\n"
                     "  iostat.artm,\n"
                     "  iostat.awtm,\n"
                     "  iostat.qsz \n"
                     "FROM\n"
                     "  iostat,\n"
                     "  snapshot\n"
                     "WHERE\n"
                     "  iostat.snapshot=snapshot.id\n" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE TABLE IF NOT EXISTS nic (\n"
                     "  id      INTEGER PRIMARY KEY NOT NULL, -- nic primary key\n"
                     "  device  TEXT NOT NULL, -- nic device name\n"
                     "  mac     TEXT NOT NULL, -- nic MAC address\n"
                     "  syspath TEXT NOT NULL, -- nic sysfs path\n"
                     "  UNIQUE (mac,syspath)\n"
                     ")" );
        ddl.execute();
        ddl.reset();
        ddl.prepare( "CREATE TABLE IF NOT EXISTS netstat (\n"
                     "  snapshot INTEGER NOT NULL, -- snapshot id\n"
                     "  nic      INTEGER NOT NULL,      -- nic id\n"
                     "  rxbs     REAL NOT NULL,        -- average bytes received per second\n"
                     "  txbs     REAL NOT NULL,        -- average bytes transmitted per second\n"
                     "  rxpkts   REAL NOT NULL,      -- average received packets per second\n"
                     "  txpkts   REAL NOT NULL,      -- average transmitted packets per second\n"
                     "  rxerrs   REAL NOT NULL,      -- average receive errors per seoncd\n"
                     "  txerrs   REAL NOT NULL,      -- average transmit errors per second\n"
                     "  PRIMARY KEY (snapshot,nic),\n"
                     "  FOREIGN KEY (nic) REFERENCES nic(id),\n"
                     "  FOREIGN KEY (snapshot) REFERENCES snapshot(id)\n"
                     ")" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE INDEX IF NOT EXISTS i_netstat_nic ON netstat( nic )" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE TABLE IF NOT EXISTS vmstat (\n"
                     "  snapshot INTEGER PRIMARY KEY NOT NULL, -- snapshot id\n"
                     "  realmem  REAL NOT NULL, -- real memory in bytes\n"
                     "  unused   REAL NOT NULL, -- unused real memory in bytes\n"
                     "  commitas REAL NOT NULL, -- committed address space in bytes\n"
                     "  anon     REAL NOT NULL, -- anonymous memory in bytes\n"
                     "  file     REAL NOT NULL, -- file (page cache) memory in bytes\n"
                     "  shmem    REAL NOT NULL, -- shared memory in bytes\n"
                     "  slab     REAL NOT NULL, -- kernel slab in bytes\n"
                     "  pagetbls REAL NOT NULL, -- page tables in bytes\n"
                     "  dirty    REAL NOT NULL, -- dirty file pages in bytes\n"
                     "  pgins    REAL NOT NULL, -- average pages paged in per second\n"
                     "  pgouts   REAL NOT NULL, -- average pages paged out per second\n"
                     "  swpins   REAL NOT NULL, -- average pages swapped in per second\n"
                     "  swpouts  REAL NOT NULL, -- average pages swapped out per second\n"
                     "  hptotal  REAL NOT NULL, -- total huge page size in bytes\n"
                     "  hprsvd   REAL NOT NULL, -- reserved huge pages in bytes\n"
                     "  hpfree   REAL NOT NULL, -- free huge pages in bytes\n"
                     "  thpanon  REAL NOT NULL, -- anonymous transparent hugepages in bytes\n"
                     "  mlock    REAL NOT NULL, -- mlocked memory in bytes\n"
                     "  mapped   REAL NOT NULL, -- mapped memory in bytes\n"
                     "  swpused  REAL NOT NULL, -- used swap space in bytes\n"
                     "  swpsize  REAL NOT NULL, -- total swap space in bytes\n"
                     "  minflts  REAL NOT NULL, -- average minor faults per second\n"
                     "  majflts  REAL NOT NULL, -- average major faults per second\n"
                     "  allocs   REAL NOT NULL, -- average memory allocations per second\n"
                     "  frees    REAL NOT NULL, -- average memory frees per second\n"
                     "  FOREIGN KEY (snapshot)  REFERENCES snapshot(id)\n"
                     ")" );
        ddl.execute();
        ddl.reset();
        ddl.prepare( "CREATE TABLE IF NOT EXISTS cmd (\n"
                     "  id  INTEGER PRIMARY KEY NOT NULL, -- command id\n"
                     "  cmd TEXT not null,                -- command name\n"
                     "  args TEXT not null,               -- command arguments\n"
                     "  UNIQUE (cmd,args)\n"
                     ")" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE TABLE IF NOT EXISTS wchan (\n"
                     "  id    INTEGER NOT NULL, -- wchan id\n"
                     "  wchan TEXT NOT NULL,    -- wchan\n"
                     "  PRIMARY KEY (id)\n"
                     ")" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "REPLACE INTO wchan (id, wchan) VALUES (0,'none')" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE TABLE IF NOT EXISTS procstat (\n"
                     "  snapshot  INTEGER NOT NULL, -- snapshot id\n"
                     "  pid       INTEGER NOT NULL, -- process id\n"
                     "  state     TEXT    NOT NULL,    -- process state\n"
                     "  uid       INTEGER NOT NULL, -- process uid\n"
                     "  cmd       INTEGER NOT NULL, -- command id\n"
                     "  usercpu   REAL    NOT NULL,    -- average user mode cpu seconds per second\n"
                     "  systemcpu REAL    NOT NULL,    -- average system mode cpu seconds per second\n"
                     "  iotime    REAL    NOT NULL,    -- average iowait mode cpu seconds per second\n"
                     "  minflt    REAL    NOT NULL,    -- average minor faults per second\n"
                     "  majflt    REAL    NOT NULL,    -- average major faults per second\n"
                     "  rss       REAL    NOT NULL,    -- resident set size in system page size units at snapshot end\n"
                     "  vsz       REAL    NOT NULL,    -- virtual memory size at snapshot end\n"
                     "  wchan     INTEGER NOT NULL,    -- kernel wait channel\n"
                     "  PRIMARY KEY (snapshot,pid),\n"
                     "  FOREIGN KEY (snapshot)  REFERENCES snapshot(id)\n"
                     "  FOREIGN KEY (cmd)  REFERENCES cmd(id)\n"
                     "  FOREIGN KEY (wchan)  REFERENCES wchan(id)\n"
                     ")" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE INDEX IF NOT EXISTS i_procstat_cmd ON procstat( cmd )" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE INDEX IF NOT EXISTS i_procstat_ps ON procstat( pid, snapshot )" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE TABLE IF NOT EXISTS resstat (\n"
                     "  snapshot    INTEGER PRIMARY KEY NOT NULL, -- snapshot id\n"
                     "  processes   INTEGER NOT NULL,             -- number of processes at snapshot end \n"
                     "  users       INTEGER NOT NULL,             -- number of (distinct) users logged on at snapshot end\n"
                     "  logins      INTEGER NOT NULL,             -- number of login sessions at snapshot end\n"
                     "  files_open  INTEGER NOT NULL,             -- number of open files at snapshot end\n"
                     "  files_max   INTEGER NOT NULL,             -- open files limit at snapshot end\n"
                     "  inodes_open INTEGER NOT NULL,             -- number of open inodes at snapshot end\n"
                     "  inodes_free INTEGER NOT NULL,             -- number of free inodes at snapshot end\n"
                     "  FOREIGN KEY (snapshot)  REFERENCES snapshot(id)\n"
                     ")" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE VIEW IF NOT EXISTS v_resstat AS \n"
                     "SELECT\n"
                     "  snapshot.id,\n"
                     "  datetime(snapshot.istart,'unixepoch') istart,\n"
                     "  datetime(snapshot.istop,'unixepoch') istop,\n"
                     "  resstat.processes,\n"
                     "  resstat.users,\n"
                     "  resstat.logins,\n"
                     "  resstat.files_open,\n"
                     "  resstat.inodes_open \n"
                     "FROM\n"
                     "  snapshot,\n"
                     "  resstat\n"
                     "WHERE\n"
                     "  resstat.snapshot=snapshot.id\n" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE TABLE IF NOT EXISTS mountpoint (\n"
                     "  id INTEGER PRIMARY KEY NOT NULL, -- mountpoint id\n"
                     "  mountpoint TEXT NOT NULL,        -- mountpoint directory\n"
                     "  UNIQUE (mountpoint)\n"
                     ")\n" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE TABLE IF NOT EXISTS mountstat (\n"
                     "  snapshot INTEGER NOT NULL,   -- snapshot id\n"
                     "  mountpoint INTEGER NOT NULL, -- mountpoint id\n"
                     "  util     REAL NOT NULL,      -- average IO seconds per second \n"
                     "  svctm    REAL NOT NULL,      -- average IO service time\n"
                     "  rs       REAL NOT NULL,      -- average reads per second\n"
                     "  ws       REAL NOT NULL,      -- average writes per second\n"
                     "  rbs      REAL NOT NULL,      -- average bytes read per second\n"
                     "  wbs      REAL NOT NULL,      -- average bytes written per second\n"
                     "  artm     REAL NOT NULL,      -- average read duration in seconds, includes queue time\n"
                     "  awtm     REAL NOT NULL,      -- average write duration in seconds, includes queue time\n"
                     "  growth   REAL NOT NULL,      -- increase/decrease of the mountpoint (filesystem) in bytes in this snapshot\n"
                     "  used     REAL NOT NULL,      -- size of the mountpoint (filesystem) in bytes\n"
                     "  PRIMARY KEY (snapshot,mountpoint),\n"
                     "  FOREIGN KEY (snapshot)  REFERENCES snapshot(id),\n"
                     "  FOREIGN KEY (mountpoint)  REFERENCES mountpoint(id)\n"
                     ")" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE INDEX IF NOT EXISTS i_mountstat_mountpoint ON mountstat( mountpoint )" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE TABLE IF NOT EXISTS tcpkey (\n"
                     "  id       INTEGER NOT NULL, -- tcpkey id\n"
                     "  ip       TEXT NOT NULL,    -- ip address\n"
                     "  port     INTEGER NOT NULL, -- port number\n"
                     "  uid      INTEGER NOT NULL, -- userid owning the socket\n"
                     "  PRIMARY KEY (id),\n"
                     "  UNIQUE(ip,port,uid)\n"
                     ")" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE TABLE IF NOT EXISTS tcpserverstat (\n"
                     "  snapshot INTEGER NOT NULL, -- snapshot id\n"
                     "  tcpkey   INTEGER NOT NULL, -- tcpkey id\n"
                     "  esta     INTEGER NOT NULL, -- number of established connections at snapshot end\n"
                     "  PRIMARY KEY (snapshot,tcpkey),\n"
                     "  FOREIGN KEY (snapshot)  REFERENCES snapshot(id),\n"
                     "  FOREIGN KEY (tcpkey)  REFERENCES tcpkey(id)\n"
                     ")" );
        ddl.execute();

        ddl.reset();
        ddl.prepare( "CREATE TABLE IF NOT EXISTS tcpclientstat (\n"
                     "  snapshot INTEGER NOT NULL, -- snapshot id\n"
                     "  tcpkey   INTEGER NOT NULL, -- tcpkey id\n"
                     "  esta     INTEGER NOT NULL, -- number of established connections at snapshot end\n"
                     "  PRIMARY KEY (snapshot,tcpkey),\n"
                     "  FOREIGN KEY (snapshot)  REFERENCES snapshot(id),\n"
                     "  FOREIGN KEY (tcpkey)  REFERENCES tcpkey(id)\n"
                     ")" );
        ddl.execute();
        ddl.close();
      }

      void deleteSnapshots( persist::Database &db, long snapid ) {
        persist::DML dml( db );
        dml.prepare( "delete from cpustat where snapshot<=:snapid" );
        dml.bind( 1, snapid );
        dml.execute();
        dml.reset();

        dml.prepare( "delete from schedstat where snapshot<=:snapid" );
        dml.bind( 1, snapid );
        dml.execute();
        dml.reset();

        dml.prepare( "delete from iostat where snapshot<=:snapid" );
        dml.bind( 1, snapid );
        dml.execute();
        dml.reset();

        dml.prepare( "delete from netstat where snapshot<=:snapid" );
        dml.bind( 1, snapid );
        dml.execute();
        dml.reset();

        dml.prepare( "delete from vmstat where snapshot<=:snapid" );
        dml.bind( 1, snapid );
        dml.execute();
        dml.reset();

        dml.prepare( "delete from procstat where snapshot<=:snapid" );
        dml.bind( 1, snapid );
        dml.execute();
        dml.reset();

        dml.prepare( "delete from resstat where snapshot<=:snapid" );
        dml.bind( 1, snapid );
        dml.execute();
        dml.reset();

        dml.prepare( "delete from mountstat where snapshot<=:snapid" );
        dml.bind( 1, snapid );
        dml.execute();
        dml.reset();

        dml.prepare( "delete from tcpserverstat where snapshot<=:snapid" );
        dml.bind( 1, snapid );
        dml.execute();
        dml.reset();

        dml.prepare( "delete from tcpclientstat where snapshot<=:snapid" );
        dml.bind( 1, snapid );
        dml.execute();
        dml.reset();

        dml.prepare( "delete from snapshot where id<=:snapid" );
        dml.bind( 1, snapid );
        dml.execute();
        dml.reset();

        dml.prepare( "delete from disk where id not in (select distinct disk from iostat)" );
        dml.execute();
        dml.reset();

        dml.prepare( "delete from nic where id not in (select distinct nic from netstat)" );
        dml.execute();
        dml.reset();

        dml.prepare( "delete from cmd where id not in (select distinct cmd from procstat)" );
        dml.execute();
        dml.reset();

        dml.prepare( "delete from tcpkey where id not in (select distinct tcpkey from tcpserverstat union select distinct tcpkey from tcpclientstat)" );
        dml.execute();
        dml.reset();

        dml.prepare( "delete from mountpoint where id not in (select distinct mountpoint from mountstat)" );
        dml.execute();
        dml.close();

        dml.prepare( "delete from wchan where id not in (select distinct wchan from procstat)" );
        dml.execute();
        dml.close();
      }

      void shrinkDB( persist::Database &db, const std::string filename ) {
        const int delete_batch = 32;;
        long max_db_size = util::ConfigFile::getConfig()->getIntValue("MAX_DB_SIZE");
        struct stat buf;
        int r = stat( filename.c_str(), &buf );
        if ( r ) throw Oops( __FILE__, __LINE__, "stat on database file '" + filename + "' failed" );
        long minid = 0;
        long maxid = 0;
        while ( buf.st_size > max_db_size * 1024 * 1024 ) {
          persist::Query qry_minid( db );
          qry_minid.prepare( "SELECT min(id),max(id) FROM snapshot" );
          if ( qry_minid.step() ) {
            minid = qry_minid.getLong( 0 );
            maxid = qry_minid.getLong( 1 );
          }
          qry_minid.close();
          if ( maxid - minid < delete_batch * 3 ) break;
          std::stringstream ss;
          ss << "removing snapshots <= " << minid + delete_batch << ", " << maxid-minid-delete_batch << " remaining";
          sysLog( LOG_DEBUG, util::ConfigFile::getConfig()->getIntValue("LOG_LEVEL"), ss.str() );
          db.begin();
          deleteSnapshots( db, minid + delete_batch );
          db.commit();
          vacuumAnalyze(db);
          r = stat( filename.c_str(), &buf );
          if ( r ) throw Oops( __FILE__, __LINE__, "stat on database file '" + filename + "' failed" );
        }
      }

      void applyRetention( persist::Database &db ) {
        persist::Query qry( db );
        std::string retain = "-" + util::ConfigFile::getConfig()->getValue("RETAIN_DAYS") + " days";
        qry.prepare( "SELECT max(id) FROM snapshot WHERE istop < strftime( '%s', datetime('now',:retain) )" );
        qry.bind(1,retain);
        if ( qry.step() ) {
          long snapid = qry.getLong( 0 );
          deleteSnapshots( db, snapid );
        }
        qry.close();
      }

      void vacuumAnalyze( persist::Database &db ) {
        persist::DDL ddl( db );
        ddl.prepare( "VACUUM" );
        ddl.execute();
        ddl.reset();
        ddl.prepare( "ANALYZE" );
        ddl.execute();
        ddl.close();
      }

      void updateSchema( persist::Database &db ) {
        int db_version = db.getUserVersion();
        if ( db_version < schema_version ) {
          std::stringstream ss;
          ss << "upgrading schema from version " << db_version << " to version " << schema_version;
          sysLog( LOG_STAT, util::ConfigFile::getConfig()->getIntValue("LOG_LEVEL"), ss.str() );
        }
        db.setUserVersion( schema_version );
      }


    }; // namespace lard
  }; // namespace tools
}; // namespace leanux
