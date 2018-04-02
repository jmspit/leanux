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
 * leanux::usb c++ source file.
 */
#include "oops.hpp"
#include "pci.hpp"
#include "util.hpp"
#include "usb.hpp"
#include "device.hpp"

#include <fstream>
#include <sstream>
#include <iomanip>

#include <string.h>

#include <iostream>

#include <dirent.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <algorithm>
#include <libgen.h>


namespace leanux {

  namespace usb {

    std::string USBDeviceDatabase = "/usr/share/misc/pci.ids";

    void init() {
      if ( util::fileReadAccess( "/usr/share/misc/usb.ids" ) ) {
        USBDeviceDatabase = "/usr/share/misc/usb.ids";
      } else
      if ( util::fileReadAccess( "/usr/share/hwdata/usb.ids" ) ) {
        USBDeviceDatabase = "/usr/share/hwdata/usb.ids";
      } else
      if ( util::fileReadAccess( "/usr/share/usb.ids" ) ) {
        USBDeviceDatabase = "/usr/share/usb.ids";
      } else throw Oops( __FILE__, __LINE__, "leanux cannot find usb.ids file" );
    }

    void enumUSBDevices( std::list<USBDevicePath> &paths ) {
      paths.clear();
      DIR *d;
      struct dirent *dir;
      std::string resolved_path;
      d = opendir( (sysdevice::sysbus_root + "/usb/devices").c_str() );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( dir->d_name[0] != '.' && ! strchr( dir->d_name, ':' ) ) {
            std::string path = sysdevice::sysbus_root + "/usb/devices/" + (std::string)dir->d_name;
            resolved_path = util::realPath( path );
            paths.push_back( resolved_path );
          }
        }
        closedir( d );
      }
    }

    USBHardwareClass getUSBDeviceHardwareClass( const USBDevicePath &path ) {
      std::string device =  path;
      if ( !util::fileReadAccess( device + "/bDeviceClass" ) ) return usbVendorSpecific;
      else return (USBHardwareClass)util::fileReadHexString( device + "/bDeviceClass" );
    }

    USBHardwareClass getUSBInterfaceHardwareClass( const USBDevicePath &path ) {
      std::string device = path;
      if ( util::fileReadAccess( device + "/bInterfaceClass" ) )
        return (USBHardwareClass)util::fileReadHexString( device + "/bInterfaceClass" );
      else if ( util::fileReadAccess( device + "/bInterfaceSubClass" ) )
        return (USBHardwareClass)util::fileReadHexString( device + "/bInterfaceSubClass" );
      else return (USBHardwareClass)0x0;
    }

    std::string getUSBHardwareClassStr( USBHardwareClass c ) {
      switch ( c ) {
        case usbInterfaceDescriptor : return "per interface";
        case usbAudio : return "audio";
        case usbCommAndCDC : return "communications and CDC control";
        case usbHID : return "human interface device";
        case usbPhysical : return "physical";
        case usbImage : return "image";
        case usbPrinter : return "printer";
        case usbMassStorage : return "mass storage";
        case usbHub : return "hub";
        case usbCDCData : return "CDC data";
        case usbSmartCard : return "smart card";
        case usbContentSecurity : return "content security";
        case usbVideo : return "video";
        case usbPersonalHealthcare : return "personal healthcare";
        case usbAudioVideo : return "audio/video";
        case usbBillboard : return "billboard";
        case usbDiagnostic : return "diagnostic";
        case usbWireless : return "wireless";
        case usbMisc : return "miscellaneous";
        case usbVendorSpecific : return "vendor specific";
        default : return "undefined";
      }
    }

    bool getUSBHardwareId( const USBDevicePath &path, USBHardwareId &id ) {
      std::string device = path;
      if ( !util::fileReadAccess( device + "/idVendor" ) ) return false;
      id.idVendor = util::fileReadHexString( device + "/idVendor" );
      if ( !util::fileReadAccess( device + "/idProduct" ) ) return false;
      id.idProduct = util::fileReadHexString( device + "/idProduct" );
      return true;
    }

    bool getUSBHardwareInfo( const USBHardwareId &id, USBHardwareInfo &inf ) {
      inf.idVendor = "";
      inf.idProduct = "";
      std::stringstream ss;
      ss << std::hex;
      ss << std::setfill('0') << std::setw(4) << id.idVendor;
      std::string hex_vendor = ss.str();
      ss.str("");
      ss << std::setfill('0') << std::setw(4) << id.idProduct;
      std::string hex_product = ss.str();
      inf.idProduct = "unknown product id 0x" + hex_product;

      std::ifstream f( USBDeviceDatabase.c_str() );
      if ( !f.good() ) throw Oops( __FILE__, __LINE__, "error opening usb.ids" );

      int state = 0;

      bool giveup = false;
      while ( f.good() && !f.eof() && !giveup && state != 2 ) {
        std::string s = "";
        getline( f, s );

        if ( s[0] != '#' ) {

          switch ( state ) {
            case 0:
              if ( strncmp( s.c_str(), hex_vendor.c_str(), 4 ) == 0 ) {
                inf.idVendor = s.substr( 6 );
                state = 1;
              }
              break;
            case 1:
              if ( s[0] != '\t' ) {
                //we hit another vendor line, so device not found
                giveup = true;
              } else {
                if ( strncmp( s.c_str()+1, hex_product.c_str(), 4 ) == 0 ) {
                  inf.idProduct = s.substr(7);
                  state = 2;
                }
              }
              break;
          }
        }
      }
      return ( state >= 1 );
    }

    USBDevicePath getParent( const USBDevicePath &path ) {
      size_t pos = path.rfind("/");
      if ( pos != std::string::npos ) return path.substr( 0, pos );
      else return "";
    }

  };

};
