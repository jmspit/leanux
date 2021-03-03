# leanux

Leanux is a C++ API to Linux performance and configuration data and includes a few tools utilizing this API.

The C++ API eases access to system configuration data provided by the `/sys` pseudo filesystem, so including CPUs, block devices, network devices and so on. Additionally, the C++ API exposes performance data from the `/proc` pseudo fillesystem.

The (C++) binaries produced by this repository are

| component | purpose |
|------|---------|
| libleanux.so | Shared library backing the leanux C++ API, required by the below tools |
| [lard](man_lard.pdf) | daemon that logs performance and base configuration data to a SQLite database file |
| [lmon](man_lmon.pdf) | ncurses (tty) based performance viewer, both real-time and browsing historic data from the lard daemon, with colors, dynamic screen resizing |
| [lrep](man_lrep.pdf) | generates html reports on lard data |
| [lsys](man_lsys.pdf) | command-line tool to retrieve sysfs info for administrative use |
| [lblk](man_lblk.pdf) | command-line tool to retrieve block device and filesystem info for administrative use |
| labbix | Zabbix disk discovery and disk performance statistics for GNU/Linux |

See [leanux on github pages](https://jmspit.github.io/leanux) for more documentation. If you are on github pages now,
you can view the doxygen code documentation as [here](doxygen/html/index.html), and the links in the above table will link to pdf documents.
## Building from source

Builds should work on any Linux host that satisfies

The build depends on

  - A C++ toolchain with c++11 capability
  - sqlite3 development headers
  - ncurses development headers

Download a [release](https://github.com/jmspit/leanux/releases)

```bash
$ tar zxvf leanux-1.0.5-snapshot.tar.gz
$ cd leanux-1.0.5-snapshot
```

Or clone the master branch

```bash
$ git clone git@github.com:jmspit/leanux.git
$ cd leanux
```

Or clone a specific release [tag](https://github.com/jmspit/leanux/tags) or [branch](https://github.com/jmspit/leanux/branches)

```bash
$ git clone git@github.com:jmspit/leanux.git
$ cd leanux
$ git checkout development
```

In any case, to build a Release build

```bash
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build . --config Release
```

