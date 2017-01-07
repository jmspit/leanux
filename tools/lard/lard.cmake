set( LARD_NAME "lard" )
set( LARD_DESCR "daemon that logs performance data to a sqlite database" )

set( LARD_CONF_DATABASE_PAGE_SIZE_DEFAULT "4096" )
set( LARD_CONF_DATABASE_PAGE_SIZE_DESCR "sqlite3 database page size" )
set( LARD_CONF_DATABASE_PAGE_SIZE_COMMENT "a larger page size shows some space and performance benefits at the expense of some memory and a bigger WAL file. this parameter only has effect when the database is first created" )

set( LARD_CONF_LOG_LEVEL_DEFAULT "2" )
set( LARD_CONF_LOG_LEVEL_DESCR "0 only errors, 1 +warnings, 2 +status, 3 +info, 4 +debug" )
set( LARD_CONF_LOG_LEVEL_COMMENT "specify which log messages are written to the syslog" )

set( LARD_CONF_MAX_DISKS_DEFAULT "16" )
set( LARD_CONF_MAX_DISKS_DESCR "limit the number of disks for which statistics are stored each snapshot" )

set( LARD_CONF_MAX_MOUNTS_DEFAULT "16" )
set( LARD_CONF_MAX_MOUNTS_DESCR "limit the number of mount points for which statistics are stored each snapshot" )

set( LARD_CONF_MAX_PROCESSES_DEFAULT "16" )
set( LARD_CONF_MAX_PROCESSES_DESCR "limit the number of processes for which statistics are stored each snapshot" )

set( LARD_CONF_RETAIN_DAYS_DEFAULT "31" )
set( LARD_CONF_RETAIN_DAYS_DESCR "limit the number of days a snapshot is retained" )
set( LARD_CONF_RETAIN_DAYS_COMMENT "each maintenance interval, snapshots exceeding RETAIN_DAYS are deleted. note that MAX_DB_SIZE takes precedence over RETAIN_DAYS if MAX_DB_SIZE>0" )

set( LARD_CONF_MAX_DB_SIZE_DEFAULT "50" )
set( LARD_CONF_MAX_DB_SIZE_DESCR "maximum database file size in MiB" )
set( LARD_CONF_MAX_DB_SIZE_COMMENT "excess snapshots are deleted regardless of RETAIN_DAYS, a value of 0 implies no limit. use as absolute upper limit to prevent unplanned growth of the database file" )

set( LARD_CONF_SNAPSHOT_CHECKPOINT_DEFAULT "6" )
set( LARD_CONF_SNAPSHOT_CHECKPOINT_DESCR "issue a sqlite checkpoint each SNAPSHOT_CHECKPOINT snapshots" )
set( LARD_CONF_SNAPSHOT_CHECKPOINT_COMMENT "effectively controls the maximum size of sqlite WAL file, a checkpoint merges changes written to the WAL into the database file" )

set( LARD_CONF_SNAPSHOT_INTERVAL_DEFAULT "300" )
set( LARD_CONF_SNAPSHOT_INTERVAL_DESCR "snapshot interval in seconds" )
set( LARD_CONF_SNAPSHOT_INTERVAL_COMMENT "sets the snapshot frequency. note that changing this on an existing lard database can produce awkward results in lrep report timeline charts" )

set( LARD_CONF_MAINTENANCE_INTERVAL_DEFAULT "120" )
set( LARD_CONF_MAINTENANCE_INTERVAL_DESCR "maintenance (purge) interval in minutes" )
set( LARD_CONF_MAINTENANCE_INTERVAL_COMMENT "each maintenance interval snapshots that exceed either MAX_DB_SIZE or RETAIN_DAYS are removed, and the database is vacuumed and analyzed" )

set( LARD_SYSDB_PATH "/var/lib/lard" )
set( LARD_SYSDB_FILE "${LARD_SYSDB_PATH}/lard.db" )
set( LARD_SYSCONF_DIR "/etc/lard" )
set( LARD_SYSCONF_FILE "${LARD_SYSCONF_DIR}/lard.conf" )
set( LARD_SYSVINIT_FILE "/etc/init.d/lard" )
set( LARD_USER "leanux" )


# generate configure header
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/lard-config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/lard-config.hpp @ONLY)

