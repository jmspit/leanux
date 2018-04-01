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
 * leanux::SysDevice c++ source file.
 */

#include "device.hpp"
#include "block.hpp"
#include "util.hpp"
#include "oops.hpp"
#include "net.hpp"
#include "pci.hpp"
#include "usb.hpp"
#include "block.hpp"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <dirent.h>
#include <errno.h>

namespace leanux {

  namespace sysdevice {

    const SysDevicePath sysdevice_root = "/sys/devices";
    const SysDevicePath sysbus_root = "/sys/bus";

    void SysDevice::tokenize( const SysDevicePath &path, std::list<std::string> &tokens ) {
      std::set<std::string> omit;
      omit.insert("sys");
      omit.insert("devices");
      util::tokenize( path, tokens, '/', omit, true );
    }

    bool SysDevice::validatePath( const SysDevicePath &path ) {
      if ( leanux::util::directoryExists( path ) || leanux::util::fileReadAccess( path ) ) {
        if ( strncmp( path.c_str(), sysdevice_root.c_str(), 12 ) == 0 ) return true;
      }
      return false;
    }

    void SysDevice::getPropertyValueList( PropertyValueList &pv ) const {
      pv.clear();
      pv.push_back( PropertyValue( "sysfs", path_ ) );
      if ( getDriver().length() > 0 )
        pv.push_back( PropertyValue( "driver", getDriver() ) );
      pv.push_back( PropertyValue( "type", getTypeStr( sysdevicetype_ ) ) );
      if ( getClass().length() > 0 )
        pv.push_back( PropertyValue( "class", getClass() ) );
    }

    std::string SysDevice::getDisplayName() const {
      return leaf_;
    }

    block::MajorMinor BlockDevice::getMajorMinor() const {
      std::string devpath = path_ + "/dev";
      if ( util::fileReadAccess( devpath ) ) {
        std::string dev = util::fileReadString( devpath );
        return block::MajorMinor( dev );
      } else return block::MajorMinor::invalid;
    }

    void BlockDevice::getPropertyValueList( PropertyValueList &pv ) const {
      pv.clear();
      SysDevice::getPropertyValueList( pv );
      pv.push_back( PropertyValue( "model", getMajorMinor().getModel() ) );
      std::stringstream ss;
      ss << getMajorMinor();
      pv.push_back( PropertyValue( "major:minor", ss.str() ) );
      ss.str("");
      pv.push_back( PropertyValue( "SCSI hctl", getMajorMinor().getSCSIHCTL() ) );
      pv.push_back( PropertyValue( "modalias", getMajorMinor().getKernelModule() ) );
      pv.push_back( PropertyValue( "WWN", getMajorMinor().getWWN() ) );
      pv.push_back( PropertyValue( "serial number", getMajorMinor().getSerial() ) );
      ss.str("");
      ss << leanux::util::ByteStr( getMajorMinor().getSize(), 2 );
      pv.push_back( PropertyValue( "size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr( getMajorMinor().getReadAhead(), 2 );
      pv.push_back( PropertyValue( "read-ahead", ss.str() ) );
      ss.str("");
      pv.push_back( PropertyValue( "firmware", getMajorMinor().getRevision() ) );
      pv.push_back( PropertyValue( "rotational", getMajorMinor().getRotationalStr() ) );
      ss << leanux::util::ByteStr(getMajorMinor().getSectorSize(), 0 );
      pv.push_back( PropertyValue( "sector size", ss.str() ) );
      ss.str("");

      ss << leanux::util::ByteStr(getMajorMinor().getMinIOSize(), 0 );
      pv.push_back( PropertyValue( "min IO size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr(getMajorMinor().getMaxIOSize(), 0 );
      pv.push_back( PropertyValue( "max IO size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr(getMajorMinor().getMaxHWIOSize(), 0 );
      pv.push_back( PropertyValue( "max hw IO size", ss.str() ) );
      ss.str("");
      pv.push_back( PropertyValue( "IO scheduler", getMajorMinor().getIOScheduler() ) );
      pv.push_back( PropertyValue( "Write cache", getMajorMinor().getCacheMode() ) );

      pv.push_back( PropertyValue( "udev path", getMajorMinor().getUDevPath() ) );
      std::list<std::string> aliases;
      getMajorMinor().getAliases( aliases );
      for ( std::list<std::string>::const_iterator i = aliases.begin(); i != aliases.end(); i++ ) {
        pv.push_back( PropertyValue( "alias", *i ) );
      }
    }

    std::string BlockDevice::getDisplayName() const {
      std::stringstream ss;
      ss << getMajorMinor().getName();
      if ( getMajorMinor().getModel().length() > 0 ) {
        ss << " " << getMajorMinor().getModel();
      }
      return ss.str();
    }

    std::string BlockDevice::getDescription() const {
      std::stringstream ss;
      block::MajorMinor mm = getMajorMinor();
      ss << "dev_t=" << mm;
      std::string model = mm.getModel();
      if ( model != "" ) ss << " model=" << model;
      return ss.str();
    }

    std::string PCIDevice::getDescription() const {
      std::stringstream ss;
      pci::PCIHardwareId hwid = pci::getPCIHardwareId( path_ );
      pci::PCIHardwareInfo hwinfo;
      if ( pci::getPCIHardwareInfo( hwid, hwinfo ) ) {
        ss << "model=" << hwinfo.device;
      }
      return ss.str();
    }

    std::string PCIDevice::getClass() const {
      std::stringstream ss;
      pci::PCIHardwareId hwid = pci::getPCIHardwareId( path_ );
      pci::PCIHardwareInfo hwinfo;
      if ( pci::getPCIHardwareInfo( hwid, hwinfo ) ) {
        ss << hwinfo.pciclass;
      }
      return ss.str();
    }

    std::string USBDevice::getDescription() const {
      std::stringstream ss;
      usb::USBHardwareId hwid;
      getUSBHardwareId( path_, hwid );
      usb::USBHardwareInfo info;
      if ( getUSBHardwareInfo( hwid, info ) ) {
        //ss << "vendor=" << info.idVendor;
        ss << "model=" << info.idProduct;
      }
      return ss.str();
    }

    std::string USBDevice::getClass() const {
      std::stringstream ss;
      usb::USBHardwareId hwid;
      getUSBHardwareId( path_, hwid );
      usb::USBHardwareInfo info;
      if ( getUSBHardwareInfo( hwid, info ) ) {
        ss << usb::getUSBHardwareClassStr( usb::getUSBDeviceHardwareClass( path_ ) );
      }
      return ss.str();
    }

    std::string USBDevice::getDisplayName() const {
      usb::USBHardwareId hwid;
      if ( getUSBHardwareId( path_, hwid ) ) {
        usb::USBHardwareInfo info;
        if ( getUSBHardwareInfo( hwid, info ) )
          return info.idProduct;
        else
          return SysDevice::getDisplayName();
      } else return SysDevice::getDisplayName();
    }

    void USBDevice::getPropertyValueList( PropertyValueList &pv ) const {
      pv.clear();
      SysDevice::getPropertyValueList( pv );
      usb::USBHardwareId hwid;
      getUSBHardwareId( path_, hwid );
      usb::USBHardwareInfo info;
      if ( getUSBHardwareInfo( hwid, info ) ) {
        pv.push_back( PropertyValue( "model", info.idProduct ) );
        pv.push_back( PropertyValue( "vendor", info.idVendor ) );
      }
    }

    std::string USBBus::getDescription() const {
      std::stringstream ss;
      usb::USBHardwareId hwid;
      getUSBHardwareId( path_, hwid );
      usb::USBHardwareInfo info;
      if ( getUSBHardwareInfo( hwid, info ) ) {
        ss << "model=" << info.idProduct;
      }
      return ss.str();
    }

    std::string USBBus::getClass() const {
      std::stringstream ss;
      usb::USBHardwareId hwid;
      getUSBHardwareId( path_, hwid );
      usb::USBHardwareInfo info;
      if ( getUSBHardwareInfo( hwid, info ) ) {
        ss << usb::getUSBHardwareClassStr( usb::getUSBDeviceHardwareClass( path_ ) );
      }
      return ss.str();
    }

    std::string USBBus::getDisplayName() const {
      usb::USBHardwareId hwid;
      getUSBHardwareId( path_, hwid );
      usb::USBHardwareInfo info;
      if ( getUSBHardwareInfo( hwid, info ) )
        return info.idProduct;
      else
        return SysDevice::getDisplayName();
    }

    void USBBus::getPropertyValueList( PropertyValueList &pv ) const {
      pv.clear();
      SysDevice::getPropertyValueList( pv );
      usb::USBHardwareId hwid;
      getUSBHardwareId( path_, hwid );
      usb::USBHardwareInfo info;
      if ( getUSBHardwareInfo( hwid, info ) ) {
        pv.push_back( PropertyValue( "model", info.idProduct ) );
        pv.push_back( PropertyValue( "vendor", info.idVendor ) );
      }
    }

    std::string USBInterface::getClass() const {
      std::stringstream ss;
      ss << usb::getUSBHardwareClassStr( usb::getUSBInterfaceHardwareClass( path_ ) );
      return ss.str();
    }

    std::string USBInterface::getDescription() const {
      return "";
    }

    std::string ATAPort::getDescription() const {
      std::stringstream ss;
      ss << "ata" << std::hex << port_;
      std::string ata_port = ss.str();
      std::string ata_link = block::getATAPortLink( ata_port );
      ss.str("");
      ss << "link=" << ata_link << " speed=" << block::getATALinkSpeed( ata_port, ata_link );
      return ss.str();
    }

    bool VirtioBlockDevice::accept( SysDevicePath &path ) {
      if ( path.find("virtio") == std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_blkdev_name = 0;      // <vda>
      const unsigned int st_const_block = 1;      // block
      const unsigned int st_virtio_version = 2;   // virtio<version>
      const unsigned int st_accept = 3;
      const unsigned int st_fail = 4;
      unsigned int state = st_blkdev_name;
      util::RegExp reg;
      size_t eat_chars = 0;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_blkdev_name:
            reg.set( "(vd).*" );
            if ( reg.match( *t ) ) {
              state = st_const_block;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
          case st_const_block:
            if ( *t == "block" ) {
              state = st_virtio_version;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
          case st_virtio_version:
            reg.set( "(virtio)([[:digit:]]+)" );
            if ( reg.match( *t ) ) {
              state = st_accept;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
        }
      }
      if ( state == st_accept ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else return false;
    }

    bool VirtioNetDevice::accept( SysDevicePath &path ) {
      if ( path.find("virtio") == std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_net_name = 0;       // netdevname
      const unsigned int st_const_net = 1;      // net
      const unsigned int st_virtio_version = 2; // virtio<version>
      const unsigned int st_accept = 3;
      const unsigned int st_fail = 4;
      unsigned int state = st_net_name;
      util::RegExp reg;
      size_t eat_chars = 0;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_net_name:
            reg.set( ".*" );
            if ( reg.match( *t ) ) {
              state = st_const_net;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
          case st_const_net:
            if ( *t == "net" ) {
              state = st_virtio_version;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
          case st_virtio_version:
            reg.set( "(virtio)([[:digit:]]+)" );
            if ( reg.match( *t ) ) {
              state = st_accept;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
        }
      }
      if ( state == st_accept ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else return false;
    }

    bool MMCDevice::accept( SysDevicePath &path ) {
      if ( path.find("mmc_host") == std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_blkdev_name = 0;      // mmc<minor>
      const unsigned int st_mmc_host = 1;         // mmc_host
      const unsigned int st_accept = 2;
      const unsigned int st_fail = 3;
      unsigned int state = st_blkdev_name;
      util::RegExp reg;
      size_t eat_chars = 0;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_blkdev_name:
            reg.set( "(mmc)[[:xdigit:]]+" );
            if ( reg.match( *t ) ) {
              state = st_mmc_host;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
          case st_mmc_host:
            if ( *t == "mmc_host" ) {
              state = st_accept;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
        }
      }
      if ( state == st_accept ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else return false;
    }

    bool NVMeDevice::accept( SysDevicePath &path ) {
      if ( path.find("/nvme/") == std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_blkdev_name = 0;      // nvme0n1
      const unsigned int st_controller  = 1;      // nvme0
      const unsigned int st_nvmeroot    = 2;      // nvme
      const unsigned int st_accept = 3;
      const unsigned int st_fail = 4;
      unsigned int state = st_blkdev_name;
      util::RegExp reg;
      size_t eat_chars = 0;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_blkdev_name:
            reg.set( "(nvme)[[:xdigit:]]n[[:xdigit:]]" );
            if ( reg.match( *t ) ) {
              state = st_controller;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
          case st_controller:
            reg.set( "(nvme)[[:xdigit:]]" );
            if ( reg.match( *t ) ) {
              state = st_nvmeroot;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
          case st_nvmeroot:
            if ( *t == "nvme" ) {
              state = st_accept;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
        }
      }
      if ( state == st_accept ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else return false;
    }

    void NVMeDevice::getPropertyValueList( PropertyValueList &pv ) const {
      pv.clear();
      SysDevice::getPropertyValueList( pv );
      pv.push_back( PropertyValue( "model", getMajorMinor().getModel() ) );
      std::stringstream ss;
      ss << getMajorMinor();
      pv.push_back( PropertyValue( "major:minor", ss.str() ) );
      ss.str("");
      pv.push_back( PropertyValue( "modalias", getMajorMinor().getKernelModule() ) );
      pv.push_back( PropertyValue( "WWN", getMajorMinor().getWWN() ) );
      pv.push_back( PropertyValue( "serial number", getMajorMinor().getSerial() ) );
      ss.str("");
      ss << leanux::util::ByteStr( getMajorMinor().getSize(), 2 );
      pv.push_back( PropertyValue( "size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr( getMajorMinor().getReadAhead(), 2 );
      pv.push_back( PropertyValue( "read-ahead", ss.str() ) );
      ss.str("");
      pv.push_back( PropertyValue( "firmware", getMajorMinor().getRevision() ) );
      pv.push_back( PropertyValue( "rotational", getMajorMinor().getRotationalStr() ) );
      ss << leanux::util::ByteStr(getMajorMinor().getSectorSize(), 0 );
      pv.push_back( PropertyValue( "sector size", ss.str() ) );
      ss.str("");

      ss << leanux::util::ByteStr(getMajorMinor().getMinIOSize(), 0 );
      pv.push_back( PropertyValue( "min IO size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr(getMajorMinor().getMaxIOSize(), 0 );
      pv.push_back( PropertyValue( "max IO size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr(getMajorMinor().getMaxHWIOSize(), 0 );
      pv.push_back( PropertyValue( "max hw IO size", ss.str() ) );
      ss.str("");
      pv.push_back( PropertyValue( "IO scheduler", getMajorMinor().getIOScheduler() ) );
      pv.push_back( PropertyValue( "Write cache", getMajorMinor().getCacheMode() ) );

      pv.push_back( PropertyValue( "udev path", getMajorMinor().getUDevPath() ) );
      std::list<std::string> aliases;
      getMajorMinor().getAliases( aliases );
      for ( std::list<std::string>::const_iterator i = aliases.begin(); i != aliases.end(); i++ ) {
        pv.push_back( PropertyValue( "alias", *i ) );
      }
    }

    bool NVMePartition::accept( SysDevicePath &path ) {
      if ( path.find("/nvme/") == std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_blkdev_part = 0;      // nvme0n1p1
      const unsigned int st_blkdev_name = 1;      // nvme0n1
      const unsigned int st_controller  = 2;      // nvme0
      const unsigned int st_nvmeroot    = 3;      // nvme
      const unsigned int st_accept = 4;
      const unsigned int st_fail = 5;
      unsigned int state = st_blkdev_part;
      util::RegExp reg;
      size_t eat_chars = 0;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_blkdev_part:
            reg.set( "(nvme)[[:xdigit:]]n[[:xdigit:]]p[[:xdigit:]]" );
            if ( reg.match( *t ) ) {
              state = st_blkdev_name;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
          case st_blkdev_name:
            reg.set( "(nvme)[[:xdigit:]]n[[:xdigit:]]" );
            if ( reg.match( *t ) ) {
              state = st_controller;
            } else state = st_fail;
            break;
          case st_controller:
            reg.set( "(nvme)[[:xdigit:]]" );
            if ( reg.match( *t ) ) {
              state = st_nvmeroot;
            } else state = st_fail;
            break;
          case st_nvmeroot:
            if ( *t == "nvme" ) {
              state = st_accept;
            } else state = st_fail;
            break;
        }
      }
      if ( state == st_accept ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else return false;
    }

    void NVMePartition::getPropertyValueList( PropertyValueList &pv ) const {
      std::stringstream ss;
      pv.clear();
      SysDevice::getPropertyValueList( pv );
      ss << getMajorMinor();
      pv.push_back( PropertyValue( "major:minor", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr( getMajorMinor().getSize(), 2 );
      pv.push_back( PropertyValue( "size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr( getMajorMinor().getReadAhead(), 2 );
      pv.push_back( PropertyValue( "read-ahead", ss.str() ) );
      pv.push_back( PropertyValue( "udev path", getMajorMinor().getUDevPath() ) );
      std::list<std::string> aliases;
      getMajorMinor().getAliases( aliases );
      for ( std::list<std::string>::const_iterator i = aliases.begin(); i != aliases.end(); i++ ) {
        pv.push_back( PropertyValue( "alias", *i ) );
      }
    }

    bool MapperDevice::accept( SysDevicePath &path ) {
      if ( path.find("virtual/block") == std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_blkdev_name = 0;      // dm-<minor>
      const unsigned int st_const_block = 1;      // block
      const unsigned int st_virtual = 2;          // virtual
      const unsigned int st_accept = 3;
      const unsigned int st_fail = 4;
      unsigned int state = st_blkdev_name;
      util::RegExp reg;
      size_t eat_chars = 0;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_blkdev_name:
            reg.set( "(dm-).*" );
            if ( reg.match( *t ) ) {
              state = st_const_block;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
          case st_const_block:
            if ( *t == "block" ) {
              state = st_virtual;
              //eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
          case st_virtual:
            if ( *t == "virtual" ) {
              state = st_accept;
              //eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
        }
      }
      if ( state == st_accept ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else return false;
    }

    void MapperDevice::getPropertyValueList( PropertyValueList &pv ) const {
      pv.clear();
      SysDevice::getPropertyValueList( pv );
      std::stringstream ss;
      ss << getMajorMinor();
      pv.push_back( PropertyValue( "major:minor", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr( getMajorMinor().getSize(), 2 );
      pv.push_back( PropertyValue( "size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr( getMajorMinor().getReadAhead(), 2 );
      pv.push_back( PropertyValue( "read-ahead", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr(getMajorMinor().getSectorSize(), 0 );
      pv.push_back( PropertyValue( "sector size", ss.str() ) );
      ss.str("");

      ss << leanux::util::ByteStr(getMajorMinor().getMinIOSize(), 0 );
      pv.push_back( PropertyValue( "min IO size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr(getMajorMinor().getMaxIOSize(), 0 );
      pv.push_back( PropertyValue( "max IO size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr(getMajorMinor().getMaxHWIOSize(), 0 );
      pv.push_back( PropertyValue( "max hw IO size", ss.str() ) );
      ss.str("");

      pv.push_back( PropertyValue( "uuid", getMajorMinor().getDMUUID() ) );
      if (  getMajorMinor().getVGName().length() > 0 )
        pv.push_back( PropertyValue( "VG name", getMajorMinor().getVGName() ) );
      if (  getMajorMinor().getLVName().length() > 0 )
        pv.push_back( PropertyValue( "LV name", getMajorMinor().getLVName() ) );

      pv.push_back( PropertyValue( "udev path", getMajorMinor().getUDevPath() ) );
      std::list<std::string> aliases;
      getMajorMinor().getAliases( aliases );
      for ( std::list<std::string>::const_iterator i = aliases.begin(); i != aliases.end(); i++ ) {
        pv.push_back( PropertyValue( "alias", *i ) );
      }
    }

    bool LoopbackDevice::accept( SysDevicePath &path ) {
      if ( path.find("virtual/block/loop") == std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_loop_device = 0;
      const unsigned int st_block = 1;
      const unsigned int st_virtual = 2;
      const unsigned int st_fail = 3;
      const unsigned int st_accept = 4;
      unsigned int state = st_loop_device;
      size_t eat_chars = 0;
      util::RegExp reg;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_loop_device:
            reg.set( "loop([[:digit:]]+)" );
            if ( reg.match( *t ) ) {
              eat_chars += (*t).length() + 1;
              state = st_block;
            } else {
              state = st_fail;
            }
            break;
          case st_block:
            if ( (*t) == "block" ) {
              state = st_virtual;
            } else state = st_fail;
            break;
          case st_virtual:
            if ( (*t) == "virtual" ) {
              state = st_accept;
            } else state = st_fail;
            break;
        }
      }
      if ( state == st_accept && eat_chars ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else return false;
    }

    void LoopbackDevice::getPropertyValueList( PropertyValueList &pv ) const {
      pv.clear();
      SysDevice::getPropertyValueList( pv );
      std::stringstream ss;
      ss << getMajorMinor();
      pv.push_back( PropertyValue( "major:minor", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr( getMajorMinor().getReadAhead(), 2 );
      pv.push_back( PropertyValue( "read-ahead", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr(getMajorMinor().getSectorSize(), 0 );
      pv.push_back( PropertyValue( "sector size", ss.str() ) );
      ss.str("");

      ss << leanux::util::ByteStr(getMajorMinor().getMinIOSize(), 0 );
      pv.push_back( PropertyValue( "min IO size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr(getMajorMinor().getMaxIOSize(), 0 );
      pv.push_back( PropertyValue( "max IO size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr(getMajorMinor().getMaxHWIOSize(), 0 );
      pv.push_back( PropertyValue( "max hw IO size", ss.str() ) );
      ss.str("");
      pv.push_back( PropertyValue( "IO scheduler", getMajorMinor().getIOScheduler() ) );

      pv.push_back( PropertyValue( "udev path", getMajorMinor().getUDevPath() ) );
      std::list<std::string> aliases;
      getMajorMinor().getAliases( aliases );
      for ( std::list<std::string>::const_iterator i = aliases.begin(); i != aliases.end(); i++ ) {
        pv.push_back( PropertyValue( "alias", *i ) );
      }
    }

    bool RamDiskDevice::accept( SysDevicePath &path ) {
      if ( path.find("virtual/block/ram") == std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_ram_device = 0;
      const unsigned int st_block = 1;
      const unsigned int st_virtual = 2;
      const unsigned int st_fail = 3;
      const unsigned int st_accept = 4;
      unsigned int state = st_ram_device;
      size_t eat_chars = 0;
      util::RegExp reg;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_ram_device:
            reg.set( "ram([[:digit:]]+)" );
            if ( reg.match( *t ) ) {
              eat_chars += (*t).length() + 1;
              state = st_block;
            } else {
              state = st_fail;
            }
            break;
          case st_block:
            if ( (*t) == "block" ) {
              state = st_virtual;
            } else state = st_fail;
            break;
          case st_virtual:
            if ( (*t) == "virtual" ) {
              state = st_accept;
            } else state = st_fail;
            break;
        }
      }
      if ( state == st_accept && eat_chars ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else return false;
    }

    void RamDiskDevice::getPropertyValueList( PropertyValueList &pv ) const {
      pv.clear();
      SysDevice::getPropertyValueList( pv );
      std::stringstream ss;
      ss << getMajorMinor();
      pv.push_back( PropertyValue( "major:minor", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr( getMajorMinor().getSize(), 2 );
      pv.push_back( PropertyValue( "size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr( getMajorMinor().getReadAhead(), 2 );
      pv.push_back( PropertyValue( "read-ahead", ss.str() ) );
      ss.str("");
      pv.push_back( PropertyValue( "rotational", getMajorMinor().getRotationalStr() ) );
      ss << leanux::util::ByteStr(getMajorMinor().getSectorSize(), 0 );
      pv.push_back( PropertyValue( "sector size", ss.str() ) );
      ss.str("");

      ss << leanux::util::ByteStr(getMajorMinor().getMinIOSize(), 0 );
      pv.push_back( PropertyValue( "min IO size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr(getMajorMinor().getMaxIOSize(), 0 );
      pv.push_back( PropertyValue( "max IO size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr(getMajorMinor().getMaxHWIOSize(), 0 );
      pv.push_back( PropertyValue( "max hw IO size", ss.str() ) );
      ss.str("");
      pv.push_back( PropertyValue( "IO scheduler", getMajorMinor().getIOScheduler() ) );

      pv.push_back( PropertyValue( "udev path", getMajorMinor().getUDevPath() ) );
      std::list<std::string> aliases;
      getMajorMinor().getAliases( aliases );
      for ( std::list<std::string>::const_iterator i = aliases.begin(); i != aliases.end(); i++ ) {
        pv.push_back( PropertyValue( "alias", *i ) );
      }
    }

    bool BCacheDevice::accept( SysDevicePath &path ) {
      if ( path.find("virtual/block") == std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_blkdev_name = 0;      // bcacheminor>
      const unsigned int st_const_block = 1;      // block
      const unsigned int st_virtual = 2;          // virtual
      const unsigned int st_accept = 3;
      const unsigned int st_fail = 4;
      unsigned int state = st_blkdev_name;
      util::RegExp reg;
      size_t eat_chars = 0;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_blkdev_name:
            reg.set( "(bcache)([[:digit:]]+)" );
            if ( reg.match( *t ) ) {
              state = st_const_block;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
          case st_const_block:
            if ( *t == "block" ) {
              state = st_virtual;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
          case st_virtual:
            if ( *t == "virtual" ) {
              state = st_accept;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
        }
      }
      if ( state == st_accept ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars +1  );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else return false;
    }

    bool MDDevice::accept( SysDevicePath &path ) {
      if ( path.find("virtual/block") == std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_blkdev_name = 0;      // md<minor>
      const unsigned int st_const_block = 1;      // block
      const unsigned int st_virtual = 2;          // virtual
      const unsigned int st_accept = 3;
      const unsigned int st_fail = 4;
      unsigned int state = st_blkdev_name;
      util::RegExp reg;
      size_t eat_chars = 0;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_blkdev_name:
            reg.set( "(md).*" );
            if ( reg.match( *t ) ) {
              state = st_const_block;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
          case st_const_block:
            if ( *t == "block" ) {
              state = st_virtual;
            } else state = st_fail;
            break;
          case st_virtual:
            if ( *t == "virtual" ) {
              state = st_accept;
            } else state = st_fail;
            break;
        }
      }
      if ( state == st_accept ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else return false;
    }

    void MDDevice::getPropertyValueList( PropertyValueList &pv ) const {
      pv.clear();
      SysDevice::getPropertyValueList( pv );
      std::stringstream ss;
      ss << getMajorMinor();
      pv.push_back( PropertyValue( "major:minor", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr( getMajorMinor().getSize(), 2 );
      pv.push_back( PropertyValue( "size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr( getMajorMinor().getReadAhead(), 2 );
      pv.push_back( PropertyValue( "read-ahead", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr(getMajorMinor().getSectorSize(), 0 );
      pv.push_back( PropertyValue( "sector size", ss.str() ) );
      ss.str("");

      ss << leanux::util::ByteStr(getMajorMinor().getMinIOSize(), 0 );
      pv.push_back( PropertyValue( "min IO size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr(getMajorMinor().getMaxIOSize(), 0 );
      pv.push_back( PropertyValue( "max IO size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr(getMajorMinor().getMaxHWIOSize(), 0 );
      pv.push_back( PropertyValue( "max hw IO size", ss.str() ) );
      pv.push_back( PropertyValue( "IO scheduler", getMajorMinor().getIOScheduler() ) );

      pv.push_back( PropertyValue( "md name", getMajorMinor().getMDName() ) );
      pv.push_back( PropertyValue( "metadata", getMajorMinor().getMDMetaDataVersion() ) );
      pv.push_back( PropertyValue( "raid level", getMajorMinor().getMDLevel() ) );
      pv.push_back( PropertyValue( "array state", getMajorMinor().getMDArrayState() ) );
      ss.str("");
      ss << leanux::util::ByteStr( getMajorMinor().getMDChunkSize(), 0 );
      pv.push_back( PropertyValue( "chunk size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr( getMajorMinor().getMDDevices(), 0 );
      pv.push_back( PropertyValue( "#devices", ss.str() ) );


      pv.push_back( PropertyValue( "udev path", getMajorMinor().getUDevPath() ) );
      std::list<std::string> aliases;
      getMajorMinor().getAliases( aliases );
      for ( std::list<std::string>::const_iterator i = aliases.begin(); i != aliases.end(); i++ ) {
        pv.push_back( PropertyValue( "alias", *i ) );
      }
    }

    bool SASPort::accept( SysDevicePath &path ) {
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_port = 0;             // host<n>
      const unsigned int st_accept = 5;
      const unsigned int st_fail = 6;
      unsigned int state = st_port;
      util::RegExp reg;
      size_t eat_chars = 0;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_port:
            reg.set( "^port-[[:digit:]]+:[[:digit:]]+" );
            if ( reg.match( *t ) ) {
              state = st_accept;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
        }
      }
      if ( state == st_accept ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else {
        return false;
      }
    }

    bool SASEndDevice::accept( SysDevicePath &path ) {
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_device = 0;             // host<n>
      const unsigned int st_accept = 5;
      const unsigned int st_fail = 6;
      unsigned int state = st_device;
      util::RegExp reg;
      size_t eat_chars = 0;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_device:
            reg.set( "^end_device-[[:digit:]]+:[[:digit:]]+" );
            if ( reg.match( *t ) ) {
              state = st_accept;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
        }
      }
      if ( state == st_accept ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else {
        return false;
      }
    }

    std::string SCSIDevice::getDescription() const {
      std::stringstream ss;
      ss << BlockDevice::getDescription();
      ss << " hctl=" << address_;
      return ss.str();
    }

    bool SCSIDevice::accept( SysDevicePath &path ) {
      if ( path.find("target") == std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_blkdev_name = 0;      // <sda>
      const unsigned int st_const_block = 1;      // block
      const unsigned int st_scsi_addr = 2;        // <0:0:0:0>
      const unsigned int st_scsi_target = 3;      // target<0:0:0>
      const unsigned int st_accept = 4;
      const unsigned int st_fail = 5;
      unsigned int state = st_blkdev_name;
      util::RegExp reg;
      size_t eat_chars = 0;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_blkdev_name:
            reg.set( "^s(d|r).*" );
            if ( reg.match( *t ) ) {
              state = st_const_block;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
          case st_const_block:
            if ( *t == "block" ) {
              state = st_scsi_addr;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
          case st_scsi_addr:
            reg.set( "^[[:digit:]]+:[[:digit:]]+:[[:digit:]]+:[[:digit:]]+$" );
            if ( !reg.match( *t ) ) state = st_fail;
            else {
              state = st_scsi_target;
              address_ = *t;
              eat_chars += (*t).length() + 1;
            }
            break;
          case st_scsi_target:
            reg.set( "^(target)[[:digit:]]+:[[:digit:]]+:[[:digit:]]+$" );
            if ( !reg.match( *t ) ) state = st_fail;
            else {
              state = st_accept;
              eat_chars += (*t).length() + 1;
            }
            break;
        }
      }
      if ( state == st_accept ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else {
        address_ = "";
        return false;
      }
    }

    bool SCSIHost::accept( SysDevicePath &path ) {
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_host = 0;             // host<n>
      const unsigned int st_accept = 5;
      const unsigned int st_fail = 6;
      unsigned int state = st_host;
      util::RegExp reg;
      size_t eat_chars = 0;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_host:
            reg.set( "^host[[:digit:]]+" );
            if ( reg.match( *t ) ) {
              state = st_accept;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
        }
      }
      if ( state == st_accept ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else {
        return false;
      }
    }

    bool SCSIRPort::accept( SysDevicePath &path ) {
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_rport = 0;    // rport-<n>:<n>-<n>
      const unsigned int st_accept = 5;
      const unsigned int st_fail = 6;
      unsigned int state = st_rport;
      util::RegExp reg;
      size_t eat_chars = 0;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_rport:
            reg.set( "^rport-[[:digit:]]+:[[:digit:]]+-[[:digit:]]+" );
            if ( reg.match( *t ) ) {
              state = st_accept;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
        }
      }
      if ( state == st_accept ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else {
        return false;
      }
    }

    bool SCSIVPort::accept( SysDevicePath &path ) {
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_rport = 0;    // vport-<n>:<n>-<n>
      const unsigned int st_accept = 5;
      const unsigned int st_fail = 6;
      unsigned int state = st_rport;
      util::RegExp reg;
      size_t eat_chars = 0;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_rport:
            reg.set( "^vport-[[:digit:]]+:[[:digit:]]+-[[:digit:]]+" );
            if ( reg.match( *t ) ) {
              state = st_accept;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
        }
      }
      if ( state == st_accept ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else {
        return false;
      }
    }


    std::string BlockPartition::getDisplayName() const {
      return getMajorMinor().getName();
    }

    void BlockPartition::getPropertyValueList( PropertyValueList &pv ) const {
      std::stringstream ss;
      pv.clear();
      SysDevice::getPropertyValueList( pv );
      ss << getMajorMinor();
      pv.push_back( PropertyValue( "major:minor", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr( getMajorMinor().getSize(), 2 );
      pv.push_back( PropertyValue( "size", ss.str() ) );
      ss.str("");
      ss << leanux::util::ByteStr( getMajorMinor().getReadAhead(), 2 );
      pv.push_back( PropertyValue( "read-ahead", ss.str() ) );
      pv.push_back( PropertyValue( "udev path", getMajorMinor().getUDevPath() ) );
      std::list<std::string> aliases;
      getMajorMinor().getAliases( aliases );
      for ( std::list<std::string>::const_iterator i = aliases.begin(); i != aliases.end(); i++ ) {
        pv.push_back( PropertyValue( "alias", *i ) );
      }
    }

    std::string iSCSIDevice::getDescription() const {
      std::stringstream ss;
      ss << SCSIDevice::getDescription();
      ss << " session=" << session_;
      block::MajorMinor m = getMajorMinor();
      if ( m.isValid() ) {
        ss << " target=" << getTargetAddress() << ":" << getTargetPort();
        ss << " iqn=" << getTargetIQN();
      }
      return ss.str();
    }

    std::string iSCSIDevice::getTargetAddress() const {
      size_t p = path_.find( session_ );
      if ( p != std::string::npos ) {
        std::string sesroot = path_.substr(0, p + session_.length() + 1 );
        std::string conndir = util::findDir( sesroot, "connection" );
        std::string tgtpath = sesroot + conndir + "/iscsi_connection/" + conndir + "/address";
        if ( util::fileReadAccess( tgtpath ) ) return util::fileReadString( tgtpath );
      }
      return "";
    }

    std::string iSCSIDevice::getTargetPort() const {
      size_t p = path_.find( session_ );
      if ( p != std::string::npos ) {
        std::string sesroot = path_.substr(0, p + session_.length() + 1 );
        std::string conndir = util::findDir( sesroot, "connection" );
        std::string tgtpath = sesroot + conndir + "/iscsi_connection/" + conndir + "/port";
        if ( util::fileReadAccess( tgtpath ) ) return util::fileReadString( tgtpath );
      }
      return "";
    }

    std::string iSCSIDevice::getTargetIQN() const {
      size_t p = path_.find( session_ );
      if ( p != std::string::npos ) {
        std::string sesroot = path_.substr(0, p + session_.length() + 1 );
        std::string iqnpath = sesroot + "iscsi_session/" + session_ + "/targetname";
        if ( util::fileReadAccess( iqnpath ) ) return util::fileReadString( iqnpath );
      }
      return "";
    }

    bool iSCSIDevice::accept( SysDevicePath &path ) {
      if ( path.find("session") == std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_blkdev_name = 0;      // <sda>
      const unsigned int st_const_block = 1;      // block
      const unsigned int st_scsi_addr = 2;        // <0:0:0:0>
      const unsigned int st_scsi_target = 3;      // target<0:0:0>
      const unsigned int st_scsi_session = 4;       // session<0>
      const unsigned int st_scsi_host = 5;        // host<0>
      const unsigned int st_scsi_platform = 6;    // platform
      const unsigned int st_accept = 7;
      const unsigned int st_fail = 8;
      unsigned int state = st_blkdev_name;
      util::RegExp reg;
      size_t eat_chars = 0;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_blkdev_name:
            reg.set( "s(d|r).*" );
            if ( reg.match( *t ) ) {
              state = st_const_block;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
          case st_const_block:
            if ( *t == "block" ) {
              state = st_scsi_addr;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
          case st_scsi_addr:
            reg.set( "[[:digit:]]+:[[:digit:]]+:[[:digit:]]+:[[:digit:]]+" );
            if ( !reg.match( *t ) ) state = st_fail;
            else {
              state = st_scsi_target;
              address_ = *t;
              eat_chars += (*t).length() + 1;
            }
            break;
          case st_scsi_target:
            reg.set( "(target)[[:digit:]]+:[[:digit:]]+:[[:digit:]]+" );
            if ( !reg.match( *t ) ) state = st_fail;
            else {
              state = st_scsi_session;
              eat_chars += (*t).length() + 1;
            }
            break;
          case st_scsi_session:
            reg.set( "(session)[[:digit:]]+" );
            if ( !reg.match( *t ) ) state = st_fail;
            else {
              state = st_scsi_host;
              session_ = (*t);
              eat_chars += (*t).length() + 1;
            }
            break;
          case st_scsi_host:
            reg.set( "(host)[[:digit:]]+" );
            if ( !reg.match( *t ) ) state = st_fail;
            else {
              state = st_scsi_platform;
              eat_chars += (*t).length() + 1;
            }
            break;
          case st_scsi_platform:
            if ( *t != "platform" ) state = st_fail;
            else {
              state = st_accept;
              eat_chars += (*t).length() + 1;
            }
            break;
        }
      }
      if ( state == st_accept ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else {
        address_ = "";
        session_ = "";
        return false;
      }
    }

    bool ATAPort::accept( SysDevicePath &path ) {
      if ( path.find("ata") == std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_port = 0;      // ata<port>
      const unsigned int st_accept = 1;
      const unsigned int st_fail = 2;
      unsigned int state = st_port;
      int r;
      size_t eat_chars = 0;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_port:
            r = sscanf( (*t).c_str(), "ata%x", &port_ );
            if ( r != 1 ) state = st_fail;
            else {
              state = st_accept;
              eat_chars += (*t).length() + 1;
            }
            break;
        }
      }
      if ( state == st_accept ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else return false;
    }

    void ATAPort::getPropertyValueList( PropertyValueList &pv ) const {
      pv.clear();
      SysDevice::getPropertyValueList( pv );
      std::string port = leanux::block::getATAPortName( path_ );
      std::string link = leanux::block::getATAPortLink( port );
      pv.push_back( PropertyValue( "port", leanux::block::getATAPortName( path_ ) ) );
      pv.push_back( PropertyValue( "link", leanux::block::getATAPortLink( leanux::block::getATAPortName( path_ ) ) ) );
      pv.push_back( PropertyValue( "speed", leanux::block::getATALinkSpeed( port, link ) ) );
    }

    void PCIDevice::getPropertyValueList( PropertyValueList &pv ) const {
      pv.clear();
      SysDevice::getPropertyValueList( pv );

      leanux::pci::PCIHardwareId  pcihwid = leanux::pci::getPCIHardwareId( path_ );
      leanux::pci::PCIHardwareInfo  pcihwinfo;
      leanux::pci::getPCIHardwareInfo( pcihwid, pcihwinfo );

      if ( pcihwinfo.device.length() > 0 ) {
        pv.push_back( PropertyValue( "model", pcihwinfo.device ) );
        pv.push_back( PropertyValue( "vendor", pcihwinfo.vendor ) );
      }
      std::stringstream ss;
      ss << leanux::pci::getPCIDeviceIRQ( path_ );
      pv.push_back( PropertyValue( "irq", ss.str() ) );
    }

    bool PCIDevice::accept( SysDevicePath &path ) {
      if ( path.find("pci") == std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_hw_addr = 0;      // 0000:00:1c.7
      const unsigned int st_pcibus = 1;       // pci0000:00
      const unsigned int st_accept = 2;
      const unsigned int st_fail = 3;
      unsigned int state = st_hw_addr;
      size_t eat_chars = 0;
      util::RegExp reg;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_hw_addr:
            reg.set( "[[:xdigit:]]{4}:[[:xdigit:]]{2}:[[:xdigit:]]{2}.[[:xdigit:]]+" );
            if ( !reg.match( *t ) ) {
              state = st_pcibus;
              t--;
            } else {
              state = st_pcibus;
              if ( !eat_chars ) eat_chars += (*t).length() + 1;
            }
            break;
          case st_pcibus:
            reg.set( "(pci)[[:xdigit:]]{4}:[[:xdigit:]]{2}" );
             if ( reg.match( *t ) ) {
              state = st_accept;
            }
        }
      }
      if ( state == st_accept && eat_chars > 0 ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else return false;
    }

    std::string PCIDevice::getDisplayName() const {
      leanux::pci::PCIHardwareId  pcihwid = leanux::pci::getPCIHardwareId( path_ );
      leanux::pci::PCIHardwareInfo  pcihwinfo;
      leanux::pci::getPCIHardwareInfo( pcihwid, pcihwinfo );
      if ( pcihwinfo.device.length() > 0 )
        return pcihwinfo.device;
      else
        return SysDevice::getDisplayName();
    }

    bool PCIBus::accept( SysDevicePath &path ) {
      bool accept = false;
      if ( util::directoryExists( path + "/pci_bus" ) ) {
        path_ = path;
        size_t p = path.rfind( "/" );
        leaf_ = path_.substr( p + 1 );
        path = path.substr( 0,  p );
        accept = true;
      }
      return accept;
    }

    std::string PCIBus::getDisplayName() const {
      leanux::pci::PCIHardwareId  pcihwid = leanux::pci::getPCIHardwareId( path_ );
      leanux::pci::PCIHardwareInfo  pcihwinfo;
      leanux::pci::getPCIHardwareInfo( pcihwid, pcihwinfo );
      if ( pcihwinfo.device.length() > 0 )
        return pcihwinfo.device;
      else
        return SysDevice::getDisplayName();
    }

    std::string PCIBus::getDescription() const {
      std::stringstream ss;
      pci::PCIHardwareId hwid = pci::getPCIHardwareId( path_ );
      pci::PCIHardwareInfo hwinfo;
      if ( pci::getPCIHardwareInfo( hwid, hwinfo ) ) {
        ss << "model=" << hwinfo.device;
      }
      return ss.str();
    }

    void PCIBus::getPropertyValueList( PropertyValueList &pv ) const {
      pv.clear();
      SysDevice::getPropertyValueList( pv );

      leanux::pci::PCIHardwareId  pcihwid = leanux::pci::getPCIHardwareId( path_ );
      leanux::pci::PCIHardwareInfo  pcihwinfo;
      leanux::pci::getPCIHardwareInfo( pcihwid, pcihwinfo );

      if ( pcihwinfo.device.length() > 0 ) {
        pv.push_back( PropertyValue( "model", pcihwinfo.device ) );
        pv.push_back( PropertyValue( "vendor", pcihwinfo.vendor ) );
      }
      std::stringstream ss;
      ss << leanux::pci::getPCIDeviceIRQ( path_ );
      pv.push_back( PropertyValue( "irq", ss.str() ) );
    }

    /**
     * http://www.makelinux.net/ldd3/chp-13-sect-2
     */
    bool USBInterface::accept( SysDevicePath &path ) {
      if ( path.find("usb") == std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_usb_config = 0;     // 6-1.5:1.0
      const unsigned int st_usb_device = 1;        // 6-1.5, 6-1, (6-1.5.3 ?)
      const unsigned int st_usb_bus = 2;           // usb6
      const unsigned int st_fail = 3;
      const unsigned int st_accept = 4;
      unsigned int state = st_usb_config;
      size_t eat_chars = 0;
      util::RegExp reg;
      bool match = false;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_usb_config:
            reg.set( "([[:digit:]]+-[[:digit:]]+)(\\.[[:digit:]]+)*(:([[:digit:]]+.[[:digit:]]+)+)+" );
            if ( reg.match( *t ) ) {
              if ( !match ) eat_chars += (*t).length() + 1;
              match = true;
            }
            state = st_usb_device;
            break;
          case st_usb_device:
            reg.set( "([[:digit:]]+-[[:digit:]]+)+(\\.[[:digit:]]+)*" );
            if ( !reg.match( *t ) ) {
              state = st_usb_bus;
              t--;
            }
            break;
          case st_usb_bus:
            reg.set( "(usb)([[:digit:]]+)" );
            if ( reg.match( *t ) ) state = st_accept; else state = st_fail;
            break;
        }
      }
      if ( state == st_accept && match ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else return false;
    }

    /**
     * http://www.makelinux.net/ldd3/chp-13-sect-2
     */
    bool USBDevice::accept( SysDevicePath &path ) {
      if ( path.find("usb") == std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_usb_device = 0;        // 6-1.5, 6-1, (6-1.5.3 ?)
      const unsigned int st_usb_bus = 1;           // usb6
      const unsigned int st_fail = 2;
      const unsigned int st_accept = 3;
      unsigned int state = st_usb_device;
      size_t eat_chars = 0;
      util::RegExp reg;
      bool match = false;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_usb_device:
            reg.set( "([[:digit:]]+-[[:digit:]]+)+(\\.[[:digit:]]+)*" );
            if ( reg.match( *t ) ) {
              if ( !match ) eat_chars += (*t).length() + 1;
              match = true;
              break;
            } else {
              state = st_usb_bus;
            }
          case st_usb_bus:
            reg.set( "(usb)([[:digit:]]+)" );
            if ( reg.match( *t ) ) state = st_accept; else state = st_fail;
            break;
        }
      }
      if ( state == st_accept && match ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else return false;
    }

    bool USBBus::accept( SysDevicePath &path ) {
      if ( path.find("usb") == std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      util::RegExp reg;
      reg.set( "(usb)([[:digit:]]+)" );
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end(); t++ ) {
        if ( reg.match( *t ) ) {
          path_ = path;
          leaf_ = path.substr( path.length() - (*t).length() );
          path = path.substr( 0, path.length() - (*t).length() - 1 );
          return true;
        } else return false;
      }
      return false;
    }

    bool NetDevice::accept( SysDevicePath &path ) {
      if ( path.find("net") == std::string::npos ||
           path.find("virtual/net") != std::string::npos ||
           path.find("virtio") != std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_net_device = 0;       // very wide range of possibilities
      const unsigned int st_net = 1;              // net
      const unsigned int st_fail = 2;
      const unsigned int st_accept = 3;
      unsigned int state = st_net_device;
      size_t eat_chars = 0;
      util::RegExp reg;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_net_device:
            reg.set( "(.+)([[:digit:]]+)" );
            if ( reg.match( *t ) ) {
              eat_chars += (*t).length() + 1;
              state = st_net;
            } else {
              state = st_fail;
            }
            break;
          case st_net:
            if ( (*t) == "net" ) {
              state = st_accept;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
        }
      }
      if ( state == st_accept && eat_chars ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else return false;
    }

    bool BlockPartition::accept( SysDevicePath &path ) {
      if ( path.find("block") == std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_part_name = 0;        // rely on block namespace
      const unsigned int st_blkdev_name = 1;      // <sda>
      const unsigned int st_const_block = 2;      // block
      const unsigned int st_accept = 6;
      const unsigned int st_fail = 7;
      unsigned int state = st_part_name;
      util::RegExp reg;
      size_t eat_chars = 0;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_part_name:
            if ( block::MajorMinor::getMajorMinorByName( *t ) != block::MajorMinor::invalid &&
                 block::MajorMinor::getMajorMinorByName( *t ).isPartition() ) {
              state = st_blkdev_name;
              eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
          case st_blkdev_name:
            if ( block::MajorMinor::getMajorMinorByName( *t ) != block::MajorMinor::invalid &&
                 !block::MajorMinor::getMajorMinorByName( *t ).isPartition() ) {
              state = st_const_block;
            } else state = st_const_block;
            break;
          case st_const_block:
            if ( *t == "block" ) {
              state = st_accept;
            } else state = st_fail;
            break;
        }
      }
      if ( state == st_accept ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else {
        return false;
      }
    }

    void NetDevice::getPropertyValueList( PropertyValueList &pv ) const {
      pv.clear();
      SysDevice::getPropertyValueList( pv );
      std::string netdev =  leanux::util::leafDir( path_ );
      pv.push_back( PropertyValue( "dev", netdev ) );
      std::stringstream ss;
      ss << (leanux::net::getDeviceCarrier(netdev)==1?"yes":"no");
      pv.push_back( PropertyValue( "carrier", ss.str() ) );
      if ( leanux::net::getDeviceCarrier(netdev) ) pv.push_back( PropertyValue( "duplex", leanux::net::getDeviceDuplex(netdev) ) );
      pv.push_back( PropertyValue( "MAC", leanux::net::getDeviceMACAddress(netdev) ) );
      leanux::net::OUI oui;
      leanux::net::getMACOUI(leanux::net::getDeviceMACAddress(netdev), oui);
      pv.push_back( PropertyValue( "MAC vendor", oui.vendor ) );
      pv.push_back( PropertyValue( "MAC country", oui.countrycode ) );
      pv.push_back( PropertyValue( "state", leanux::net::getDeviceOperState(netdev) ) );
      ss.str("");
      ss << leanux::net::getDeviceSpeed(netdev) << " Mbps";
      if ( leanux::net::getDeviceCarrier(netdev) ) pv.push_back( PropertyValue( "speed", ss.str() ) );
      std::list<std::string> ipv4;
      leanux::net::getDeviceIP4Addresses( netdev, ipv4 );
      for ( std::list<std::string>::const_iterator i = ipv4.begin(); i != ipv4.end(); i++ ) {
        pv.push_back( PropertyValue( "ipv4", *i ) );
      }
      std::list<std::string> ipv6;
      leanux::net::getDeviceIP6Addresses( netdev, ipv6 );
      for ( std::list<std::string>::const_iterator i = ipv6.begin(); i != ipv6.end(); i++ ) {
        pv.push_back( PropertyValue( "ipv6", *i ) );
      }
    }

    bool VirtualRoot::accept( SysDevicePath &path ) {
      if ( path == "/sys/devices/virtual" ) {
        path_ = path;
        leaf_ = "virtual";
        path = "/sys/devices";
        return true;
      } else return false;
    }

    bool VirtualBlockRoot::accept( SysDevicePath &path ) {
      if ( path == "/sys/devices/virtual/block" ) {
        path_ = path;
        leaf_ = "block";
        path = "/sys/devices/virtual";
        return true;
      } else return false;
    }

    bool VirtualNetRoot::accept( SysDevicePath &path ) {
      if ( path == "/sys/devices/virtual/net" ) {
        path_ = path;
        leaf_ = "net";
        path = "/sys/devices/virtual";
        return true;
      } else return false;
    }

    bool VirtualNetDevice::accept( SysDevicePath &path ) {
      if ( path.find("virtual/net") == std::string::npos ) return false;
      std::list<std::string> tokens;
      tokenize( path, tokens );
      const unsigned int st_net_device = 0;       // very wide range of possibilities
      const unsigned int st_net = 1;              // net
      const unsigned int st_virtual = 2;          // net
      const unsigned int st_fail = 3;
      const unsigned int st_accept = 4;
      unsigned int state = st_net_device;
      size_t eat_chars = 0;
      util::RegExp reg;
      for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end() && state != st_accept && state != st_fail; t++ ) {
        switch ( state ) {
          case st_net_device:
            reg.set( "(lo)|(.+)([[:digit:]]+)" );
            if ( reg.match( *t ) ) {
              eat_chars += (*t).length() + 1;
              state = st_net;
            } else {
              state = st_fail;
            }
            break;
          case st_net:
            if ( (*t) == "net" ) {
              state = st_virtual;
              //eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
          case st_virtual:
            if ( (*t) == "virtual" ) {
              state = st_accept;
              //eat_chars += (*t).length() + 1;
            } else state = st_fail;
            break;
        }
      }
      if ( state == st_accept && eat_chars ) {
        path_ = path;
        leaf_ = path.substr( path.length() - eat_chars + 1 );
        path = path.substr( 0, path.length() - eat_chars );
        return true;
      } else return false;
    }

    void enumDevices( std::list<SysDevicePath> &paths ) {
      std::list<SysDevicePath> b;
      std::list<SysDevicePath> n;
      std::list<SysDevicePath> p;
      std::list<SysDevicePath> pb;
      std::list<SysDevicePath> u;
      std::list<SysDevicePath> a;
      std::list<SysDevicePath> sh;
      enumBlockDevices( b );
      enumNetDevices( n );
      enumPCIBusses( pb );
      enumPCIDevices( p );
      enumUSBDevices( u );
      enumATADevices( a );
      enumSCSIHostDevices( sh );
      std::set<std::string> upaths;
      upaths.insert( b.begin(), b.end() );
      upaths.insert( n.begin(), n.end() );
      upaths.insert( p.begin(), p.end() );
      upaths.insert( pb.begin(), pb.end() );
      upaths.insert( u.begin(), u.end() );
      upaths.insert( a.begin(), a.end() );
      upaths.insert( sh.begin(), sh.end() );
      paths.clear();
      paths.insert( paths.end(), upaths.begin(), upaths.end() );
    }

    void enumBlockDevices( std::list<SysDevicePath> &paths ) {
      paths.clear();
      DIR *d;
      struct dirent *dir;
      d = opendir( "/sys/class/block" );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( strlen(dir->d_name) > 0 && dir->d_name[0] != '.' ) {
            paths.push_back( util::realPath( "/sys/class/block/" + (std::string)dir->d_name ) );
          }
        }
      }
      closedir( d );
    }

    void enumNetDevices( std::list<SysDevicePath> &paths ) {
      paths.clear();
      DIR *d;
      struct dirent *dir;
      d = opendir( "/sys/class/net" );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( strlen(dir->d_name) > 0 && dir->d_name[0] != '.' ) {
            paths.push_back( util::realPath( "/sys/class/net/" + (std::string)dir->d_name ) );
          }
        }
      }
      closedir( d );
    }

    void enumPCIBusses( std::list<SysDevicePath> &paths ) {
      paths.clear();
      DIR *d;
      struct dirent *dir;
      d = opendir( "/sys/class/pci_bus" );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( strlen(dir->d_name) > 0 && dir->d_name[0] != '.' ) {
            paths.push_back( util::realPath( "/sys/class/pci_bus/" + (std::string)dir->d_name + "/device" ) );
          }
        }
      }
      closedir( d );
    }

    void enumPCIDevices( std::list<SysDevicePath> &paths ) {
      paths.clear();
      DIR *d;
      struct dirent *dir;
      d = opendir( "/sys/bus/pci/devices" );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( strlen(dir->d_name) > 0 && dir->d_name[0] != '.' ) {
            paths.push_back( util::realPath( "/sys/bus/pci/devices/" + (std::string)dir->d_name ) );
          }
        }
      }
      closedir( d );
    }

    void enumUSBDevices( std::list<SysDevicePath> &paths ) {
      paths.clear();
      DIR *d;
      struct dirent *dir;
      d = opendir( "/sys/bus/usb/devices" );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( strlen(dir->d_name) > 0 && dir->d_name[0] != '.' ) {
            paths.push_back( util::realPath( "/sys/bus/usb/devices/" + (std::string)dir->d_name ) );
          }
        }
      }
      closedir( d );
    }

    void enumATADevices( std::list<SysDevicePath> &paths ) {
      paths.clear();
      DIR *d;
      struct dirent *dir;
      d = opendir( "/sys/class/ata_port" );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( strlen(dir->d_name) > 0 && dir->d_name[0] != '.' ) {
            paths.push_back( util::realPath( "/sys/class/ata_port/" + (std::string)dir->d_name + "/device" ) );
          }
        }
      }
      closedir( d );
    }

    void enumSCSIHostDevices( std::list<SysDevicePath> &paths ) {
      paths.clear();
      DIR *d;
      struct dirent *dir;
      d = opendir( "/sys/class/scsi_host" );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( strlen(dir->d_name) > 0 && dir->d_name[0] != '.' ) {
            paths.push_back( util::realPath( util::realPath( "/sys/class/scsi_host/" + (std::string)dir->d_name ) + "/../.." ) );
          }
        }
      }
      closedir( d );
    }

    SysDevice* leafDetect( const SysDevicePath& path, SysDevicePath &parent ) {
      if ( SysDevice::validatePath( path ) ) {
        UnknownSysDevice unknown;
        std::string wpath = path;
        VirtioBlockDevice virtio;
        VirtioNetDevice virtionet;
        SCSIDevice scsi;
        SASPort sas_port;
        SASEndDevice sas_end_device;
        SCSIHost scsi_host;
        SCSIRPort scsi_rport;
        SCSIVPort scsi_vport;
        iSCSIDevice iscsi;
        BCacheDevice bcache;
        ATAPort ata;
        PCIDevice pci;
        PCIBus pcibus;
        USBInterface usbiface;
        USBDevice usbdev;
        USBBus usbbus;
        MapperDevice dm;
        MDDevice md;
        MMCDevice mmc;
        NVMeDevice nvme;
        NVMePartition nvmepart;
        NetDevice net;
        VirtualRoot vroot;
        VirtualBlockRoot vblockroot;
        VirtualNetRoot vnetroot;
        VirtualNetDevice vnet;
        LoopbackDevice loopback;
        RamDiskDevice ramdisk;
        BlockPartition part;
        if ( wpath.find( "/block/" ) != std::string::npos || wpath.find("virtual/block") != std::string::npos ) {
          if ( iscsi.accept( wpath ) ) { parent = wpath; return new iSCSIDevice(iscsi); }
          else if ( scsi.accept( wpath ) ) { parent = wpath; return new SCSIDevice(scsi); }
          else if ( dm.accept( wpath ) ) { parent = wpath; return new MapperDevice(dm); }
          else if ( md.accept( wpath ) ) { parent = wpath; return new MDDevice(md); }
          else if ( mmc.accept( wpath ) ) { parent = wpath; return new MMCDevice(mmc); }
          else if ( virtio.accept( wpath ) ) { parent = wpath; return new VirtioBlockDevice(virtio); }
          else if ( part.accept( wpath ) ) { parent = wpath; return new BlockPartition(part); }
          else if ( bcache.accept( wpath ) ) { parent = wpath; return new BCacheDevice(bcache); }
          else if ( loopback.accept( wpath ) ) { parent = wpath; return new LoopbackDevice(loopback); }
          else if ( ramdisk.accept( wpath ) ) { parent = wpath; return new RamDiskDevice(ramdisk); }
          else if ( vblockroot.accept( wpath ) ) { parent = wpath; return new VirtualBlockRoot(vblockroot); }
        }
        if ( nvmepart.accept( wpath ) ) { parent = wpath; return new NVMePartition(nvmepart); }
        if ( nvme.accept( wpath ) ) { parent = wpath; return new NVMeDevice(nvme); }
        if ( wpath.find( "/pci" ) != std::string::npos ) {
          if ( pcibus.accept( wpath ) ) { parent = wpath; return new PCIBus(pcibus); }
          else if ( pci.accept( wpath ) ) { parent = wpath; return new PCIDevice(pci); }
        }
        if ( wpath.find( "/usb" ) != std::string::npos ) {
          if ( usbiface.accept( wpath ) ) { parent = wpath; return new USBInterface(usbiface); }
          else if ( usbdev.accept( wpath ) ) { parent = wpath; return new USBDevice(usbdev); }
          else if ( usbbus.accept( wpath ) ) { parent = wpath; return new USBBus(usbbus); }
        }
        if ( wpath.find( "/net" ) != std::string::npos ) {
          if ( net.accept( wpath ) ) { parent = wpath; return new NetDevice(net); }
          else if ( vnet.accept( wpath ) ) { parent = wpath; return new VirtualNetDevice(vnet); }
          else if ( virtionet.accept( wpath ) ) { parent = wpath; return new VirtioNetDevice(virtionet); }
          else if ( vnetroot.accept( wpath ) ) { parent = wpath; return new VirtualNetRoot(vnetroot); }
        }
        if ( ata.accept( wpath ) ) { parent = wpath; return new ATAPort(ata); }
        if ( scsi_host.accept( wpath ) ) { parent = wpath; return new SCSIHost(scsi_host); }
        if ( scsi_rport.accept( wpath ) ) { parent = wpath; return new SCSIRPort(scsi_rport); }
        if ( scsi_vport.accept( wpath ) ) { parent = wpath; return new SCSIVPort(scsi_vport); }
        if ( sas_port.accept( wpath ) ) { parent = wpath; return new SASPort(sas_port); }
        if ( sas_end_device.accept( wpath ) ) { parent = wpath; return new SASEndDevice(sas_end_device); }
        if ( vroot.accept( wpath ) ) { parent = wpath; return new VirtualRoot(vroot); }
        //give up, revert to UnknownSysDevice
        if ( unknown.accept(wpath) ) { parent = ""; return new UnknownSysDevice(unknown); };
      }
      return 0;
    }

    bool treeDetect( const SysDevicePath& path, std::list<SysDevice*> &devices ) {
      devices.clear();
      if ( SysDevice::validatePath( path ) ) {
        std::string wpath = path;
        while ( wpath.length() > sysdevice_root.length() ) {
          std::string parent = "";
          SysDevice* result = leafDetect( wpath, parent );
          if ( result && result->getType() != SysDevice::sdtUnknown ) {
            devices.push_front( result );
            wpath = parent;
          } else {
            if ( result ) delete result;
            return false;
          }
        }
        return true;
      } else throw Oops( __FILE__, __LINE__, "invalid sys device path '" + path + "'" );
      return false;
    }

    std::string SysDevice::getTypeStr( SysDeviceType dt ) {
      switch ( dt ) {
        case sdtATAPort : return "ATA port";
        case sdtBlock : return "block device";
        case sdtBlockPartition : return "partition";
        case sdtMapperDevice : return "devicemapper";
        case sdtPCIBus : return "PCI bus";
        case sdtPCIDevice : return "PCI device";
        case sdtSASPort : return "SAS port";
        case sdtSASEndDevice : return "SAS end device";
        case sdtSCSIDevice : return "SCSI device";
        case sdtSCSIHost : return "SCSI host";
        case sdtSCSIRPort : return "SCSI rport";
        case sdtSCSIVPort : return "SCSI vport";
        case sdtiSCSIDevice : return "iSCSI device";
        case sdtBCacheDevice : return "bcache device";
        case sdtUSBBus : return "USB bus";
        case sdtUSBInterface : return "USB interface";
        case sdtUSBDevice : return "USB device";
        case sdtVirtioBlockDevice : return "virtio block device";
        case sdtVirtioNetDevice : return "virtio net device";
        case sdtMetaDisk : return "MetaDisk device";
        case sdtNVMeDevice : return "NVMe device";
        case sdtNVMePartition : return "partition";
        case sdtMMCDevice : return "MMC device";
        case sdtNetDevice : return "network device";
        case sdtVirtualRoot : return "virtual root";
        case sdtVirtualBlockRoot : return "virtual block root";
        case sdtVirtualNetRoot : return "virtual net root";
        case sdtVirtualNetDevice : return "virtual network device";
        case sdtLoopbackDevice : return "loopback device";
        case sdtRAMDiskDevice : return "ramdisk device";
        case sdtUnknown : return "unknown device";
        default: return "undefined SysDeviceType";
      }
    }

    std::string SysDevice::getDriver() const {
      std::string rp = util::realPath( path_ + "/driver" );
      if ( util::directoryExists( rp ) ) {
        std::istringstream iss(rp);
        std::string token = "";
        while ( std::getline( iss, token, '/' ) ) {
        }
        return token;
      } else return "";
    }

    std::string BlockDevice::getDriver() const {
      std::string rp = util::realPath( path_ + "/../../driver" );
      if ( util::directoryExists( rp ) ) {
        std::istringstream iss(rp);
        std::string token = "";
        while ( std::getline( iss, token, '/' ) ) {
        }
        return token;
      } else return "";
    }

  }; // namespace device

}; // namespace leanux
