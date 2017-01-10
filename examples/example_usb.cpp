//========================================================================
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
#include "usb.hpp"
#include "device.hpp"
#include "pci.hpp"
#include "oops.hpp"
#include "util.hpp"
#include "system.hpp"

#include <iomanip>
#include <iostream>

using namespace std;

int main() {
  try {
    leanux::init();
    list<leanux::usb::USBDevicePath> paths;
    leanux::usb::enumUSBDevices( paths );
    for ( list<leanux::usb::USBDevicePath>::const_iterator u = paths.begin(); u != paths.end(); u++ ) {
      cout << *u << endl;
      leanux::usb::USBHardwareId hardware;
      if ( getUSBHardwareId( *u, hardware ) ) {
        leanux::usb::USBHardwareInfo hwinfo;
        cout << hex << setw(4) << setfill('0') << hardware.idVendor << ":" << hex << setw(4) << setfill('0') << hardware.idProduct << " = ";
        if ( leanux::usb::getUSBHardwareInfo( hardware, hwinfo ) ) {
          cout << hwinfo.idVendor << " : " << hwinfo.idProduct << endl;
        }
        cout << "parent: " << leanux::usb::getParent( *u ) << endl;
      }
      cout << endl;
    }

  }
  catch ( leanux::Oops &oops ) {
    cout << oops << endl;
    return 1;
  }
  return 0;
}
