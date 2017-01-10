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
 * leanux::usb c++ header file.
 */
#ifndef LEANUX_USB_HPP
#define LEANUX_USB_HPP

#include <string>
#include <list>
#include <ostream>
#include <iomanip>
#include <oops.hpp>

/**
 * \example example_usb.cpp
 * leanux::usb example.
 */

namespace leanux {

  /**
   * @namespace leanux::usb
   * USB device API.
   */

  namespace usb {

    /**
     * initialize usb namespace.
     */
    void init() throw (Oops);

    /**
     * The location of the system's PCI device database.
     * This varies among GNU/Linux distributions, set by
     * and see pci::init().
     */
    extern std::string USBDeviceDatabase;

    /**
     * Indentifies hardware (documented in usb.ids).
     */
    struct USBHardwareId {
      unsigned int idVendor;
      unsigned int idProduct;
    };

    /**
     * human readable USBHardwareId translated through usb.ids.
     */
    struct USBHardwareInfo {
      std::string idVendor;
      std::string idProduct;
    };

    /**
     * Identifies an USB device or interface hardware class.
     * http://www.usb.org/developers/defined_class
     */
    enum USBHardwareClass {
      usbInterfaceDescriptor = 0x00,
      usbAudio = 0x01,
      usbCommAndCDC = 0x02,
      usbHID = 0x03,
      usbPhysical = 0x05,
      usbImage = 0x06,
      usbPrinter = 0x07,
      usbMassStorage = 0x08,
      usbHub = 0x09,
      usbCDCData = 0x0a,
      usbSmartCard = 0x0b,
      usbContentSecurity = 0x0d,
      usbVideo = 0x0e,
      usbPersonalHealthcare = 0x0f,
      usbAudioVideo = 0x10,
      usbBillboard = 0x11,
      usbDiagnostic = 0xdc,
      usbWireless = 0xe0,
      usbMisc = 0xef,
      usbVendorSpecific = 0xff
    };

    /**
     * Typedef for usb device absolute paths.
     * When connected to a PCI bus, such a path may be
     * '/sys/devices/pci0000:00/0000:00:1a.0/usb5/5-1'.
     * @see enumUSBDevices to obtain valid device paths for all connected USB devices.
     */
    typedef std::string USBDevicePath;

    USBHardwareClass getUSBDeviceHardwareClass( const USBDevicePath& );

    USBHardwareClass getUSBInterfaceHardwareClass( const USBDevicePath& );

    std::string getUSBHardwareClassStr( USBHardwareClass c );

    /**
     * Enumerate the USB devices attached to the system.
     * @see USBDevicePath.
     */
    void enumUSBDevices( std::list<USBDevicePath> &paths );

    /**
     * Get the USBHardwareId from an USBDevicePath.
     */
    bool getUSBHardwareId( const USBDevicePath &path, USBHardwareId &id );

    /**
     * Translate an USBHardwareId into an USBHardwareInfo by using the
     * system (/usr/share) usb.ids database.
     */
    bool getUSBHardwareInfo( const USBHardwareId &id, USBHardwareInfo &inf );

    /**
     * Get the parent device for the USBDevice.
     */
    USBDevicePath getParent( const USBDevicePath &path );


  }; // namespace usb

}; // namespace leanux

#endif
