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
 * leanux::pci c++ source file.
 */
#include "oops.hpp"
#include "pci.hpp"
#include "util.hpp"
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


namespace leanux {

  namespace pci {

    std::string PCIDeviceDatabase = "/usr/share/misc/pci.ids";

    const PCIAddress NullPCIAddress = {0xffff, 0xffff, 0xffff, 0xffff};

    void init() throw ( Oops ) {
      if ( util::fileReadAccess( "/usr/share/misc/pci.ids" ) ) {
        PCIDeviceDatabase = "/usr/share/misc/pci.ids";
      } else
      if ( util::fileReadAccess( "/usr/share/hwdata/pci.ids" ) ) {
        PCIDeviceDatabase = "/usr/share/hwdata/pci.ids";
      } else
      if ( util::fileReadAccess( "/usr/share/pci.ids" ) ) {
        PCIDeviceDatabase = "/usr/share/pci.ids";
      } else throw Oops( __FILE__, __LINE__, "leanux cannot find pci.ids file" );
    }

    std::string getEBDF( const PCIAddress &id ) {
      std::stringstream ss;
      ss << std::hex;
      ss << std::setfill('0') << std::setw(4) << id.domain;
      ss << ":";
      ss << std::setfill('0') << std::setw(2) << id.bus;
      ss << ":";
      ss << std::setfill('0') << std::setw(2) << id.device;
      ss << ".";
      ss << std::setfill('0') << std::setw(1) << id.function;
      return ss.str();
    }

    std::string getBDF( const PCIAddress &id ) {
      std::stringstream ss;
      ss << std::hex;
      ss << std::setfill('0') << std::setw(2) << id.bus;
      ss << ":";
      ss << std::setfill('0') << std::setw(2) << id.device;
      ss << ".";
      ss << std::setfill('0') << std::setw(1) << id.function;
      return ss.str();
    }

    PCIAddress decodeEBDF( const std::string &s ) {
      PCIAddress a = NullPCIAddress;
      std::stringstream ss;
      ss.str(s);
      ss >> std::hex;
      ss >> a.domain;
      if ( !ss.good() ) return NullPCIAddress;
      ss.get();
      ss >> a.bus;
      if ( !ss.good() ) return NullPCIAddress;
      ss.get();
      ss >> a.device;
      if ( !ss.good() ) return NullPCIAddress;
      ss.get();
      ss >> a.function;
      if ( !ss.good() ) return NullPCIAddress;
      return a;
    }

    bool isPCIDevice( const PCIDevicePath &path ) {
      std::list<PCIDevicePath> paths;
      pci::enumPCIDevicePaths( paths );
      for ( std::list<std::string>::const_iterator i = paths.begin(); i != paths.end(); i++ ) {
        if ( *i == path ) return true;
      }
      return false;
    }

    PCIHardwareClass getPCIHardwareClass( unsigned int classcode ) {
      unsigned int classcode_ = (classcode & 0xff0000) >> 16;

      switch ( classcode_ ) {
        case 0x01 : return pciMassStorage;
        case 0x02 : return pciNetwork;
        case 0x03 : return pciDisplay;
        case 0x04 : return pciMultimedia;
        case 0x05 : return pciMemory;
        case 0x06 : return pciBridge;
        case 0x07 : return pciSatelliteCommunication;
        case 0x08 : return pciBaseSystemPeripheral;
        case 0x09 : return pciInput;
        case 0x0a : return pciDockingStation;
        case 0x0b : return pciProcessor;
        case 0x0c : return pciSerialBus;
        case 0x0d : return pciWireless;
        case 0x0e : return pciIntelligentIO;
        case 0x0f : return pciSatelliteCommunication;
        case 0x10 : return pciEncryption;
        case 0x11 : return pciDataAquisistion;
        case 0xff : return pciUnclassified;
        default: return pciUnclassified;
      }
    }

    std::string getPCIHardwareClassString( unsigned int classcode ) {
      unsigned int classcode_ = (classcode & 0xff0000) >> 16;
      std::stringstream ss;
      switch ( classcode_ ) {
        case 0x01 : ss << "mass storage controller"; break;
        case 0x02 : ss << "network controller"; break;
        case 0x03 : ss << "display controller"; break;
        case 0x04 : ss << "multimedia controller"; break;
        case 0x05 : ss << "memory controller"; break;
        case 0x06 : ss << "bridge device"; break;
        case 0x07 : ss << "simple communications controller"; break;
        case 0x08 : ss << "base system peripheral"; break;
        case 0x09 : ss << "input device"; break;
        case 0x0a : ss << "docking station"; break;
        case 0x0b : ss << "processor"; break;
        case 0x0c : ss << "serial bus controller"; break;
        case 0x0d : ss << "wireless controller"; break;
        case 0x0e : ss << "intelligent I/O controller"; break;
        case 0x0f : ss << "satellite communication controller"; break;
        case 0x10 : ss << "encryption controller"; break;
        case 0x11 : ss << "data aquisition controller"; break;
        case 0xff : ss << "unclassified"; break;
        default: ss << "reserved"; break;
      }
      return ss.str();
    }

    std::string getPCISubClassString( unsigned int classcode ) {
      unsigned int classcode_ = (classcode & 0xff0000) >> 16;
      unsigned int subclass_ = (classcode & 0x00ff00) >> 8;
      unsigned int function_ = classcode & 0x0000ff;

      std::stringstream ss;
      ss << std::hex;

      switch( classcode_ ) {

        case pciMassStorage :
          switch ( subclass_ ) {
            case 0x00 : ss << "SCSI bus controller"; break;
            case 0x01 : ss << "IDE controller"; break;
            case 0x02 : ss << "floppy disk controller"; break;
            case 0x03 : ss << "IPI bus controller"; break;
            case 0x04 : ss << "RAID controller"; break;
            case 0x05 :
              ss << "ATA controller";
              if ( function_ == 0x20 ) ss << " (single DMA)";
              else if ( function_ == 0x30 ) ss << " (chained DMA)";
              break;
            case 0x06 :
              ss << "serial ATA";
              if ( function_ == 0x00 ) ss << " (vendor specific interface)";
              else if ( function_ == 0x01 ) ss << " (AHCI 1.0)";
              break;
            case 0x07 : ss << "serial attached SCSI"; break;
            case 0x08 : ss << "Non-Volatile memory controller"; break;
            case 0x80 : ss << "other mass storage controller"; break;
            default   : ss << "unknown pciMassStorage subclass (" << subclass_ << ")"; break;
          }
          break;

        case pciNetwork :
          switch ( subclass_ ) {
            case 0x00 : ss << "ethernet controller"; break;
            case 0x01 : ss << "token ring controller"; break;
            case 0x02 : ss << "FDDI controller"; break;
            case 0x03 : ss << "ATM controller"; break;
            case 0x04 : ss << "ISDN controller"; break;
            case 0x05 : ss << "WorldFip controller"; break;
            case 0x06 : ss << "PICMG 2.14 multi computing"; break;
            case 0x80 : ss << "other network controller"; break;
            default   : ss << "unknown pciNetwork subclass (" << subclass_ << ")"; break;
          }
          break;

        case pciDisplay :
          switch ( subclass_ ) {
            case 0x00:
              if ( function_ == 0x00 ) ss << "VGA compatible controller";
              else if ( function_ == 0x01 ) ss << "8512 compatible controller";
              else ss << "unknown function for pciDisplay 0x00 function " << function_;
              break;
            case 0x01 : ss << "XGA controller"; break;
            case 0x02 : ss << "3D controller (not VGA compatible)"; break;
            case 0x80 : ss << "other display controller"; break;
            default   : ss << "unknown pciDisplay subclass (" << subclass_ << ")"; break;
          }
          break;

        case pciMultimedia:
          switch ( subclass_ ) {
            case 0x00 : ss << "video device"; break;
            case 0x01 : ss << "audio device"; break;
            case 0x02 : ss << "computer telephony device"; break;
            case 0x03 : ss << "high definition audio device"; break;
            case 0x80 : ss << "other multimedia device"; break;
            default   : ss << "unknown pciMultimedia subclass (" << subclass_ << ")"; break;
          }
          break;

        case pciMemory:
          switch ( subclass_ ) {
            case 0x00 : ss << "RAM controller"; break;
            case 0x01 : ss << "flash controller"; break;
            case 0x80 : ss << "other memory controller"; break;
            default   : ss << "unknown pciMemory subclass (" << subclass_ << ")"; break;
          }
          break;

        case pciBridge:
          switch ( subclass_ ) {
            case 0x00 : ss << "host bridge"; break;
            case 0x01 : ss << "ISA bridge"; break;
            case 0x02 : ss << "EISA bridge"; break;
            case 0x03 : ss << "MCA bridge"; break;
            case 0x04 :
              if ( function_ == 0x00 )
                ss << "PCI-to-PCI bridge";
              else if ( function_ == 0x01 )
                ss << "PCI-to-PCI bridge (subtractive decode)";
              else
                ss << "unknown function for pciBridge 0x04 function " << function_;
              break;
            case 0x05 : ss << "PCMCIA bridge"; break;
            case 0x06 : ss << "NuBus bridge"; break;
            case 0x07 : ss << "CardBus bridge"; break;
            case 0x08 : ss << "RACEway bridge"; break;
            case 0x09 :
              if ( function_ == 0x40 ) ss << "PCI-to-PCI bridge (semi transparent primary)";
              else if ( function_ == 0x80 ) ss << "PCI-to-PCI bridge (semi transparent secondary)";
              else ss << "unknown function for pciBridge 0x09 function " << function_;
              break;
            case 0x0a : ss << "Infinibrand-to-PCI host bridge"; break;
            case 0x80 : ss << "other bridge device"; break;
            default   : ss << "unknown pciBridge subclass (" << subclass_ << ")"; break;
          }
          break;

        case pciSimpleCommunication:
          switch ( subclass_ ) {
            case 0x00:
              switch ( function_ ) {
                case 0x00: ss << "generic XT compatible serial controller"; break;
                case 0x01: ss << "16450 compatible serial controller"; break;
                case 0x02: ss << "16550 compatible serial controller"; break;
                case 0x03: ss << "16650 compatible serial controller"; break;
                case 0x04: ss << "16750 compatible serial controller"; break;
                case 0x05: ss << "16850 compatible serial controller"; break;
                case 0x06: ss << "16950 compatible serial controller"; break;
                default  : ss << "unknown function for pciSimpleCommunication 0x00 function " << function_;
              }
              break;
            case 0x01:
              switch ( function_ ) {
                case 0x00: ss << "parallel port"; break;
                case 0x01: ss << "bi-directional parallel port"; break;
                case 0x02: ss << "ECP 1.X compliant parallel port"; break;
                case 0x03: ss << "IEEE 1284 controller"; break;
                case 0xfe: ss << "IEEE 1284 target device"; break;
                default  : ss << "unknown function for pciSimpleCommunication 0x01 function " << function_;
              }
              break;
            case 0x02: ss << "multiport serial controller"; break;
            case 0x03:
              switch ( function_ ) {
                case 0x00: ss << "generic modem"; break;
                case 0x01: ss << "hayes compatible modem (16450 compatible interface)"; break;
                case 0x02: ss << "hayes compatible modem (16550 compatible interface)"; break;
                case 0x03: ss << "hayes compatible modem (16650 compatible interface)"; break;
                case 0x04: ss << "hayes compatible modem (16750 compatible interface)"; break;
                default  : ss << "unknown function for pciSimpleCommunication 0x03 function " << function_;
              }
              break;
            case 0x04: ss << "IEEE 488.1/2 (GPIB) controller"; break;
            case 0x05: ss << "smart card"; break;
            case 0x80: ss << "other communications device"; break;
            default  : ss << "unknown pciSimpleCommunication subclass (" << subclass_ << ")"; break;
          }
          break;

        case pciBaseSystemPeripheral:
          switch ( subclass_ ) {
            case 0x00:
              switch ( function_ ) {
                case 0x00 : ss << "generic 8259 PIC"; break;
                case 0x01 : ss << "ISA PIC"; break;
                case 0x02 : ss << "EISA PIC"; break;
                case 0x10 : ss << "I/O APIC interrupt controller"; break;
                case 0x20 : ss << "I/O(x) APIC interrupt controller"; break;
                default   : ss << "unknown function for pciBaseSystemPeripheral 0x00 function " << function_;
              }
              break;
            case 0x01:
              switch ( function_ ) {
                case 0x00 : ss << "generic 8237 DMA controller"; break;
                case 0x01 : ss << "ISA DMA controller"; break;
                case 0x02 : ss << "EISA DMA controller"; break;
                default   : ss << "unknown function for pciBaseSystemPeripheral 0x01 function " << function_;
              }
              break;
            case 0x02:
              switch ( function_ ) {
                case 0x00 : ss << "generic 8254 system timer"; break;
                case 0x01 : ss << "ISA system timer"; break;
                case 0x02 : ss << "EISA system timer"; break;
                default   : ss << "unknown function for pciBaseSystemPeripheral 0x02 function " << function_;
              }
              break;
            case 0x03:
              switch ( function_ ) {
                case 0x00 : ss << "generic RTC controller"; break;
                case 0x01 : ss << "ISA RTC controller"; break;
                default   : ss << "unknown function for pciBaseSystemPeripheral 0x03 function " << function_;
              }
              break;
            case 0x04: ss << "generic PCI hot-plug controller"; break;
            case 0x80: ss << "other system peripheral"; break;
            default  : ss << "unknown pciBaseSystemPeripheral subclass (" << subclass_ << ")"; break;
          }
          break;

        case pciInput:
          switch ( subclass_ ) {
            case 0x00: ss << "keyboard controller"; break;
            case 0x01: ss << "digitizer"; break;
            case 0x02: ss << "mouse controller"; break;
            case 0x03: ss << "scanner controller"; break;
            case 0x04:
              if ( function_ == 0x00 )
                ss << "gameport controller (generic)";
              else if ( function_ == 0x10 )
                ss << "gameport controller (legacy)";
              else ss << "unknown function for pciInput 0x04 function " << function_;
              break;
            case 0x80 : ss << "other input controller"; break;
            default  : ss << "unknown pciInput subclass (" << subclass_ << ")"; break;
          }
          break;

        case pciDockingStation:
          switch ( subclass_ ) {
            case 0x00  : ss << "generic docking station"; break;
            case 0x80  : ss << "other docking station"; break;
            default    : ss << "unknown pciDockingStation subclass (" << subclass_ << ")"; break;
          }
          break;

        case pciProcessor:
          switch( subclass_ ) {
            case 0x00 : ss << "386 processor"; break;
            case 0x01 : ss << "486 processor"; break;
            case 0x02 : ss << "pentium processor"; break;
            case 0x10 : ss << "alpha processor"; break;
            case 0x20 : ss << "PowerPC processor"; break;
            case 0x30 : ss << "MIPS processor"; break;
            case 0x40 : ss << "co-processor"; break;
            default   : ss << "unknown pciProcessor subclass (" << subclass_ << ")"; break;
          }
          break;

        case pciSerialBus:
          switch ( subclass_ ) {
            case 0x00:
              if ( function_ == 0x00 ) ss << "IEEE1394 controller (firewire)";
              else if ( function_ == 0x10 ) ss << "IEEE1394 controller (1394 OpenHCI spec)";
              else ss << "unknown function for pciSerialBus 0x00 function " << function_;
              break;
            case 0x01: ss << "ACCESS bus"; break;
            case 0x02: ss << "SSA"; break;
            case 0x03:
              switch ( function_ ) {
                case 0x00: ss << "USB (universal host controller spec)"; break;
                case 0x10: ss << "USB (open host controller spec)"; break;
                case 0x20: ss << "USB2 host controller (Intel EHCI)"; break;
                case 0x30: ss << "USB3 host controller (Intel xHCI)"; break;
                case 0x80: ss << "USB"; break;
                case 0xfe: ss << "USB (no host controller)"; break;
                default  : ss << "unknown function for pciSerialBus 0x03 function " << function_;
              }
              break;
            case 0x04: ss << "fibre channel"; break;
            case 0x05: ss << "SMBus"; break;
            case 0x06: ss << "Infiniband"; break;
            case 0x07:
              switch ( function_ ) {
                case 0x00 : ss << "IPMI SMIC interface"; break;
                case 0x01 : ss << "IPMI kybd controller style interface"; break;
                case 0x02 : ss << "IPMI block transfer interface"; break;
                default   : ss << "unknown function for pciSerialBus 0x07 function " << function_;
              }
              break;
            case 0x08: ss << "SERCOS interface (IEC 61491)"; break;
            case 0x09: ss << "CANbus"; break;
            default   : ss << "unknown pciSerialBus subclass (" << subclass_ << ")"; break;
          }
          break;

        case pciWireless:
          switch ( subclass_ ) {
            case 0x00: ss << "iRDA compatible controller"; break;
            case 0x01: ss << "consumer IR controller"; break;
            case 0x10: ss << "RF controller"; break;
            case 0x11: ss << "bluetooth controller"; break;
            case 0x12: ss << "broadband controller"; break;
            case 0x20: ss << "ethernet controller (802.11a)"; break;
            case 0x21: ss << "ethernet controller (802.11b)"; break;
            case 0x80: ss << "other wireless controller"; break;
            default   : ss << "unknown pciWireless subclass (" << subclass_ << ")"; break;
          }
          break;

        case pciIntelligentIO:
          if ( subclass_ == 0x00 && function_ == 0x00 )
            ss << "message FIFO";
          else if ( subclass_ == 0x00 && function_ != 0x00 )
            ss << "I20 architecture";
          else
            ss << "unknown pciIntelligentIO subclass (" << subclass_ << ")";
          break;

        case pciSatelliteCommunication:
          switch ( subclass_ ) {
            case 0x01: ss << "TV controller"; break;
            case 0x02: ss << "audio controller"; break;
            case 0x03: ss << "voice controller"; break;
            case 0x04: ss << "data controller"; break;
            default  : ss << "unknown pciSatelliteCommunication subclass(" << subclass_ << ")"; break;
          }
          break;

        case pciEncryption:
          switch ( subclass_ ) {
            case 0x00 : ss << "network and computing encryption/decryption"; break;
            case 0x10 : ss << "entertainment encryption/decryption"; break;
            case 0x80 : ss << "other encryption/decryption"; break;
            default   : ss << "unknown pciEncryption subclass (" << subclass_ << ")"; break;
          }
          break;

        case pciDataAquisistion:
          switch ( subclass_ ) {
            case 0x00: ss << "DPIO module"; break;
            case 0x01: ss << "performance counter"; break;
            case 0x10: ss << "communication synchronization, time and frequency measurement"; break;
            case 0x20: ss << "management card"; break;
            case 0x80: ss << "other data aquisition or signal processing controller"; break;
            default  : ss << "unknown pciDataAquisistion subclass (" << subclass_ << ")"; break;
          }
          break;
        default:
          ss << "invalid PCI class code (" << classcode_ << ")";


      }
      return ss.str();
    }

    bool getPCIHardwareInfo( const PCIHardwareId &id, PCIHardwareInfo &inf ) {
      inf.vendor = "";
      inf.device = "";
      inf.subdevice = "";
      inf.pciclass = getPCIHardwareClassString( id.pciclass );
      inf.pcisubclass = getPCISubClassString( id.pciclass );
      std::stringstream ss;
      ss << std::hex;
      ss << std::setfill('0') << std::setw(4) << id.vendor;
      std::string hex_vendor = ss.str();
      ss.str("");
      ss << std::setfill('0') << std::setw(4) << id.device;
      std::string hex_device = ss.str();
      ss.str("");
      ss << std::setfill('0') << std::setw(4) << id.subvendor << ' ' << id.subdevice;
      std::string hex_subdevice = ss.str();

      std::ifstream f( PCIDeviceDatabase.c_str() );
      if ( !f.good() ) throw Oops( __FILE__, __LINE__, "error opening pci.ids" );

      /**
       * 0 = initial      (aka EVENDOR_NOT_FOUND)
       * 1 = vendor found (aka EDEVICE_NOT_FOUND)
       * 2 = device found
       * 3 = subdevice found
       */
      int state = 0;

      bool giveup = false;
      while ( f.good() && !f.eof() && !giveup && state != 3 ) {
        std::string s = "";
        getline( f, s );

        if ( s[0] != '#' ) {

          switch ( state ) {
            case 0:
              if ( strncmp( s.c_str(), hex_vendor.c_str(), 4 ) == 0 ) {
                inf.vendor = s.substr( 6 );
                state = 1;
              }
              break;
            case 1:
              if ( s[0] != '\t' ) {
                //we hit another vendor line, so device not found
                giveup = true;
              } else {
                if ( strncmp( s.c_str()+1, hex_device.c_str(), 4 ) == 0 ) {
                  inf.device = s.substr(7);
                  state = 2;
                }
              }
              break;
            case 2:
              if ( s[0] != '\t' || s[1] != '\t' ) {
                //possible subvendors exhausted, giveup
                giveup = true;
              } else {
                if ( strncmp( s.c_str()+2, hex_subdevice.c_str(), 9 ) == 0 ) {
                  inf.subdevice = s.substr(13);
                  state = 3;
                }
              }
              break;

          }
        }
      }
      return ( state == 2 || state == 3 );
    }

    /**
     * get PCI device paths relative to /sys/devices/
     */
    void enumPCIDevicePaths( std::list<PCIDevicePath> &paths ) {
      paths.clear();
      DIR *d;
      struct dirent *dir;
      d = opendir( (sysdevice::sysbus_root + "/pci/devices").c_str() );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( dir->d_name[0] != '.' ) {
            std::string path = sysdevice::sysbus_root + "/pci/devices/" + (std::string)dir->d_name;
            std::string resolved_path = util::realPath( path );
            if ( resolved_path.length() > 13 ) paths.push_back( resolved_path.substr(13) );
          }
        }
        closedir( d );
      }
    }

    PCIDevicePath findPCIDeviceByIRQ( unsigned int irq ) {
      std::list<std::string> paths;
      pci::enumPCIDevicePaths( paths );
      for ( std::list<std::string>::const_iterator i = paths.begin(); i != paths.end(); i++ ) {
        std::string irqpath = sysdevice::sysdevice_root + "/" + *i + "/irq";
        if ( util::fileReadAccess( irqpath ) ) {
          unsigned int firq = util::fileReadInt( irqpath );
          if ( firq == irq ) return *i;
        }
      }
      return "";
    }

    PCIDevicePath findPCIDeviceByATAPort( const std::string& port ) {
      std::list<PCIDevicePath> paths;
      pci::enumPCIDevicePaths( paths );
      for ( std::list<std::string>::const_iterator i = paths.begin(); i != paths.end(); i++ ) {
        PCIHardwareId devid = getPCIHardwareId( *i );
        if ( getPCIHardwareClass( devid.pciclass ) == pciMassStorage ) {
          std::list<std::string> these_ports;
          enumPCIATAPorts( *i, these_ports );
          for ( std::list<std::string>::const_iterator j = these_ports.begin(); j != these_ports.end(); j++ ) {
            if ( (*j) == (*i + "/" + port) ) return *i;
          }
        }
      }
      return "";
    }

    unsigned int getPCIDeviceIRQ( const PCIDevicePath& device ) {
      std::string irqpath = sysdevice::sysdevice_root + "/" + device + "/irq";
      if ( util::fileReadAccess( irqpath ) ) {
        return util::fileReadInt( irqpath );
      } else return 0;
    }

    void enumPCIATAPorts( const PCIDevicePath& device, std::list<std::string> &ports ) {
      PCIHardwareId devid = getPCIHardwareId( device );
      if ( getPCIHardwareClass( devid.pciclass ) != pciMassStorage ) throw Oops( __FILE__, __LINE__, "can only request ATA devices on mass storage controllers" );
      ports.clear();
      DIR *d;
      struct dirent *dir;
      d = opendir( (sysdevice::sysdevice_root + "/" + device).c_str() );
      std::list<std::string> pcidirlist;
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( strncmp( dir->d_name, "ata", 3 ) == 0 ) {
            ports.push_back( device + "/" + dir->d_name );
          }
        }
        closedir( d );
      }
    }

    std::string getATADeviceName( const PCIDevicePath& device ) {
      std::string result = "";
      std::string path = sysdevice::sysdevice_root + "/" + device;
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

    std::string getATAPortName( const PCIDevicePath& device ) {
      std::string result = "";
      size_t p = device.find_last_of( "/" );
      if ( p != std::string::npos ) {
        result = device.substr( p );
      }
      return result;
    }

    std::string getEtherNetworkInterfaceName( const PCIDevicePath& device ) {
      std::string result = "";
      std::string path = sysdevice::sysdevice_root + "/" + device + "/net";
      DIR *d;
      struct dirent *dir;
      d = opendir( path.c_str() );
      if ( d ) {
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

    PCIHardwareId getPCIHardwareId( const PCIDevicePath& devicepath ) {
      PCIHardwareId result;
      try {
        result.pciclass = util::fileReadHexString( sysdevice::sysdevice_root + "/" + devicepath + "/class" );
        result.vendor = util::fileReadHexString( sysdevice::sysdevice_root + "/" + devicepath + "/vendor" );
        result.device = util::fileReadHexString( sysdevice::sysdevice_root + "/" + devicepath + "/device" );
        result.subvendor = util::fileReadHexString( sysdevice::sysdevice_root + "/" + devicepath + "/subsystem_vendor" );
        result.subdevice = util::fileReadHexString( sysdevice::sysdevice_root + "/" + devicepath + "/subsystem_device" );
      }
      catch ( Oops & ) {}
      return result;
    }

  }

}
