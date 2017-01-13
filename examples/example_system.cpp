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

//========================================================================
//  Author: Jan-Marten Spit
//========================================================================
#include "system.hpp"
#include "oops.hpp"
#include "util.hpp"

#include <iomanip>
#include <iostream>

using namespace std;

int main() {
  try {
    leanux::init();
    leanux::system::Distribution dist = leanux::system::getDistribution();
    int w = 21;
    cout << setw(w) << left << "distribution" << ":" << dist.release << endl;
    cout << setw(w) << left << "dist_id" << ":" << dist.type << endl;
    cout << setw(w) << left << "kernel" << ":" << leanux::system::getKernelVersion() << endl;
    cout << setw(w) << left << "architecture" << ":" << leanux::system::getArchitecture() << endl;
    cout << setw(w) << left << "node name" << ":" << leanux::system::getNodeName() << endl;
    cout << setw(w) << left << "page size" << ":" << leanux::system::getPageSize() << endl;
    cout << setw(w) << left << "big endian" << ":" << leanux::system::isBigEndian() << endl;
    cout << setw(w) << left << "system board" << ":" << leanux::system::getBoardName() << endl;
    cout << setw(w) << left << "system board vendor" << ":" << leanux::system::getBoardVendor() << endl;
    cout << setw(w) << left << "system board version" << ":" << leanux::system::getBoardVersion() << endl;
    cout << setw(w) << left << "chassis type" << ":" << leanux::system::getChassisTypeString() << endl;
    cout << setw(w) << left << "uptime" << ":" << fixed << setprecision(2) << leanux::system::getUptime() << " seconds" << endl;
    time_t btime = leanux::system::getBootTime();
    cout << setw(w) << left << "btime" << ":" << ctime(&btime);
    cout << setw(w) << left << "user Hz" << ":" << leanux::system::getUserHz() << endl;

    cout << "number of logins: " << leanux::system::getNumLogins() << endl;

    unsigned long open_files, max_open_files;
    leanux::system::getOpenFiles( &open_files, &max_open_files );

    unsigned long processes;
    leanux::system::getNumProcesses( &processes );
  }
  catch ( leanux::Oops &oops ) {
    cout << oops << endl;
    return 1;
  }
  return 0;
}
