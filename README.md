# leanux

This is the Git code repository for leanux, a C++ API and tools to Linux performance and configuration data.

The C++ API eases access to system configuration data provided by the
`/sysfs` pseudo filesystem, so including block devices, network devices and so on. Additionally, the C++ API exposes performance data from the `/proc`
pseudo fillesystem.

On top of the C++ API a number of tools are implemented:

| component | purpose |
|------|---------|
| libleanux.so | Shared library backing the leanux C++ API, required by the below tools |
| [lard](https://www.o-rho.com/leanux/lard) | daemon that logs performance and base configuration data to a SQLite database file |
| [lmon](doc/lmon.md) | ncurses (tty) based performance viewer, both real-time and browsing historic data from the lard daemon, with colors, dynamic screen resizing |
| [lrep](https://www.o-rho.com/leanux/lrep) | generates html reports on lard data |
| [lsys](https://www.o-rho.com/leanux/lsys) | command-line tool to retrieve sysfs info for administrative use |
| [lblk](https://www.o-rho.com/leanux/lblk) | command-line tool to retrieve block device and filesystem info for administrative use |
| [labbix](https://www.o-rho.com/leanux/labbix) | Zabbix disk discovery and disk performance statistics for GNU/Linux |

## Building from source

```bash
$ git clone git@github.com:jmspit/leanux.git
$ mkdir -p leanux/build
$ cd leanux/build
$ cmake ..
$ cmake --build . --config Release
```
