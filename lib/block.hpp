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
 * @file block.hpp
 * leanux::block c++ header file.
 */
#ifndef LEANUX_BLOCK_HPP
#define LEANUX_BLOCK_HPP

#include <string>
#include <ostream>
#include <list>
#include <map>
#include <vector>
#include <iostream>

#include <stdlib.h>
#include <linux/kdev_t.h>
#include <stdio.h>

namespace leanux {

  /**
   * block device API.
   *
   * Block devices are represented by instances of the MajorMinor class,
   * of which the member functions retrieve per-block device
   * statistics. Block devices present on the system are enumerated into lists
   * of MajorMinor objects with one of the enum* functions, such as
   * enumWholeDisks(std::list< MajorMinor > &devices).
   */
  namespace block {

    /**
     * Initialize the block API.
     * @see leanux::init.
     */
    void init();

    /** Mounted filesystem configuration. */
    struct MountInfo {
      /** The name of the device containing the filesystem. */
      std::string device;
      /** The mountpoint of the filesystem. */
      std::string mountpoint;
      /** The type of filesystem. */
      std::string fstype;
      /** The mount attributes. */
      std::string attrs;
    };

    /**
     * Enumeration to discern device types.
     */
    enum DeviceClass {
      Unknown,
      RAMDisk,
      FloppyDisk,
      IDEDisk,
      IDEDiskPartition,
      Loopback,
      SCSIDisk,
      SCSIDiskPartition,
      MetaDisk,
      SCSICD,
      DeviceMapper,
      LVM,
      MultiPath,
      VirtioDisk,
      VirtioDiskPartition,
      NVMeDisk,
      NVMeDiskPartition,
      MMCDisk,
      MMCDiskPartition,
      BCacheDisk,
      BCacheDiskPartition
    };

    /**
     * Device stats from /proc/diskstats
     * @see iostats.txt in the Linux kernel docs.
     */
    struct DeviceStats {
      unsigned long reads;          /**< number of read operations */
      unsigned long reads_merged;   /**< number of read operations merged with other read operations */
      unsigned long read_sectors;   /**< number of sectors read */
      unsigned long read_ms;        /**< number of milliseconds spent reading */
      unsigned long writes;         /**< number of write operations */
      unsigned long writes_merged;  /**< number of writes merged with other write operations */
      unsigned long write_sectors;  /**< number of sectors written */
      unsigned long write_ms;       /**< number of milliseconds spent writing */
      unsigned long io_in_progress; /**< number of io's in progress (or pending and queued) */
      unsigned long io_ms;          /**< number of milliseconds spent doing IO */
      unsigned long io_weighted_ms; /**< not sure. */
    };

    /**
     * Datatype for major:minor pairs.
     * @see invalid.
     */
    class MajorMinor {
      public:

        /**
         * Construct from major and minor values.
         * @param maj the major number.
         * @param min the minor number.
         */
        MajorMinor( unsigned long maj, unsigned long min ) {
          dev_ = MKDEV(maj,min);
        }

        /**
         * Construct from Linux dev_t.
         * @param dev the Linux dev_t.
         */
        MajorMinor( dev_t dev ) {
          dev_ = dev;
        }

        /**
         * Default constructor
         */
        MajorMinor() {
          dev_ = MKDEV(0,0);
        }

        /**
         * Construct from string.
         * @param s string as in '8:0'
         */
        MajorMinor( const std::string& s ) {
          *this = s;
        }

        /**
         * Assign a string.
         * @param s string as in '8:0'.
         * @return reference to this.
         */
        MajorMinor& operator=(const std::string &s );

        /**
         * Test equality.
         * @param m MajorMinor to compare to.
         * @return true if *this == m.
         */
        inline bool operator==( const MajorMinor& m ) const {
          return m.dev_ == dev_;
        }

        /**
         * Test inequality.
         * @param m MajorMinor to compare to.
         * @return true if *this != m.
         */
        inline bool operator!=( const MajorMinor& m ) const {
          return m.dev_ != dev_;
        }

        /**
         * Get the major.
         * @return the major.
         */
        long getMajor() const { return MAJOR(dev_); };

        /**
         * Get the minor.
         * @return the minor.
         */
        long getMinor() const { return MINOR(dev_); };

        /**
         * compare MajorMinor objects.
         * @param m the MajorMinor to compoare this to.
         * @return true of this < m.
         */
        inline bool operator<( const MajorMinor& m ) const {
          return m.dev_ > dev_;
        }

        /**
         * determine if the major represents a SCSI disk.
         * @param major the device major.
         * @return true when the major represents a SCSI disk.
         */
        static bool isSCSIDisk( int major ) {
          return major == 8 || (major >= 65 && major <= 71) || (major >= 128 && major <= 135 );
        }


        /**
         * determine if the major represents an IDE disk.
         * @param major the device major.
         * @return true when the major represents an IDE disk.
         */
        static bool isIDEDisk( int major  ) {
          return major == 3 || major == 22 || major == 33 || major == 34;
        }

        /**
         * determine if the MajorMinor represents a virtio disk.
         * @param m the device MajorMinor.
         * @return true when the MajorMinor represents a virtio disk.
         */
        static bool isVirtIODisk( const MajorMinor &m );

        /**
         * determine if the MajorMinor represents a NVMe disk.
         * @param m the device MajorMinor.
         * @return true when the MajorMinor represents a NVMe disk.
         */
        static bool isNVMeDisk( const MajorMinor &m );

        /**
         * determine if the MajorMinor represents an MMC disk.
         * @param m the device MajorMinor.
         * @return true when the MajorMinor represents an MMC disk.
         */
        static bool isMMCDisk( const MajorMinor &m );

        /**
         * determine if the MajorMinor represents a bcache disk.
         * @param m the device MajorMinor.
         * @return true when the MajorMinor represents a bcache disk.
         */
        static bool isBCacheDisk( const MajorMinor &m );

        /**
         * determine if the MajorMinor represents a MetaDisk (software raid).
         * @param major the device MajorMinor.
         * @return true when the MajorMinor represents a MetaDisk.
         */
        static inline bool isMetaDisk( int major ) {
          return major == 9;
        }

        /**
         * determine if this MajorMinor represents a MetaDisk (software raid).
         * @return true when the MajorMinor represents a MetaDisk.
         */
        inline bool isMetaDisk() const {
          return isMetaDisk( MAJOR(dev_) );
        }

        /**
         * Check if a disk is a whole disk (and not a partition).
         * @return true if the MajorMinor represents a whole IDE or SCSI disk.
         */
        bool isWholeDisk() const {
          if ( isSCSIDisk( MAJOR(dev_) ) ) return (MINOR(dev_) % 16) == 0;
          else if ( isIDEDisk( MAJOR(dev_) ) ) return (MINOR(dev_) % 64 ) == 0;
          else if ( isVirtIODisk( *this ) && MINOR(dev_) % 16 == 0 ) return true;
          else if ( isNVMeDisk( *this ) && !isPartition() ) return true;
          else if ( isMMCDisk( *this ) && !isPartition() ) return true;
          else return false;
        }

        /**
         * @return true if the block device is a disk partition.
         */
        bool isPartition() const {
          if ( isSCSIDisk( MAJOR(dev_) ) ) return (MINOR(dev_) % 16) != 0;
          else if ( isIDEDisk( MAJOR(dev_) ) ) return (MINOR(dev_) % 64 ) != 0;
          else if ( isVirtIODisk( MajorMinor( MAJOR(dev_), 0  )  ) ) return (MINOR(dev_) % 16 ) != 0;
          else if ( isNVMeDisk( MajorMinor( MAJOR(dev_), MINOR(dev_) ) ) ) {
            unsigned int host = 0;
            unsigned int disk = 0;
            unsigned int partition = 0;
            std::string devname = getNameByMajorMinor( *this );
            int r = sscanf( devname.c_str(), "nvme%un%up%u", &host, &disk, &partition );
            return r == 3;
          }
          else if ( isMMCDisk( MajorMinor( MAJOR(dev_), MINOR(dev_) ) ) ) {
            unsigned int disk = 0;
            unsigned int partition = 0;
            std::string devname = getNameByMajorMinor( * this );
            int r = sscanf( devname.c_str(), "mmcblk%up%u", &disk, &partition );
            return r == 2;
          }
          else if ( isBCacheDisk( MajorMinor( MAJOR(dev_), MINOR(dev_) ) ) ) {
            // it seems creating partitions on bcache devices does not really work
            // but if it would/will it would look like this
            unsigned int disk = 0;
            unsigned int partition = 0;
            std::string devname = getNameByMajorMinor( * this );
            int r = sscanf( devname.c_str(), "bcache%up%u", &disk, &partition );
            return r == 2;
          }
          else return false;
        }

        /**
         * Derive the MajorMinor of the whole disk holding a partition. If the parameter partiton is not a partition,
         * given MajorMinor is returned.
         * @param partition the MajorMinor of the partition.
         * @return the MajorMinor of the wholedisk.
         */
        static MajorMinor deriveWholeDisk( const MajorMinor& partition );

        /**
         * Get the MajorMinor for the device name.
         * @param devicename the device name to find.
         * @return the MajorMinor of the device, or
         * MajorMinor::invalid when the devicename is not a block device.
         */
        static MajorMinor getMajorMinorByName( const std::string& devicename );

        static MajorMinor getMajorMinorByDeviceFile( const std::string& devicefile );

        /**
         * Get the DeviceName for the MajorMinor.
         * Note that this DeviceName is the name as it appears in /proc/diskstats
         * and /sys/class/block (or genreally, sysfs). This name may differ from the name as it
         * appears under /dev, as an udev rule may have renamed the devicefile. So 'sdc'
         * may be represented by '/dev/oracleasm/disk1', which is the 'DeviceFile', not
         * the 'DeviceName'.
         * @param m the device MajorMinor.
         * @return the name of the device,
         */
        static std::string getNameByMajorMinor( const MajorMinor &m );

        /**
         * Get the DeviceFile for the MajorMinor, such as '/dev/sda'.
         * @see getNameByMajorMinor
         * @param m the device MajorMinor.
         * @return the name of the device,
         */
        static std::string getDeviceFileByMajorMinor( const MajorMinor &m );

        /**
         * get the device name for this MajorMinor.
         * @return the device name.
         */
        std::string getName() const { return getNameByMajorMinor( *this ); };

        /**
         * get the devicefile for this MajorMinor.
         * @return the device file.
         */
        std::string getDeviceFile() const { return getDeviceFileByMajorMinor( *this ); };

        /**
         * Invalid MajorMinor.
         */
        static const MajorMinor invalid;

        /**
         * Test MajorMinor validity.
         * @see invalid.
         * @return true when this MajorMinor is invalid.
         */
        inline bool isValid() const { return *this != invalid; };

        /**
         * return the MajorMinor dev_t.
         * @return the MajorMinor dev_t.
         */
        dev_t getDevT() const { return dev_; };

        /**
         * Get the DeviceClass for a MajorMinor.
         */
        DeviceClass getClass() const;

        /**
         * Get a descriptive string for the device type of the MajorMinor.
         */
        std::string getClassStr() const;

        /**
         * return the full device path as it appears under /sys/devices/block
         */
        std::string getSysPath() const;

        /**
         * Get the serial number for the device.
         */
        std::string getSerial() const;

        /**
         * Get the udev path for the device.
         * gentoo, ubuntu
         * /run/udev/data/b{major}:{minor}.
         * suse, redhat
         * /dev/.udev/db/block:{devicename}.
         * suse 11.3
         * /dev/.udev/db/b{major}:{minor}.
         * @return the udev path as described or an empty string if not found
         */
        std::string getUDevPath() const;


        /**
         * Chech if a disk is mechanical or solid state.
         * @return treu when the device is rotational.
         */
        bool getRotational() const;

        /**
         * Get a descriptive string of the rotational nature of the disk,
         * @return strings like 'SSD' or '7200RPM spindle'.
         */
        std::string getRotationalStr() const;

        /**
         * get the device sector size. yields correct result for partitions
         * as it returns the sector size of the device holding the partition.
         * @return the device sector size in bytes.
         */
        unsigned long getSectorSize() const;

        /**
         * firmware revision of the device
         */
        std::string getRevision() const;

        /**
         * Return the size (capacity) of the device in bytes.
         * @return the device capacity in bytes.
         */
        unsigned long getSize() const;

        /**
         * return the device dm name (or an empty string if not a dm device)
         * @return the device dm name or an empty string.
         */
        std::string getDMName() const;

        /**
         * return the device dm uuid (or an empty string if not a dm device)
         * @return the device dm name or an empty string.
         */
        std::string getDMUUID() const;

        /**
         * return the device dm target type (or an empty string if not a d device)
         * @return the device dm target type or an empty string.
         */
        std::string getDMTargetTypes() const;

        /**
         * Get the model for the device.
         * @return the device model or empty if unavailable.
         */
        std::string getModel() const;

        /**
         * get the kernel module (driver) used for the block device, such as 'scsi' or 'virtio'.
         * @return the kernel module.
         */
        std::string getKernelModule() const;

        /**
         * Get the rotation speed of a device.
         * @return the rotational speed if found, zero therwise. SSD's also report 0,
         * @see getRotational.
         */
        unsigned long getRPM() const;

        /**
         * Get the filesystem type of the block device.
         * @return the block device filesystem.
         */
        std::string getFSType() const;

        /**
         * Get the usage type of the block device.
         * @return the block device usage type.
         */
        std::string getFSUsage() const;

        /**
         * Get the VG name the block device belongs to, or empty string if the device is not
         * a LVM PV.
         * @return the LVM VG name.
         */
        std::string getVGName() const;

        /**
         * Get the LV name the block device belongs to, or empty string if the device is not
         * a LVM PV.
         * @return the LVM LV name.
         */
        std::string getLVName() const;

        /**
         * return VG and LV name for a LVM device.
         * @param vgname will be set to VG name if function returns true.
         * @param lvname will be set to LV name if function returns true.
         * @return false if either E:DM_VG_NAME or E:DM_LV_NAME is not found in udev data fro the device.
         */
        bool getLVMInfo( std::string &vgname, std::string& lvname ) const;

        /**
         * return the VG name the PV belongs to, or empty if the device is
         * not a LVM PV.
         * @return the VG name to which m is a physical volume.
         */
        std::string getLVMPV2VG() const;

        /**
         * Get the device MetaDisk RAID level or empty if the device is not an MetaDisk.
         * @return the MetaDisk RAID level.
         */
        std::string getMDLevel() const;

        /**
         * Get the device MetaDisk name or empty if the device is not an MetaDisk.
         * @return the MetaDisk name.
         */
        std::string getMDName() const;

        /**
         * Get the MD chunck size, valid for MetaDisk block devices.
         * @return the chunk size or zero.
         */
        unsigned long getMDChunkSize() const;

        /**
         * Get the MD metadata version, valid for MetaDisk block devices.
         * @return the metadata version or empty.
         */
        std::string getMDMetaDataVersion() const;

        /**
         * Get the MD array state, valid for MetaDisk block devices.
         * @return the Array state or empty.
         */
        std::string getMDArrayState() const;

        /**
         * Get the raid disks participating in the MD array specified my MajorMinor.
         * @param disks vector of MajorMinor for eachtof the disks (block devices) participating in the array. The vector index matches
         * the index of the MajorMinor in the array.
         */
        void getMDRaidDisks( std::vector<MajorMinor> &disks ) const;

        /**
         * Get a string representing raid disk states as seen in /proc/mdstat
         * @return the MDRaidDiskStates string.
         */
        std::string getMDRaidDiskStates() const;

        /**
         * Get the number of members in the MetaDisk or zero if not a MetaDisk.
         * @return the number of members in the MetaDisk.
         */
        unsigned long getMDDevices() const;

        /**
         * get a list of device aliases, returned as full paths.
         * @param aliases list of device aliases.
         */
        void getAliases( std::list<std::string> &aliases ) const;

        /**
         * get the host:channel:target:lun addess for the device.
         * @return the HCTL or an empty string if not found.
         */
        std::string getSCSIHCTL() const;

        /**
         * get the iSCSI target address for the device.
         * @return the target adddress or an empty string if not found.
         */
        std::string getiSCSITargetAddress() const;

        std::string getiSCSITargetPort() const;

        /**
         * get the SCSI disk caching mode (write back/write through).
         * @return the SCSI disk caching mode or an empty string if not found.
         */
        std::string getCacheMode() const;

        /**
         * get the IO scheduler (elevator) configured for the device.
         * @return the IO scheduler configured.
         */
        std::string getIOScheduler() const;

        /**
         * get the maximum IO size the hardware device reports to support.
         * @return the maximum hardware IO size in bytes.
         */
        unsigned long getMaxHWIOSize() const;

        /**
         * get the maximum IO size configured to the device.
         * @return the maximum configured IO size in bytes.
         */
        unsigned long getMaxIOSize() const;

        /**
         * get the minimum IO size the device supports.
         * @return the minimum IO size in bytes.
         */
        unsigned long getMinIOSize() const;

        /**
         * get the read-ahead size for the device
         * @return the read-ahead size in bytes.
         */
        unsigned long getReadAhead() const;

        /**
         * Get the ATA port for the block MajorMinor.
         * @return the ATA port or empty if the block device is not ATA connected.
         */
        std::string getATAPort() const;

        /**
         * Some block devices do not have a WWN, so there is no
         * guarentee a WWN exists.
         * @return the WWN of the disk or empty if no WWN assigned to the disk
         */
        std::string getWWN() const;

        /**
         * Retrieve a string identifying a disk on a best-efffort basis.
         * If the disk has a WWN, that is returned,
         * else if the disk has a serial number, that is returned,
         * else the device name is returned, in absence of required truth.
         * virtio and vmware disks can lack a WWN and a serial number, so these
         * disk types are impossible to identify (without tagging their contents).
         * @return a best-effort id for the disk.
         */
        std::string getDiskId() const;

        /**
         * Get a list of partition devices in the disk,
         * @param partitions list of device names filled.
         */
        void getPartitions( std::list<std::string> &partitions ) const;

        /**
         * get MountInfo on devices with a mounted filesystem.
         * @param info the MountInfo to fill.
         * @return false if the device (or it's aliases) does not appear in /proc/mounts.
         */
        bool getMountInfo ( MountInfo &info ) const;

        /** Get a list of devices holding (using) the device m.
         * @param holders the devices holding m.
         */
        void getHolders( std::list<std::string> &holders ) const;

        /** Get a list of devices slave to the device m.
         * @param slaves providing m.
         */
        void getSlaves( std::list<std::string> &slaves ) const;

        /**
         * get performance statistics for the block device specified by MajorMinor.
         * note that all numbers are totals since boot.
         * @param stats the statistics to populate.
         * @return false if the MajorMinor is not found.
         */
        bool getStats( DeviceStats& stats ) const;

        /**
         * Get a loose description of what the device mm is.
         */
        static std::string getDescription( const block::MajorMinor &mm );

      private:
        /** (re)build the cached mapping between device names and MajorMinor numbers. */
        static void buildCache();

        /**
         * The corresponding Linux dev_t.
         */
        dev_t dev_;

        /** time of last rebuild. */
        static struct timeval cacheage_;

        /** cache of device name to MajorMinor mapping. */
        static std::map<std::string,MajorMinor> name2mm_;

        /** cache of MajorMinor to device name mapping. */
        static std::map<MajorMinor,std::string> mm2name_;

        /** cache of devicefile to MajorMinor mapping. */
        static std::map<std::string,MajorMinor> file2mm_;

        /** cache of MajorMinor to devicefile mapping. */
        static std::map<MajorMinor,std::string> mm2file_;

    };

    /**
     * Write MajorMinor to stream as a string, eg {8,0} is written as '8:0'.
     */
    inline std::ostream& operator<<( std::ostream& s, const MajorMinor& m ) {
      s << MAJOR(m.getDevT()) << ":" << MINOR(m.getDevT());
      return s;
    }

    /**
     * Get the ATA port link.
     * @param port the ATA port as in 'ata1'.
     * @return the ATA port link.
     */
    std::string getATAPortLink( const std::string& port );

    /**
     * Get the ATA port link speed.
     * @param ata_port the ATA port as in 'ata1'.
     * @param ata_link the ATA port link as in 'link1'.
     * @return the ATA port link speed.
     */
    std::string getATALinkSpeed( const std::string& ata_port, const std::string& ata_link );

    /**
     * Get a map of MajorMinor to MountInfo from /proc/mounts.
     * @param mounts receives the map.
     */
    void enumMounts( std::map<MajorMinor,MountInfo> &mounts );

    /**
     * Variant of enumMounts that accepts a cache of previously detected mappings between
     * device special file names and MajorMinor. This cache will be updated when
     * entries are absent, so the first call may specify an empty cache, causing
     * subsequent calls to hit and avoid a relatively costly lookup.
     * @param mounts receives the map.
     * @param cache read through on cache miss, add to cache.
     */
    void enumMounts( std::map<MajorMinor,MountInfo> &mounts, std::map<std::string,MajorMinor> &cache );

    /**
     * get the MajorMinor for a device file or aliases to it, resolves
     * the devicefile with realpath.
     * @param devicefile the device special file or an alias (link) to it.
     * @return the MajorMinor or MajorMinor::invalid
     */
    MajorMinor getFileMajorMinor( const std::string &devicefile );

    /**
     * Used bytes over all mounted filesystems.
     * @return the number of bytes used.
     */
    unsigned long getMountUsedBytes();

    /**
     * Bytes used on the filesystem.
     * @param mount the filesystem mountpoint (or any filesystem file).
     * @return the number of bytes used.
     */
    unsigned long getMountUsedBytes( const std::string &mount );

    /**
     * get a list of all block devices
     * @param devices the device list to fill
     */
    void enumDevices( std::list<MajorMinor> &devices );

    /**
     * get a list of block devices of the specified DeviceClass
     * @param devices the list to fill
     * @param t list only these type of block devices
     */
    void enumDevices( std::list<MajorMinor> &devices, DeviceClass t );

    /**
     * get a list of whole disks (exclude partitions).
     * @param devices list of whole disk MajorMinors.
     * @see MajorMinor::isWholeDisk
     */
    void enumWholeDisks( std::list<MajorMinor> &devices );

    /**
     * get a list of LVM physical volumes
     * @param devices list of LVM physical volumes
     */
    void enumLVMPVS( std::list<MajorMinor> &devices );

    /**
     * total attached storage.
     * identical WWN's (multipath, software raid) are counted only once.
     * @return the attached storage size in bytes.
     */
    unsigned long getAttachedStorageSize();

    /**
     * Get the number of attached whole disks.
     * @return the number of attached whole disks.
     */
    unsigned long getAttachedWholeDisks();

    /**
     * Default < operator for DeviceStats type.
     * @param s1 DeviceStats 1
     * @param s2 DeviceStats 2
     * @return 1 if s1 < s2, 0 otherwise.
     */
    inline int operator<( const DeviceStats &s1, const DeviceStats &s2 ) {
      return s1.io_ms > s2.io_ms ||
             ( s1.io_ms == s2.io_ms && s1.reads + s1.writes > s2.reads + s2.writes );
    }

    inline int operator==( const DeviceStats &s1, const DeviceStats &s2 ) {
      return s1.io_ms         == s2.io_ms &&
             s1.reads         == s2.reads &&
             s1.writes        == s2.writes &&
             s1.read_sectors  == s2.read_sectors &&
             s1.write_sectors == s2.write_sectors &&
             s1.read_ms       == s2.read_ms &&
             s1.write_ms      == s2.write_ms &&
             s1.reads_merged  == s2.reads_merged &&
             s1.writes_merged == s2.writes_merged;
    }

    /** map of MajorMinor to DeviceStats. */
    typedef std::map<MajorMinor,DeviceStats> DeviceStatsMap;

    /** vector of MajorMinor */
    typedef std::vector<MajorMinor> MajorMinorVector;

    /**
     * get block device statistics into a DeviceStatsMap.
     * @param statsmap the DeviceStatsMap to fill
     */
    void getStats( DeviceStatsMap &statsmap );

    /**
     * create a delta of two DeviceStatsMaps. Only MajorMinor objects
     * present in both snapshots are returned in delta.
     * @param snap1 first (earlier) DeviceStatsMap.
     * @param snap2 second (later) DeviceStatsMap.
     * @param delta DeviceStatsMap receiving the delta.
     * @param vec out unsorted vector of MajorMinor entries in the delta map
     * @see StatsSorter
     */
    void deltaDeviceStats( const DeviceStatsMap &snap1, const DeviceStatsMap &snap2, DeviceStatsMap &delta, MajorMinorVector &vec );

    /** sorter for DeviceStatsMap delta. */
    class StatsSorter {
      public:
        /**
         * Initialize the StatsSorter.
         * @param delta a pointer to the DeviceStatsMap backing the sort.
         */
        StatsSorter( const DeviceStatsMap *delta ) { delta_ = delta; };

        /**
         * functor on two arghuments to sort.
         * @param m1 first MajorMinor.
         * @param m2 second MajorMinor.
         * @return true when m1 < m2.
         */
        int operator()( MajorMinor m1, MajorMinor m2 );

      private:
        /** pointer to the DeviceStatsMap backing the StatsSorter. */
        const DeviceStatsMap *delta_;
    };

  }

}

#endif
