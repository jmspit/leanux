//========================================================================
//
// This file is part of the leanux toolkit.
//
// Copyright (C) 2015-2016 Jan-Marten Spit https://github.com/jmspit/leanux
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
#ifndef LEANUX_SYSTEM_HPP
#define LEANUX_SYSTEM_HPP

#include <string>

#include <unistd.h>
#include <sys/types.h>

#include "oops.hpp"


/**
 * \example example_system.cpp
 * leanux::system example.
 */

/**
 * Linux performance and configuration API
 */
namespace leanux {

  /**
   * Initialize leanux, run checks to verify leanux is compatible with the runtime environment.
   * This must be the first call to make against leanux.
   * @throw Oops if something is wrong.
   */
  void init();

  /**
   * System / kernel detail API
   */
  namespace system {

    /**
     * The node name of the system.
     * Not to be confused with a network hostname.
     * @throw Oops if call to uname fails.
     * @return the system node name.
     */
    std::string getNodeName();

    /**
     * Get the kernel version.
     * @throw Oops if call to uname fails.
     * @return the kernel version.
     */
    std::string getKernelVersion();

    /**
     * Get the system architecture.
     * @throw Oops if call to uname fails.
     * @return the system architecture as in uname -m, for example x86_64.
     */
    std::string getArchitecture();

    /**
     * Get the system page size.
     * @throw Oops if call to sysconf fails.
     * @return the system page size.
     */
    long getPageSize();

    /**
     * Return true when the system is big endian.
     */
    bool isBigEndian();

    /**
     * Get the name of the system board.
     */
    std::string getBoardName();

    /**
     * Get the vendor of the system board.
     */
    std::string getBoardVendor();

    /**
     * Get the version of the system board.
     */
    std::string getBoardVersion();

    /**
     * Enumerate chassis types.
     */
    enum ChassisType {
      /** typically used for virtual machines. */
      ChassisTypeOther=0x1,
      ChassisTypeUnknown=0x2,
      /** Typical desktop PC */
      ChassisTypeDesktop=0x3,
      ChassisTypeLowProfileDesktop=0x4,
      ChassisTypePizzaBox=0x5,
      ChassisTypeMiniTower=0x6,
      ChassisTypeTower=0x7,
      ChassisTypePortable=0x8,
      ChassisTypeLaptop=0x9,
      ChassisTypeNotebook=0xa,
      ChassisTypeHandHeld=0xb,
      ChassisTypeDockingStation=0xc,
      ChassisTypeAllInOne=0xd,
      ChassisTypeSubNotebook=0xe,
      ChassisTypeSpaceSaving=0xf,
      ChassisTypeLunchBox=0x10,
      ChassisTypeMainServerChassis=0x11,
      ChassisTypeExpansionChassis=0x12,
      ChassisTypeSubChassis=0x13,
      ChassisTypeBusExpansionChassis=0x14,
      ChassisTypePeripheralChassis=0x15,
      ChassisTypeRAIDChassis=0x16,
      ChassisTypeRackMountChassis=0x17,
      ChassisTypeSealedCasePC=0x18,
      ChassisTypeMultiSystemChassis=0x19,
      ChassisTypeCompactPCI=0x1a,
      ChassisTypeAdvancedTCA=0x1b,
      ChassisTypeBlade=0x1c,
      ChassisTypeBladeEnclosure=0x1d
    };

    /**
     * Get the system chassis type.
     */
    ChassisType getChassisType();

    /**
     * Get the system chassis type as a std::string.
     */
    std::string getChassisTypeString();

    /**
    * Get the system uptime in seconds (with centisecond precision).
    * Taken from /proc/uptime.
    * @return the system uptime in seconds.
    */
    double getUptime();

    /**
     * Time in seconds since  1970-01-01 00:00:00 +0000 (UTC)
     */
    time_t getBootTime();

    /**
     * Get username from a uid.
     * @param uid the uid for which to find the username.
     * @return the username for the uid, or an ampty std::string if not found.
     */
    std::string getUserName( uid_t uid );

    /**
     * Get the number of clock ticks per second.
     * Used as unit of measure in /proc/stat (and others).
     * @return the number of clock ticks per second.
     */
    long getUserHz();

    /**
     * get the current and maximum number of open files.
     * @param used receives the number of file handles currently allocated.
     * @param max receives the maximum number of file handles that can be allocated.
     */
    void getOpenFiles( unsigned long *used, unsigned long *max );

    /**
     * get the used and free number of open inodes.
     */
    void getOpenInodes( unsigned long *used, unsigned long *free );

    /**
     * Get the number of processes on the system
     */
    void getNumProcesses( unsigned long *processes );

    /**
     * The number of user logins as reported by who.
     * @return the number of user logins.
     */
    unsigned int getNumLogins();

    /**
     * The number of distinct users logged in.
     * @return the number of distinct users logged in.
     */
    unsigned int getNumLoginUsers();

    /**
     * GNU/Linux distribution enumeration.
     */
    enum DistributionType {
      /** distribution not recognized. */
      Unkown,
      /** Gentoo */
      Gentoo,
      /** Debian */
      Debian,
      /** RedHat */
      RedHat,
      /** SUSE */
      OpenSuSE,
      /** Ubuntu */
      Ubuntu,
      /** Fedora */
      Fedora,
      /** Mint */
      Mint,
      /** Arch */
      Arch,
      /** CentOS */
      CentOS
    };

    /**
     * Distribution identifier.
     */
    struct Distribution {
      /** The distribution type. */
      DistributionType type;
      /** The distribution release. */
      std::string release;
    };

    /**
     * Detect the GNU/Linux distribution.
     * @return the detected GNU/Linux distribution type, which may be Unkown.
     */
    Distribution getDistribution();

  }

}


#endif
