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
 * leanux::net c++ source file.
 */
#include "system.hpp"
#include "net.hpp"
#include "util.hpp"


#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <fstream>
#include <set>
#include <cstdlib>
#include <syslog.h>

#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>

namespace leanux {

  namespace net {

    std::string MACOUIDatabase = "/usr/share/misc/oui.txt";

    void init() {
      if ( util::fileReadAccess( "/usr/share/misc/oui.txt" ) ) {
        MACOUIDatabase = "/usr/share/misc/oui.txt";
      } else
      if ( util::fileReadAccess( "/usr/share/hwdata/oui.txt" ) ) {
        MACOUIDatabase = "/usr/share/hwdata/oui.txt";
      } else
      if ( util::fileReadAccess( "/usr/share/oui.txt" ) ) {
        MACOUIDatabase = "/usr/share/oui.txt";
      } else
      if ( util::fileReadAccess( "/usr/share/ieee-data/oui.txt" ) ) {
        MACOUIDatabase = "/usr/share/ieee-data/oui.txt";
      } else
      throw Oops( __FILE__, __LINE__, "leanux cannot find oui.txt file" );
    }

    std::string getDeviceOperState( const std::string &device ) {
      std::ifstream i( std::string("/sys/class/net/" + device + "/operstate").c_str() );
      std::string r;
      i >> r;
      return r;
    }

    unsigned long getDeviceSpeed( const std::string &device ) {
      unsigned long r = 0;
      try {
        std::ifstream i( std::string("/sys/class/net/" + device + "/speed").c_str() );
        i >> r;
      }
      catch ( ... ) {
      }
      return r;
    }

    int getDeviceCarrier( const std::string &device ) {
      int r = 0;
      try {
        std::ifstream i( std::string("/sys/class/net/" + device + "/carrier").c_str() );
        i >> r;
      }
      catch ( ... ) {
      }
      return r;
    }

    std::string getDeviceDuplex( const std::string &device ) {
      std::string r = "";
      try {
        std::ifstream i( std::string("/sys/class/net/" + device + "/duplex").c_str() );
        i >> r;
      }
      catch ( ... ) {
      }
      return r;
    }

    std::string getDeviceMACAddress( const std::string &device ) {
      std::ifstream i( std::string("/sys/class/net/" + device + "/address").c_str() );
      std::string r;
      i >> r;
      return r;
    }

    void getMACOUI( const std::string &mac, OUI &oui ) {
      oui.vendor = "";
      oui.countrycode = "";
      oui.address.clear();
      std::string search = mac.substr(0,2) + mac.substr(3,2) + mac.substr(6,2);
      std::transform(search.begin(), search.end(), search.begin(), toupper);

      std::ifstream f( MACOUIDatabase.c_str() );
      if ( !f.good() ) throw Oops( __FILE__, __LINE__, "error opening oui.txt" );

      while ( f.good() && !f.eof() ) {
        std::string s = "";
        getline( f, s );
        if ( strncmp( search.c_str(), s.c_str(), 6 ) == 0 ) {
          size_t lasttab = s.find_last_of( '\t', std::string::npos );
          if ( lasttab == std::string::npos || lasttab >= s.length() - 1 )
            throw Oops( __FILE__, __LINE__, "parse error in oui.txt" );
          else {
            // the remainder of the first string is the vendor
            oui.vendor = s.substr( lasttab + 1 );
          }
          std::vector<std::string> tmp;
          while ( f.good() && tmp.size() < 10 ) {
            getline( f, s );
            if ( s == "" ) break;
            lasttab = s.find_last_of( '\t', std::string::npos );
            if ( lasttab != std::string::npos ) tmp.push_back( s.substr(lasttab+1) ); else break;
          }
          if ( tmp.size() < 3 ) throw Oops( __FILE__, __LINE__, "parse error in oui.txt" );
          for ( size_t j = 0; j < tmp.size()-1; j++ ) {
            oui.address.push_back( tmp[j] );
          }
          oui.countrycode = tmp[tmp.size()-1];
          break;
        }
      }
    }

    void getDeviceIP4Addresses( const std::string &device, std::list<std::string> &addrlist ) {
      addrlist.clear();
      struct ifaddrs *ifap, *ifa;
      struct sockaddr_in *sa;
      char *addr;
      std::string result = "";

      getifaddrs (&ifap);
      for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family==AF_INET) {
          sa = (struct sockaddr_in *) ifa->ifa_addr;
          addr = inet_ntoa(sa->sin_addr);
          if ( ifa->ifa_name == device ) {
            addrlist.push_back( addr );
          }
        }
      }
      freeifaddrs(ifap);
    }

    void getDeviceIP6Addresses( const std::string &device, std::list<std::string> &addrlist ) {
      addrlist.clear();
      std::ifstream i( "/proc/net/if_inet6" );
      while ( i.good() && ! i.eof() ) {
        std::string s;
        getline( i, s );
        char dev_buf[128];
        in6_addr in6;
        int r = sscanf( s.c_str(), "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx %*u %*u %*u %*u %128s",
          &(in6.s6_addr[0]), &(in6.s6_addr[1]), &(in6.s6_addr[2]), &(in6.s6_addr[3]),
          &(in6.s6_addr[4]), &(in6.s6_addr[5]), &(in6.s6_addr[6]), &(in6.s6_addr[7]),
          &(in6.s6_addr[8]), &(in6.s6_addr[9]), &(in6.s6_addr[10]), &(in6.s6_addr[11]),
          &(in6.s6_addr[12]), &(in6.s6_addr[13]), &(in6.s6_addr[14]), &(in6.s6_addr[15]),
          dev_buf );
        if ( dev_buf == device && r == 17 ) {
          addrlist.push_back( convertAF_INET6( &in6 ) );
        }
      }
    }

    std::string getDeviceByIP( const std::string& ip ) {
      std::list<std::string> devices;
      enumDevices( devices );
      for ( std::list<std::string>::const_iterator d = devices.begin(); d != devices.end(); d++ ) {
        std::list<std::string> ipv4;
        getDeviceIP4Addresses( (*d), ipv4 );
        for ( std::list<std::string>::const_iterator i = ipv4.begin(); i != ipv4.end(); i++ ) {
          if ( *i == ip ) return *d;
        }
        std::list<std::string> ipv6;
        getDeviceIP6Addresses( (*d), ipv6 );
        for ( std::list<std::string>::const_iterator i = ipv6.begin(); i != ipv6.end(); i++ ) {
          if ( *i == ip ) return *d;
        }
      }
      return "";
    }

    std::string convertAF_INET( unsigned long addr ) {
      char buf[INET_ADDRSTRLEN];
      inet_ntop( AF_INET, &addr, buf, INET_ADDRSTRLEN );
      return buf;
    }

    std::string convertAF_INET6( const struct in6_addr* addr ) {
      char buf[INET6_ADDRSTRLEN];
      inet_ntop( AF_INET6, addr, buf, INET6_ADDRSTRLEN );
      return buf;
    }

    /**
     * parse a TCP4 std::string from /proc/net/tcp.
     * @param s the std::string to parse.
     * @param info the TCP4SocketInfo to fill.
     * @return false if s cannot be parsed.
     */
    bool parseTCP4Line( const std::string &s, TCP4SocketInfo &info ) {
      TCP4SocketInfo tmp;
      unsigned int sl = 0;
      int r = sscanf( s.c_str(), "%u: %lx:%x %lx:%x %x %lx:%lx %*x:%*x %*x %u %*u %lu",
        &sl, &tmp.local_addr, &tmp.local_port, &tmp.remote_addr, &tmp.remote_port, &tmp.tcp_state, &tmp.tx_queue, &tmp.rx_queue, &tmp.uid, &tmp.inode );
      if ( r == 10 ) {
        info = tmp;
        return true;
      } else return false;
    }

    /**
     * parse a TCP6 std::string from /proc/net/tcp.
     * @param s the std::string to parse.
     * @param info the TCP6SocketInfo to fill.
     * @return false if s cannot be parsed.
     */
    bool parseTCP6Line( const std::string &s, TCP6SocketInfo &info ) {
      TCP6SocketInfo tmp;
      unsigned int sl = 0;
      int r = sscanf( s.c_str(), "%u: %2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx:%x %2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx:%x %x %lx:%lx %*x:%*x %*x %u %*u %lu",
        &sl,
        &(tmp.local_addr.s6_addr[3]), &(tmp.local_addr.s6_addr[2]), &(tmp.local_addr.s6_addr[1]), &(tmp.local_addr.s6_addr[0]),
        &(tmp.local_addr.s6_addr[7]), &(tmp.local_addr.s6_addr[6]), &(tmp.local_addr.s6_addr[5]), &(tmp.local_addr.s6_addr[4]),
        &(tmp.local_addr.s6_addr[11]), &(tmp.local_addr.s6_addr[10]), &(tmp.local_addr.s6_addr[9]), &(tmp.local_addr.s6_addr[8]),
        &(tmp.local_addr.s6_addr[15]), &(tmp.local_addr.s6_addr[14]), &(tmp.local_addr.s6_addr[13]), &(tmp.local_addr.s6_addr[12]),
        &(tmp.local_port),
        &(tmp.remote_addr.s6_addr[3]), &(tmp.remote_addr.s6_addr[2]), &(tmp.remote_addr.s6_addr[1]), &(tmp.remote_addr.s6_addr[0]),
        &(tmp.remote_addr.s6_addr[7]), &(tmp.remote_addr.s6_addr[6]), &(tmp.remote_addr.s6_addr[5]), &(tmp.remote_addr.s6_addr[4]),
        &(tmp.remote_addr.s6_addr[11]), &(tmp.remote_addr.s6_addr[10]), &(tmp.remote_addr.s6_addr[9]), &(tmp.remote_addr.s6_addr[8]),
        &(tmp.remote_addr.s6_addr[15]), &(tmp.remote_addr.s6_addr[14]), &(tmp.remote_addr.s6_addr[13]), &(tmp.remote_addr.s6_addr[12]),
        &(tmp.remote_port),
        &(tmp.tcp_state),
        &tmp.tx_queue,
        &tmp.rx_queue,
        &tmp.uid,
        &tmp.inode
        );
      if ( r > 0 ) {
        info = tmp;
        return true;
      } else return false;
    }

    void enumDevices( std::list<std::string> &devices ) {
      devices.clear();
      std::ifstream ifs( "/proc/net/dev" );
      if ( ! ifs.good() ) throw Oops( __FILE__, __LINE__, "unable to read /proc/net/dev" );
      while ( ifs.good() && ! ifs.eof() ) {
        std::string s;
        char dev_buf[128];
        getline( ifs, s );
        unsigned long dummy;
        int r = sscanf( s.c_str(), "%128s %lu", dev_buf, &dummy );
        if ( strlen( dev_buf ) > 0 && strlen( dev_buf ) < 128 ) dev_buf[ strlen( dev_buf ) - 1 ] = 0;
        if ( r == 2 ) devices.push_back(dev_buf);
      }
    }

    void enumTCP4Sockets( std::list<TCP4SocketInfo> &sockets ) {
      std::ifstream i( "/proc/net/tcp" );
      sockets.clear();
      while ( i.good() && ! i.eof() ) {
        std::string s;
        getline( i, s );
        TCP4SocketInfo inf;
        if ( parseTCP4Line( s, inf ) ) {
          sockets.push_back( inf );
        }
      }
    }

    void enumTCP6Sockets( std::list<TCP6SocketInfo> &sockets ) {
      std::ifstream i( "/proc/net/tcp6" );
      sockets.clear();
      while ( i.good() && ! i.eof() ) {
        std::string s;
        getline( i, s );
        TCP6SocketInfo inf;
        if ( parseTCP6Line( s, inf ) ) {
          sockets.push_back( inf );
        }
      }
    }

    std::string getTCPStateString( int tcp_state ) {
      switch( tcp_state ) {
        case TCP_ESTABLISHED:
          return "ESTABLISHED";
        case TCP_SYN_SENT:
          return "SYN_SENT";
        case TCP_SYN_RECV:
          return "SYN_RECV";
        case TCP_FIN_WAIT1:
          return "FIN_WAIT1";
        case TCP_FIN_WAIT2:
          return "FIN_WAIT2";
        case TCP_TIME_WAIT:
          return "TIME_WAIT";
        case TCP_CLOSE:
          return "CLOSE";
        case TCP_CLOSE_WAIT:
          return "CLOSE_WAIT";
        case TCP_LAST_ACK:
          return "LAST_ACK";
        case TCP_LISTEN:
          return "LISTEN";
        case TCP_CLOSING:
          return "CLOSING";
        default:
          std::stringstream ss;
          ss << "Unknown TCP state (" << tcp_state << ")";
          return ss.str();
      }
    }

    std::ostream& operator<<( std::ostream& os, const TCP4SocketInfo &inf ) {
      os << "(tcp4 socket";
      os << " status=" << getTCPStateString( inf.tcp_state );
      os << " user=" << system::getUserName( inf.uid );
      os << " inode=" << inf.inode;
      os << " rxq=" << inf.rx_queue;
      os << " txq=" << inf.tx_queue;
      std::stringstream ss;
      ss << " local=" << convertAF_INET( inf.local_addr ) << ":" << inf.local_port;
      os << ss.str();
      ss.str("");
      if ( inf.tcp_state != TCP_LISTEN ) {
        os << " remote=";
        ss << convertAF_INET( inf.remote_addr ) << ":" << inf.remote_port;
        os << ss.str();
      }
      os << ")";
      return os;
    }

    int whatAF_INET( const std::string &addr ) {
      struct addrinfo hint, *res = NULL;
      int ret;
      memset(&hint, '\0', sizeof hint);
      hint.ai_family = PF_UNSPEC;
      hint.ai_flags = AI_NUMERICHOST;
      ret = getaddrinfo( addr.c_str(), NULL, &hint, &res);
      if ( ret ) throw Oops( __FILE__, __LINE__, gai_strerror(ret) );
      return res->ai_family;
    }

    std::string resolveIP( const std::string &addr ) {
      if ( whatAF_INET( addr ) == AF_INET ) {
        struct sockaddr_in sa;
        sa.sin_family = AF_INET;
        int r = inet_pton( AF_INET, addr.c_str(), &sa.sin_addr );
        if ( r != 1 ) throw Oops( __FILE__, __LINE__, "not a valid ip4 address" );
        char hbuf[NI_MAXHOST];
        memset( hbuf, '\0', sizeof(hbuf) );
        r= getnameinfo( (struct sockaddr*)&sa, sizeof(sa), hbuf, sizeof(hbuf), NULL, 0, NI_NAMEREQD );
        if ( r == 8 ) throw Oops( __FILE__, __LINE__, gai_strerror(r) );
        else if ( r == EAI_NONAME ) return addr;
        return hbuf;
      } else if ( whatAF_INET( addr ) == AF_INET6 ) {
        struct sockaddr_in6 sa;
        sa.sin6_family = AF_INET6;
        int r = inet_pton( AF_INET6, addr.c_str(), &sa.sin6_addr );
        if ( r != 1 ) throw Oops( __FILE__, __LINE__, "not a valid ipv6 address" );
        char hbuf[NI_MAXHOST];
        memset( hbuf, '\0', sizeof(hbuf) );
        r= getnameinfo( (struct sockaddr*)&sa, sizeof(sa), hbuf, sizeof(hbuf), NULL, 0, NI_NAMEREQD );
        if ( r ) {
          if ( r == EAI_NONAME ) return addr; //else throw Oops( __FILE__, __LINE__, gai_strerror(r) );
        }
        return hbuf;
      }
      return "";
    }

    std::string getServiceName( int port ) {
      struct servent* se = getservbyport( htons(port), NULL );
      if ( se ) return se->s_name; else {
        std::stringstream ss;
        ss << port;
        return ss.str();
      }
    }

    std::string getSysPath( const std::string &device ) {
      std::string rel = "/sys/class/net/" + device;
      if ( util::directoryExists( rel ) ) {
        return util::realPath( rel );
      }
      return "";
    }

    bool findTCP4SocketByINode( ino_t inode, TCP4SocketInfo &info ) {
      bool result = false;
      std::ifstream i( "/proc/net/tcp" );
      while ( i.good() && ! i.eof() ) {
        std::string s;
        getline( i, s );
        TCP4SocketInfo tmp;
        if ( parseTCP4Line( s, tmp ) && tmp.inode == inode ) {
          info = tmp;
          result = true;
          break;
        }
      }
      return result;
    }

    bool findUnixDomainSocketByINode( ino_t inode, UnixDomainSocketInfo &info ) {
      bool result = false;
      std::ifstream i( "/proc/net/unix" );
      while ( i.good() && ! i.eof() ) {
        std::string s;
        getline( i, s );
        UnixDomainSocketInfo tmp;
        char buf[2048];
        memset( buf, 0, sizeof(buf) );
        int r = sscanf( s.c_str(), "%*x: %lu %*u %lu %*u %u %lu %s",
                                   &tmp.refcount, &tmp.flags, &tmp.state, &tmp.inode, buf );
        if ( r >= 4  && tmp.inode == inode ) {
          info = tmp;
          info.path = buf;
          result = true;
          break;
        }
      }
      return result;
    }

    std::ostream& operator<<( std::ostream& os, const UnixDomainSocketInfo &inf ) {
      os << "(uds socket";
      os << " refcount=" << inf.refcount;
      os << " state=" << inf.state;
      os << " inode=" << inf.inode;
      os << " path=" << inf.path;
      os << ")";
      return os;
    }

    /**
     * parse a line from /proc/net/udp.
     */
    bool parseUDP4Line( const std::string &s, UDP4SocketInfo &info ) {
      UDP4SocketInfo tmp;
      unsigned int sl = 0;
      int r = sscanf( s.c_str(), "%u: %lx:%x %lx:%x %x %lx:%lx %*x:%*x %*x %u %*u %u",
        &sl, &tmp.local_addr, &tmp.local_port, &tmp.remote_addr, &tmp.remote_port, &tmp.tcp_state, &tmp.tx_queue, &tmp.rx_queue, &tmp.uid, &tmp.inode );
      if ( r == 10 ) {
        info = tmp;
        return true;
      } else return false;
    }

    bool findUDP4SocketByINode( ino_t inode, UDP4SocketInfo &info ) {
      bool result = false;
      std::ifstream i( "/proc/net/udp" );
      while ( i.good() && ! i.eof() ) {
        std::string s;
        getline( i, s );
        UDP4SocketInfo tmp;
        if ( parseUDP4Line( s, tmp ) && tmp.inode == inode ) {
          info = tmp;
          result = true;
          break;
        }
      }
      return result;
    }

    std::ostream& operator<<( std::ostream& os, const UDP4SocketInfo &inf ) {
      os << "(udp4 socket";
      os << " status=" << getTCPStateString( inf.tcp_state );
      os << " user=" << system::getUserName( inf.uid );
      os << " inode=" << inf.inode;
      os << " rxq=" << inf.rx_queue;
      os << " txq=" << inf.tx_queue;
      std::stringstream ss;
      ss << " local=" << convertAF_INET( inf.local_addr ) << ":" << inf.local_port;
      os << ss.str();
      ss.str("");
      if ( inf.tcp_state != TCP_LISTEN ) {
        os << " remote=";
        ss << convertAF_INET( inf.remote_addr ) << ":" << inf.remote_port;
        os << ss.str();
      }
      os << ")";
      return os;
    }

    void getNetDeviceStat( NetDeviceStatDeviceMap &stats ) {
      stats.clear();
      std::ifstream ifs( "/proc/net/dev" );
      if ( ! ifs.good() ) throw Oops( __FILE__, __LINE__, "unable to read /proc/net/dev" );
      while ( ifs.good() && ! ifs.eof() ) {
        std::string s;
        char dev_buf[128];
        getline( ifs, s );
        NetDeviceStat ns;
        int r = sscanf( s.c_str(), "%128s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                                   dev_buf,
                                   &ns.rx_bytes,
                                   &ns.rx_packets,
                                   &ns.rx_errors,
                                   &ns.rx_dropped,
                                   &ns.rx_fifo,
                                   &ns.rx_frame,
                                   &ns.rx_compressed,
                                   &ns.rx_multicast,
                                   &ns.tx_bytes,
                                   &ns.tx_packets,
                                   &ns.tx_errors,
                                   &ns.tx_dropped,
                                   &ns.tx_fifo,
                                   &ns.tx_collisions,
                                   &ns.tx_carrier,
                                   &ns.tx_compressed );
        if ( strlen( dev_buf ) > 0 && strlen( dev_buf ) < 128 ) dev_buf[ strlen( dev_buf ) - 1 ] = 0;
        if ( r == 17 ) stats[dev_buf] = ns;
      }
    }

    void getNetDeviceStatDelta( const NetDeviceStatDeviceMap& snap1, const NetDeviceStatDeviceMap& snap2, NetDeviceStatDeviceVector& delta ) {
      delta.clear();
      for ( NetDeviceStatDeviceMap::const_iterator s2 = snap2.begin(); s2 != snap2.end(); ++s2 ) {
        NetDeviceStatDeviceMap::const_iterator s1 = snap1.find( s2->first );
        NetDeviceStat d;
        d.device = s2->first;
        if ( s1 != snap1.end() ) {
          d.rx_bytes = s2->second.rx_bytes - s1->second.rx_bytes;
          d.rx_packets = s2->second.rx_packets - s1->second.rx_packets;
          d.rx_errors = s2->second.rx_errors - s1->second.rx_errors;
          d.rx_dropped = s2->second.rx_dropped - s1->second.rx_dropped;
          d.rx_fifo = s2->second.rx_fifo - s1->second.rx_fifo;
          d.rx_frame = s2->second.rx_frame - s1->second.rx_frame;
          d.rx_compressed = s2->second.rx_compressed - s1->second.rx_compressed;
          d.rx_multicast = s2->second.rx_multicast - s1->second.rx_multicast;

          d.tx_bytes = s2->second.tx_bytes - s1->second.tx_bytes;
          d.tx_packets = s2->second.tx_packets - s1->second.tx_packets;
          d.tx_errors = s2->second.tx_errors - s1->second.tx_errors;
          d.tx_dropped = s2->second.tx_dropped - s1->second.tx_dropped;
          d.tx_fifo = s2->second.tx_fifo - s1->second.tx_fifo;
          d.tx_collisions = s2->second.tx_collisions - s1->second.tx_collisions;
          d.tx_carrier = s2->second.tx_carrier - s1->second.tx_carrier;
          d.tx_compressed = s2->second.tx_compressed - s1->second.tx_compressed;
        } else {
          d.rx_bytes = s2->second.rx_bytes;
          d.rx_packets = s2->second.rx_packets;
          d.rx_errors = s2->second.rx_errors;
          d.rx_dropped = s2->second.rx_dropped;
          d.rx_fifo = s2->second.rx_fifo;
          d.rx_frame = s2->second.rx_frame;
          d.rx_compressed = s2->second.rx_compressed;
          d.rx_multicast = s2->second.rx_multicast;

          d.tx_bytes = s2->second.tx_bytes;
          d.tx_packets = s2->second.tx_packets;
          d.tx_errors = s2->second.tx_errors;
          d.tx_dropped = s2->second.tx_dropped;
          d.tx_fifo = s2->second.tx_fifo;
          d.tx_collisions = s2->second.tx_collisions;
          d.tx_carrier = s2->second.tx_carrier;
          d.tx_compressed = s2->second.tx_compressed;
        }
        delta.push_back( d );
      }
      sort( delta.begin(), delta.end() );
    }

    void getTCPConnectionCounters( std::list<TCPKeyCounter> &servers, std::list<TCPKeyCounter> &clients ) {
      servers.clear();
      clients.clear();
      std::list<net::TCP4SocketInfo> conn4;
      std::set<unsigned int> server_ports4;
      std::list<net::TCP4SocketInfo> esta4;
      net::enumTCP4Sockets( conn4 );

      for ( std::list<net::TCP4SocketInfo>::const_iterator c = conn4.begin(); c != conn4.end(); c++ ) {
        if ( (*c).tcp_state == net::TCP_LISTEN ) server_ports4.insert( (*c).local_port );
        else if ( (*c).tcp_state == net::TCP_ESTABLISHED ) esta4.push_back( (*c) );
      }

      std::map<TCPKey, unsigned int> server_map;
      std::map<TCPKey, unsigned int> client_map;

      for ( std::list<net::TCP4SocketInfo>::const_iterator i = esta4.begin(); i != esta4.end(); i++ ) {
        if ( server_ports4.find( (*i).local_port  ) != server_ports4.end() ) {
          server_map[ TCPKey( net::convertAF_INET( (*i).local_addr ), (*i).local_port, (*i).uid ) ] += 1;
        } else {
          client_map[ TCPKey( net::convertAF_INET( (*i).remote_addr ), (*i).remote_port, (*i).uid ) ] += 1;
        }
      }

      std::list<net::TCP6SocketInfo> conn6;
      std::set<unsigned int> server_ports6;
      std::list<net::TCP6SocketInfo> esta6;
      net::enumTCP6Sockets( conn6 );

      for ( std::list<net::TCP6SocketInfo>::const_iterator c = conn6.begin(); c != conn6.end(); c++ ) {
        if ( (*c).tcp_state == net::TCP_LISTEN ) server_ports6.insert( (*c).local_port );
        else if ( (*c).tcp_state == net::TCP_ESTABLISHED ) esta6.push_back( (*c) );
      }

      for ( std::list<net::TCP6SocketInfo>::const_iterator i = esta6.begin(); i != esta6.end(); i++ ) {
        if ( server_ports6.find( (*i).local_port  ) != server_ports6.end() ) {
          server_map[ TCPKey( net::convertAF_INET6( &(*i).local_addr ), (*i).local_port, (*i).uid ) ] += 1;
        } else {
          client_map[ TCPKey( net::convertAF_INET6( &(*i).remote_addr ), (*i).remote_port, (*i).uid ) ] += 1;
        }
      }

      for ( std::map<TCPKey, unsigned int>::const_iterator i = server_map.begin(); i != server_map.end(); ++i ) {
        servers.push_back( TCPKeyCounter( i->first, i->second ) );
      }
      servers.sort();

      for ( std::map<TCPKey, unsigned int>::const_iterator i = client_map.begin(); i != client_map.end(); ++i ) {
        clients.push_back( TCPKeyCounter( i->first, i->second ) );
      }
      clients.sort();
    }
    
      
    std::vector<std::string> ProcNetDeviceStatNames = {
        "SyncookiesSent",
        "SyncookiesRecv",
        "SyncookiesFailed",
        "EmbryonicRsts",
        "PruneCalled",
        "RcvPruned",
        "OfoPruned",
        "OutOfWindowIcmps",
        "LockDroppedIcmps",
        "ArpFilter",
        "TW",
        "TWRecycled",
        "TWKilled",
        "PAWSActive",
        "PAWSEstab",
        "DelayedACKs",
        "DelayedACKLocked",
        "DelayedACKLost",
        "ListenOverflows",
        "ListenDrops",
        "TCPHPHits",
        "TCPPureAcks",
        "TCPHPAcks",
        "TCPRenoRecovery",
        "TCPSackRecovery",
        "TCPSACKReneging",
        "TCPSACKReorder",
        "TCPRenoReorder",
        "TCPTSReorder",
        "TCPFullUndo",
        "TCPPartialUndo",
        "TCPDSACKUndo",
        "TCPLossUndo",
        "TCPLostRetransmit",
        "TCPRenoFailures",
        "TCPSackFailures",
        "TCPLossFailures",
        "TCPFastRetrans",
        "TCPSlowStartRetrans",
        "TCPTimeouts",
        "TCPLossProbes",
        "TCPLossProbeRecovery",
        "TCPRenoRecoveryFail",
        "TCPSackRecoveryFail",
        "TCPRcvCollapsed",
        "TCPBacklogCoalesce",
        "TCPDSACKOldSent",
        "TCPDSACKOfoSent",
        "TCPDSACKRecv",
        "TCPDSACKOfoRecv",
        "TCPAbortOnData",
        "TCPAbortOnClose",
        "TCPAbortOnMemory",
        "TCPAbortOnTimeout",
        "TCPAbortOnLinger",
        "TCPAbortFailed",
        "TCPMemoryPressures",
        "TCPMemoryPressuresChrono",
        "TCPSACKDiscard",
        "TCPDSACKIgnoredOld",
        "TCPDSACKIgnoredNoUndo",
        "TCPSpuriousRTOs",
        "TCPMD5NotFound",
        "TCPMD5Unexpected",
        "TCPMD5Failure",
        "TCPSackShifted",
        "TCPSackMerged",
        "TCPSackShiftFallback",
        "TCPBacklogDrop",
        "PFMemallocDrop",
        "TCPMinTTLDrop",
        "TCPDeferAcceptDrop",
        "IPReversePathFilter",
        "TCPTimeWaitOverflow",
        "TCPReqQFullDoCookies",
        "TCPReqQFullDrop",
        "TCPRetransFail",
        "TCPRcvCoalesce",
        "TCPOFOQueue",
        "TCPOFODrop",
        "TCPOFOMerge",
        "TCPChallengeACK",
        "TCPSYNChallenge",
        "TCPFastOpenActive",
        "TCPFastOpenActiveFail",
        "TCPFastOpenPassive",
        "TCPFastOpenPassiveFail",
        "TCPFastOpenListenOverflow",
        "TCPFastOpenCookieReqd",
        "TCPFastOpenBlackhole",
        "TCPSpuriousRtxHostQueues",
        "BusyPollRxPackets",
        "TCPAutoCorking",
        "TCPFromZeroWindowAdv",
        "TCPToZeroWindowAdv",
        "TCPWantZeroWindowAdv",
        "TCPSynRetrans",
        "TCPOrigDataSent",
        "TCPHystartTrainDetect",
        "TCPHystartTrainCwnd",
        "TCPHystartDelayDetect",
        "TCPHystartDelayCwnd",
        "TCPACKSkippedSynRecv",
        "TCPACKSkippedPAWS",
        "TCPACKSkippedSeq",
        "TCPACKSkippedFinWait2",
        "TCPACKSkippedTimeWait",
        "TCPACKSkippedChallenge",
        "TCPWinProbe",
        "TCPKeepAlive",
        "TCPMTUPFail",
        "TCPMTUPSuccess",
        "TCPDelivered",
        "TCPDeliveredCE",
        "TCPAckCompressed",
        "TCPZeroWindowDrop",
        "TCPRcvQDrop",
        "TCPWqueueTooBig",
        "TCPFastOpenPassiveAltKey",
        "TcpTimeoutRehash",
        "TcpDuplicateDataRehash",
        "TCPDSACKRecvSegs",
        "TCPDSACKIgnoredDubious",
        "TCPMigrateReqSuccess",
        "TCPMigrateReqFailure",        
    };
    
    static std::map<std::string,unsigned long> ProcNetDeviceStatNameIndexMap;
    static std::map<unsigned long,std::string> ProcNetDeviceStatIndexNameMap;
    
    void discoverTCPStat() {
      std::ifstream ifs( "/proc/net/netstat" );
      if ( ! ifs.good() ) throw Oops( __FILE__, __LINE__, "unable to read /proc/net/netstat" );
      bool first = true;        
      while ( ifs.good() && ! ifs.eof() ) {
        std::string s;
        getline( ifs, s );        
        if ( s.substr(0,7) == "TcpExt:" && first ) {
          std::stringstream line(s);
          std::string token;
          size_t idx = 0;
          while (getline(line, token, ' ')) {
            if ( idx > 0 ) {
              ProcNetDeviceStatNameIndexMap[token] = idx - 1;
              ProcNetDeviceStatIndexNameMap[idx-1] = token;              
            }
            idx++;
          }
          first = false;
        }
      }    
    }  
    
    void getTCPStatNetstat( TCPStat &stats ) {      
      std::ifstream ifs( "/proc/net/netstat" );
      if ( ! ifs.good() ) throw Oops( __FILE__, __LINE__, "unable to read /proc/net/netstat" );
      bool first = true;        
      while ( ifs.good() && ! ifs.eof() ) {
        std::string s;
        getline( ifs, s );        
        if ( s.substr(0,7) == "TcpExt:" ) {
          if ( !first ) {
            std::stringstream line(s);
            std::string value;
            size_t idx = 0;
            while (getline(line, value, ' ')) {
              if ( idx > 0 ) {
                std::string token = ProcNetDeviceStatIndexNameMap[idx-1];
                if ( token == "SyncookiesSent" ) stats.SyncookiesSent = std::atol(value.c_str());
                else if ( token == "SyncookiesRecv" ) stats.SyncookiesRecv = std::atol(value.c_str());
                else if ( token == "SyncookiesFailed" ) stats.SyncookiesFailed = std::atol(value.c_str());
                else if ( token == "EmbryonicRsts" ) stats.EmbryonicRsts = std::atol(value.c_str());
                else if ( token == "PruneCalled" ) stats.PruneCalled = std::atol(value.c_str());
                else if ( token == "RcvPruned" ) stats.RcvPruned = std::atol(value.c_str());
                else if ( token == "OfoPruned" ) stats.OfoPruned = std::atol(value.c_str());
                else if ( token == "OutOfWindowIcmps" ) stats.OutOfWindowIcmps = std::atol(value.c_str());
                else if ( token == "LockDroppedIcmps" ) stats.LockDroppedIcmps = std::atol(value.c_str());
                else if ( token == "ArpFilter" ) stats.ArpFilter = std::atol(value.c_str());
                else if ( token == "TW" ) stats.TW = std::atol(value.c_str());
                else if ( token == "TWRecycled" ) stats.TWRecycled = std::atol(value.c_str());
                else if ( token == "TWKilled" ) stats.TWKilled = std::atol(value.c_str());
                else if ( token == "PAWSActive" ) stats.PAWSActive = std::atol(value.c_str());
                else if ( token == "PAWSEstab" ) stats.PAWSEstab = std::atol(value.c_str());
                else if ( token == "DelayedACKs" ) stats.DelayedACKs = std::atol(value.c_str());
                else if ( token == "DelayedACKLocked" ) stats.DelayedACKLocked = std::atol(value.c_str());
                else if ( token == "DelayedACKLost" ) stats.DelayedACKLost = std::atol(value.c_str());
                else if ( token == "ListenOverflows" ) stats.ListenOverflows = std::atol(value.c_str());
                else if ( token == "ListenDrops" ) stats.ListenDrops = std::atol(value.c_str());
                else if ( token == "TCPHPHits" ) stats.TCPHPHits = std::atol(value.c_str());
                else if ( token == "TCPPureAcks" ) stats.TCPPureAcks = std::atol(value.c_str());
                else if ( token == "TCPHPAcks" ) stats.TCPHPAcks = std::atol(value.c_str());
                else if ( token == "TCPRenoRecovery" ) stats.TCPRenoRecovery = std::atol(value.c_str());
                else if ( token == "TCPSackRecovery" ) stats.TCPSackRecovery = std::atol(value.c_str());
                else if ( token == "TCPSACKReneging" ) stats.TCPSACKReneging = std::atol(value.c_str());
                else if ( token == "TCPSACKReorder" ) stats.TCPSACKReorder = std::atol(value.c_str());
                else if ( token == "TCPRenoReorder" ) stats.TCPRenoReorder = std::atol(value.c_str());
                else if ( token == "TCPTSReorder" ) stats.TCPTSReorder = std::atol(value.c_str());
                else if ( token == "TCPFullUndo" ) stats.TCPFullUndo = std::atol(value.c_str());
                else if ( token == "TCPPartialUndo" ) stats.TCPPartialUndo = std::atol(value.c_str());
                else if ( token == "TCPDSACKUndo" ) stats.TCPDSACKUndo = std::atol(value.c_str());
                else if ( token == "TCPLossUndo" ) stats.TCPLossUndo = std::atol(value.c_str());
                else if ( token == "TCPLostRetransmit" ) stats.TCPLostRetransmit = std::atol(value.c_str());
                else if ( token == "TCPRenoFailures" ) stats.TCPRenoFailures = std::atol(value.c_str());
                else if ( token == "TCPSackFailures" ) stats.TCPSackFailures = std::atol(value.c_str());
                else if ( token == "TCPLossFailures" ) stats.TCPLossFailures = std::atol(value.c_str());
                else if ( token == "TCPFastRetrans" ) stats.TCPFastRetrans = std::atol(value.c_str());
                else if ( token == "TCPSlowStartRetrans" ) stats.TCPSlowStartRetrans = std::atol(value.c_str());
                else if ( token == "TCPTimeouts" ) stats.TCPTimeouts = std::atol(value.c_str());
                else if ( token == "TCPLossProbes" ) stats.TCPLossProbes = std::atol(value.c_str());
                else if ( token == "TCPLossProbeRecovery" ) stats.TCPLossProbeRecovery = std::atol(value.c_str());
                else if ( token == "TCPRenoRecoveryFail" ) stats.TCPRenoRecoveryFail = std::atol(value.c_str());
                else if ( token == "TCPSackRecoveryFail" ) stats.TCPSackRecoveryFail = std::atol(value.c_str());
                else if ( token == "TCPRcvCollapsed" ) stats.TCPRcvCollapsed = std::atol(value.c_str());
                else if ( token == "TCPBacklogCoalesce" ) stats.TCPBacklogCoalesce = std::atol(value.c_str());
                else if ( token == "TCPDSACKOldSent" ) stats.TCPDSACKOldSent = std::atol(value.c_str());
                else if ( token == "TCPDSACKOfoSent" ) stats.TCPDSACKOfoSent = std::atol(value.c_str());
                else if ( token == "TCPDSACKRecv" ) stats.TCPDSACKRecv = std::atol(value.c_str());
                else if ( token == "TCPDSACKOfoRecv" ) stats.TCPDSACKOfoRecv = std::atol(value.c_str());
                else if ( token == "TCPAbortOnData" ) stats.TCPAbortOnData = std::atol(value.c_str());
                else if ( token == "TCPAbortOnClose" ) stats.TCPAbortOnClose = std::atol(value.c_str());
                else if ( token == "TCPAbortOnMemory" ) stats.TCPAbortOnMemory = std::atol(value.c_str());
                else if ( token == "TCPAbortOnTimeout" ) stats.TCPAbortOnTimeout = std::atol(value.c_str());
                else if ( token == "TCPAbortOnLinger" ) stats.TCPAbortOnLinger = std::atol(value.c_str());
                else if ( token == "TCPAbortFailed" ) stats.TCPAbortFailed = std::atol(value.c_str());
                else if ( token == "TCPMemoryPressures" ) stats.TCPMemoryPressures = std::atol(value.c_str());
                else if ( token == "TCPMemoryPressuresChrono" ) stats.TCPMemoryPressuresChrono = std::atol(value.c_str());
                else if ( token == "TCPSACKDiscard" ) stats.TCPSACKDiscard = std::atol(value.c_str());
                else if ( token == "TCPDSACKIgnoredOld" ) stats.TCPDSACKIgnoredOld = std::atol(value.c_str());
                else if ( token == "TCPDSACKIgnoredNoUndo" ) stats.TCPDSACKIgnoredNoUndo = std::atol(value.c_str());
                else if ( token == "TCPSpuriousRTOs" ) stats.TCPSpuriousRTOs = std::atol(value.c_str());
                else if ( token == "TCPMD5NotFound" ) stats.TCPMD5NotFound = std::atol(value.c_str());
                else if ( token == "TCPMD5Unexpected" ) stats.TCPMD5Unexpected = std::atol(value.c_str());
                else if ( token == "TCPMD5Failure" ) stats.TCPMD5Failure = std::atol(value.c_str());
                else if ( token == "TCPSackShifted" ) stats.TCPSackShifted = std::atol(value.c_str());
                else if ( token == "TCPSackMerged" ) stats.TCPSackMerged = std::atol(value.c_str());
                else if ( token == "TCPSackShiftFallback" ) stats.TCPSackShiftFallback = std::atol(value.c_str());
                else if ( token == "TCPBacklogDrop" ) stats.TCPBacklogDrop = std::atol(value.c_str());
                else if ( token == "PFMemallocDrop" ) stats.PFMemallocDrop = std::atol(value.c_str());
                else if ( token == "TCPMinTTLDrop" ) stats.TCPMinTTLDrop = std::atol(value.c_str());
                else if ( token == "TCPDeferAcceptDrop" ) stats.TCPDeferAcceptDrop = std::atol(value.c_str());
                else if ( token == "IPReversePathFilter" ) stats.IPReversePathFilter = std::atol(value.c_str());
                else if ( token == "TCPTimeWaitOverflow" ) stats.TCPTimeWaitOverflow = std::atol(value.c_str());
                else if ( token == "TCPReqQFullDoCookies" ) stats.TCPReqQFullDoCookies = std::atol(value.c_str());
                else if ( token == "TCPReqQFullDrop" ) stats.TCPReqQFullDrop = std::atol(value.c_str());
                else if ( token == "TCPRetransFail" ) stats.TCPRetransFail = std::atol(value.c_str());
                else if ( token == "TCPRcvCoalesce" ) stats.TCPRcvCoalesce = std::atol(value.c_str());
                else if ( token == "TCPOFOQueue" ) stats.TCPOFOQueue = std::atol(value.c_str());
                else if ( token == "TCPOFODrop" ) stats.TCPOFODrop = std::atol(value.c_str());
                else if ( token == "TCPOFOMerge" ) stats.TCPOFOMerge = std::atol(value.c_str());
                else if ( token == "TCPChallengeACK" ) stats.TCPChallengeACK = std::atol(value.c_str());
                else if ( token == "TCPSYNChallenge" ) stats.TCPSYNChallenge = std::atol(value.c_str());
                else if ( token == "TCPFastOpenActive" ) stats.TCPFastOpenActive = std::atol(value.c_str());
                else if ( token == "TCPFastOpenActiveFail" ) stats.TCPFastOpenActiveFail = std::atol(value.c_str());
                else if ( token == "TCPFastOpenPassive" ) stats.TCPFastOpenPassive = std::atol(value.c_str());
                else if ( token == "TCPFastOpenPassiveFail" ) stats.TCPFastOpenPassiveFail = std::atol(value.c_str());
                else if ( token == "TCPFastOpenListenOverflow" ) stats.TCPFastOpenListenOverflow = std::atol(value.c_str());
                else if ( token == "TCPFastOpenCookieReqd" ) stats.TCPFastOpenCookieReqd = std::atol(value.c_str());
                else if ( token == "TCPFastOpenBlackhole" ) stats.TCPFastOpenBlackhole = std::atol(value.c_str());
                else if ( token == "TCPSpuriousRtxHostQueues" ) stats.TCPSpuriousRtxHostQueues = std::atol(value.c_str());
                else if ( token == "BusyPollRxPackets" ) stats.BusyPollRxPackets = std::atol(value.c_str());
                else if ( token == "TCPAutoCorking" ) stats.TCPAutoCorking = std::atol(value.c_str());
                else if ( token == "TCPFromZeroWindowAdv" ) stats.TCPFromZeroWindowAdv = std::atol(value.c_str());
                else if ( token == "TCPToZeroWindowAdv" ) stats.TCPToZeroWindowAdv = std::atol(value.c_str());
                else if ( token == "TCPWantZeroWindowAdv" ) stats.TCPWantZeroWindowAdv = std::atol(value.c_str());
                else if ( token == "TCPSynRetrans" ) stats.TCPSynRetrans = std::atol(value.c_str());
                else if ( token == "TCPOrigDataSent" ) stats.TCPOrigDataSent = std::atol(value.c_str());
                else if ( token == "TCPHystartTrainDetect" ) stats.TCPHystartTrainDetect = std::atol(value.c_str());
                else if ( token == "TCPHystartTrainCwnd" ) stats.TCPHystartTrainCwnd = std::atol(value.c_str());
                else if ( token == "TCPHystartDelayDetect" ) stats.TCPHystartDelayDetect = std::atol(value.c_str());
                else if ( token == "TCPHystartDelayCwnd" ) stats.TCPHystartDelayCwnd = std::atol(value.c_str());
                else if ( token == "TCPACKSkippedSynRecv" ) stats.TCPACKSkippedSynRecv = std::atol(value.c_str());
                else if ( token == "TCPACKSkippedPAWS" ) stats.TCPACKSkippedPAWS = std::atol(value.c_str());
                else if ( token == "TCPACKSkippedSeq" ) stats.TCPACKSkippedSeq = std::atol(value.c_str());
                else if ( token == "TCPACKSkippedFinWait2" ) stats.TCPACKSkippedFinWait2 = std::atol(value.c_str());
                else if ( token == "TCPACKSkippedTimeWait" ) stats.TCPACKSkippedTimeWait = std::atol(value.c_str());
                else if ( token == "TCPACKSkippedChallenge" ) stats.TCPACKSkippedChallenge = std::atol(value.c_str());
                else if ( token == "TCPWinProbe" ) stats.TCPWinProbe = std::atol(value.c_str());
                else if ( token == "TCPKeepAlive" ) stats.TCPKeepAlive = std::atol(value.c_str());
                else if ( token == "TCPMTUPFail" ) stats.TCPMTUPFail = std::atol(value.c_str());
                else if ( token == "TCPMTUPSuccess" ) stats.TCPMTUPSuccess = std::atol(value.c_str());
                else if ( token == "TCPDelivered" ) stats.TCPDelivered = std::atol(value.c_str());
                else if ( token == "TCPDeliveredCE" ) stats.TCPDeliveredCE = std::atol(value.c_str());
                else if ( token == "TCPAckCompressed" ) stats.TCPAckCompressed = std::atol(value.c_str());
                else if ( token == "TCPZeroWindowDrop" ) stats.TCPZeroWindowDrop = std::atol(value.c_str());
                else if ( token == "TCPRcvQDrop" ) stats.TCPRcvQDrop = std::atol(value.c_str());
                else if ( token == "TCPWqueueTooBig" ) stats.TCPWqueueTooBig = std::atol(value.c_str());
                else if ( token == "TCPFastOpenPassiveAltKey" ) stats.TCPFastOpenPassiveAltKey = std::atol(value.c_str());
                else if ( token == "TcpTimeoutRehash" ) stats.TcpTimeoutRehash = std::atol(value.c_str());
                else if ( token == "TcpDuplicateDataRehash" ) stats.TcpDuplicateDataRehash = std::atol(value.c_str());
                else if ( token == "TCPDSACKRecvSegs" ) stats.TCPDSACKRecvSegs = std::atol(value.c_str());
                else if ( token == "TCPDSACKIgnoredDubious" ) stats.TCPDSACKIgnoredDubious = std::atol(value.c_str());
                else if ( token == "TCPMigrateReqSuccess" ) stats.TCPMigrateReqSuccess = std::atol(value.c_str());
                else if ( token == "TCPMigrateReqFailure" ) stats.TCPMigrateReqFailure = std::atol(value.c_str());
                else throw Oops( __FILE__, __LINE__, "unknown entry '" + token + "' in /proc/net/netstat" );

                //std::cout << "idx " << idx << " token " << token << " value " << value << std::endl;                
              }
              idx++;
            }
          }
          first = false;
        }
      }       
    }  
    
    void getTCPStatSnmp( TCPStat &stats ) {
      std::ifstream ifs( "/proc/net/snmp" );
      if ( ! ifs.good() ) throw Oops( __FILE__, __LINE__, "unable to read /proc/net/netstat" );
      bool first = true;        
      while ( ifs.good() && ! ifs.eof() ) {
        std::string s;
        getline( ifs, s );        
        if ( s.substr(0,4) == "Tcp:" ) {
          if ( !first ) {
            std::stringstream line(s);
            std::string value;
            size_t idx = 0;
            while (getline(line, value, ' ')) {
              switch (idx) {
                case 1: stats.RtoAlgorithm = atol(value.c_str()); break;
                case 2: stats.RtoMin = atol(value.c_str()); break;
                case 3: stats.RtoMax = atol(value.c_str()); break;
                case 4: stats.MaxConn = atol(value.c_str()); break;
                case 5: stats.ActiveOpens = atol(value.c_str()); break;
                case 6: stats.PassiveOpens = atol(value.c_str()); break;
                case 7: stats.AttemptFails = atol(value.c_str()); break;
                case 8: stats.EstabResets = atol(value.c_str()); break;
                case 9: stats.CurrEstab = atol(value.c_str()); break;
                case 10: stats.InSegs = atol(value.c_str()); break;
                case 11: stats.OutSegs = atol(value.c_str()); break;
                case 12: stats.RetransSegs = atol(value.c_str()); break;
                case 13: stats.InErrs = atol(value.c_str()); break;
                case 14: stats.OutRsts = atol(value.c_str()); break;
                case 15: stats.InCsumErrors = atol(value.c_str()); break;
                default: break;
                std::cout << "idx " << idx << " value " << atol(value.c_str()) << std::endl;
              }
              idx++;
            }
          }
          first = false;
        }
      }       
    }

  }

}
