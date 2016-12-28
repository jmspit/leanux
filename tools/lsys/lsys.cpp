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
#include "device.hpp"
#include "net.hpp"
#include "system.hpp"
#include "pci.hpp"
#include "tabular.hpp"
#include "usb.hpp"
#include "util.hpp"
#include "leanux-config.hpp"
#include "natsort.hpp"
#include <algorithm>
#include <iostream>
#include <list>
#include <set>
#include <string>

using namespace leanux;

/**
 * lsysdev [device options] <device>
 * lsysdev [list options]
 *
 */
struct Options {
  bool        opt_h;   /**< -h : show help */
  bool        opt_v;   /**< -v : show version */
  bool        opt_l;   /**< -l : list all */
  std::string opt_f;   /**< -f field[=filter],..: select display fields */
  std::string opt_t;   /**< -t type : limit to specified type */
  std::string device;  /**< the specified sysdevice (empty if not specified) */
};

/**
 * the default or user specified options.
 */
Options options;

/**
 * type for mapping of user friendly names to SysDeviceType
 */
typedef std::map<std::string, sysdevice::SysDevice::SysDeviceType> TypeMap;
typedef std::map<sysdevice::SysDevice::SysDeviceType,std::string> RTypeMap;

/**
 * type for mapping of field name to a set of supported SysDeviceTypes
 */
typedef std::map<std::string, std::set<sysdevice::SysDevice::SysDeviceType> > TypeFields;
typedef std::map<sysdevice::SysDevice::SysDeviceType, std::set<std::string> > RTypeFields;

/**
 * The global TypeMap.
 */
TypeMap type_map;
RTypeMap rtype_map;

/**
 * The global TypeFields.
 */
TypeFields type_fields;
RTypeFields rtype_fields;

/**
 * type for fields to display.
 */
typedef std::list<std::string> DisplayFields;

/**
 * The global default or user specified display fields
 */
DisplayFields display_fields;

struct Filter {
  bool negate;        /**< negate the result */
  std::string filter; /**< regexp */
};

/**
 * type for map of field names to field filters.
 */
typedef std::map<std::string, Filter> FilterMap;

/**
 * global map of field names to field filters.
 */
FilterMap filter_map;

/**
 * Transform command line arguments into options.
 * @param argc as from main.
 * @param argv as from main.
 * @return false when the command line arguments could not be parsed.
 */
bool getOptions( int argc, char* argv[] ) {
  options.opt_v = false;
  options.opt_l = false;
  options.opt_t = "";
  options.device = "";
  int opt;
  while ( (opt = getopt( argc, argv, "hvlt:f:" ) ) != -1 ) {
    switch ( opt ) {
      case 'f':
        options.opt_f = optarg;
        break;
      case 't':
        options.opt_t = optarg;
        break;
      case 'v':
        options.opt_v = true;
        break;
      case 'h':
        options.opt_h = true;
        break;
      case 'l':
        options.opt_l = true;
        break;
      default:
        return false;
    };
  }
  if ( optind == argc -1 ) {
    options.device = argv[optind];
  } else if ( optind < argc -1 ) return false;

  // the user specified type selector must exist
  if ( options.opt_t != "" ) {
    TypeMap::const_iterator tm = type_map.find( options.opt_t );
    if ( tm == type_map.end() ) {
      std::cerr << "invalid type '" << options.opt_t << "'" << std::endl;
      return false;
    }
  }

  // the user specified display field must exist
  if ( options.opt_f != "" ) {
    std::set<std::string> empty;
    DisplayFields temp_fields;
    util::tokenize( options.opt_f, temp_fields, ',', empty );
    for ( DisplayFields::const_iterator dc = temp_fields.begin(); dc != temp_fields.end(); dc++ ) {
      std::string field = "";
      bool hidden = false;
      Filter temp_filter;
      size_t p = (*dc).find_first_not_of("abcdefghijklmnopqrstuvwxyz_0123456789*");
      if ( p != std::string::npos ) {
        if ( (*dc)[p] != '=' && (*dc)[p] != '-' ) {
          std::cerr << "invalid field specification '" << *dc << "'" << std::endl;
          return false;
        }
        temp_filter.negate = (*dc)[p] == '-';
        temp_filter.filter = (*dc).substr(p+1);
        field = (*dc).substr(0,p);
      } else {
        field = *dc;
        temp_filter.filter = "";
        temp_filter.negate = false;
      }
      if ( field.length()> 0 && field[0] == '*' ) {
        hidden = true;
        field = field.substr(1);
      }
      TypeFields::const_iterator found = type_fields.find(field);
      if ( found != type_fields.end() ) {
        if (!hidden ) display_fields.push_back(field);
        if ( temp_filter.filter != "" ) {
          filter_map[field] = temp_filter;
        }
      } else {
        std::cerr << "invalid field '" << field << "'" << std::endl;
        return false;
      }
    }
  }

  return true;
};

void printVersion() {
   std::cout << LEANUX_VERSION << std::endl;
}

/**
 * Print command help.
 */
void printHelp() {
  std::cout << "lsys - system device viewer" << std::endl;
  std::cout << "  usage:" << std::endl;
  std::cout << "    lsys <device>" << std::endl;
  std::cout << "    lsys [-t type] [-f fieldspec,fieldspec,..]" << std::endl;
  std::cout << std::endl;
  std::cout << "    fieldspec is either " << std::endl;
  std::cout << "      field        : display the field " << std::endl;
  std::cout << "      field=regex  : display the field and match" << std::endl;
  std::cout << "      field-regex  : display the field and negate match" << std::endl;
  std::cout << "      *field=regex : hide the field and match" << std::endl;
  std::cout << "      *field-regex : hide the field and negate match" << std::endl;
  std::cout << std::endl;
  std::cout << "    valid types (-t): " << std::endl;
  for ( TypeMap::const_iterator t = type_map.begin(); t != type_map.end(); ++t ) {
    std::cout << "      " << std::setw(12) << t->first << " ( ";
    for ( std::set<std::string>::const_iterator f = rtype_fields[t->second].begin();
          f != rtype_fields[t->second].end(); f++ ) {
      if ( f != rtype_fields[t->second].begin() ) std::cout << ", ";
      std::cout << *f;
    }
    std::cout << " )" << std::endl;
  }
  std::cout << std::endl;
  std::cout << "    valid fields (-f): " << std::endl;

  for ( TypeFields::const_iterator f = type_fields.begin(); f != type_fields.end(); ++f ) {
    std::cout << "      " << std::setw(12) << f->first << " ( ";
    for ( std::set<sysdevice::SysDevice::SysDeviceType>::const_iterator t = f->second.begin(); t != f->second.end(); t++ ) {
      if ( t != f->second.begin() ) std::cout << ", ";
      std::cout << rtype_map[*t];
    }
    std::cout << " )" << std::endl;
  }
}

/**
 * attempt to make sense of user provided device string.
 */
std::string fullSysDevice( const std::string &device ) {
  std::list<std::string> trylist;
  trylist.push_back( device );
  trylist.push_back( "/sys/devices/" + device );
  trylist.push_back( "/sys/class/block/" + device );
  trylist.push_back( "/sys/class/net/" + device );
  trylist.push_back( "/sys/bus/pci/devices/" + device );
  trylist.push_back( "/sys/bus/usb/devices/" + device );

  for ( std::list<std::string>::const_iterator i = trylist.begin(); i != trylist.end(); i++ ) {
    std::string rp = util::realPath( *i );
    if ( util::directoryExists(rp) || util::fileReadAccess(rp) ) {
      return rp;
    }
  }
  return "";
}

void sysfsTree( const std::string &sysfs) {
  tools::Tabular tab;
  tab.addColumn( "sysfs device", false );
  tab.addColumn( "type", false );
  tab.addColumn( "driver", false );
  tab.addColumn( "class", false );
  tab.addColumn( "attributes", false );
  std::list<leanux::sysdevice::SysDevice*> devices;
  size_t path_indent = 0;
  if ( leanux::sysdevice::treeDetect( sysfs, devices ) ) {
    if ( !devices.size() ) std::cerr << "0 devices found" << std::endl;
    unsigned int depth = 0;
    for ( std::list<leanux::sysdevice::SysDevice*>::const_iterator d = devices.begin(); d != devices.end(); d++, depth++ ) {
      tab.appendString( "sysfs device", std::string(depth,' ') + util::shortenString( (*d)->getPath().substr(path_indent), 160, '_' ) );
      tab.appendString( "type", leanux::sysdevice::SysDevice::getTypeStr( (*d)->getType() ) );
      tab.appendString( "driver", (*d)->getDriver()  );
      tab.appendString( "class", (*d)->getClass()  );
      tab.appendString( "attributes", (*d)->getDescription() );
      path_indent += ((*d)->getPath().length() - path_indent) + 1;
    }
    tab.dump(std::cout);
    leanux::sysdevice::destroy( devices );
  } else std::cerr << "detection error" << std::endl;
}

/**
 * get named field attributes for the SysDevice.
 */
std::string getLeafColStr( const sysdevice::SysDevice* leaf, const std::string &field ) {
  if ( field == "dev_t" ) {
    const sysdevice::BlockDevice* block_device = dynamic_cast<const sysdevice::BlockDevice*>(leaf);
    if ( block_device ) {
      std::stringstream ss;
      ss << block_device->getMajorMinor();
      return ss.str();
    }
  } else if ( field == "class" ) {
    const sysdevice::PCIDevice* pci_device = dynamic_cast<const sysdevice::PCIDevice*>(leaf);
    if ( pci_device ) {
      pci::PCIHardwareId hwid = pci::getPCIHardwareId( leaf->getPath() );
      pci::PCIHardwareInfo hwinfo;
      if ( pci::getPCIHardwareInfo( hwid, hwinfo ) ) {
        return hwinfo.pciclass;
      }
    }
    const sysdevice::USBInterface* usb_iface = dynamic_cast<const sysdevice::USBInterface*>(leaf);
    if ( usb_iface ) {
      return usb::getUSBHardwareClassStr( usb::getUSBInterfaceHardwareClass( leaf->getPath() ) );
    }
    const sysdevice::USBBus* usb_bus = dynamic_cast<const sysdevice::USBBus*>(leaf);
    if ( usb_bus ) {
      return usb::getUSBHardwareClassStr( usb::getUSBDeviceHardwareClass( leaf->getPath() ) );
    }
    const sysdevice::USBDevice* usb_device = dynamic_cast<const sysdevice::USBDevice*>(leaf);
    if ( usb_device ) {
      return usb::getUSBHardwareClassStr( usb::getUSBDeviceHardwareClass( leaf->getPath() ) );
    }
  } else if ( field == "driver" ) {
    return leaf->getDriver();
  } else if ( field == "sysfs" ) {
    return leaf->getPath();
  } else if ( field == "ip4" ) {
    const sysdevice::NetDevice* net_device = dynamic_cast<const sysdevice::NetDevice*>(leaf);
    if ( net_device ) {
      std::list<std::string> ip4;
      net::getDeviceIP4Addresses( util::leafDir( net_device->getPath() ), ip4 );
      std::stringstream ss;
      for ( std::list<std::string>::const_iterator i = ip4.begin(); i != ip4.end(); i ++ ) {
        if ( i != ip4.begin() ) ss << ", ";
        ss << *i;
      }
      return ss.str();
    }
  } else if ( field == "ip6" ) {
    const sysdevice::NetDevice* net_device = dynamic_cast<const sysdevice::NetDevice*>(leaf);
    if ( net_device ) {
      std::list<std::string> ip6;
      net::getDeviceIP6Addresses( util::leafDir( net_device->getPath() ), ip6 );
      std::stringstream ss;
      for ( std::list<std::string>::const_iterator i = ip6.begin(); i != ip6.end(); i ++ ) {
        if ( i != ip6.begin() ) ss << ", ";
        ss << *i;
      }
      return ss.str();
    }
  } else if ( field == "iqn" ) {
    const sysdevice::iSCSIDevice* iscsi_device = dynamic_cast<const sysdevice::iSCSIDevice*>(leaf);
    if ( iscsi_device ) {
      return iscsi_device->getTargetIQN();
    }
  } else if ( field == "irq" ) {
    const sysdevice::PCIBus* pci_bus = dynamic_cast<const sysdevice::PCIBus*>(leaf);
    if ( pci_bus ) {
      std::stringstream ss;
      ss << pci::getPCIDeviceIRQ( pci_bus->getPath() );
      return ss.str();
    }
    const sysdevice::PCIDevice* pci_device = dynamic_cast<const sysdevice::PCIDevice*>(leaf);
    if ( pci_device ) {
      std::stringstream ss;
      ss << pci::getPCIDeviceIRQ( pci_device->getPath() );
      return ss.str();
    }
  } else if ( field == "model" ) {
    const sysdevice::BlockDevice* block_device = dynamic_cast<const sysdevice::BlockDevice*>(leaf);
    if ( block_device ) {
      return block::getModel( block_device->getMajorMinor() );
    }
    const sysdevice::PCIDevice* pci_device = dynamic_cast<const sysdevice::PCIDevice*>(leaf);
    if ( pci_device ) {
      pci::PCIHardwareId hwid = pci::getPCIHardwareId( leaf->getPath() );
      pci::PCIHardwareInfo hwinfo;
      if ( pci::getPCIHardwareInfo( hwid, hwinfo ) ) {
        return hwinfo.device;
      }
    }
    const sysdevice::PCIBus* pci_bus = dynamic_cast<const sysdevice::PCIBus*>(leaf);
    if ( pci_bus ) {
      pci::PCIHardwareId hwid = pci::getPCIHardwareId( leaf->getPath() );
      pci::PCIHardwareInfo hwinfo;
      if ( pci::getPCIHardwareInfo( hwid, hwinfo ) ) {
        return hwinfo.device;
      }
    }
    const sysdevice::USBDevice* usb_device = dynamic_cast<const sysdevice::USBDevice*>(leaf);
    if ( usb_device ) {
      usb::USBHardwareId hwid;
      getUSBHardwareId( leaf->getPath(), hwid );
      usb::USBHardwareInfo info;
      if ( getUSBHardwareInfo( hwid, info ) ) {
        return info.idProduct;
      }
    }
  } else if ( field == "read" ) {
    const sysdevice::BlockDevice* block_device = dynamic_cast<const sysdevice::BlockDevice*>(leaf);
    if ( block_device ) {
      block::DeviceStats stats;
      block::getStats( block_device->getMajorMinor(), stats );
      std::stringstream ss;
      ss << util::ByteStr( stats.read_sectors * getSectorSize( block_device->getMajorMinor() ), 3 );
      return ss.str();
    }
    const sysdevice::NetDevice* net_device = dynamic_cast<const sysdevice::NetDevice*>(leaf);
    if ( net_device ) {
      std::string device = util::leafDir( net_device->getPath() );
      net::NetStatDeviceMap stats;
      net::getNetStat( stats );
      std::stringstream ss;
      ss << util::ByteStr( stats[device].rx_bytes, 3 );
      return ss.str();
    }
  } else if ( field == "sched" ) {
    const sysdevice::BlockDevice* block_device = dynamic_cast<const sysdevice::BlockDevice*>(leaf);
    if ( block_device ) {
      return getIOScheduler( block_device->getMajorMinor() );
    }
  } else if ( field == "size" ) {
    const sysdevice::BlockDevice* block_device = dynamic_cast<const sysdevice::BlockDevice*>(leaf);
    if ( block_device ) {
      return util::ByteStr( block::getSize( block_device->getMajorMinor() ), 3 );
    }
  } else if ( field == "speed" ) {
    const sysdevice::ATAPort* ata_port = dynamic_cast<const sysdevice::ATAPort*>(leaf);
    if ( ata_port ) {
      std::stringstream ss;
      ss << "ata" << std::hex << ata_port->getPort();
      std::string ata_port = ss.str();
      std::string ata_link = block::getATAPortLink( ata_port );
      return block::getATALinkSpeed( ata_port, ata_link );
    }
    const sysdevice::NetDevice* net_device = dynamic_cast<const sysdevice::NetDevice*>(leaf);
    if ( net_device ) {
      return util::ByteStr( net::getDeviceSpeed( util::leafDir( net_device->getPath() ) ) * 1.0E6 / 8.0, 3 );
    }
  } else if ( field == "svctm" ) {
    const sysdevice::BlockDevice* block_device = dynamic_cast<const sysdevice::BlockDevice*>(leaf);
    if ( block_device ) {
      block::DeviceStats stats;
      block::getStats( block_device->getMajorMinor(), stats );
      if ( stats.reads+stats.writes > 0 )
        return util::TimeStrSec( (stats.io_ms/1000.0) / (double)(stats.reads+stats.writes) );
    }
  } else if ( field == "target" ) {
    const sysdevice::iSCSIDevice* iscsi_device = dynamic_cast<const sysdevice::iSCSIDevice*>(leaf);
    if ( iscsi_device ) {
      std::stringstream ss;
      ss << iscsi_device->getTargetAddress() << ":" << iscsi_device->getTargetPort();
      return ss.str();
    }
  } else if ( field == "uuid" ) {
    const sysdevice::MapperDevice* mapper_device = dynamic_cast<const sysdevice::MapperDevice*>(leaf);
    if ( mapper_device ) {
      return block::getDMUUID( mapper_device->getMajorMinor() );
    }
  } else if ( field == "vendor" ) {
    const sysdevice::PCIDevice* pci_device = dynamic_cast<const sysdevice::PCIDevice*>(leaf);
    if ( pci_device ) {
      pci::PCIHardwareId hwid = pci::getPCIHardwareId( leaf->getPath() );
      pci::PCIHardwareInfo hwinfo;
      if ( pci::getPCIHardwareInfo( hwid, hwinfo ) ) {
        return hwinfo.vendor;
      }
    }
    const sysdevice::PCIBus* pci_bus = dynamic_cast<const sysdevice::PCIBus*>(leaf);
    if ( pci_bus ) {
      pci::PCIHardwareId hwid = pci::getPCIHardwareId( leaf->getPath() );
      pci::PCIHardwareInfo hwinfo;
      if ( pci::getPCIHardwareInfo( hwid, hwinfo ) ) {
        return hwinfo.vendor;
      }
    }
    const sysdevice::USBDevice* usb_device = dynamic_cast<const sysdevice::USBDevice*>(leaf);
    if ( usb_device ) {
      usb::USBHardwareId hwid;
      getUSBHardwareId( leaf->getPath(), hwid );
      usb::USBHardwareInfo info;
      if ( getUSBHardwareInfo( hwid, info ) ) {
        return info.idVendor;
      }
    }
  } else if ( field == "written" ) {
    const sysdevice::BlockDevice* block_device = dynamic_cast<const sysdevice::BlockDevice*>(leaf);
    if ( block_device ) {
      block::DeviceStats stats;
      block::getStats( block_device->getMajorMinor(), stats );
      std::stringstream ss;
      ss << util::ByteStr( stats.write_sectors * block::getSectorSize( block_device->getMajorMinor() ), 3 );
      return ss.str();
    }
    const sysdevice::NetDevice* net_device = dynamic_cast<const sysdevice::NetDevice*>(leaf);
    if ( net_device ) {
      std::string device = util::leafDir( net_device->getPath() );
      net::NetStatDeviceMap stats;
      net::getNetStat( stats );
      std::stringstream ss;
      ss << util::ByteStr( stats[device].tx_bytes, 3 );
      return ss.str();
    }
  } else if ( field == "wwn" ) {
    const sysdevice::SCSIDevice* scsi_device = dynamic_cast<const sysdevice::SCSIDevice*>(leaf);
    if ( scsi_device ) {
      return block::getWWN( scsi_device->getMajorMinor() );
    }
  }
  return "";
}

/**
 * list devices, filtered if opt_t is set.
 */
void listDevices( const std::list<sysdevice::SysDevicePath> &devices, sysdevice::SysDevice::SysDeviceType t ) {
  tools::Tabular tab;
  tab.addColumn( "sysfs", false );
  tab.addColumn( "type", false );
  for ( DisplayFields::const_iterator dc = display_fields.begin(); dc != display_fields.end(); dc++ ) {
    if ( *dc != "sysfs" && *dc != "type" ) tab.addColumn( *dc, false );
  }
  for ( std::list<sysdevice::SysDevicePath>::const_iterator d = devices.begin(); d != devices.end(); d++ ) {
    std::string parent = "";
    sysdevice::SysDevice* leaf = sysdevice::leafDetect( *d, parent );
    if ( leaf ) {
      if ( options.opt_t == "" || leaf->matchSysDeviceType( t ) ) {
        std::map<std::string,std::string> temp;
        bool include = true;
        for ( FilterMap::const_iterator fm = filter_map.begin(); fm != filter_map.end(); ++fm ) {
          std::string s = getLeafColStr( leaf, fm->first );
          util::RegExp reg;
          reg.set( fm->second.filter );
          if ( (!fm->second.negate && !reg.match( s ) ) ||
               (fm->second.negate && reg.match( s ) ) ) {
            include = false;
            break;
          }
        }
        if ( include ) {
          temp["sysfs"] = leaf->getPath();
          temp["type"] = leanux::sysdevice::SysDevice::getTypeStr( leaf->getType() );
          for ( DisplayFields::const_iterator dc = display_fields.begin(); dc != display_fields.end(); dc++ ) {
            temp[*dc] = getLeafColStr( leaf, *dc );
          }
          for ( std::map<std::string,std::string>::const_iterator t = temp.begin(); t != temp.end(); ++t ) {
            tab.appendString( t->first, t->second );
          }
        }
      }
      delete leaf;
    }
  }
  tab.dump(std::cout);
}

/**
 * Run based on options.
 */
void runOptions() {
  if ( options.opt_h ) {
    printHelp();
    return;
  } else if ( options.opt_v ) {
    printVersion();
    return;
  }
  if ( options.device == "" ) {
    std::list<sysdevice::SysDevicePath> devices;
    sysdevice::enumDevices( devices );
    devices.sort( stlhexnatstrlt );
    listDevices( devices, type_map[options.opt_t] );
  } else {
    std::string sysfs = fullSysDevice(options.device);
    if ( sysfs != "" ) sysfsTree( sysfs ); else std::cerr << "cannot find '" << options.device << "'" << std::endl;
  }
}

/**
 * register devices types and maintain mappings between type and string representation.
 */
void registerType( const std::string & st, sysdevice::SysDevice::SysDeviceType et ) {
  type_map[st] = et;
  rtype_map[et] = st;
}

void registerField( const std::string& field, sysdevice::SysDevice::SysDeviceType type ) {
   type_fields[field].insert( type );
   rtype_fields[type].insert( field );
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

    registerType( "ata" , sysdevice::SysDevice::sdtATAPort );
    registerType( "bcache" , sysdevice::SysDevice::sdtBCacheDevice );
    registerType( "block" , sysdevice::SysDevice::sdtBlock );
    registerType( "iscsi" , sysdevice::SysDevice::sdtiSCSIDevice );
    registerType( "mapper" , sysdevice::SysDevice::sdtMapperDevice );
    registerType( "metadisk" , sysdevice::SysDevice::sdtMetaDisk );
    registerType( "mmc" , sysdevice::SysDevice::sdtMMCDevice );
    registerType( "net" , sysdevice::SysDevice::sdtNetDevice );
    registerType( "nvme" , sysdevice::SysDevice::sdtNVMeDevice );
    registerType( "part" , sysdevice::SysDevice::sdtBlockPartition );
    registerType( "pci" , sysdevice::SysDevice::sdtPCIDevice );
    registerType( "pcibus" , sysdevice::SysDevice::sdtPCIBus );
    registerType( "rport" , sysdevice::SysDevice::sdtSCSIRPort );
    registerType( "vport" , sysdevice::SysDevice::sdtSCSIVPort );
    registerType( "scsi" , sysdevice::SysDevice::sdtSCSIDevice );
    registerType( "usb" , sysdevice::SysDevice::sdtUSBDevice );
    registerType( "usbbus" , sysdevice::SysDevice::sdtUSBBus );
    registerType( "usbiface" , sysdevice::SysDevice::sdtUSBInterface );
    registerType( "virtioblk" , sysdevice::SysDevice::sdtVirtioBlockDevice );
    registerType( "virtionet" , sysdevice::SysDevice::sdtVirtioNetDevice );


    registerField( "dev_t", sysdevice::SysDevice::sdtBlock );
    registerField( "dev_t", sysdevice::SysDevice::sdtBlockPartition );
    registerField( "dev_t", sysdevice::SysDevice::sdtiSCSIDevice );
    registerField( "dev_t", sysdevice::SysDevice::sdtMMCDevice );
    registerField( "dev_t", sysdevice::SysDevice::sdtNVMeDevice );
    registerField( "dev_t", sysdevice::SysDevice::sdtSCSIDevice );
    registerField( "dev_t", sysdevice::SysDevice::sdtVirtioBlockDevice );
    registerField( "dev_t", sysdevice::SysDevice::sdtMapperDevice );
    registerField( "dev_t", sysdevice::SysDevice::sdtMetaDisk );

    //registerField( "class", sysdevice::SysDevice::sdtPCIBus );
    registerField( "class", sysdevice::SysDevice::sdtPCIDevice );
    registerField( "class", sysdevice::SysDevice::sdtUSBBus );
    registerField( "class", sysdevice::SysDevice::sdtUSBDevice );
    registerField( "class", sysdevice::SysDevice::sdtUSBInterface );

    registerField( "driver", sysdevice::SysDevice::sdtBlock );
    registerField( "driver", sysdevice::SysDevice::sdtiSCSIDevice );
    registerField( "driver", sysdevice::SysDevice::sdtMMCDevice );
    registerField( "driver", sysdevice::SysDevice::sdtNVMeDevice );
    registerField( "driver", sysdevice::SysDevice::sdtPCIDevice );
    registerField( "driver", sysdevice::SysDevice::sdtPCIBus );
    registerField( "driver", sysdevice::SysDevice::sdtSCSIDevice );
    registerField( "driver", sysdevice::SysDevice::sdtUSBDevice );
    registerField( "driver", sysdevice::SysDevice::sdtVirtioBlockDevice );
    registerField( "driver", sysdevice::SysDevice::sdtVirtioNetDevice );

    registerField( "ip4", sysdevice::SysDevice::sdtNetDevice );
    registerField( "ip6", sysdevice::SysDevice::sdtNetDevice );

    registerField( "iqn", sysdevice::SysDevice::sdtiSCSIDevice );
    registerField( "irq", sysdevice::SysDevice::sdtPCIDevice );
    registerField( "irq", sysdevice::SysDevice::sdtPCIBus );

    registerField( "model", sysdevice::SysDevice::sdtBlock );
    registerField( "model", sysdevice::SysDevice::sdtiSCSIDevice );
    registerField( "model", sysdevice::SysDevice::sdtMMCDevice );
    registerField( "model", sysdevice::SysDevice::sdtNVMeDevice );
    registerField( "model", sysdevice::SysDevice::sdtPCIDevice );
    registerField( "model", sysdevice::SysDevice::sdtSCSIDevice );
    registerField( "model", sysdevice::SysDevice::sdtUSBDevice );
    registerField( "model", sysdevice::SysDevice::sdtVirtioBlockDevice );
    registerField( "model", sysdevice::SysDevice::sdtVirtioNetDevice );

    registerField( "read", sysdevice::SysDevice::sdtBlock );
    registerField( "read", sysdevice::SysDevice::sdtBlockPartition );
    registerField( "read", sysdevice::SysDevice::sdtiSCSIDevice );
    registerField( "read", sysdevice::SysDevice::sdtMMCDevice );
    registerField( "read", sysdevice::SysDevice::sdtNVMeDevice );
    registerField( "read", sysdevice::SysDevice::sdtSCSIDevice );
    registerField( "read", sysdevice::SysDevice::sdtVirtioBlockDevice );
    registerField( "read", sysdevice::SysDevice::sdtVirtioNetDevice );
    registerField( "read", sysdevice::SysDevice::sdtMapperDevice );
    registerField( "read", sysdevice::SysDevice::sdtNetDevice );
    registerField( "read", sysdevice::SysDevice::sdtMetaDisk );

    registerField( "size", sysdevice::SysDevice::sdtBlock );
    registerField( "size", sysdevice::SysDevice::sdtBlockPartition );
    registerField( "size", sysdevice::SysDevice::sdtiSCSIDevice );
    registerField( "size", sysdevice::SysDevice::sdtMMCDevice );
    registerField( "size", sysdevice::SysDevice::sdtNVMeDevice );
    registerField( "size", sysdevice::SysDevice::sdtSCSIDevice );
    registerField( "size", sysdevice::SysDevice::sdtVirtioBlockDevice );
    registerField( "size", sysdevice::SysDevice::sdtVirtioNetDevice );
    registerField( "size", sysdevice::SysDevice::sdtMapperDevice );
    registerField( "size", sysdevice::SysDevice::sdtMetaDisk );

    registerField( "speed", sysdevice::SysDevice::sdtATAPort );
    registerField( "speed", sysdevice::SysDevice::sdtNetDevice );

    registerField( "svctm", sysdevice::SysDevice::sdtBlock );
    registerField( "svctm", sysdevice::SysDevice::sdtBlockPartition );
    registerField( "svctm", sysdevice::SysDevice::sdtiSCSIDevice );
    registerField( "svctm", sysdevice::SysDevice::sdtMMCDevice );
    registerField( "svctm", sysdevice::SysDevice::sdtNVMeDevice );
    registerField( "svctm", sysdevice::SysDevice::sdtSCSIDevice );
    registerField( "svctm", sysdevice::SysDevice::sdtVirtioBlockDevice );
    registerField( "svctm", sysdevice::SysDevice::sdtMapperDevice );

    registerField( "target", sysdevice::SysDevice::sdtiSCSIDevice );

    registerField( "sched", sysdevice::SysDevice::sdtBlock );
    registerField( "sched", sysdevice::SysDevice::sdtiSCSIDevice );
    registerField( "sched", sysdevice::SysDevice::sdtMMCDevice );
    registerField( "sched", sysdevice::SysDevice::sdtNVMeDevice );
    registerField( "sched", sysdevice::SysDevice::sdtSCSIDevice );
    registerField( "sched", sysdevice::SysDevice::sdtVirtioBlockDevice );
    registerField( "sched", sysdevice::SysDevice::sdtVirtioNetDevice );

    registerField( "sysfs", sysdevice::SysDevice::sdtATAPort );
    registerField( "sysfs", sysdevice::SysDevice::sdtBCacheDevice );
    registerField( "sysfs", sysdevice::SysDevice::sdtBlock );
    registerField( "sysfs", sysdevice::SysDevice::sdtBlockPartition );
    registerField( "sysfs", sysdevice::SysDevice::sdtiSCSIDevice );
    registerField( "sysfs", sysdevice::SysDevice::sdtMapperDevice );
    registerField( "sysfs", sysdevice::SysDevice::sdtMetaDisk );
    registerField( "sysfs", sysdevice::SysDevice::sdtMMCDevice );
    registerField( "sysfs", sysdevice::SysDevice::sdtNetDevice );
    registerField( "sysfs", sysdevice::SysDevice::sdtNVMeDevice );
    registerField( "sysfs", sysdevice::SysDevice::sdtPCIDevice );
    registerField( "sysfs", sysdevice::SysDevice::sdtSCSIDevice );
    registerField( "sysfs", sysdevice::SysDevice::sdtUSBDevice );
    registerField( "sysfs", sysdevice::SysDevice::sdtUSBBus );
    registerField( "sysfs", sysdevice::SysDevice::sdtUSBInterface );
    registerField( "sysfs", sysdevice::SysDevice::sdtVirtioBlockDevice );
    registerField( "sysfs", sysdevice::SysDevice::sdtVirtioNetDevice );
    registerField( "sysfs", sysdevice::SysDevice::sdtSCSIRPort );
    registerField( "sysfs", sysdevice::SysDevice::sdtSCSIVPort );

    registerField( "uuid", sysdevice::SysDevice::sdtMapperDevice );

    registerField( "vendor", sysdevice::SysDevice::sdtPCIDevice );
    registerField( "vendor", sysdevice::SysDevice::sdtUSBDevice );

    registerField( "written", sysdevice::SysDevice::sdtBlock );
    registerField( "written", sysdevice::SysDevice::sdtBlockPartition );
    registerField( "written", sysdevice::SysDevice::sdtiSCSIDevice );
    registerField( "written", sysdevice::SysDevice::sdtMMCDevice );
    registerField( "written", sysdevice::SysDevice::sdtNVMeDevice );
    registerField( "written", sysdevice::SysDevice::sdtSCSIDevice );
    registerField( "written", sysdevice::SysDevice::sdtVirtioBlockDevice );
    registerField( "written", sysdevice::SysDevice::sdtVirtioNetDevice );
    registerField( "written", sysdevice::SysDevice::sdtMapperDevice );
    registerField( "written", sysdevice::SysDevice::sdtNetDevice );
    registerField( "written", sysdevice::SysDevice::sdtMetaDisk );

    registerField( "wwn", sysdevice::SysDevice::sdtSCSIDevice );
    registerField( "wwn", sysdevice::SysDevice::sdtiSCSIDevice );

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
