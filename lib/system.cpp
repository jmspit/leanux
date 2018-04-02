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
 * leanux::system c++ header file.
 */
#include "system.hpp"
#include "oops.hpp"
#include "util.hpp"
#include "block.hpp"
#include "pci.hpp"
#include "usb.hpp"
#include "net.hpp"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>

#include <arpa/inet.h>
#include <errno.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>

#include <utmp.h>
#include <set>

namespace leanux {

  /**
   * The minimum kernel version required by leanux.
   */
  const std::string kernel_required = "2.6.24";

  void init() {
    std::string version = system::getKernelVersion();
    if ( version < kernel_required )
      throw Oops( __FILE__, __LINE__, "leanux requires at least " + kernel_required + ", this kernel is " + version );
    if ( !util::directoryExists( "/proc" ) ) throw Oops( __FILE__, __LINE__, "leanux requires /proc procfs" );
    if ( !util::directoryExists( "/sys" ) ) throw Oops( __FILE__, __LINE__, "leanux requires /sys sysfs" );
    pci::init();
    block::init();
    net::init();
    usb::init();
  }

  namespace system {

    std::string getKernelVersion() {
      struct utsname buf;
      int r = uname( &buf );
      if ( r ) throw Oops( __FILE__, __LINE__, errno );
      std::stringstream result;
      int i = 0;
      while ( isdigit(buf.release[i]) || buf.release[i] == '.' ) result << buf.release[i++];
      return result.str();
    }

    std::string getNodeName() {
      struct utsname buf;
      int r = uname( &buf );
      if ( r ) throw Oops( __FILE__, __LINE__, errno );
      return buf.nodename;
    }

    std::string getArchitecture() {
      struct utsname buf;
      int r = uname( &buf );
      if ( r ) throw Oops( __FILE__, __LINE__, errno );
      return buf.machine;
    }

    long getPageSize() {
      long r = sysconf(_SC_PAGESIZE);
      if ( r == -1 ) throw Oops( __FILE__, __LINE__, errno );
      return r;
    }

    bool isBigEndian() {
      return htonl(long(1968))==long(1968);
    }

    std::string  getBoardName() {
      try {
        return util::fileReadString( "/sys/class/dmi/id/board_name" );
      }
      catch ( const Oops &oops ) {
        return "";
      }
    }

    std::string  getBoardVendor() {
      try {
        return util::fileReadString( "/sys/class/dmi/id/board_vendor" );
      }
      catch ( const Oops &oops ) {
        return "";
      }
    }

    std::string  getBoardVersion() {
      try {
        return util::fileReadString( "/sys/class/dmi/id/board_version" );
      }
      catch ( const Oops &oops ) {
        return "";
      }
    }

    ChassisType getChassisType() {
      if ( util::fileReadAccess( "/sys/class/dmi/id/chassis_type" ) )
        return (ChassisType)util::fileReadInt( "/sys/class/dmi/id/chassis_type" );
      else
        return ChassisTypeUnknown;
    }

    std::string getChassisTypeString() {
      ChassisType t = getChassisType();
      std::stringstream ss;
      switch ( t ) {
        case ChassisTypeOther : ss << "virtual machine"; break;
        case ChassisTypeDesktop : ss << "desktop"; break;
        case ChassisTypeUnknown : ss << "unknown"; break;
        case ChassisTypePortable : ss << "portable"; break;
        case ChassisTypeLaptop : ss << "laptop"; break;
        case ChassisTypeNotebook : ss << "notebook"; break;
        case ChassisTypeHandHeld : ss << "hand held"; break;
        case ChassisTypeExpansionChassis : ss << "expansion chassis"; break;
        case ChassisTypeSealedCasePC : ss << "sealed case PC"; break;
        default : ss << "unhandled! (0x" << std::hex << t << ")"; break;
      }
      return ss.str();
    }

    double getUptime() {
      double d;
      std::ifstream i( "/proc/uptime" );
      i >> d;
      return d;
    }

    time_t getBootTime() {
      std::ifstream ifs( "/proc/stat" );
      double t = 0;
      while ( ifs.good() && !ifs.eof() ) {
        std::string  s;
        getline( ifs, s );
        if ( sscanf( s.c_str(), "btime %lf", &t ) == 1 ) return (time_t)t; else t= 0;
      }
      throw Oops( __FILE__, __LINE__, "no btime in /proc/stat" );
      return (time_t)t;
    }

    std::string getUserName( uid_t uid ) {
      struct passwd *pw = getpwuid(uid);
      if (pw) {
        return pw->pw_name;
      } else return "unknown";
    }

    long getUserHz() {
      return sysconf( _SC_CLK_TCK );
    }

    void getOpenFiles( unsigned long *used, unsigned long *max ) {
      std::ifstream ifs( "/proc/sys/fs/file-nr" );
      unsigned long dummy;
      if ( ifs.good() && !ifs.eof() ) {
        std::string s = "";
        getline( ifs, s );
        if ( sscanf( s.c_str(), "%lu %lu %lu", used, &dummy, max ) != 3 ) {
          throw Oops( __FILE__, __LINE__, "failed to parse /proc/sys/fs/file-nr" );
        }
      }
    }

    void getOpenInodes( unsigned long *used, unsigned long *free ) {
      std::ifstream ifs( "/proc/sys/fs/inode-nr" );
      if ( ifs.good() && !ifs.eof() ) {
        std::string s = "";
        getline( ifs, s );
        if ( sscanf( s.c_str(), "%lu %lu", used, free ) != 2 ) {
          throw Oops( __FILE__, __LINE__, "failed to parse /proc/sys/fs/inode-nr" );
        }
      }
    }

    void getNumProcesses( unsigned long *processes ) {
      std::ifstream ifs( "/proc/loadavg" );
      if ( ifs.good() && !ifs.eof() ) {
        std::string s = "";
        getline( ifs, s );
        int r = sscanf( s.c_str(), "%*f %*f %*f %*u/%lu", processes );
        if ( r != 1 ) {
          throw Oops( __FILE__, __LINE__, "failed to parse /proc/loadavg" );
        }
      }
    }

    unsigned int getNumLogins() {
      setutent();
      struct utmp* res;
      std::set<std::string> cnt;
      while ( (res = getutent()) ) {
        if ( res->ut_type == USER_PROCESS ) {
          cnt.insert( res->ut_line );
        }
      }
      endutent();
      return cnt.size();
    }

    unsigned int getNumLoginUsers() {
      setutent();
      struct utmp* res;
      std::set<std::string> cnt;
      while ( (res = getutent()) ) {
        if ( res->ut_type == USER_PROCESS ) {
          cnt.insert( res->ut_user );
        }
      }
      endutent();
      return cnt.size();
    }

    /**
     * @private
     * Read the ID from /etc/os-release.
     * @return the ID or an empty std::string if not found.
     */
    std::string readOSReleaseDistribId() {
      std::string result = "";
      std::ifstream ifs("/etc/os-release");
      while ( ifs.good() ) {
        std::string s = "";
        getline( ifs, s );
        if ( strncmp( s.c_str(), "ID=", 3 ) == 0 ) {
          for ( size_t i = 3; i < s.length(); i++ ) {
            if ( s[i] != '"' ) {
              result += s[i];
            }
          }
          break;
        }
      }
      return result;
    };

    /**
     * @private
     * Read the PRETTY_NAME from /etc/os-release.
     * @return the PRETTY_NAME or an empty std::string if not found.
     */
    std::string readOSReleasePrettyName() {
      std::string result = "";
      std::ifstream ifs("/etc/os-release");
      while ( ifs.good() ) {
        std::string s = "";
        getline( ifs, s );
        if ( strncmp( s.c_str(), "PRETTY_NAME=", 12 ) == 0 ) {
          for ( size_t i = 12; i < s.length(); i++ ) {
            if ( s[i] != '"' ) {
              result += s[i];
            }
          }
          break;
        }
      }
      return result;
    };

    /**
     * @private
     * Read the NAME from /etc/os-release.
     * @return the NAME or an empty std::string if not found.
     */
    std::string readOSReleaseName() {
      std::string result = "";
      std::ifstream ifs("/etc/os-release");
      while ( ifs.good() ) {
        std::string s = "";
        getline( ifs, s );
        if ( strncmp( s.c_str(), "NAME=", 5 ) == 0 ) {
          for ( size_t i = 5; i < s.length(); i++ ) {
            if ( s[i] != '"' ) {
              result += s[i];
            }
          }
          break;
        }
      }
      return result;
    };

    /**
     * @private
     * Read the VERSION from /etc/os-release.
     * @return the VERSION or an empty std::string if not found.
     */
    std::string readOSReleaseVersion() {
      std::string result = "";
      std::ifstream ifs("/etc/os-release");
      while ( ifs.good() ) {
        std::string s = "";
        getline( ifs, s );
        if ( strncmp( s.c_str(), "VERSION=", 8 ) == 0 ) {
          for ( size_t i = 8; i < s.length(); i++ ) {
            if ( s[i] != '"' ) {
              result += s[i];
            }
          }
          break;
        }
      }
      return result;
    };

    /**
     * @private
     * Read the DISTRIB_ID from /etc/lsb-release.
     * @return the DISTRIB_ID or an empty std::string if not found.
     */
    std::string readLSBReleaseDistribId() {
      std::string result = "";
      std::ifstream ifs("/etc/lsb-release");
      if ( ifs.good() ) {
        std::string s = "";
        getline( ifs, s );
        if ( strncmp( s.c_str(), "DISTRIB_ID=", 11 ) == 0 ) {
          for ( size_t i = 11; i < s.length(); i++ ) {
            if ( s[i] != '"' ) {
              result += s[i];
            }
          }
        }
      }
      return result;
    };

    /**
     * @private
     * Read the DISTRIB_DESCRIPTION from /etc/lsb-release.
     * @return the DISTRIB_DESCRIPTION or an empty std::string if not found.
     */
    std::string readLSBReleaseDistribDescription() {
      std::string result = "";
      std::ifstream ifs("/etc/lsb-release");
      if ( ifs.good() ) {
        std::string s = "";
        getline( ifs, s );
        if ( strncmp( s.c_str(), "DISTRIB_DESCRIPTION=", 20 ) == 0 ) {
          for ( size_t i = 20; i < s.length(); i++ ) {
            if ( s[i] != '"' ) {
              result += s[i];
            }
          }
        }
      }
      return result;
    };

    Distribution getDistribution() {
      Distribution dist;
      dist.type = Unkown;
      dist.release = "";
      // test for os-release
      if ( util::fileReadAccess( "/etc/os-release" ) ) {
        std::string distrib_id = readOSReleaseDistribId();
        dist.release = readOSReleaseName();
        if ( readOSReleaseVersion().length() > 0 ) dist.release = dist.release + " " + readOSReleaseVersion();
        if ( distrib_id == "gentoo" ) {
          dist.type = Gentoo;
        } else if ( distrib_id == "opensuse" ) {
          dist.type = OpenSuSE;
        } else if ( distrib_id == "centos" ) {
          dist.type = CentOS;
        } else if ( distrib_id == "ubuntu" ) {
          dist.type = Ubuntu;
        } else if ( distrib_id == "debian" ) {
          dist.type = Debian;
        }
      } else
      // test for lsb-release
      if ( util::fileReadAccess( "/etc/lsb-release" ) ) {
        std::string distrib_id = readLSBReleaseDistribId();
        if ( distrib_id == "Gentoo" ) {
          dist.type = Gentoo;
          dist.release = util::fileReadString( "/etc/gentoo-release" );
        } else if ( distrib_id == "Ubuntu" ) {
          dist.type = Ubuntu;
          dist.release = readLSBReleaseDistribDescription();
        } else if ( distrib_id == "centos" ) {
          dist.type = Ubuntu;
          dist.release = readLSBReleaseDistribDescription();
        }
      } else {
        // no lsb-release, alternative detection
        if ( util::fileReadAccess( "/etc/gentoo-release" ) ) {
          dist.type = Gentoo;
          dist.release = util::fileReadString( "/etc/gentoo-release" );
        } else if ( util::fileReadAccess( "/etc/debian-version" ) ) {
          dist.type = Debian;
          dist.release = util::fileReadString( "/etc/debian-version" );
        } else if ( util::fileReadAccess( "/etc/redhat-release" ) ) {
          dist.type = RedHat;
          dist.release = util::fileReadString( "/etc/redhat-release" );
        } else if ( util::fileReadAccess( "/etc/SuSE-release" ) ) {
          dist.type = OpenSuSE;
          dist.release = util::fileReadString( "/etc/SuSE-release" );
        }
      }
      return dist;
    }

  }

}
