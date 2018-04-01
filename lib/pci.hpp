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
 * leanux::pci c++ header file.
 */
#ifndef LEANUX_PCI_HPP
#define LEANUX_PCI_HPP

#include <string>
#include <list>
#include <ostream>
#include <iomanip>
#include "oops.hpp"

/**
 * \example example_pci.cpp
 * leanux::pci example.
 */

namespace leanux {


  namespace pci {

  /**
   * @namespace leanux::pci
   * PCI device API.
   */

    /**
     * The location of the system's PCI device database.
     * This varies among GNU/Linux distributions, set by
     * and see pci::init().
     */
    extern std::string PCIDeviceDatabase;

    /**
     * Initialize pci namespace.
     */
    void init() throw ( Oops );

    /**
     * String path relative to /sys/devices identifying the PCI device
     * on the system, typedeffed for API clarity.
     */
    typedef std::string PCIDevicePath;

    bool isPCIDevice( const PCIDevicePath &path );

    /**
     * PCI address identifying a PCI device function.
     */
    struct PCIAddress {
      /** domain. */
      unsigned int domain;
      /** bus */
      unsigned int bus;
      /** device */
      unsigned int device;
      /** function */
      unsigned int function;

      /**
       * Compare PCIAddress structs.
       * @param id the PCIAddress to compare to
       * @return true when *this < id
       */
      bool operator<( const PCIAddress& id ) const {
        return domain < id.domain ||
               (domain == id.domain && bus < id.bus) ||
               (domain == id.domain && bus == id.bus && device < id.device) ||
               (domain == id.domain && bus == id.bus && device == id.device && function < id.function );
      }

      /**
       * Compare PCIAddress structs.
       * @param id the PCIAddress to compare to
       * @return true when *this == id
       */
      bool operator==( const PCIAddress& id ) const {
        return domain == id.domain && bus == id.bus && device == id.device && function == id.function;
      }

    };

    /**
     * Invalid PCIAddress.
     */
    extern const PCIAddress NullPCIAddress;

    /**
     * Get the EBDF format std::string for a PCIId.
     * 0000:03:04.0 (BDF with domain extension):
     * 0000 = domain
     * 03   = bus number
     * 04   = device number
     * 0    = function number
     * @see getBDF
     * @param id the PCIAddress to be translated to EBDF
     * @return the EBDF notation for PCIAddress
     */
    std::string getEBDF( const PCIAddress &id );

    /**
     * Get the BDF format std::string for a PCIId
     * 03:04.0
     * 03   = bus number
     * 04   = device number
     * 0    = function number
     * @see getEBDF
     * @param id the PCIAddress to be translated to BDF
     * @return the BDF notation for PCIAddress
     */
    std::string getBDF( const PCIAddress &id );

    /**
     * Decode an EBDF std::string into a PCIAddress.
     * @param s the EBDF std::string to decode.
     * @return the PCIAddress that s specifies in EBDF.
     */
    PCIAddress decodeEBDF( const std::string &s );

    /**
     * enumerate main PCI hardware classes
     */
    enum PCIHardwareClass {
      pciOld = 0x0,
      pciMassStorage = 0x01,
      pciNetwork = 0x02,
      pciDisplay = 0x03,
      pciMultimedia  = 0x04,
      pciMemory = 0x05,
      pciBridge = 0x06,
      pciSimpleCommunication = 0x07,
      pciBaseSystemPeripheral = 0x08,
      pciInput = 0x09,
      pciDockingStation = 0x0a,
      pciProcessor = 0x0b,
      pciSerialBus = 0x0c,
      pciWireless = 0x0d,
      pciIntelligentIO = 0x0e,
      pciSatelliteCommunication = 0x0f,
      pciEncryption = 0x10,
      pciDataAquisistion = 0x11,
      pciUnclassified = 0xff
    };

    /**
     * Transform a PCI class code to a PCIHardwareClass.
     * http://wiki.osdev.org/PCI#Class_Codes
     */
    PCIHardwareClass getPCIHardwareClass( unsigned int classcode );

    /**
     * Transform a PCI class code to class name.
     * http://wiki.osdev.org/PCI#Class_Codes
     */
    std::string getPCIHardwareClassString( unsigned int classcode );

    /**
     * Get the PCI subclass name for a PCI class code
     * http://wiki.osdev.org/PCI#Class_Codes
     */
     std::string getPCISubClassString( unsigned int classcode );

    /**
     * Identifies PCI hardware.
     * @see getPCIHardwareId
     */
    struct PCIHardwareId {
      unsigned int pciclass;
      unsigned int vendor;
      unsigned int device;
      unsigned int subvendor;
      unsigned int subdevice;
    };

    /**
     * Human readable variant of PCIHardwareId.
     * @see getPCIHardwareInfo.
     */
    struct PCIHardwareInfo {
      std::string pciclass;
      std::string pcisubclass;
      std::string vendor;
      std::string device;
      std::string subdevice;
    };

    /**
     * Get PCI device info based on (vendor,device)
     * function retrieves data from /usr/share/misc/pci.ids
     * @param id the PCIHardwareId of the PCI device.
     * @param inf the PCIHardwareInfo to fill.
     * @return true when the device is found, in which case subdevicestring may be empty.
     */
    bool getPCIHardwareInfo( const PCIHardwareId& id,
                             PCIHardwareInfo &inf );

    /**
     * get a complete std::list of PCIDevicePaths pointing to PCI devices
     * under /sys/devices.
     * @param paths the std::list of device paths to fill.
     * @see getPCIHardwareId to convert a device path std::string to a PCIHardwareId.
     */
    void enumPCIDevicePaths( std::list<PCIDevicePath> &paths );

    /**
     * get PCIHardwareId for a PCI device path relative to /sys/devives/
     */
    PCIHardwareId getPCIHardwareId( const PCIDevicePath& devicepath );

    /**
     * return a device path relative to /sys/devices to the PCI
     * device assigned the irq
     * @param irq the sought irq
     * @return the device path realtive to /sys/devices/ if found, empty std::string otherwise
     */
    PCIDevicePath findPCIDeviceByIRQ( unsigned int irq );

    /**
     * return a device path relative to /sys/devices to the PCI
     * device providing the ATA port.
     * @param port the sought ATA port
     * @return the device path realtive to /sys/devices/ if found, empty std::string otherwise
     */
    PCIDevicePath findPCIDeviceByATAPort( const std::string& port );

    /**
     * Get the IRQ assigned to a PCI device
     */
    unsigned int getPCIDeviceIRQ( const PCIDevicePath& device );

    /**
     * Enumerate ata ports for a given PCI device. Only valid to call on pciMassStorage devcies.
     * @param device the PCIDevicePath.
     * @param ports a std::list of ata ports.
     */
    void enumPCIATAPorts( const PCIDevicePath& device, std::list<std::string> &ports );

    /**
     * handy for debugging.
     */
    inline std::ostream& operator<<( std::ostream& os, PCIAddress a ) {
      os << std::hex;
      os << std::setfill('0') << std::setw(4) << a.domain;
      os << ':';
      os << std::setfill('0') << std::setw(2) << a.bus;
      os << ':';
      os << std::setfill('0') << std::setw(2) << a.device;
      os << '.';
      os << a.function;
      return os;
    }

    /**
     * Get the name of an ethernet network communication device, such as 'eth0'.
     * @param device the network device to get the name for.
     * @return the network device name or empty if not found.
     */
    std::string getEtherNetworkInterfaceName( const PCIDevicePath& device );

    /**
     * handy for debugging.
     */
    inline std::ostream& operator<<( std::ostream& os, PCIHardwareInfo i ) {
      os << "pciclass    : " << i.pciclass << std::endl;
      os << "pcisubclass : " << i.pcisubclass << std::endl;
      os << "vendor      : " << i.vendor << std::endl;
      os << "device      : " << i.device << std::endl;
      os << "subdevice   : " << i.subdevice << std::endl;
      return os;
    }

  } // namespace pci
} // namespace leanux

#endif
