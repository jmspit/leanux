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

#include "system.hpp"
#include "block.hpp"
#include "oops.hpp"
#include "util.hpp"
#include "device.hpp"
#include "tabular.hpp"
#include <iostream>

using namespace std;
using namespace leanux;
using namespace leanux::util;

void dumpDeviceDescr( const string &prefix, const block::MajorMinor &mm ) {
  string devname = block::MajorMinor::getNameByMajorMinor( mm );
  cout << prefix << devname << " " << mm << " - ";
  cout << ByteStr( block::getSize(mm), 3 ) << " ";
  if ( block::getClass( mm ) == block::DeviceMapper ) {
    string vgname = "";
    string lvname = "";
    if ( block::getLVMInfo( mm, vgname, lvname ) ) {
      cout << "LVM " << vgname << ":" << lvname << " as " << block::getFSType(mm);
    }
  } else if ( block::getClass( mm ) == block::MetaDisk ) {
    cout << block::getMDName(mm) << " " << block::getMDDevices(mm) << "x" << block::getMDLevel(mm) << " as " << block::getFSType(mm);
  } else {
    cout << block::getClassStr(mm) << " as " << block::getFSType(mm);
  }
  if ( mm.isWholeDisk() ) {
    /**
    if ( block::getRotational(mm) ) {
      cout << " " << block::getRPM(mm) << "RPM";
    } else {
      cout << " SSD";
    }
    cout << " " << block::getModel(mm) << " rev" << block::getRevision(mm) << " wwn=" << block::getWWN(mm);
    */
  }
  if ( block::getFSUsage(mm) == "filesystem" ) {
    block::MountInfo info;
    if ( block::getMountInfo( mm, info ) ) {
      cout << " mounted on " << info.mountpoint;
    }
  }
  cout << endl;
}

void listHolders( const block::MajorMinor &mm, string &prefix ) {
  //cout << prefix << mm << " ";
  list<string> holders;
  string devname = block::MajorMinor::getNameByMajorMinor( mm );
  dumpDeviceDescr( prefix, mm );
  block::getHolders( mm, holders );
  if ( holders.size() > 0 )  {
    prefix += "  ";
  }
  for ( list<string>::const_iterator d = holders.begin(); d != holders.end(); d++ ) {
    listHolders( block::MajorMinor::getMajorMinorByName( (*d) ), prefix );
  }
}

int main( int argc, char* argv[] ) {
  try {
    init();

    list<block::MajorMinor> wholedisks;
    block::enumWholeDisks( wholedisks );
    for ( list<block::MajorMinor>::const_iterator wd = wholedisks.begin(); wd != wholedisks.end(); wd++ ) {
      list<string> partitions;
      block::getPartitions( *wd, partitions );
      if ( partitions.size() > 0 ) {
        dumpDeviceDescr( "", *wd );
        for ( list<string>::const_iterator p = partitions.begin(); p != partitions.end(); p++ ) {
          string prefix = "  ";
          listHolders( block::MajorMinor::getMajorMinorByName(*p) , prefix );
        }
      } else {
        string prefix = "";
        listHolders( *wd, prefix );
      }
      cout << endl;
    }
    std::cout << "total used bytes across filesystems: " << block::getMountUsedBytes() << std::endl;
  }
  catch ( leanux::Oops &oops ) {
    cerr << oops << endl;
    return 1;
  }
  return 0;
}

