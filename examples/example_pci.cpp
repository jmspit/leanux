//========================================================================
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
 * \example example_pci.cpp
 * leanux::pci example!!!.
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
#include "util.hpp"

using namespace std;
using namespace leanux;

void coutValuePair( const string &title, const string& value ) {
  cout << left<< setw(30) << title << right << ": " << value << endl;
}

void coutValuePair( const string &title, long value ) {
  cout << left<< setw(30) << title << right << ": " << value << endl;
}

int main( int argc, char* argv[] ) {
  try {
    init();

    list<string> paths;
    pci::enumPCIDevicePaths( paths );
    for ( list<string>::const_iterator i = paths.begin(); i != paths.end(); i++ ) {
      pci::PCIHardwareId devid = pci::getPCIHardwareId( *i );
      pci::PCIHardwareInfo inf;
      pci::getPCIHardwareInfo( devid, inf );
      cout << *i << endl;
      cout << "class     : " << inf.pciclass << endl;
      cout << "subclass  : " << inf.pcisubclass << endl;
      cout << "vendor    : " << inf.vendor << endl;
      cout << "device    : " << inf.device << endl;
      if ( inf.subdevice.length() > 0 ) cout << "subdevice : " << inf.subdevice << endl;
      cout << "irq       : " << pci::getPCIDeviceIRQ( *i ) << endl;
      if ( pci::getPCIHardwareClass( devid.pciclass ) == pci::pciMassStorage ) {
        list<string> ata_ports;
        pci::enumPCIATAPorts( *i, ata_ports );
        for ( list<string>::const_iterator i = ata_ports.begin(); i != ata_ports.end(); i++ ) {
          cout << "ata port  : " << *i << endl;
          if (  pci::getATADeviceName( *i ) != "" ) {
            block::MajorMinor mm = block::MajorMinor::getMajorMinorByName( pci::getATADeviceName( *i ) );
            cout << "          : /dev/" << pci::getATADeviceName( *i ) << endl;
            cout << "          : type=" << mm.getClassStr() << endl;
            cout << "          : model=" << mm.getModel() << " rev=" << mm.getRevision() << endl;
            cout << "          : major:minor=" << block::MajorMinor::getMajorMinorByName( pci::getATADeviceName( *i ) ) << endl;
            cout << "          : udev path=" << mm.getUDevPath() << endl;
            if ( mm.getRotational() ) cout << "          : rotational" << endl; else cout << "          : " << "SSD" << endl;
            cout << "          : size=" << mm.getSize() << " (" << util::ByteStr( mm.getSize(), 3 ) << ")" << endl;
            cout << "          : serial=" << mm.getSerial() << endl;
            cout << "          : RPM=" << mm.getRPM() << endl;
            cout << "          : WWN=" << mm.getWWN() << endl;
            block::DeviceStats stats;
            mm.getStats( stats );
            cout << setprecision(2) << fixed;
            cout << "          : read  await=" << (double)stats.read_ms / stats.reads << "ms" << endl;
            cout << "          : write await=" << (double)stats.write_ms / (stats.writes) << "ms" << endl;
            cout << "          : svctm      =" << (double)stats.io_ms / (stats.writes+stats.reads) << "ms" << endl;
          } else
            cout << " unused" << endl;
        }
      } else if ( pci::getPCIHardwareClass( devid.pciclass ) == pci::pciNetwork ) {
          cout << "          : interface=" << pci::getEtherNetworkInterfaceName( *i ) << " ";
          cout << net::getDeviceMACAddress( pci::getEtherNetworkInterfaceName( *i ) ) << " ";
          cout << net::getDeviceOperState( pci::getEtherNetworkInterfaceName( *i ) ) << " ";
          if ( net::getDeviceOperState( pci::getEtherNetworkInterfaceName( *i ) ) == "up" ) {
            cout << net::getDeviceSpeed( pci::getEtherNetworkInterfaceName( *i ) ) << "Mbps ";
            list< string > addrlist;
            net::getDeviceIP4Addresses( pci::getEtherNetworkInterfaceName( *i ), addrlist );
            for ( list< string >::const_iterator j = addrlist.begin(); j != addrlist.end(); j++ ) {
              cout << *j << " ";
            }
            net::getDeviceIP6Addresses( pci::getEtherNetworkInterfaceName( *i ), addrlist );
            for ( list< string >::const_iterator j = addrlist.begin(); j != addrlist.end(); j++ ) {
              cout << *j << " ";
            }
          }
          cout << endl;
      }
      cout << endl;
    }


  }
  catch ( const Oops &oops ) {
    cerr << argv[0] << " abnormal exit because:" << endl;
    cerr << oops << endl;
    return 1;
  }
  return 0;
}
