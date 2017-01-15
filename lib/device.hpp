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
 * leanux::SysDevice c++ header file.
 */

#ifndef LEANUX_DEVICE_HPP
#define LEANUX_DEVICE_HPP

#include <list>
#include <string>
#include <vector>

#include "block.hpp"

namespace leanux {

  /**
   * system device API.
   * @see SysDevice::SysDeviceType
   * for supported types.
   */
  namespace sysdevice {

    /**
     * A string representing a path on the sysfs filesystem.
     */
    typedef std::string SysDevicePath;

    /**
     * const for '/sys/devices'
     */
    extern const SysDevicePath sysdevice_root;

    /**
     * const for '/sys/bus'
     */
    extern const SysDevicePath sysbus_root;

    /**
     * Generic SysDevice, utility class for device detection from a SysDevicePath.
     * @see detect.
     */
    class SysDevice {
      public:

        /**
         * SysDevice type
         */
        enum SysDeviceType {
          sdtAbstract,            /**@internal */
          sdtATAPort,             /**< ATA/SATA port */
          sdtBCacheDevice,        /**< bcache block device */
          sdtBlock,               /**< generic block device */
          sdtiSCSIDevice,         /**< iSCSI device */
          sdtMapperDevice,        /**< device mapper block device */
          sdtMetaDisk,            /**< MetaDisk */
          sdtMMCDevice,           /**< MMC disk */
          sdtNetDevice,           /**< network device */
          sdtNVMeDevice,          /**< NVMe disk */
          sdtBlockPartition,      /**< generic block partition */
          sdtPCIBus,              /**< PCI bus */
          sdtPCIDevice,           /**< PCI device */
          sdtSASPort,             /**< SAS port */
          sdtSASEndDevice,        /**< SAS end device */
          sdtSCSIDevice,          /**< SCSI device */
          sdtSCSIHost,            /**< SCSI host */
          sdtSCSIRPort,           /**< SCSI rport */
          sdtSCSIVPort,           /**< SCSI rport */
          sdtUSBBus,              /**< USB bus */
          sdtUSBInterface,        /**< USB interface (USB device operation mode) */
          sdtUSBDevice,           /**< USB device */
          sdtVirtioBlockDevice,   /**< virtio block device */
          sdtVirtioNetDevice,     /**< virtio network device */
          sdtVirtualNetDevice,    /**< virtual network device */
          sdtUnknown              /**< unknown device */
        };

        /**
         * Default constructor.
         */
        SysDevice() { path_ = ""; sysdevicetype_ = sdtAbstract; };

        /**
         * Copy constructor.
         */
        SysDevice( const SysDevice &src) { path_ = src.path_; sysdevicetype_ = src.sysdevicetype_; };

        /**
         * Destructor.
         */
        virtual ~SysDevice() {};

        /**
         * Return true if the SysDevice recognizes itself in the
         * trailing part of path. If so, the path is stripped so
         * that it ends with the parent device, which may be empty for
         * top level devices.
         * Should be implemented by descendent classes.
         * @param path the SysDevicePath to accept.
         * @return true when the SysDevice descendent accepts the path as valid.
         */
        virtual bool accept( SysDevicePath &path ) { return false; };

        /**
         * Test SysDevicePath validity; path must exist (be readable)
         * and located under /sys/devices. Both conditions are not enough to
         * classify it as a SysDevice, but each SysDevice does have a path
         * that returns true here.
         */
        static bool validatePath( const SysDevicePath &path );

        /**
         * Get the path for the SysDevice.
         * @return the SysDevice path.
         */
        SysDevicePath getPath() const { return path_; };

        /**
         * Get the SysDeviceType. @see SysDeviceType.
         * @return the SysDevice type.
         */
        virtual SysDeviceType getType() const { return sysdevicetype_; };

        /**
         * test if this SysDevice is of type t.
         * @param t the SysDeviceType to test.
         * @return true if this has SysDeviceType t.
         */
        virtual bool matchSysDeviceType( SysDeviceType t) const { return sysdevicetype_ == t; };

        /**
         * Get a string representation for the SysDeviceType.
         * @return string representation of the SysDeviceType.
         */
        static std::string getTypeStr( SysDeviceType );

        /**
         * Get a human readable description of this SysDevice.
         */
        virtual std::string getDescription() const { return ""; }

        /**
         * Get the driver for this device.
         * @return the driver, or an empty string if no driver associated.
         */
        virtual std::string getDriver() const;

        /**
         * Get the device class for this device.
         * @return the device class, or an empty string if undetermined.
         */
        virtual std::string getClass() const { return ""; };

        /**
         * Transform a SysDevicePath into a list of tokens in reverse
         * order.
         */
        static void tokenize( const SysDevicePath &path, std::list<std::string> &tokens );

      protected:

        /**
         * The path for this device.
         */
        SysDevicePath path_;

        /**
         * The SysDevice SysDeviceType.
         */
        SysDeviceType sysdevicetype_;

    };

    /**
     * Generic block device.
     */
    class BlockDevice : public SysDevice {
      public:
        BlockDevice() : SysDevice() {};
        BlockDevice( const BlockDevice &src ) : SysDevice( src ) {};
        block::MajorMinor getMajorMinor() const;
        virtual std::string getDescription() const;
        virtual std::string getDriver() const;
      protected:
    };

    /**
     * Generic block device partition.
     */
    class BlockPartition : public BlockDevice {
      public:
        BlockPartition() : BlockDevice() { sysdevicetype_ = sdtBlockPartition; };
        BlockPartition( const BlockPartition &src ) : BlockDevice( src ) {};
        virtual std::string getDriver() const { return ""; };
        virtual bool accept( SysDevicePath &path );
        virtual bool matchSysDeviceType( SysDeviceType t) const { return sysdevicetype_ == t || t == sdtBlock; };
      protected:
    };

    /**
     * Generic networking device.
     */
    class NetDevice : public SysDevice {
      public:
        NetDevice() : SysDevice() { sysdevicetype_ = sdtNetDevice; };
        NetDevice( const NetDevice &src ) : SysDevice( src ) {};
        virtual bool accept( SysDevicePath &path );
      protected:
    };

    /**
     * Virtual networking device.
     */
    class VirtualNetDevice : public NetDevice {
      public:
        VirtualNetDevice() : NetDevice() { sysdevicetype_ = sdtVirtualNetDevice; };
        VirtualNetDevice( const VirtualNetDevice &src ) : NetDevice( src ) {};
        virtual bool accept( SysDevicePath &path );
        virtual bool matchSysDeviceType( SysDeviceType t) const { return sysdevicetype_ == t || t == sdtNetDevice; };
      protected:
    };

    /**
     * Generic bus device.
     */
    class BusDevice : public SysDevice {
      public:
        BusDevice() : SysDevice() {};
        BusDevice( const BusDevice &src ) : SysDevice( src ) {};
        virtual std::string getClass() const { return "bus"; };
      protected:
    };

    /**
     * Always says yes, so last resort, eats entire path.
     */
    class UnknownSysDevice : public SysDevice {
      public:
        UnknownSysDevice() : SysDevice() { sysdevicetype_ = sdtUnknown; };
        UnknownSysDevice( const UnknownSysDevice &src ) : SysDevice( src ) {};
        virtual bool accept( SysDevicePath &path ) { path = ""; return true; };
    };

    /**
     * Say yes to SAS ports
     */
    class SASPort : public SysDevice {
      public:
        SASPort() : SysDevice() { sysdevicetype_ = sdtSASPort; };
        SASPort( const SASPort &src ) : SysDevice( src ) {};
        virtual bool accept( SysDevicePath &path );
      protected:
    };

    /**
     * Say yes to SAS end devices
     */
    class SASEndDevice : public SysDevice {
      public:
        SASEndDevice() : SysDevice() { sysdevicetype_ = sdtSASEndDevice; };
        SASEndDevice( const SASEndDevice &src ) : SysDevice( src ) {};
        virtual bool accept( SysDevicePath &path );
      protected:
    };

    /**
     * Say yes to SCSI devices
     */
    class SCSIDevice : public BlockDevice {
      public:
        SCSIDevice() : BlockDevice() { sysdevicetype_ = sdtSCSIDevice; };
        SCSIDevice( const SCSIDevice &src ) : BlockDevice( src ) { address_ = src.address_; };
        virtual bool accept( SysDevicePath &path );
        virtual std::string getDescription() const;
        std::string getAddress() const { return address_; };
        virtual bool matchSysDeviceType( SysDeviceType t) const { return sysdevicetype_ == t || t == sdtBlock; };
      protected:
        std::string address_;
    };

    /**
     * Say yes to SCSI hosts
     */
    class SCSIHost : public SysDevice {
      public:
        SCSIHost() : SysDevice() { sysdevicetype_ = sdtSCSIHost; };
        SCSIHost( const SCSIHost &src ) : SysDevice( src ) { host_ = src.host_; };
        virtual bool accept( SysDevicePath &path );
        std::string getHost() const { return host_; };
      protected:
        std::string host_;
    };

    /**
     * Say yes to SCSI rport
     */
    class SCSIRPort : public SysDevice {
      public:
        SCSIRPort() : SysDevice() { sysdevicetype_ = sdtSCSIRPort; };
        SCSIRPort( const SCSIRPort &src ) : SysDevice( src ) {};
        virtual bool accept( SysDevicePath &path );
      protected:
    };

    /**
     * Say yes to SCSI vport
     */
    class SCSIVPort : public SysDevice {
      public:
        SCSIVPort() : SysDevice() { sysdevicetype_ = sdtSCSIVPort; };
        SCSIVPort( const SCSIVPort &src ) : SysDevice( src ) {};
        virtual bool accept( SysDevicePath &path );
      protected:
    };

    /**
     * Say yes to iSCSI devices
     */
    class iSCSIDevice : public SCSIDevice {
      public:
        iSCSIDevice() : SCSIDevice() { sysdevicetype_ = sdtiSCSIDevice; };
        iSCSIDevice( const iSCSIDevice &src ) : SCSIDevice( src ) { session_ = src.session_; };
        virtual std::string getDescription() const;
        virtual bool accept( SysDevicePath &path );
        std::string getTargetAddress() const;
        std::string getTargetPort() const;
        std::string getTargetIQN() const;
        virtual bool matchSysDeviceType( SysDeviceType t) const { return sysdevicetype_ == t || t == sdtBlock || t == sdtSCSIDevice; };
      protected:
        std::string session_;
    };

    /**
     * Say yes to MMC devices
     */
    class MMCDevice : public BlockDevice {
      public:
        MMCDevice() : BlockDevice() { sysdevicetype_ = sdtMMCDevice; };
        MMCDevice( const MMCDevice &src ) : BlockDevice( src ) {};
        virtual bool accept( SysDevicePath &path );
        virtual bool matchSysDeviceType( SysDeviceType t) const { return sysdevicetype_ == t || t == sdtBlock; };
      protected:
    };

    /**
     * Say yes to NVMe devices
     */
    class NVMeDevice : public BlockDevice {
      public:
        NVMeDevice() : BlockDevice() { sysdevicetype_ = sdtNVMeDevice; };
        NVMeDevice( const NVMeDevice &src ) : BlockDevice( src ) {};
        virtual bool accept( SysDevicePath &path );
        virtual bool matchSysDeviceType( SysDeviceType t) const { return sysdevicetype_ == t || t == sdtBlock; };
      protected:
    };

    /**
     * Say yes to bcache devices
     */
    class BCacheDevice : public BlockDevice {
      public:
        BCacheDevice() : BlockDevice() { sysdevicetype_ = sdtBCacheDevice; };
        BCacheDevice( const BCacheDevice &src ) : BlockDevice( src ) {};
        virtual bool accept( SysDevicePath &path );
        virtual bool matchSysDeviceType( SysDeviceType t) const { return sysdevicetype_ == t || t == sdtBlock; };
      protected:
    };

    /**
     * Say yes to (S)ATA ports
     */
    class ATAPort : public SysDevice {
      public:
        ATAPort() : SysDevice() { sysdevicetype_ = sdtATAPort; };
        ATAPort( const ATAPort &src ) : SysDevice( src ) { port_ = src.port_; };
        virtual bool accept( SysDevicePath &path );
        virtual std::string getDescription() const;
        unsigned int getPort() const { return port_; };
      protected:
        unsigned int port_;
    };

    /**
     * Say yes to virtio block devices
     */
    class VirtioBlockDevice : public BlockDevice {
      public:
        VirtioBlockDevice() : BlockDevice() { sysdevicetype_ = sdtVirtioBlockDevice; };
        VirtioBlockDevice( const VirtioBlockDevice &src ) : BlockDevice( src ) {};
        virtual bool accept( SysDevicePath &path );
        virtual bool matchSysDeviceType( SysDeviceType t) const { return sysdevicetype_ == t || t == sdtBlock; };
    };

    /**
     * Say yes to virtio net devices
     */
    class VirtioNetDevice : public NetDevice {
      public:
        VirtioNetDevice() : NetDevice() { sysdevicetype_ = sdtVirtioNetDevice; };
        VirtioNetDevice( const VirtioNetDevice &src ) : NetDevice( src ) {};
        virtual bool accept( SysDevicePath &path );
        virtual bool matchSysDeviceType( SysDeviceType t) const { return sysdevicetype_ == t || t == sdtNetDevice; };
    };

    /**
     * Say yes to devicemapper devices.
     */
    class MapperDevice : public BlockDevice {
      public:
        MapperDevice() : BlockDevice() { sysdevicetype_ = sdtMapperDevice; };
        MapperDevice( const MapperDevice &src ) : BlockDevice( src ) {};
        virtual bool accept( SysDevicePath &path );
        virtual bool matchSysDeviceType( SysDeviceType t) const { return sysdevicetype_ == t || t == sdtBlock; };
    };

    /**
     * Say yes to metadisk devices.
     */
    class MDDevice : public BlockDevice {
      public:
        MDDevice() : BlockDevice() { sysdevicetype_ = sdtMetaDisk; };
        MDDevice( const MDDevice &src ) : BlockDevice( src ) {};
        virtual bool accept( SysDevicePath &path );
        virtual bool matchSysDeviceType( SysDeviceType t) const { return sysdevicetype_ == t || t == sdtBlock; };
    };

    /**
     * Say yes to PCI devices.
     */
    class PCIDevice : public SysDevice {
      public:
        PCIDevice() : SysDevice() { sysdevicetype_ = sdtPCIDevice; };
        PCIDevice( const PCIDevice &src ) : SysDevice( src ) {};
        virtual bool accept( SysDevicePath &path );
        virtual std::string getDescription() const;
        virtual std::string getClass() const;
    };

    /**
     * Say yes to PCI busses.
     */
    class PCIBus : public BusDevice {
      public:
        PCIBus() : BusDevice() { sysdevicetype_ = sdtPCIBus; };
        PCIBus( const PCIBus &src ) : BusDevice( src ) {};
        virtual bool accept( SysDevicePath &path );
        virtual std::string getDescription() const;
    };

    /**
     * Say yes to USB devices.
     */
    class USBDevice : public SysDevice {
      public:
        USBDevice() : SysDevice() { sysdevicetype_ = sdtUSBDevice; };
        USBDevice( const USBDevice &src ) : SysDevice( src ) {};
        virtual bool accept( SysDevicePath &path );
        virtual std::string getDescription() const;
        virtual std::string getClass() const;
    };

    /**
     * Say yes to USB configurations (operation mode an USB device).
     */
    class USBInterface : public USBDevice {
      public:
        USBInterface() : USBDevice() { sysdevicetype_ = sdtUSBInterface; };
        USBInterface( const USBInterface &src ) : USBDevice( src ) {};
        virtual bool accept( SysDevicePath &path );
        virtual std::string getDescription() const;
        virtual std::string getClass() const;
    };

    /**
     * Say yes to USB busses.
     */
    class USBBus : public BusDevice {
      public:
        USBBus() : BusDevice() { sysdevicetype_ = sdtUSBBus; };
        USBBus( const USBBus &src ) : BusDevice( src ) {};
        virtual bool accept( SysDevicePath &path );
        virtual std::string getDescription() const;
        virtual std::string getClass() const;
    };

    /**
     * Enumerate all devices.
     * @param paths recevies a list of SysDevicePath paths.
     */
    void enumDevices( std::list<SysDevicePath> &paths );

    /**
     * Enumerate all block devices.
     * @param paths recevies a list of SysDevicePath paths.
     */
    void enumBlockDevices( std::list<SysDevicePath> &paths );

    /**
     * Enumerate all network devices.
     * @param paths recevies a list of SysDevicePath paths.
     */
    void enumNetDevices( std::list<SysDevicePath> &paths );

    /**
     * Enumerate all PCI bus devices.
     * @param paths recevies a list of SysDevicePath paths.
     */
    void enumPCIBusses( std::list<SysDevicePath> &paths );

    /**
     * Enumerate all PCI devices.
     * @param paths recevies a list of SysDevicePath paths.
     */
    void enumPCIDevices( std::list<SysDevicePath> &paths );

    /**
     * Enumerate all USB devices.
     * @param paths recevies a list of SysDevicePath paths.
     */
    void enumUSBDevices( std::list<SysDevicePath> &paths );

    /**
     * Enumerate all (S)ATA devices.
     * @param paths recevies a list of SysDevicePath paths.
     */
    void enumATADevices( std::list<SysDevicePath> &paths );

    /**
     * Enumerate all SCSI host devices.
     * @param paths recevies a list of SysDevicePath paths.
     */
    void enumSCSIHostDevices( std::list<SysDevicePath> &paths );

    /**
     * Return the leaf SysDevice detected in path.
     * @param path the path to detect.
     * @param parent set to the parent path of the returned device, or empty when detection fails.
     * @return the detected SysDevice or NULL.
     */
    SysDevice* leafDetect( const SysDevicePath& path, SysDevicePath &parent );

    /**
     * populates devices with the devices detected in path.
     * The first device in the list is the top level device, the last device
     * represents path. The previous device is the current device parent.
     * Note that the caller should delete the objects returned in devices.
     * @param path the path to detect. this must be a valid sysfs path
     * to a device on the system.
     * @param devices list of devices detected in the path.
     * @return false if detection fails.
     */
    bool treeDetect( const SysDevicePath& path, std::list<SysDevice*> &devices );

    /**
     * Destroy list of SysDevice* as returned by treeDetect.
     */
    inline void destroy( std::list<SysDevice*> &devices ) {
      for ( std::list<SysDevice*>::iterator i = devices.begin(); i != devices.end(); i++ ) {
        if ( *i ) delete (*i);
      }
    }

  }; // namespace device

}; // namespace leanux

#endif
