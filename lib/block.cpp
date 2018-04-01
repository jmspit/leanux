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
 * @file block.cpp
 * leanux::block c++ source file.
 */
#include "block.hpp"
#include "oops.hpp"
#include "util.hpp"
#include "device.hpp"

#include <string.h>
//#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>


#include <iostream>
#include <fstream>
#include <limits>
#include <sstream>

namespace leanux {

  namespace block {

    std::map< std::string, MajorMinor > MajorMinor::name2mm_;

    std::map< MajorMinor, std::string> MajorMinor::mm2name_;

    std::map< std::string, MajorMinor > MajorMinor::file2mm_;

    std::map< MajorMinor, std::string> MajorMinor::mm2file_;

    const MajorMinor MajorMinor::invalid = MajorMinor( 0, 0 );

    /**
     * base directory for runtime udev information.
     * @see init
     */
    std::string udev_path = "";

    /**
     * Three naming conventions for udev block device entries.
     * @see init
     */
    enum udevMode {
      umBlockMM,    /**< block8:0 */
      umBlockName,  /**< block:sda */
      umBMM         /**< b8:0 */
    };


    /**
     * The detected udevMode.
     * @see init.
     */
    udevMode udev_mode = umBlockMM;

    /**
     * Detect the udevMode from the given MajorMinor and udev_path.
     */
    udevMode modeFromMajorMinor( const MajorMinor& m, const std::string &udev_path ) {
      std::stringstream ss;
      ss << "b" << m;
      if ( util::fileReadAccess( udev_path + ss.str() ) ) return umBMM; else {
        ss.str("");
        ss << "block" << m;
        if ( util::fileReadAccess( udev_path + ss.str() ) ) return umBlockMM;
        else {
          ss.str("");
          ss << "block:" << MajorMinor::getNameByMajorMinor( m );
          if ( util::fileReadAccess( udev_path + ss.str() ) ) return umBlockName; else {
            ss.str("");
            ss << "unable to find udev block device info in path ";
            ss << udev_path << " for maj:min " << m;
            throw Oops( __FILE__, __LINE__, ss.str() );
          }
        }
      }
      return umBlockMM;
    }

    void init() {
      //detect udev path
      if ( util::directoryExists( "/dev/.udev/db" ) ) {
        udev_path = "/dev/.udev/db/";
      } else if ( util::directoryExists( "/run/udev/data" ) ) {
        udev_path = "/run/udev/data/";
      } else if ( util::directoryExists( "/var/run/udev/data" ) ) {
        udev_path = "/var/run/udev/data/";
      }
      if ( udev_path == "" ) throw Oops( __FILE__, __LINE__, "cannot find udev run data" );
      //detect udev block device format
      std::stringstream ss;
      //first attempt to stat the root (/) filesystem to get a disk
      struct stat rootfs;
      if ( !stat("/",&rootfs) ) {
        MajorMinor rootdevice(rootfs.st_dev);
        // btrfs (sigh...) has no sane 'major', so btrfs breaks the stat call returning major 0.
        if ( rootdevice.isValid() && rootdevice.getMajor() != 0 ) {
          udev_mode = modeFromMajorMinor( rootdevice, udev_path );
          return;
        }
      }
      std::list<MajorMinor> wd;
      enumWholeDisks( wd );
      if ( wd.size() > 0 ) {
        udev_mode = modeFromMajorMinor( wd.front(), udev_path );
      } else throw Oops( __FILE__, __LINE__, "did not detect any whole disks" );
    }

    MajorMinor& MajorMinor::operator=(const std::string &s ) {
      std::string smajor = "";
      std::string sminor = "";
      int state = 0;
      size_t p = 0;
      while ( p < s.length() ) {
        switch( state ) {
          case 0:
            if ( s[p] == ':' ) state = 1; else
            if ( !isdigit( s[p] ) ) throw Oops( __FILE__, __LINE__, "invalid MajorMinor std::string" );
            else smajor += s[p];
            p++;
            break;
          case 1:
            if ( !isdigit( s[p] ) ) throw Oops( __FILE__, __LINE__, "invalid MajorMinor std::string" );
            else sminor += s[p];
            p++;
            break;
        }
      }
      dev_ = MKDEV( strtoul( smajor.c_str(), 0, 10 ), strtoul( sminor.c_str(), 0, 10 ) );
      return *this;
    }

    void MajorMinor::buildCache() {
      name2mm_.clear();
      mm2name_.clear();
      std::ifstream i( "/proc/diskstats" );
      if ( !i.good() ) throw Oops( __FILE__, __LINE__, "failed to open '/proc/diskstats'" );
      unsigned int major, minor;
      std::string device;
      while ( i.good() ) {
        i >> major >> minor >> device;
        name2mm_[ device ] = MajorMinor( major, minor );
        mm2name_[ MajorMinor( major, minor ) ] = device;
        //check devicefile
        std::list<std::string> aliasses;
        MajorMinor( major, minor ).getAliases( aliasses );
        bool nodevice = true;
        for ( std::list<std::string>::const_iterator a = aliasses.begin(); a != aliasses.end(); a++ ) {
          std::string rp = util::realPath( *a );
          if ( rp != "" ) {
            file2mm_[ rp ] = MajorMinor( major, minor );
            mm2file_[ MajorMinor( major, minor ) ] = rp;
            nodevice = false;
            break;
          }
        }
        if ( nodevice ) {
          std::string rp = "/dev/" + device;
          file2mm_[ rp ] = MajorMinor( major, minor );
          mm2file_[ MajorMinor( major, minor ) ] = rp;
        }

        i.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
    }

    MajorMinor MajorMinor::getMajorMinorByName( const std::string& devicename ) {
      std::map< std::string, MajorMinor >::const_iterator i = name2mm_.find( devicename );
      if ( i != name2mm_.end() ) {
        return i->second;
      } else {
        buildCache();
        i = name2mm_.find( devicename );
        if ( i != name2mm_.end() ) {
          return i->second;
        }
      }
      return MajorMinor::invalid;
    }

    MajorMinor MajorMinor::getMajorMinorByDeviceFile( const std::string& devicefile ) {
      std::map< std::string, MajorMinor >::const_iterator i = file2mm_.find( devicefile );
      if ( i != file2mm_.end() ) {
        return i->second;
      } else {
        buildCache();
        i = file2mm_.find( devicefile );
        if ( i != file2mm_.end() ) {
          return i->second;
        }
      }
      return MajorMinor::invalid;
    }

    std::string MajorMinor::getNameByMajorMinor( const MajorMinor &m ) {
      std::map< MajorMinor, std::string >::const_iterator i = mm2name_.find( m );
      if ( i != mm2name_.end() ) {
        return i->second;
      } else {
        buildCache();
        i = mm2name_.find( m );
        if ( i != mm2name_.end() ) {
          return i->second;
        }
      }
      return "";
    }

    std::string MajorMinor::getDeviceFileByMajorMinor( const MajorMinor &m ) {
      std::map< MajorMinor, std::string >::const_iterator i = mm2file_.find( m );
      if ( i != mm2file_.end() ) {
        return i->second;
      } else {
        buildCache();
        i = mm2file_.find( m );
        if ( i != mm2file_.end() ) {
          return i->second;
        }
      }
      return "";
    }

    /**
     * Get a loose description of what the device mm is.
     */
    std::string MajorMinor::getDescription() const {
      block::DeviceClass dclass = getClass();
      std::stringstream ss;
      ss.str("");
      if ( dclass == block::MetaDisk ) {
        ss << getMDDevices() << " disk " << getMDLevel() << " ";
      }
      block::MountInfo info;
      if ( getMountInfo( info ) ) {
        if ( dclass == block::LVM ) {
          ss << getDMName() << " ";
        }
        ss << info.mountpoint << " (" << info.fstype << ")";
      } else {
        if ( getFSType() == "LVM2_member" ) {
          ss << "PV in VG " << getLVMPV2VG();
        } else if ( getDMName() != "" ) {
          ss << getDMName() << " " << getFSType();
        } else if ( getSCSIHCTL() != "" ) {
          ss << getSCSIHCTL();
        } else {
          ss << getFSType();
        }
      }
      return ss.str();
    }

    unsigned long MajorMinor::getSCSIIODone() const {
      unsigned long result = 0;
      try {
        result = util::fileReadHexString( "/sys/class/block/" + MajorMinor::getNameByMajorMinor(*this) + "/device/iodone_cnt" );
      }
      catch ( Oops &oops ) {
      }
      return result;
    }

    unsigned long MajorMinor::getSCSIIORequest() const {
      unsigned long result = 0;
      try {
        result = util::fileReadHexString( "/sys/class/block/" + MajorMinor::getNameByMajorMinor(*this) + "/device/iorequest_cnt" );
      }
      catch ( Oops &oops ) {
      }
      return result;
    }

    unsigned long MajorMinor::getSCSIIOError() const {
      unsigned long result = 0;
      try {
        result = util::fileReadHexString( "/sys/class/block/" + MajorMinor::getNameByMajorMinor(*this) + "/device/ioerr_cnt" );
      }
      catch ( Oops &oops ) {
      }
      return result;
    }

    MajorMinor MajorMinor::deriveWholeDisk( const MajorMinor& partition ) {
      if ( isSCSIDisk( MAJOR(partition.dev_) ) ) {
        return MajorMinor( MKDEV(MAJOR(partition.dev_), MINOR(partition.dev_) - MINOR(partition.dev_) % 16  ) );
      } else if ( isIDEDisk( MAJOR(partition.dev_) ) ) {
        return MajorMinor( MKDEV(MAJOR(partition.dev_), MINOR(partition.dev_) - MINOR(partition.dev_) % 64  ) );
      } else if ( isVirtIODisk( MajorMinor( MAJOR(partition.dev_), 0 ) ) ) {
        return MajorMinor( MKDEV(MAJOR(partition.dev_), MINOR(partition.dev_) - MINOR(partition.dev_) % 16  ) );
      } else if ( isNVMeDisk( MajorMinor( MAJOR(partition.dev_), 0 ) ) ) {
        std::string devname = getNameByMajorMinor( partition );
        unsigned int host = 0;
        unsigned int disk = 0;
        int r = sscanf( devname.c_str(), "nvme%un%u", &host, &disk );
        if ( r != 2 ) throw leanux::Oops( __FILE__, __LINE__, "unable to parse NVMe device name" );
        std::stringstream ss;
        ss << std::fixed << "nvme" << host << "n" << disk;
        return getMajorMinorByName( ss.str() );
      } else if ( isMMCDisk( partition ) ) {
        std::string devname = getNameByMajorMinor( partition );
        unsigned int disk = 0;
        int r = sscanf( devname.c_str(), "mmcblk%u", &disk );
        if ( r != 1 ) throw leanux::Oops( __FILE__, __LINE__, "unable to parse MMC device name" );
        std::stringstream ss;
        ss << std::fixed << "mmcblk" << disk;
        return getMajorMinorByName( ss.str() );
      } else if ( isBCacheDisk( partition ) ) {
        std::string devname = getNameByMajorMinor( partition );
        unsigned int disk = 0;
        int r = sscanf( devname.c_str(), "bcache%u", &disk );
        if ( r != 1 ) throw leanux::Oops( __FILE__, __LINE__, "unable to parse bcache device name" );
        std::stringstream ss;
        ss << std::fixed << "bcache" << disk;
        return getMajorMinorByName( ss.str() );
      }
      return partition;
    }


    bool MajorMinor::isVirtIODisk( const MajorMinor &m ) {
      std::string devname = getNameByMajorMinor( m );
      return strncmp( "vd", devname.c_str(), 2 ) == 0;
    }

    bool MajorMinor::isNVMeDisk( const MajorMinor &m ) {
      unsigned int host = 0;
      unsigned int disk = 0;
      std::string devname = getNameByMajorMinor( m );
      int r = sscanf( devname.c_str(), "nvme%un%u", &host, &disk );
      return r == 2;
    }

    bool MajorMinor::isMMCDisk( const MajorMinor &m ) {
      unsigned int disk = 0;
      std::string devname = getNameByMajorMinor( m );
      int r = sscanf( devname.c_str(), "mmcblk%u", &disk);
      return r == 1;
    }

    bool MajorMinor::isBCacheDisk( const MajorMinor &m ) {
      unsigned int disk = 0;
      std::string devname = getNameByMajorMinor( m );
      int r = sscanf( devname.c_str(), "bcache%u", &disk);
      return r == 1;
    }

    DeviceClass MajorMinor::getClass() const {
      if ( MajorMinor::isSCSIDisk( getMajor() ) ) {
        if ( isPartition() ) return SCSIDiskPartition; else return SCSIDisk;
      } else if ( MajorMinor::isIDEDisk( getMajor() ) ) {
        if ( isPartition() ) return IDEDiskPartition; else return IDEDisk;
      } else if ( MajorMinor::isVirtIODisk( *this ) ) {
        if ( isPartition() ) return VirtioDiskPartition; else return VirtioDisk;
      } else if ( MajorMinor::isNVMeDisk( *this ) ) {
        if ( isPartition() ) return NVMeDiskPartition; else return NVMeDisk;
      } else if ( MajorMinor::isMMCDisk( *this ) ) {
        if ( isPartition() ) return MMCDiskPartition; else return MMCDisk;
      } else if ( MajorMinor::isBCacheDisk( *this ) ) {
        if ( isPartition() ) return BCacheDiskPartition; else return BCacheDisk;
      } else {
        std::string lvname;
        std::string dm_uuid;
        lvname = getLVName();
        if ( lvname != "" ) return LVM;
        dm_uuid = getDMUUID();
        if ( dm_uuid.find("mpath-") != std::string::npos ) return MultiPath;

        switch ( getMajor() ) {
          case 1 : return RAMDisk;
          case 2 : return FloppyDisk;
          case 7 : return Loopback;
          case 9 : return MetaDisk;
          case 11 : return SCSICD;
          default : return Unknown;
        }
      }
      return Unknown;
    }

    std::string MajorMinor::getClassStr() const {
      DeviceClass dclass = getClass();
      switch ( dclass ) {
        case SCSIDisk:
          return "SCSI disk";
        case IDEDisk:
          return "IDE disk";
        case VirtioDisk:
          return "Virtio disk";
        case NVMeDisk:
          return "NVMe disk";
        case MetaDisk:
          return "MetaDisk";
        case MMCDisk:
          return "MMC disk";
        case BCacheDisk:
          return "bcache disk";
        case LVM:
          return "LVM LV";
        case MultiPath:
          return "multipath";
        case RAMDisk:
          return "RAM disk";
        case Loopback:
          return "loopback device";
        case SCSICD:
          return "SCSI CD/DVD";
        case FloppyDisk:
          return "Floppy disk";
        case BCacheDiskPartition:
        case MMCDiskPartition:
        case NVMeDiskPartition:
        case VirtioDiskPartition:
        case IDEDiskPartition:
        case SCSIDiskPartition:
          return "partition";
        case Unknown:
          return "unknown";
        default :
          return "bug";
      }
      return "";
    }

    std::string MajorMinor::getSysPath() const {
      std::string devname = "";
      if ( isPartition() )
        devname = "/sys/block/" + MajorMinor::deriveWholeDisk(*this).getName();
      else
        devname = "/sys/block/" + getName();
      std::string resolved_path = util::realPath( devname );
      if ( resolved_path == devname) throw Oops( __FILE__, __LINE__, "realpath failed on'" + devname +"'" );
      else {
          if ( isPartition() ) return resolved_path + "/" + getName();
          else return resolved_path;
        };
    }

    std::string MajorMinor::getSerial() const {
      std::string file = "/sys/class/block/" + MajorMinor::getNameByMajorMinor(*this) + "/device/serial";
      if ( util::fileReadAccess( file ) )
        return util::fileReadString( "/sys/class/block/" + MajorMinor::getNameByMajorMinor(*this) + "/device/serial" );
      else {
        std::string udevp = getUDevPath();
        std::ifstream i( udevp.c_str() );
        std::string line = "";
        std::string result = "";
        while ( i.good() ) {
          getline( i, line );
          if ( strncmp( line.c_str(), "E:ID_SERIAL_SHORT=", 18 ) == 0 ) {
            result = line.substr(18);
            break;
          }
        }
        return result;
      }
    }

    std::string MajorMinor::getUDevPath() const {
      std::stringstream ss;
      switch ( udev_mode ) {
        case umBlockMM :
          ss << "block" << *this;
          break;
        case umBMM :
          ss << "b" << *this;
          break;
        case umBlockName :
          ss << "block:" << MajorMinor::getNameByMajorMinor( *this );
          break;
      }
      return udev_path + ss.str();
    }

    bool MajorMinor::getRotational() const {
      bool result = true;
      std::stringstream ss;
      MajorMinor mm = *this;
      if ( isPartition() ) mm = MajorMinor::deriveWholeDisk(*this);
      try {
        result = util::fileReadString( "/sys/class/block/" + MajorMinor::getNameByMajorMinor(mm) + "/queue/rotational" )[0] == '1';
      }
      catch ( Oops & oops ) {
      }
      return result;
    }

    std::string MajorMinor::getRotationalStr() const {
      bool rotational = getRotational();
      if ( rotational ) {
        std::stringstream ss;
        ss << getRPM() << "RPM spindle";
        return ss.str();
      } else return "SSD";
    }

    unsigned long MajorMinor::getSectorSize() const {
      MajorMinor wholedisk = MajorMinor::deriveWholeDisk( *this );
      return util::fileReadUL( "/sys/class/block/" + MajorMinor::getNameByMajorMinor(wholedisk) + "/queue/hw_sector_size" );
    }

    std::string MajorMinor::getRevision() const {
      try {
        std::string result =  util::fileReadString( "/sys/class/block/" + MajorMinor::getNameByMajorMinor(*this) + "/device/rev" );
        return result;
      }
      catch ( const leanux::Oops &oops ) {
      }
      return "";
    }

    unsigned long MajorMinor::getSize() const {
      return util::fileReadUL( "/sys/class/block/" + MajorMinor::getNameByMajorMinor(*this) + "/size" ) * 512UL;
    }

    std::string MajorMinor::getDMName() const {
      std::string result = "";
      try {
        result = util::fileReadString( "/sys/class/block/" + MajorMinor::getNameByMajorMinor(*this) + "/dm/name" );
      }
      catch ( Oops &oops ) {
      }
      return result;
    }

    std::string MajorMinor::getDMUUID() const {
      std::string result = "";
      try {
        result = util::fileReadString( "/sys/class/block/" + MajorMinor::getNameByMajorMinor(*this) + "/dm/uuid" );
      }
      catch ( Oops &oops ) {
      }
      return result;
    }

    std::string MajorMinor::getDMTargetTypes() const {
      std::string udevp = getUDevPath();
      std::ifstream i( udevp.c_str() );
      std::string line = "";
      std::string result = "";
      while ( i.good() ) {
        getline( i, line );
        if ( strncmp( line.c_str(), "E:DM_TARGET_TYPES=", 18 ) == 0 ) {
          result = line.substr(18);
          break;
        }
      }
      return result;
    }

    std::string MajorMinor::getModel() const {
      std::string file = "/sys/class/block/" + MajorMinor::getNameByMajorMinor(*this) + "/device/model";
      if ( util::fileReadAccess( file ) )
        return util::fileReadString( file );
      else {
        std::string udevp = getUDevPath();
        std::ifstream i( udevp.c_str() );
        std::string line = "";
        std::string result = "";
        while ( i.good() ) {
          getline( i, line );
          if ( strncmp( line.c_str(), "E:ID_MODEL=", 11 ) == 0 ) {
            result = line.substr(11);
          }
        }
        return result;
      }
    }

    std::string MajorMinor::getKernelModule() const {
      std::string devname  = MajorMinor::getNameByMajorMinor( *this );
      std::string result = "";
      try {
        result = util::fileReadString( "/sys/block/" + devname + "/device/modalias" );
      }
      catch ( const Oops& oops ) {
        //oddity in nvme devices
        try {
          result = util::fileReadString( "/sys/block/" + devname + "/device/device/modalias" );
        }
        catch ( const Oops& oops ) {
          return "";
        }
      }
      return result;
    }

    unsigned long MajorMinor::getRPM() const {
      std::string udevp = getUDevPath();
      std::ifstream i( udevp.c_str() );
      std::string line = "";
      unsigned long result = 0;
      while ( i.good() ) {
        getline( i, line );
        if ( strncmp( line.c_str(), "E:ID_ATA_ROTATION_RATE_RPM=", 27 ) == 0 ) {
          result = strtoul( line.substr(27).c_str(),  0, 10 );
          break;
        }
      }
      return result;
    }

    std::string MajorMinor::getFSType() const {
      std::string udevp = getUDevPath();
      std::ifstream i( udevp.c_str() );
      std::string line = "";
      std::string result = "";
      while ( i.good() ) {
        getline( i, line );
        if ( strncmp( line.c_str(), "E:ID_FS_TYPE=", 13 ) == 0 ) {
          result = line.substr(13).c_str();
          if ( result == "" ) result = "block device";
          break;
        }
      }
      return result;
    }

    std::string MajorMinor::getFSUsage() const {
      std::string udevp = getUDevPath();
      std::ifstream i( udevp.c_str() );
      std::string line = "";
      std::string result = "";
      while ( i.good() ) {
        getline( i, line );
        if ( strncmp( line.c_str(), "E:ID_FS_USAGE=", 14 ) == 0 ) {
          result = line.substr(14).c_str();
          break;
        }
      }
      return result;
    }

    std::string MajorMinor::getVGName() const {
      std::string udevp = getUDevPath();
      std::ifstream i( udevp.c_str() );
      std::string line = "";
      std::string result = "";
      while ( i.good() ) {
        getline( i, line );
        if ( strncmp( line.c_str(), "E:DM_VG_NAME=", 13 ) == 0 ) {
          result = line.substr(13).c_str();
          break;
        }
      }
      return result;
    }

    std::string MajorMinor::getLVName() const {
      std::string udevp = getUDevPath();
      std::ifstream i( udevp.c_str() );
      std::string line = "";
      std::string result = "";
      while ( i.good() ) {
        getline( i, line );
        if ( strncmp( line.c_str(), "E:DM_LV_NAME=", 13 ) == 0 ) {
          result = line.substr(13).c_str();
          break;
        }
      }
      return result;
    }

    bool MajorMinor::getLVMInfo( std::string &vgname, std::string& lvname ) const {
      vgname = "";
      lvname = "";
      std::string udevp = getUDevPath();
      std::ifstream i( udevp.c_str() );
      std::string line = "";
      std::string result = "";
      int found = 0;
      while ( i.good() && found < 2 ) {
        getline( i, line );
        if ( strncmp( line.c_str(), "E:DM_VG_NAME=", 13 ) == 0 ) {
          vgname = line.substr(13).c_str();
          found++;
        } else
        if ( strncmp( line.c_str(), "E:DM_LV_NAME=", 13 ) == 0 ) {
          lvname = line.substr(13).c_str();
          found++;
        }
      }
      return found == 2;
    }

    std::string MajorMinor::getLVMPV2VG() const {
      std::string result = "";
      std::list< std::string > holders;
      this->getHolders( holders );
      for ( std::list< std::string >:: const_iterator h = holders.begin(); h != holders.end(); h++ ) {
        if ( MajorMinor::getMajorMinorByName( *h ).isValid() ) {
          result = MajorMinor::getMajorMinorByName( *h ).getVGName();
          if ( result == "" ) {
            result = getLVMPV2VG();
          } else break;
        }
      }
      return result;
    }

    std::string MajorMinor::getMDLevel() const {
      return util::fileReadString( "/sys/class/block/" + MajorMinor::getNameByMajorMinor(*this) + "/md/level" );
    }

    std::string MajorMinor::getMDName() const {
      std::string udevp = getUDevPath();
      std::ifstream i( udevp.c_str() );
      std::string line = "";
      std::string result = "";
      while ( i.good() ) {
        getline( i, line );
        if ( strncmp( line.c_str(), "E:MD_NAME=", 10 ) == 0 ) {
          result = line.substr(10).c_str();
          break;
        }
      }
      return result;
    }

    unsigned long MajorMinor::getMDChunkSize() const {
      return util::fileReadUL( "/sys/class/block/" + MajorMinor::getNameByMajorMinor(*this) + "/md/chunk_size" );
    }

    std::string MajorMinor::getIOScheduler() const {
      std::string devname  = MajorMinor::getNameByMajorMinor( *this );
      std::string path = "/sys/class/block/" + devname + "/queue/scheduler";
      std::string scheduler = util::fileReadString( path );
      size_t p1 = scheduler.find('[');
      size_t p2 = scheduler.find(']');
      if ( p1 == std::string::npos || p2 == std::string::npos ) return scheduler; else
        return scheduler.substr(p1+1, p2-p1-1 );
    }

    std::string MajorMinor::getMDMetaDataVersion() const {
      return util::fileReadString( "/sys/class/block/" + MajorMinor::getNameByMajorMinor(*this) + "/md/metadata_version" );
    }

    std::string MajorMinor::getMDArrayState() const {
      return util::fileReadString( "/sys/class/block/" + MajorMinor::getNameByMajorMinor(*this) + "/md/array_state" );
    }

    void MajorMinor::getMDRaidDisks( std::vector<MajorMinor> &disks ) const {
      disks.clear();
      std::string devname = MajorMinor::getNameByMajorMinor(*this);
      std::string path = "/sys/class/block/" + devname + "/md/";
      DIR *d;
      struct dirent *dir;
      d = opendir( path.c_str() );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( strncmp( dir->d_name, "dev-", 4 ) == 0 ) {
            std::string dev = util::fileReadString( "/sys/class/block/" + devname + "/md/" + dir->d_name + "/block/dev" );
            disks.push_back( MajorMinor( dev ) );
          }
        }
      } else throw Oops( __FILE__, __LINE__, errno );
      closedir( d );
    }

    std::string MajorMinor::getMDRaidDiskStates() const {
      std::string result = "";
      std::vector<MajorMinor> disks;
      getMDRaidDisks( disks );
      std::string devname = MajorMinor::getNameByMajorMinor(*this);
      std::string path = "/sys/class/block/" + devname + "/md/";
      for ( unsigned long i = 0; i < disks.size(); i++ ) {
        std::stringstream p;
        p << path << "dev-" << disks[i].getName() << "/state";
        std::string state = util::fileReadString( p.str() );
        if ( state == "in_sync" ) result += 'U';
        else if ( state == "faulty" ) result += 'f';
        else if ( state == "writemostly" ) result += 'W';
        else if ( state == "blocked" ) result += 'b';
        else if ( state == "spare" ) result += 'S';
        else if ( state == "write_error" ) result += 'e';
        else if ( state == "want_replacement" ) result += 'r';
        else if ( state == "replacement" ) result += 'R';
        else result += '?';
      }
      return result;
    }

    unsigned long MajorMinor::getMDDevices() const {
      return util::fileReadUL( "/sys/class/block/" + MajorMinor::getNameByMajorMinor(*this) + "/md/raid_disks" );
    }

    void MajorMinor::getAliases( std::list< std::string > &aliases ) const {
      aliases.clear();
      std::string udevp = getUDevPath();
      std::ifstream i( udevp.c_str() );
      std::string line = "";
      while ( i.good() ) {
        getline( i, line );
        if ( strncmp( line.c_str(), "S:", 2 ) == 0 ) {
          std::string dev = "/dev/" + line.substr(2);
          struct stat st;
          if ( !stat( dev.c_str(), &st ) ) {
            if ( st.st_rdev == getDevT() ) {
              aliases.push_back( "/dev/" + line.substr(2) );
            }
          }
        }
      }
    }

    std::string MajorMinor::getSCSIHCTL() const {
      std::string path = "/sys/class/block/" + MajorMinor::getNameByMajorMinor(*this) + "/device/scsi_disk";
      return util::findDir( path, "" );
    }

    std::string MajorMinor::getCacheMode() const {
      try {
        std::string devname  = MajorMinor::getNameByMajorMinor( *this );
        std::string path = "/sys/class/block/" + devname + "/device/scsi_disk/" + getSCSIHCTL() + "/cache_type";
        return util::fileReadString( path );
      }
      catch ( const Oops &oops ) {
      }
      return "";
    }

    unsigned long MajorMinor::getMaxHWIOSize() const {
      std::string devname  = MajorMinor::getNameByMajorMinor( *this );
      return 1024 * util::fileReadUL( "/sys/class/block/" + devname + "/queue/max_hw_sectors_kb" );
    }

    unsigned long MajorMinor::getMaxIOSize() const {
      std::string devname  = MajorMinor::getNameByMajorMinor( *this );
      return 1024 * util::fileReadUL( "/sys/class/block/" + devname + "/queue/max_sectors_kb" );
    }

    unsigned long MajorMinor::getMinIOSize() const {
      std::string devname  = MajorMinor::getNameByMajorMinor( *this );
      return util::fileReadUL( "/sys/class/block/" + devname + "/queue/minimum_io_size" );
    }

    unsigned long MajorMinor::getReadAhead() const {
      MajorMinor mm = *this;
      if ( mm.isPartition() ) mm = MajorMinor::deriveWholeDisk(*this);
      std::string devname  = MajorMinor::getNameByMajorMinor( mm );
      return 1024 * util::fileReadUL( "/sys/class/block/" + devname + "/queue/read_ahead_kb" );
    }

    std::string MajorMinor::getATAPort() const {
      std::string result = "";
      std::istringstream iss(getSysPath());
      std::string token = "";
      while (std::getline(iss, token, '/')) {
        if ( strncmp( token.c_str(), "ata", 3 ) == 0 ) {
          return token;
        }
      }
      return result;
    }

    std::string MajorMinor::getWWN() const {
      if ( isNVMeDisk(*this ) ) {
        return util::fileReadString( "/sys/class/block/" + MajorMinor::getNameByMajorMinor(*this) + "/wwid" );
      } else {
        std::string udevp = getUDevPath();
        std::ifstream i( udevp.c_str() );
        std::string line = "";
        std::string result = "";
        while ( i.good() ) {
          getline( i, line );
          if ( strncmp( line.c_str(), "E:ID_WWN_WITH_EXTENSION=", 24 ) == 0 ) {
            result = line.substr(24);
            break;
          }
        }
        return result;
      }
    }

    void MajorMinor::getPartitions( std::list< std::string > &partitions ) const {
      partitions.clear();
      std::string devname = MajorMinor::getNameByMajorMinor(*this);
      std::string path = "/sys/class/block/" + devname;
      DIR *d;
      struct dirent *dir;
      d = opendir( path.c_str() );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( strncmp( devname.c_str(), dir->d_name, devname.length() ) == 0 ) {
            partitions.push_back( dir->d_name );
          }
        }
      } else throw Oops( __FILE__, __LINE__, errno );
      closedir( d );
      partitions.sort();
    }

    bool MajorMinor::getMountInfo ( MountInfo &info ) const {
      std::map<MajorMinor,MountInfo> mounts;
      enumMounts( mounts );
      std::map<MajorMinor,MountInfo>::const_iterator found = mounts.find( *this );
      if ( found != mounts.end() ) {
        info = found->second;
        return true;
      } else return false;
    }


    std::string getATAPortLink( const std::string& ata_port ) {
      std::string result = "";
      std::string path = "/sys/class/ata_port/" + ata_port + "/device/";
      result = util::findDir( path, "link" );
      return result;
    }

    std::string getATALinkSpeed( const std::string& ata_port, const std::string& ata_link ) {
      std::string result = "";
      std::string path = "/sys/class/ata_port/" + ata_port + "/device/" + ata_link + "/ata_link/" + ata_link + "/sata_spd";
      result = util::fileReadString( path );
      return result;
    }

    std::string getATADeviceName( const std::string& device ) {
      std::string result = "";
      std::string path = device;
      DIR *d;
      struct dirent *dir;
      int hurray = 0;
      d = opendir( path.c_str() );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( strncmp( dir->d_name, "host", 4 ) == 0 ) {
            path += ( "/" + (std::string)dir->d_name );
            hurray++;
            break;
          }
        }
        closedir( d );
      }
      d = opendir( path.c_str() );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( strncmp( dir->d_name, "target", 6 ) == 0 ) {
            path += ( "/" + (std::string)dir->d_name );
            hurray++;
            break;
          }
        }
        closedir( d );
      }
      d = opendir( path.c_str() );
      if ( d && hurray == 2 ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( isdigit( dir->d_name[0] ) ) {
            path += ( "/" + (std::string)dir->d_name + "/block" );
            hurray++;
            break;
          }
        }
        closedir( d );
      }
      d = opendir( path.c_str() );
      if ( d && hurray == 3 ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( dir->d_name[0] != '.' ) {
            result = dir->d_name;
            break;
          }
        }
        closedir( d );
      }
      return result;
    }

    std::string getATAPortName( const std::string& device ) {
      std::string result = "";
      size_t p = device.find_last_of( "/" );
      if ( p != std::string::npos ) {
        result = device.substr( p+1 );
      }
      return result;
    }

    MajorMinor getFileMajorMinor( const std::string &devicefile ) {
      std::string resolved_path = util::realPath( devicefile );
      struct stat st;
      int r = stat( resolved_path.c_str(), &st );
      if ( r ) return MajorMinor::invalid;
      else return MajorMinor( major(st.st_rdev), minor(st.st_rdev) );
    }

    void enumMounts( std::map<MajorMinor,MountInfo> &mounts ) {
      mounts.clear();
      std::ifstream pm( "/proc/mounts" );
      while ( pm.good() ) {
        MountInfo temp;
        int dummy;
        pm >> temp.device;
        pm >> temp.mountpoint;
        pm >> temp.fstype;
        pm >> temp.attrs;
        pm >> dummy;
        pm >> dummy;
        if ( temp.device.length() > 0 && temp.device[0] == '/' ) {
          MajorMinor m = getFileMajorMinor( temp.device );
          // hack for btrfs quirk
          if ( temp.fstype == "btrfs" ) temp.mountpoint = "btrfs:" + m.getName();
          if ( m.isValid() ) mounts[m] = temp;
        }
      }
    }

    void enumMounts( std::map<MajorMinor,MountInfo> &mounts, std::map<std::string,MajorMinor> &devicefilecache ) {
      mounts.clear();
      std::ifstream pm( "/proc/mounts" );
      while ( pm.good() ) {
        MountInfo temp;
        int dummy;
        pm >> temp.device;
        pm >> temp.mountpoint;
        pm >> temp.fstype;
        pm >> temp.attrs;
        pm >> dummy;
        pm >> dummy;
        if ( temp.device.length() > 0 && temp.device[0] == '/' ) {
          std::map<std::string,MajorMinor>::const_iterator icache = devicefilecache.find(temp.device);
          MajorMinor m = MajorMinor::invalid;
          if ( icache == devicefilecache.end() ) {
            m = getFileMajorMinor( temp.device );
            devicefilecache[temp.device] = m;
          } else
            m = icache->second;
          // hack for btrfs quirk
          if ( temp.fstype == "btrfs" ) temp.mountpoint = "btrfs:" + m.getName();
          if ( m.isValid() ) mounts[m] = temp;
        }
      }
    }

    std::string MajorMinor::getDiskId() const {
      DeviceClass cl = getClass();
      std::string id = getWWN();
      if ( id == "" ) id = getSerial();
      if ( id == "" && cl == MetaDisk ) id = getMDName();
      if ( id == "" && (cl == DeviceMapper || cl == LVM ) ) id = getDMUUID();
      if ( id == "" ) id = MajorMinor::getNameByMajorMinor( *this );
      return id;
    }

    void enumDevices( std::list<MajorMinor> &devices ) {
      std::ifstream i( "/proc/diskstats" );
      if ( !i.good() ) throw Oops( __FILE__, __LINE__, "failed to open '/proc/diskstats'" );
      int major, minor;
      devices.clear();
      while ( i.good() && ! i.eof() ) {
        i >> major >> minor;
        if ( i.good() ) devices.push_back( MajorMinor(major,minor) );
        i.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
      devices.sort();
    }

    void enumDevices( std::list<MajorMinor> &devices, DeviceClass t ) {
      std::ifstream i( "/proc/diskstats" );
      if ( !i.good() ) throw Oops( __FILE__, __LINE__, "failed to open '/proc/diskstats'" );
      int major, minor;
      devices.clear();
      while ( i.good() && ! i.eof() ) {
        i >> major >> minor;
        if ( i.good() && t == MajorMinor(major,minor).getClass() ) devices.push_back( MajorMinor(major,minor) );
        i.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
      devices.sort();
    }

    void enumLVMPVS( std::list<MajorMinor> &devices ) {
      std::list<MajorMinor> all;
      devices.clear();
      enumDevices( all );
      for ( std::list<MajorMinor>::const_iterator d = all.begin(); d != all.end(); d++ ) {
        if ( (*d).getFSType() == "LVM2_member" ) {
          devices.push_back( *d );
        }
      }
    }

    void enumWholeDisks( std::list<MajorMinor> &devices ) {
      std::ifstream i( "/proc/diskstats" );
      if ( !i.good() ) throw Oops( __FILE__, __LINE__, "failed to open '/proc/diskstats'" );
      int major, minor;
      devices.clear();
      while ( i.good() && ! i.eof() ) {
        i >> major >> minor;
        if ( i.good() && MajorMinor(major,minor).isWholeDisk() ) devices.push_back( MajorMinor(major,minor) );
        i.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
      devices.sort();
    }

    unsigned long getAttachedStorageSize() {
      std::list<MajorMinor> wd;
      enumWholeDisks( wd );
      std::map< std::string, unsigned long > wwn_sizes;
      unsigned long no_wwn_total = 0;
      for ( std::list<MajorMinor>::const_iterator d = wd.begin(); d != wd.end(); d++ ) {
        std::string wwn = (*d).getWWN();
        if ( wwn == "" )
          no_wwn_total += (*d).getSize();
        else
          wwn_sizes[wwn] = (*d).getSize();
      }
      unsigned long result = 0;
      for ( std::map< std::string, unsigned long >::const_iterator i = wwn_sizes.begin(); i != wwn_sizes.end(); i++ ) {
        result += i->second;
      }
      return result + no_wwn_total;
    }

    unsigned long getAttachedWholeDisks() {
      std::list<MajorMinor> wd;
      enumWholeDisks( wd );
      return wd.size();
    }

    void MajorMinor::getHolders( std::list< std::string > &holders ) const {
      holders.clear();
      std::string devname = MajorMinor::getNameByMajorMinor(*this);
      std::string path;
      if ( isPartition() ) {
        MajorMinor whole = MajorMinor::deriveWholeDisk( *this );
        path = "/sys/class/block/" + MajorMinor::getNameByMajorMinor( whole ) + "/" + devname;
      }
      else {
        path = "/sys/class/block/" + devname;
      }
      path += "/holders";
      DIR *d;
      struct dirent *dir;
      d = opendir( path.c_str() );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( strncmp( dir->d_name, ".", 1 ) != 0 ) {
            holders.push_back( dir->d_name );
          }
        }
      } else throw Oops( __FILE__, __LINE__, errno );
      closedir( d );
      holders.sort();
    }

    void MajorMinor::getSlaves( std::list< std::string > &slaves ) const {
      slaves.clear();
      std::string devname = MajorMinor::getNameByMajorMinor(*this);
      std::string path;
      if ( isPartition() ) {
        MajorMinor whole = MajorMinor::deriveWholeDisk( *this );
        path = "/sys/class/block/" + MajorMinor::getNameByMajorMinor( whole );
      }
      else {
        path = "/sys/class/block/" + devname;
      }
      path += "/slaves";
      DIR *d;
      struct dirent *dir;
      d = opendir( path.c_str() );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( strncmp( dir->d_name, ".", 1 ) != 0 ) {
            slaves.push_back( dir->d_name );
          }
        }
      } else throw Oops( __FILE__, __LINE__, errno );
      closedir( d );
      slaves.sort();
    }

    bool MajorMinor::getStats( DeviceStats& stats ) const {
      std::string path = getSysPath() + "/stat";
      std::ifstream i( path.c_str() );
      if ( !i.good() ) throw Oops( __FILE__, __LINE__, "failed to open '" + getSysPath() + "'" );
      i >> stats.reads;
      i >> stats.reads_merged;
      i >> stats.read_sectors;
      i >> stats.read_ms;
      i >> stats.writes;
      i >> stats.writes_merged;
      i >> stats.write_sectors;
      i >> stats.write_ms;
      i >> stats.io_in_progress;
      i >> stats.io_ms;
      i >> stats.io_weighted_ms;
      stats.iodone_cnt = getSCSIIODone();
      stats.iorequest_cnt = getSCSIIORequest();
      stats.ioerr_cnt = getSCSIIOError();
      return i.good();
    }

    void getStats( DeviceStatsMap &statsmap ) {
      statsmap.clear();
      std::ifstream i( "/proc/diskstats" );
      if ( !i.good() ) throw Oops( __FILE__, __LINE__, "failed to open '/proc/diskstats'" );
      unsigned long major, minor;
      std::string devname;
      while ( i.good() && ! i.eof() ) {
        DeviceStats stats;
        i >> major >> minor >> devname;
        if ( i.good() ) {
          i >> stats.reads;
          i >> stats.reads_merged;
          i >> stats.read_sectors;
          i >> stats.read_ms;
          i >> stats.writes;
          i >> stats.writes_merged;
          i >> stats.write_sectors;
          i >> stats.write_ms;
          i >> stats.io_in_progress;
          i >> stats.io_ms;
          i >> stats.io_weighted_ms;
          MajorMinor mm( major, minor );
          stats.iodone_cnt = mm.getSCSIIODone();
          stats.iorequest_cnt = mm.getSCSIIORequest();
          stats.ioerr_cnt = mm.getSCSIIOError();
          statsmap[ mm ] = stats;
        }
        i.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
    }

    void deltaDeviceStats( const DeviceStatsMap &snap1, const DeviceStatsMap &snap2, DeviceStatsMap &delta, MajorMinorVector &vec ) {
      // some of the values may have wrapped, so check for that.
      delta.clear();
      for ( DeviceStatsMap::const_iterator s2 = snap2.begin(); s2 != snap2.end(); ++s2 ) {
        DeviceStatsMap::const_iterator s1 = snap1.find( s2->first );
        if ( s1 != snap1.end() ) {
          if ( s2->second.reads >= s1->second.reads &&
               s2->second.reads_merged >= s1->second.reads_merged &&
               s2->second.read_sectors >= s1->second.read_sectors &&
               s2->second.read_ms >= s1->second.read_ms &&
               s2->second.writes >= s1->second.writes &&
               s2->second.writes_merged >= s1->second.writes_merged &&
               s2->second.write_sectors >= s1->second.write_sectors &&
               s2->second.write_ms >= s1->second.write_ms &&
               s2->second.io_ms >= s1->second.io_ms
            ) {
            delta[s1->first].reads = s2->second.reads - s1->second.reads;
            delta[s1->first].reads_merged = s2->second.reads_merged - s1->second.reads_merged;
            delta[s1->first].read_sectors = s2->second.read_sectors - s1->second.read_sectors;
            delta[s1->first].read_ms = s2->second.read_ms - s1->second.read_ms;
            delta[s1->first].writes = s2->second.writes - s1->second.writes;
            delta[s1->first].writes_merged = s2->second.writes_merged - s1->second.writes_merged;
            delta[s1->first].write_sectors = s2->second.write_sectors - s1->second.write_sectors;
            delta[s1->first].write_ms = s2->second.write_ms - s1->second.write_ms;
            delta[s1->first].io_in_progress = s2->second.io_in_progress - s1->second.io_in_progress;
            delta[s1->first].io_ms = s2->second.io_ms - s1->second.io_ms;
            delta[s1->first].io_weighted_ms = s2->second.io_weighted_ms - s1->second.io_weighted_ms;
            delta[s1->first].iodone_cnt = s2->second.iodone_cnt - s1->second.iodone_cnt;
            delta[s1->first].iorequest_cnt = s2->second.iorequest_cnt - s1->second.iorequest_cnt;
            delta[s1->first].ioerr_cnt = s2->second.ioerr_cnt - s1->second.ioerr_cnt;
            vec.push_back( s2->first );
          }
        }
      }
    }

    int StatsSorter::operator()( MajorMinor m1, MajorMinor m2 ) {
      DeviceStatsMap::const_iterator s1 = (*delta_).find( m1 );
      DeviceStatsMap::const_iterator s2 = (*delta_).find( m2 );
      if ( s1 != (*delta_).end() && s2 != (*delta_).end() ) {
        if ( s1->second == s2->second ) return m1 < m2;
        else return s1->second < s2->second;
      } else return m1 < m2;
    }

    unsigned long getMountUsedBytes() {
      unsigned long result = 0;
      std::ifstream pm( "/proc/mounts" );
      while ( pm.good() ) {
        MountInfo temp;
        int dummy1,dummy2;
        pm >> temp.device;
        pm >> temp.mountpoint;
        pm >> temp.fstype;
        pm >> temp.attrs;
        pm >> dummy1;
        pm >> dummy2;
        if ( temp.device.length() > 0 && temp.device[0] == '/' ) {
          struct stat stat_b;
          if ( !stat( temp.mountpoint.c_str(), &stat_b ) ) {
            struct statfs statfs_b;
            if ( !statfs( temp.mountpoint.c_str(), &statfs_b ) ) {
              result += ( stat_b.st_blksize * (statfs_b.f_blocks-statfs_b.f_bavail ));
            }
          }
        }
      }
      return result;
    }

    unsigned long getMountUsedBytes( const std::string &mount ) {
      struct stat stat_b;
      if ( !stat( mount.c_str(), &stat_b ) ) {
        struct statfs statfs_b;
        if ( !statfs( mount.c_str(), &statfs_b ) ) {
          return ( stat_b.st_blksize * (statfs_b.f_blocks-statfs_b.f_bavail ));
        }
      }
      return 0;
    }

  }

}
