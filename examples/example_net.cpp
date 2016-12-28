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

#include "net.hpp"
#include "oops.hpp"
#include "system.hpp"
#include "util.hpp"

#include <iomanip>
#include <iostream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>


using namespace std;

int main( int argc, char *argv[] ) {
  try {
    leanux::init();

    list<string> devices;
    leanux::net::enumDevices( devices );
    for ( list<string>::const_iterator i = devices.begin(); i != devices.end(); i++ ) {
      cout << "network device: " << *i << endl;
      cout << "  status  : " << leanux::net::getDeviceOperState( *i) << endl;
      cout << "  mac addr: " << leanux::net::getDeviceMACAddress( *i ) << endl;
      cout << "  carrier : " << leanux::net::getDeviceCarrier( *i ) << endl;
      cout << "  sysfs   : " << leanux::net::getSysPath( *i ) << endl;
      if ( leanux::net::getDeviceCarrier( *i ) ) {
        cout << "  duplex  : " << leanux::net::getDeviceDuplex( *i ) << endl;
        if ( leanux::net::getDeviceSpeed( *i ) > 0 ) cout << "  speed   : " << leanux::net::getDeviceSpeed( *i ) << "MB/s" << endl;
        cout << "  ipv4 addresses: " << endl;
        list<string> ip4list;
        leanux::net::getDeviceIP4Addresses( *i, ip4list );
        for ( list<string>::const_iterator p = ip4list.begin(); p != ip4list.end(); p++ ) {
          string resolved = leanux::net::resolveIP( *p );
          cout << "    " << *p;
          if ( resolved!=*p ) {
            cout << "='" << resolved << "'";
          }
          cout << endl;
        }
        cout << "  ipv6 addresses: " << endl;
        list<string> ip6list;
        leanux::net::getDeviceIP6Addresses( *i, ip6list );
        for ( list<string>::const_iterator p = ip6list.begin(); p != ip6list.end(); p++ ) {
          string resolved = leanux::net::resolveIP( *p );
          cout << "    " << *p;
          if ( resolved!=*p ) {
            cout << "='" << resolved << "'";
          }
          cout << endl;
        }
      }
      cout << endl;
    }

    list<leanux::net::TCP4SocketInfo> tcp4;
    leanux::net::enumTCP4Sockets( tcp4 );
    list<leanux::net::TCP6SocketInfo> tcp6;
    leanux::net::enumTCP6Sockets( tcp6 );

    cout <<
    setw(5) << "proto" <<
    setw(11) << "state" <<
    setw(9) << "user" <<
    setw(45) << "local address" <<
    setw(11) << "local port" <<
    setw(10) << "device" <<
    setw(45) << "remote address" <<
    setw(12) << "remote port" <<
    endl;
    cout << setw(138) << setfill('-') << '-' << setfill(' ') << endl;
    for ( list<leanux::net::TCP4SocketInfo>::const_iterator i = tcp4.begin(); i != tcp4.end(); i++ ) {
      if ( (*i).tcp_state == leanux::net::TCP_ESTABLISHED || (*i).tcp_state == leanux::net::TCP_LISTEN ) {
        cout <<
        "tcp4 " <<
        setw(11) << leanux::net::getTCPStateString( (*i).tcp_state ) <<
        setw(9) << leanux::system::getUserName( (*i).uid ) <<
        setw(45) << leanux::net::convertAF_INET( (*i).local_addr ) <<
        setw(11) << (*i).local_port <<
        setw(10) << leanux::net::getDeviceByIP( leanux::net::convertAF_INET( (*i).local_addr  ) );
        if ( (*i).tcp_state != leanux::net::TCP_LISTEN ) {
          cout <<
          setw(45) << leanux::net::convertAF_INET( (*i).remote_addr ) <<
          setw(12) << (*i).remote_port;
        }
        cout << endl;
      }
    }
    for ( list<leanux::net::TCP6SocketInfo>::const_iterator i = tcp6.begin(); i != tcp6.end(); i++ ) {
      if ( (*i).tcp_state == leanux::net::TCP_ESTABLISHED || (*i).tcp_state == leanux::net::TCP_LISTEN ) {
        cout <<
        "tcp6 " <<
        setw(11) << leanux::net::getTCPStateString( (*i).tcp_state ) <<
        setw(9) << leanux::system::getUserName( (*i).uid ) <<
        setw(45) << leanux::net::convertAF_INET6( &(*i).local_addr ) <<
        setw(11) << (*i).local_port <<
        setw(10) << leanux::net::getDeviceByIP( leanux::net::convertAF_INET6( &(*i).local_addr  ) );
        if ( (*i).tcp_state != leanux::net::TCP_LISTEN ) {
          cout <<
          setw(45) << leanux::net::convertAF_INET6( &(*i).remote_addr ) <<
          setw(12) << (*i).remote_port;
        }
        cout << endl;
      }
    }
    cout << endl;

    leanux::net::NetStatDeviceMap snap1,snap2;
    leanux::net::NetStatDeviceVector delta;
    struct timeval t1,t2;
    leanux::net::getNetStat( snap1 );
    gettimeofday( &t1, 0 );
    leanux::util::Sleep( 2, 0 );
    leanux::net::getNetStat( snap2 );
    gettimeofday( &t2, 0 );
    leanux::net::getNetStatDelta( snap1, snap2, delta );
    double dt = leanux::util::deltaTime( t1, t2 );

    cout << setw(20) << "receive";
    cout << setw(30) << "transmit";
    cout << endl;
    cout << setw(10) << "device";
    cout << setw(10) << "bw/s";
    cout << setw(10) << "pckt/s";
    cout << setw(10) << "pcktsz";
    cout << setw(10) << "bw/s";
    cout << setw(10) << "pckt/s";
    cout << setw(10) << "pcktsz";
    cout << endl;

    for ( leanux::net::NetStatDeviceVector::const_iterator i = delta.begin(); i != delta.end(); i++ ) {
      cout << setw(10) << (*i).device;
      cout << setw(10) << leanux::util::ByteStr( (*i).rx_bytes / dt, 3 );
      cout << setw(10) << fixed << setprecision(2) << (*i).rx_packets / dt;
      if ( (*i).rx_packets )
        cout << setw(10) << fixed << setprecision(2) << leanux::util::ByteStr( (*i).rx_bytes / (*i).rx_packets, 3 );
      else cout << setw(10) << " ";
      cout << setw(10) << leanux::util::ByteStr( (*i).tx_bytes / dt, 3 );
      cout << setw(10) << fixed << setprecision(2) << (*i).tx_packets / dt;
      if ( (*i).tx_packets )
        cout << setw(10) << fixed << setprecision(2) << leanux::util::ByteStr( (*i).tx_bytes / (*i).tx_packets, 3 );
      else cout << setw(10) << " ";
      cout << endl;
    }
  }
  catch ( leanux::Oops &oops ) {
    cout << oops << endl;
    return 1;
  }
  return 0;
}
