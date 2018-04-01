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

#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>

namespace leanux {

  namespace net {

    std::string MACOUIDatabase = "/usr/share/misc/oui.txt";

    void init() throw ( Oops ) {
      if ( util::fileReadAccess( "/usr/share/misc/oui.txt" ) ) {
        MACOUIDatabase = "/usr/share/misc/oui.txt";
      } else
      if ( util::fileReadAccess( "/usr/share/hwdata/oui.txt" ) ) {
        MACOUIDatabase = "/usr/share/hwdata/oui.txt";
      } else
      if ( util::fileReadAccess( "/usr/share/oui.txt" ) ) {
        MACOUIDatabase ="/usr/share/oui.txt";
      } else throw Oops( __FILE__, __LINE__, "leanux cannot find oui.txt file" );
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

    void getMACOUI( const std::string &mac, OUI &oui ) throw(Oops) {
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
        if (ifa->ifa_addr->sa_family==AF_INET) {
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
        std::stringstream neat;
        neat << std::setfill('0') << std::hex <<
          std::setw(2) << (int)tmp.local_addr.s6_addr[0] <<
          std::setw(2) << (int)tmp.local_addr.s6_addr[1]<<":"<<
          std::setw(2) << (int)tmp.local_addr.s6_addr[2] <<
          std::setw(2) << (int)tmp.local_addr.s6_addr[3]<<":"<<
          std::setw(2) << (int)tmp.local_addr.s6_addr[4] <<
          std::setw(2) << (int)tmp.local_addr.s6_addr[5]<<":"<<
          std::setw(2) << (int)tmp.local_addr.s6_addr[6] <<
          std::setw(2) << (int)tmp.local_addr.s6_addr[7]<<":"<<
          std::setw(2) << (int)tmp.local_addr.s6_addr[8] <<
          std::setw(2) << (int)tmp.local_addr.s6_addr[9]<<":"<<
          std::setw(2) << (int)tmp.local_addr.s6_addr[10] <<
          std::setw(2) << (int)tmp.local_addr.s6_addr[11]<<":"<<
          std::setw(2) << (int)tmp.local_addr.s6_addr[12] <<
          std::setw(2) << (int)tmp.local_addr.s6_addr[13]<<":"<<
          std::setw(2) << (int)tmp.local_addr.s6_addr[14] <<
          std::setw(2) << (int)tmp.local_addr.s6_addr[15];
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

    int whatAF_INET( const std::string &addr ) throw ( Oops ) {
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

    void getNetStat( NetStatDeviceMap &stats ) {
      stats.clear();
      std::ifstream ifs( "/proc/net/dev" );
      if ( ! ifs.good() ) throw Oops( __FILE__, __LINE__, "unable to read /proc/net/dev" );
      while ( ifs.good() && ! ifs.eof() ) {
        std::string s;
        char dev_buf[128];
        getline( ifs, s );
        NetStat ns;
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

    void getNetStatDelta( const NetStatDeviceMap& snap1, const NetStatDeviceMap& snap2, NetStatDeviceVector& delta ) {
      delta.clear();
      for ( NetStatDeviceMap::const_iterator s2 = snap2.begin(); s2 != snap2.end(); ++s2 ) {
        NetStatDeviceMap::const_iterator s1 = snap1.find( s2->first );
        NetStat d;
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

  }

}
