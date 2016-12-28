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
 * block informational command line tool - c++ source file.
 */
#include <iostream>
#include <iomanip>
#include <sstream>

#include "cpu.hpp"
#include "process.hpp"
#include "system.hpp"
#include "net.hpp"
#include "oops.hpp"
#include "block.hpp"
#include "pci.hpp"
#include "system.hpp"
#include "util.hpp"
#include "vmem.hpp"
#include "block.hpp"
#include "../lmon/lmon_curses.hpp" /** @tdo why? move used functionality to a lib  */
#include "leanux-config.hpp"
#include "tabular.hpp"
#include "device.hpp"

#include <signal.h>
#include <sys/time.h>
#include <algorithm>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <linux/kdev_t.h>

using namespace leanux;
using namespace leanux::tools;

/**
 * Decoded options as specified on command line.
 */
struct Options {
  bool        opt_v;   /**< -v : show version */
  bool        opt_d;   /**< -d : show only whole disks */
  std::string opt_w;   /**< -w substring : show only wwn's containing the substring */
  bool        opt_m;   /**< -d : show only MetaDisks */
  bool        opt_h;   /**< -h : show help */
  bool        opt_t;   /**< -t : shows tree */
  bool        opt_f;   /**< -f : shows filesystem info */
  bool        opt_l;   /**< -l : shows LVM details */
  std::string device;  /**< the specified device (empty if not specified) */
};

/**
 * The user specified options.
 */
Options options;

void printVersion() {
   std::cout << LEANUX_VERSION << std::endl;
}

/**
 * Print command help.
 */
void printHelp() {
  std::cout << "lblk - block device viewer." << std::endl;
  std::cout << "usage: lblk [OPTIONS]" << std::endl;
  std::cout << std::endl;
  std::cout << "lblk" << std::endl;
  std::cout << "  show all block devices" << std::endl;
  std::cout << "lblk <block device>" << std::endl;
  std::cout << "  show named block device detail." << std::endl;
  std::cout << std::endl;
  std::cout << "lblk -d" << std::endl;
  std::cout << "  list only whole disks." << std::endl;
  std::cout << std::endl;
  std::cout << "lblk -dw <substring>" << std::endl;
  std::cout << "  list whole disks with WWN containing the substring" << std::endl;
  std::cout << std::endl;
  std::cout << "lblk -l" << std::endl;
  std::cout << "  list only LVM logical volumes." << std::endl;
  std::cout << std::endl;
  std::cout << "lblk -m" << std::endl;
  std::cout << "  list only MetaDisks (Linux software raid)." << std::endl;
  std::cout << std::endl;
  std::cout << "lblk -t <block device>" << std::endl;
  std::cout << "  show slave and holder trees for the named block device" << std::endl;
  std::cout << std::endl;
  std::cout << "lblk -f <file>" << std::endl;
  std::cout << "  list storage tree supporting the named file (which may be a mount point)." << std::endl;
  std::cout << std::endl;
}

/**
 * Transform command line arguments into options.
 * @param argc as from main.
 * @param argv as from main.
 * @return false when the command line arguments could not be parsed.
 */
bool getOptions( int argc, char* argv[] ) {
  options.opt_v = false;
  options.opt_d = false;
  options.opt_m = false;
  options.opt_h = false;
  options.opt_f = false;
  options.opt_l = false;
  options.opt_w = "";
  options.device = "";
  int opt;
  while ( (opt = getopt( argc, argv, "vlfthmdw:" ) ) != -1 ) {
    switch ( opt ) {
      case 'd':
        options.opt_d = true;
        break;
     case 'w':
         options.opt_w = optarg;
         break;
     case 'm':
         options.opt_m = true;
         break;
     case 'h':
         options.opt_h = true;
         break;
     case 't':
         options.opt_t = true;
         break;
     case 'f':
         options.opt_f = true;
         break;
     case 'l':
         options.opt_l = true;
         break;
     case 'v':
         options.opt_v = true;
         break;
      default:
        return false;
    };
  }
  if ( optind == argc -1 ) {
    options.device = argv[optind];
  } else if ( optind < argc -1 ) return false;
  if ( options.opt_w.length() > 0 && ! options.opt_d ) {
    std::cerr << "cannot use -w without -d" << std::endl;
    return false;
  }
  if ( options.opt_t && options.device == "" ) {
    std::cerr << "-t requires a device argument" << std::endl;
    return false;
  }
  if ( options.opt_f && options.device == "" ) {
    std::cerr << "-f requires a file or filesystem argument" << std::endl;
    return false;
  }
  return true;
};

/**
 * Write the tree of holders above and including mm, append to tab, starting with level.
 * @param mm the device MajorMinor.
 * @param tab the Tabular to write to.
 * @param level start level (indent) for the tree.
 */
void printHolderTree( const block::MajorMinor &mm, Tabular& tab, unsigned int level = 0 ) {
  if ( tab.columnCount() == 0 ) {
    tab.addColumn( "holder tree", false );
    tab.addColumn( "dev", false );
    tab.addColumn( "class", false );
    tab.addColumn( "size" );
    tab.addColumn( "util" );
    tab.addColumn( "svct" );
    tab.addColumn( "r/(r+w)" );
    tab.addColumn( "description", false );
  }
  std::list<std::string> holders;
  std::list<std::string> parts;
  std::string devname = block::MajorMinor::getNameByMajorMinor( mm );
  std::stringstream ss;
  if ( level > 0 ) ss << std::setfill(' ') << std::setw(level*2) << " " << std::setfill(' ');
  ss << mm.getName();
  tab.appendString( "holder tree", ss.str() );
  ss.str("");
  ss << mm;
  tab.appendString( "dev", ss.str() );
  tab.appendString( "class", getClassStr( mm ) );
  tab.appendString( "size", util::ByteStr( getSize( mm ), 3 ) );

  block::DeviceStats stats;
  if ( block::getStats( mm, stats ) ) {
    tab.appendString( "util", util::NumStr( (stats.io_ms/10) / block::getUptime(mm) ) );
    if ( stats.reads+stats.writes > 0 ) {
      tab.appendString( "svct", util::TimeStrSec( (stats.io_ms/1000.0) / (double)(stats.reads+stats.writes) ) );
      tab.appendString( "r/(r+w)", util::NumStr( (double)stats.reads/(double)(stats.reads+stats.writes) ) );
    } else {
      tab.appendString( "svct", " " );
      tab.appendString( "r/(r+w)", " " );
    }
  } else {
    tab.appendString( "util", " " );
    tab.appendString( "svct", " " );
    tab.appendString( "r/(r+w)", " " );
  }

  tab.appendString( "description", block::MajorMinor::getDescription( mm ) );

  block::getPartitions( mm, parts );
  block::getHolders( mm, holders );
  for ( std::list<std::string>::const_iterator d = holders.begin(); d != holders.end(); d++ ) {
    printHolderTree( block::MajorMinor::getMajorMinorByName( (*d) ), tab, level+1 );
  }
  for ( std::list<std::string>::const_iterator d = parts.begin(); d != parts.end(); d++ ) {
    printHolderTree( block::MajorMinor::getMajorMinorByName( (*d) ), tab, level+1 );
  }
}

/**
 * Write the tree of slaves beneath and including mm, append to tab, starting with level.
 * @param mm the device MajorMinor.
 * @param tab the Tabular to write to.
 * @param level start level (indent) for the tree.
 */
void printSlaveTree( const block::MajorMinor &mm, Tabular& tab, unsigned int level = 0 ) {
  if ( tab.columnCount() == 0 ) {
    tab.addColumn( "slave tree", false );
    tab.addColumn( "dev", false );
    tab.addColumn( "class", false );
    tab.addColumn( "size" );
    tab.addColumn( "util" );
    tab.addColumn( "svct" );
    tab.addColumn( "r/(r+w)" );
    tab.addColumn( "description", false );
  }
  std::list<std::string> slaves;
  std::string devname = block::MajorMinor::getNameByMajorMinor( mm );
  std::stringstream ss;
  if ( level > 0 ) ss << std::setfill(' ') << std::setw(level*2) << " " << std::setfill(' ');
  ss << mm.getName();
  tab.appendString( "slave tree", ss.str() );
  ss.str("");
  ss << mm;
  tab.appendString( "dev", ss.str() );
  tab.appendString( "class", block::getClassStr( mm ) );
  tab.appendString( "size", util::ByteStr( block::getSize( mm ), 3 ) );

  block::DeviceStats stats;
  if ( block::getStats( mm, stats ) ) {
    tab.appendString( "util", util::NumStr( (stats.io_ms/10) / leanux::block::getUptime(mm) ) );
    if ( stats.reads+stats.writes > 0 ) {
      tab.appendString( "svct", util::TimeStrSec( (stats.io_ms/1000.0) / (double)(stats.reads+stats.writes) ) );
      tab.appendString( "r/(r+w)", util::NumStr( (double)stats.reads/(double)(stats.reads+stats.writes) ) );
    } else {
      tab.appendString( "svct", " " );
      tab.appendString( "r/(r+w)", " " );
    }
  } else {
    tab.appendString( "util", " " );
    tab.appendString( "svct", " " );
    tab.appendString( "r/(r+w)", " " );
  }

  tab.appendString( "description", block::MajorMinor::getDescription( mm ) );

  block::getSlaves( mm, slaves );
  for ( std::list<std::string>::const_iterator d = slaves.begin(); d != slaves.end(); d++ ) {
    printSlaveTree( block::MajorMinor::getMajorMinorByName( (*d) ), tab, level+1 );
  }
  if ( mm.isPartition() ) {
    block::MajorMinor wd = block::MajorMinor::deriveWholeDisk( mm );
    if ( ! (wd == mm ) ) {
      printSlaveTree( wd, tab, level+1 );
    }
  }

}

/**
 * Table listing LVM devices.
 * @param os the ostream to write to.
 */
void listLVM( std::ostream& os  ) {
  std::list<block::MajorMinor> pvs;
  block::enumLVMPVS( pvs );
  if ( pvs.size() > 0 ) {
    Tabular tab;
    for ( std::list<block::MajorMinor>::const_iterator p = pvs.begin(); p != pvs.end(); p++ ) {
      printHolderTree( *p, tab );
    }
    tab.dump(os);
    tab.clear();
    os << std::endl;
    for ( std::list<block::MajorMinor>::const_iterator p = pvs.begin(); p != pvs.end(); p++ ) {
      printSlaveTree( *p, tab );
    }
    tab.dump(os);
  } else {
    os << "no LVM physical volumes found." << std::endl;
  }
}

/**
 * Table listing all storage needed for the filesystem containing file.
 * @param os the ostream to write to.
 * @param file the file path.
 */
void listFile( std::ostream& os, const std::string &file  ) {
  struct stat st;
  if ( !stat( file.c_str(), &st ) ) {
    block::MajorMinor mm = block::MajorMinor( st.st_dev );
    if ( !mm.isValid() ) mm =  block::MajorMinor( st.st_rdev );
    if ( mm.isValid() && block::MajorMinor::getNameByMajorMinor(mm) != "" ) {
      Tabular tab;
      printSlaveTree( mm, tab );
      tab.dump( os );
    } else throw Oops( __FILE__, __LINE__, "filesystem stat returns invalid block device" );
  } else throw Oops( __FILE__, __LINE__, "file not found" );
}

/**
 * Table listing all devices.
 * @param os the ostream to write to.
 */
void listAllDevices( std::ostream& os  ) {
  std::list<block::MajorMinor> devices;
  block::enumDevices( devices );
  Tabular table;
  table.addColumn( "device", false );
  table.addColumn( "dev", false );
  table.addColumn( "class", false );
  table.addColumn( "size" );
  table.addColumn( "description", false );
  for ( std::list<block::MajorMinor>::const_iterator i = devices.begin(); i != devices.end(); i++ ) {
    table.appendString( "device", (*i).getName() );
    std::stringstream ss;
    ss << (*i);
    table.appendString( "dev", ss.str() );
    table.appendString( "class", getClassStr( *i ) );
    table.appendString( "size", util::ByteStr( getSize( *i ), 3 ) );
    table.appendString( "description", block::MajorMinor::getDescription( *i ) );
  }
  table.dump( std::cout );
}

/**
 * Table listing all disks.
 * @param os the ostream to write to.
 */
void listAllDisks( std::ostream& os  ) {
  std::list<block::MajorMinor> devices;
  block::enumWholeDisks( devices );
  Tabular table;
  table.addColumn( "disk", false );
  table.addColumn( "dev", false );
  table.addColumn( "devicefile", false );
  table.addColumn( "class", false );
  table.addColumn( "address", false );
  table.addColumn( "model", false );
  table.addColumn( "size" );
  table.addColumn( "type", false );
  table.addColumn( "sect" );
  table.addColumn( "wwn", false );
  //table.addColumn( "kmod", false );
  table.addColumn( "util" );
  table.addColumn( "svct" );
  table.addColumn( "r/(r+w)" );
  for ( std::list<block::MajorMinor>::const_iterator i = devices.begin(); i != devices.end(); i++ ) {
    if ( getSize( *i ) > 0 ) {
      if ( options.opt_w.length() == 0  || (options.opt_w.length() > 0 && getWWN( *i ).find( options.opt_w ) != std::string::npos) ) {
        table.appendString( "disk", (*i).getName() );
        std::stringstream ss;
        ss << (*i);
        table.appendString( "dev", ss.str() );
        table.appendString( "devicefile", (*i).getDeviceFile() );
        table.appendString( "class", getClassStr( *i ) );
        table.appendString( "address", getSCSIHCTL( *i ) );
        table.appendString( "model", getModel( *i ) );
        table.appendString( "size", util::ByteStr( getSize( *i ), 3 ) );
        if ( getRotational( *i ) ) {
          ss.str("");
          ss << getRPM( *i ) << "RPM";
          table.appendString( "type", ss.str() );
        } else table.appendString( "type", "SSD" );
        table.appendString( "sect", util::ByteStr( getSectorSize( *i ), 3 ) );
        table.appendString( "wwn", getWWN( *i ) );

        //table.appendString( "kmod", getKernelModule( *i ) );
        block::DeviceStats stats;
        if ( block::getStats( *i, stats ) ) {
          if ( stats.reads+stats.writes > 0 ) {
            table.appendString( "util", util::NumStr( (stats.io_ms/10) / leanux::block::getUptime(*i) ) );
            table.appendString( "svct", util::TimeStrSec( (stats.io_ms/1000.0) / (double)(stats.reads+stats.writes) ) );
            table.appendString( "r/(r+w)", util::NumStr( (double)stats.reads/(double)(stats.reads+stats.writes) ) );
          } else {
            table.appendString( "util", " " );
            table.appendString( "svct", " " );
            table.appendString( "r/(r+w)", "" );
          }
        }
      }
    }
  }
  table.dump( std::cout );
}

/**
 * Table listing all MetaDisks.
 * @param os the ostream to write to.
 */
void listAllMetaDisks( std::ostream& os  ) {
  std::list<block::MajorMinor> devices;
  block::enumDevices( devices );
  Tabular table;
  table.addColumn( "metadisk" );
  table.addColumn( "dev" );
  table.addColumn( "name" );
  table.addColumn( "size" );
  table.addColumn( "level" );
  table.addColumn( "disks" );
  table.addColumn( "chunk" );
  table.addColumn( "metadata" );
  table.addColumn( "array" );
  table.addColumn( "devices" );
  table.addColumn( "state" );
  unsigned int count = 0;
  for ( std::list<block::MajorMinor>::const_iterator i = devices.begin(); i != devices.end(); i++ ) {
    if ( (*i).isMetaDisk() ) {
      count++;
      table.appendString( "metadisk", (*i).getName() );
      std::stringstream ss;
      ss << (*i);
      table.appendString( "dev", ss.str() );
      table.appendString( "name", block::getMDName( *i ) );
      table.appendString( "size", util::ByteStr( getSize( *i ), 3 ) );
      table.appendString( "level", block::getMDLevel( *i ) );
      ss.str("");
      ss << block::getMDDevices( *i );
      table.appendString( "disks", ss.str() );
      table.appendString( "chunk", util::ByteStr( block::getMDChunkSize( *i ), 3) );
      table.appendString( "metadata", block::getMDMetaDataVersion( *i ) );
      ss.str("");
      ss << block::getMDArrayState( *i );
      table.appendString( "array", ss.str() );
      std::vector<block::MajorMinor> disks;
      block::getMDRaidDisks( *i, disks );
      ss.str("");
      for ( std::vector<block::MajorMinor>::const_iterator d = disks.begin(); d != disks.end(); d++ ) {
        if ( d != disks.begin() ) ss << "," << (*d).getName() ; else ss << (*d).getName();
      }

      table.appendString( "devices", ss.str() );
      table.appendString( "state", block::getMDRaidDiskStates( *i ) );
    }
  }
  if ( count ) table.dump( std::cout ); else std::cout << "no metadisks found." << std::endl;
}

/**
 * Write disk details.
 * @param mm the device MajorMinor.
 * @param os the ostream to write to.
 */
void detailDisk( const block::MajorMinor &mm, std::ostream &os ) {
  Tabular tab;
  tab.addColumn("property");
  tab.addColumn("value", false );
  std::stringstream ss;
  ss << mm;
  tab.appendString( "property", "major:minor" );
  tab.appendString( "value", ss.str() );

  tab.appendString( "property", "devicename" );
  tab.appendString( "value", mm.getName() );
  
  tab.appendString( "property", "devicefile" );
  tab.appendString( "value", mm.getDeviceFile() );  

  tab.appendString( "property", "sysfs path" );
  tab.appendString( "value", block::getSysPath( mm ) );

  tab.appendString( "property", "class" );
  tab.appendString( "value", block::getClassStr( mm ) );

  tab.appendString( "property", "size" );
  tab.appendString( "value", util::ByteStr( getSize( mm ), 3 ) );

  tab.appendString( "property", "sector size" );
  tab.appendString( "value", util::ByteStr( getSectorSize( mm ), 3 ) );

  tab.appendString( "property", "model" );
  tab.appendString( "value", block::getModel( mm ) );

  if ( !mm.isPartition() ) {
    tab.appendString( "property", "revision" );
    tab.appendString( "value", block::getRevision( mm ) );

    tab.appendString( "property", "serial" );
    tab.appendString( "value", block::getSerial( mm ) );

    tab.appendString( "property", "WWN" );
    tab.appendString( "value", block::getWWN( mm ) );

    tab.appendString( "property", "diskid (internal)" );
    tab.appendString( "value", block::getDiskId( mm ) );

    tab.appendString( "property", "type" );
    ss.str("");
    if ( getRotational( mm ) ) {
      if ( block::getRPM( mm ) > 0 )
        ss << "spindle (" << block::getRPM( mm ) << "RPM)";
      else
        ss << "virtual";
      tab.appendString( "value", ss.str() );
    } else {
      tab.appendString( "value", "SSD" );
    }

    tab.appendString( "property", "module" );
    tab.appendString( "value", block::getKernelModule( mm ) );

    std::string hctl = block::getSCSIHCTL( mm );
    if ( hctl != "" ) {
      tab.appendString( "property", "SCSI address" );
      tab.appendString( "value", hctl );
    }
    std::string cache_type = block::getCacheMode( mm );
    if ( cache_type != "" ) {
      tab.appendString( "property", "cache type" );
      tab.appendString( "value", cache_type );
    }

    tab.appendString( "property", "IO scheduler" );
    tab.appendString( "value", block::getIOScheduler( mm ) );

    tab.appendString( "property", "hw max IO size" );
    tab.appendString( "value", util::ByteStr( block::getMaxHWIOSize( mm ), 3 ) );

    tab.appendString( "property", "max IO size" );
    tab.appendString( "value", util::ByteStr( block::getMaxIOSize( mm ), 3 ) );

    tab.appendString( "property", "min IO size" );
    tab.appendString( "value", util::ByteStr( block::getMinIOSize( mm ), 3 ) );

    tab.appendString( "property", "read ahead" );
    tab.appendString( "value", util::ByteStr( block::getReadAhead( mm ), 3 ) );

  }

  tab.appendString( "property", "fs type" );
  tab.appendString( "value", block::getFSType( mm ) );

  tab.appendString( "property", "fs use" );
  tab.appendString( "value", block::getFSUsage( mm ) );

  tab.appendString( "property", "device uptime" );
  tab.appendString( "value", util::TimeStrSec( block::getUptime( mm ) ) );

  std::list<std::string> aliases;
  block::getAliases( mm, aliases );
  for ( std::list<std::string>::const_iterator alias = aliases.begin(); alias != aliases.end(); alias++ ) {
    tab.appendString( "property", "alias" );
    tab.appendString( "value", *alias );
  }

  block::DeviceStats stats;
  if ( block::getStats( mm, stats ) ) {
    if ( stats.reads+stats.writes > 0 ) {
      tab.appendString( "property", "%util" );
      tab.appendString( "value", util::NumStr( (stats.io_ms/10.0) / leanux::block::getUptime(mm), 3 ) );
      tab.appendString( "property", "service time" );
      tab.appendString( "value", util::TimeStrSec( (stats.io_ms/1000.0) / (double)(stats.reads+stats.writes) ) );
      tab.appendString( "property", "r/(r+w) ratio" );
      tab.appendString( "value", util::NumStr( (double)stats.reads/(double)(stats.reads+stats.writes) ) );
    }
  }

  tab.dump( os );

  tab.clear();

  os << std::endl;
  std::list<leanux::sysdevice::SysDevice*> devices;
  size_t path_indent = 0;
  block::MajorMinor wholedisk = mm;
  if ( mm.isPartition() ) wholedisk = block::MajorMinor::deriveWholeDisk( mm );
  if ( leanux::sysdevice::treeDetect( getSysPath(wholedisk), devices ) ) {
    tab.addColumn( "sysfs device", false );
    tab.addColumn( "type", false );
    tab.addColumn( "driver", false );
    tab.addColumn( "class", false );
    tab.addColumn( "descr", false );
    unsigned int depth = 0;
    for ( std::list<leanux::sysdevice::SysDevice*>::const_iterator d = devices.begin(); d != devices.end(); d++, depth++ ) {
      tab.appendString( "sysfs device", std::string(depth,' ') + (*d)->getPath().substr(path_indent) );
      tab.appendString( "type", leanux::sysdevice::SysDevice::getTypeStr( (*d)->getType() ) );
      tab.appendString( "driver", (*d)->getDriver()  );
      tab.appendString( "class", (*d)->getClass()  );
      tab.appendString( "descr", (*d)->getDescription() );
      path_indent += ((*d)->getPath().length() - path_indent) + 1;
    }
    tab.dump(os);
    leanux::sysdevice::destroy( devices );
  } else std::cerr << "detection error" << std::endl;

}

/**
 * Write MetaDisk details.
 * @param mm the device MajorMinor.
 * @param os the ostream to write to.
 */
void detailMetaDisk( const block::MajorMinor &mm, std::ostream &os ) {
  unsigned int wc = 15;
  os << std::setw(wc) << "dev: " << mm << std::endl;
  os << std::setw(wc) << "device: " << mm.getName() << std::endl;
  os << std::setw(wc) << "devicepath: " << block::getSysPath(mm) << std::endl;
  os << std::setw(wc) << "description: " << block::MajorMinor::getDescription(mm) << std::endl;
  os << std::setw(wc) << "class: " << block::getClassStr( mm ) << std::endl;
  os << std::setw(wc) << "name: " << block::getMDName( mm ) << std::endl;
  os << std::setw(wc) << "size: " << util::ByteStr( block::getSize( mm ), 3 ) << std::endl;
  os << std::setw(wc) << "metadata: " << block::getMDMetaDataVersion( mm ) << std::endl;
  os << std::setw(wc) << "level: " << block::getMDLevel( mm ) << std::endl;
  os << std::setw(wc) << "chunk: " << util::ByteStr( block::getMDChunkSize( mm ), 3 ) << " (" << block::getMDChunkSize( mm ) << ")" << std::endl;
  os << std::setw(wc) << "disks: " << block::getMDDevices( mm ) << std::endl;
  os << std::setw(wc) << "array: " << block::getMDArrayState( mm ) << std::endl;
  os << std::setw(wc) << "state: " << block::getMDRaidDiskStates( mm ) << std::endl;
  os << std::setw(wc) << "raid devices: ";

  std::vector<block::MajorMinor> disks;
  getMDRaidDisks( mm, disks );
  for ( std::vector<block::MajorMinor>::const_iterator d = disks.begin(); d != disks.end(); d++ ) {
    if ( d != disks.begin() ) os << "," << (*d).getName() ; else os << (*d).getName();
  }
  os << std::endl;

  std::list<std::string> aliases;
  getAliases( mm, aliases );
  for ( std::list<std::string>::const_iterator alias = aliases.begin(); alias != aliases.end(); alias++ ) {
    os << std::setw(wc) << "alias: " << *alias << std::endl;
  }
}

/**
 * Write generic block device details.
 * @param mm the device MajorMinor.
 * @param os the ostream to write to.
 */
void detailGenericBlockDevice( const block::MajorMinor &mm, std::ostream &os ) {
  unsigned int wc = 15;
  os << std::setw(wc) << "dev: " << mm << std::endl;
  os << std::setw(wc) << "device: " << mm.getName() << std::endl;
  os << std::setw(wc) << "class: " << getClassStr( mm ) << std::endl;
}

/**
 * Write device mapper details.
 * @param mm the device MajorMinor.
 * @param os the ostream to write to.
 */
void detailDeviceMapper( const block::MajorMinor &mm, std::ostream &os ) {
  unsigned int wc = 15;
  os << std::setw(wc) << "dev: " << mm << std::endl;
  os << std::setw(wc) << "device: " << mm.getName() << std::endl;
  os << std::setw(wc) << "class: " << block::getClassStr( mm ) << std::endl;
}

/**
 * Write LVM details.
 * @param mm the device MajorMinor.
 * @param os the ostream to write to.
 */
void detailLVM( const block::MajorMinor &mm, std::ostream &os ) {
  unsigned int wc = 16;
  os << std::setw(wc) << "dev: " << mm << std::endl;
  os << std::setw(wc) << "devicename: " << mm.getName() << std::endl;
  os << std::setw(wc) << "devicefile: " << mm.getDeviceFile() << std::endl;
  os << std::setw(wc) << "devicepath: " << block::getSysPath(mm) << std::endl;
  os << std::setw(wc) << "class: " << getClassStr( mm ) << std::endl;
  os << std::setw(wc) << "disk id: " << getDiskId( mm ) << std::endl;
  os << std::setw(wc) << "name: " << getDMName( mm ) << std::endl;
  os << std::setw(wc) << "VG: " << getVGName( mm ) << std::endl;
  os << std::setw(wc) << "LV: " << getLVName( mm ) << std::endl;
  os << std::setw(wc) << "size: " << util::ByteStr( getSize( mm ), 3 ) << std::endl;

  os << std::setw(wc) << "IO scheduler: " << block::getIOScheduler( mm ) << std::endl;
  os << std::setw(wc) << "sector: " << util::ByteStr( block::getSectorSize( mm ), 3 ) << std::endl;
  os << std::setw(wc) << "max hw IO size: " << util::ByteStr( block::getMaxHWIOSize( mm ), 3 ) << std::endl;
  os << std::setw(wc) << "max IO size: " << util::ByteStr( block::getMaxIOSize( mm ), 3 ) << std::endl;
  os << std::setw(wc) << "min IO size: " << util::ByteStr( block::getMinIOSize( mm ), 3 ) << std::endl;
  os << std::setw(wc) << "read-ahead: " << util::ByteStr( block::getReadAhead( mm ), 3 ) << std::endl;

  std::list<std::string> aliases;
  getAliases( mm, aliases );
  for ( std::list<std::string>::const_iterator alias = aliases.begin(); alias != aliases.end(); alias++ ) {
    os << std::setw(wc) << "alias: " << *alias << std::endl;
  }
}

/**
 * do smart things with user input for device name
 * so that we are not overly picky in user specified device names
 */
block::MajorMinor parseDeviceArg() {
  // is it just a  device name?
  block::MajorMinor mm = block::MajorMinor::getMajorMinorByName( options.device );
  if ( !mm.isValid() ) {
    //is it a device path?
    std::string resolved_path = util::realPath( options.device );
    struct stat st;
    if ( !stat( resolved_path.c_str(), &st ) ) {
      return block::MajorMinor( MAJOR(st.st_rdev), MINOR(st.st_rdev) );
    }
    //maybe it is a mapper device?
    std::stringstream ss;
    ss << "/dev/mapper/" << options.device;
    if ( !stat( ss.str().c_str(), &st ) ) {
      return block::MajorMinor( MAJOR(st.st_rdev), MINOR(st.st_rdev) );
    }
    //or a by-id
    ss.str("");
    ss << "/dev/disk/by-id/" << options.device;
    if ( !stat( ss.str().c_str(), &st ) ) {
      return block::MajorMinor( MAJOR(st.st_rdev), MINOR(st.st_rdev) );
    }
    //or a by-uuid
    ss.str("");
    ss << "/dev/disk/by-uuid/" << options.device;
    if ( !stat( ss.str().c_str(), &st ) ) {
      return block::MajorMinor( MAJOR(st.st_rdev), MINOR(st.st_rdev) );
    }
  } else return mm;
  return block::MajorMinor::invalid;
}

/**
 * Direct output run from options.
 */
void runOptions() {
  if ( !options.opt_v ) {
    if ( !options.opt_h ) {
      if ( options.device != "" ) {
        if ( !options.opt_f ) {
          block::MajorMinor mm = parseDeviceArg();
          if ( mm.isValid() ) {
            if ( options.opt_t ) {
              Tabular htab;
              printHolderTree( mm, htab );
              if ( htab.rowCount() > 2 ) {
                htab.dump( std::cout );
              }
              Tabular stab;
              printSlaveTree( mm, stab );
              if ( stab.rowCount() > 2 ) {
                if ( htab.rowCount() > 2 ) std::cout << std::endl;
                stab.dump( std::cout );
              }
            } else {
              if ( block::MajorMinor::isSCSIDisk( mm.getMajor() ) ||
                   block::MajorMinor::isIDEDisk( mm.getMajor() ) ||
                   block::MajorMinor::isVirtIODisk( mm ) ||
                   block::MajorMinor::isNVMeDisk( mm ) ||
                   block::MajorMinor::isMMCDisk( mm ) ||
                   block::MajorMinor::isBCacheDisk( mm ) ) {
                detailDisk( mm, std::cout );
              } else if ( block::MajorMinor::isMetaDisk( mm.getMajor() ) ) {
                detailMetaDisk( mm, std::cout );
              } else if ( block::getClass( mm ) == block::DeviceMapper ) {
                detailDeviceMapper( mm, std::cout );
              } else if ( block::getClass( mm ) == block::LVM ) {
                detailLVM( mm, std::cout );
              } else {
                detailGenericBlockDevice( mm, std::cout );
              }
            }
          } else  {
            std::cerr << "'" << options.device << "' is not a block device" << std::endl;
          }
        } else if ( options.opt_f ) {
          if ( options.device != "" ) {
            listFile( std::cout, options.device );
          }
        }
      } else {
        if ( options.opt_d ) {
          listAllDisks( std::cout );
        }
        if ( options.opt_m ) {
          listAllMetaDisks( std::cout );
        }
        if ( options.opt_l ) {
          listLVM( std::cout );
        }
        if ( !options.opt_d && !options.opt_m && !options.opt_l ) {
          listAllDevices( std::cout );
        }
      }
    } else printHelp();
  } else printVersion();
}

/**
 * Entry point.
 * @param argc number of command line arguments.
 * @param argv command line arguments.
 * @return 0 on succes, 1 on Oops, 2 on other errors.
 */
int main( int argc, char* argv[] ) {
  try {
    leanux::init();

    if ( getOptions( argc, argv ) ) {
      runOptions();
    } else printHelp();
  }
  catch ( const Oops &oops ) {
    std::cerr << oops.getMessage() << std::endl;
    return 1;
  }
  catch ( ... ) {
    std::cerr << "unhandled exception" << std::endl;
    return 2;
  }

  return 0;
}
