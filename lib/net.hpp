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
 * leanux::net c++ header file.
 */
#ifndef LEANUX_NET_HPP
#define LEANUX_NET_HPP

#include "oops.hpp"

#include <string>
#include <ostream>
#include <list>
#include <map>
#include <vector>

#include <arpa/inet.h>

namespace leanux {

  /**
   * network configuration and performance API.
   */
  namespace net {

    /**
     * initialize net library
     */
    void init();

    /**
     * TCP connection states.
     */
    enum TCPState {
          TCP_ESTABLISHED = 1,
          TCP_SYN_SENT,
          TCP_SYN_RECV,
          TCP_FIN_WAIT1,
          TCP_FIN_WAIT2,
          TCP_TIME_WAIT,
          TCP_CLOSE,
          TCP_CLOSE_WAIT,
          TCP_LAST_ACK,
          TCP_LISTEN,
          TCP_CLOSING };

    /**
     * Get a network device's operating state.
     * @param device the device name (as in 'eth0').
     * @return either 'up' or 'down'.
     */
    std::string getDeviceOperState( const std::string &device );

    /**
     * Get the device speed in Mb/s.
     * @param device the device name.
     * @return the device speed in Mb/s.
     */
    unsigned long getDeviceSpeed( const std::string &device );

    /**
     * Get the device speed in Mb/s.
     * @param device the device name.
     * @return the device carrier.
     */
    int getDeviceCarrier( const std::string &device );

    /**
     * Get the device's MAC address.
     */
    std::string getDeviceMACAddress( const std::string &device );

    struct OUI {
      std::string vendor;
      std::string countrycode;
      std::list<std::string> address;
    };

    void getMACOUI( const std::string &mac, OUI &oui );

    /**
     * Get the duplex mode of the device.
     * @param device the device name.
     * @return the duplex mode.
     */
    std::string getDeviceDuplex( const std::string &device );

    /**
     * Get a list of IP4 adresses assigned to the device.
     * @param device the device name.
     * @param addrlist the list of IP4 addresses to fill.
     */
    void getDeviceIP4Addresses( const std::string &device, std::list<std::string> &addrlist );

    /**
     * Get a list of IP6 adresses assigned to the device.
     * @param device the device name.
     * @param addrlist the list of IP6 addresses to fill.
     */
    void getDeviceIP6Addresses( const std::string &device, std::list<std::string> &addrlist );

    /**
     * Return the device name configured with the given ip address.
     * @param ip the ip address, either IPV4 or IPV6.
     * @return device name or empty if not found.
     */
    std::string getDeviceByIP( const std::string& ip );

    /**
     * IPv4 socket
     */
    struct TCP4SocketInfo {
      /** local address in network format. @see convertAF_INET. */
      unsigned long local_addr;
      /** the local port. */
      unsigned int local_port;
      /** remote address in network format. @see convertAF_INET. */
      unsigned long remote_addr;
      /** the remote port. */
      unsigned int remote_port;
      /** state of the socket, one of the Linux TCP_ defines. @see getTCPStateString. */
      unsigned int tcp_state;
      /** entries in transmit queue. */
      unsigned long tx_queue;
      /** entries in receive queue. */
      unsigned long rx_queue;
      /** uid owning the socket. @see leanux::system::getUserName(). */
      unsigned int uid;
      /** the inode of the socket. */
      ino_t inode;
    };

    /**
     * IPv4 socket
     */
    struct TCP6SocketInfo {
      /** local address in network format. @see convertAF_INET. */
      struct in6_addr local_addr;
      /** the local port. */
      unsigned int local_port;
      /** remote address in network format. @see convertAF_INET. */
      in6_addr remote_addr;
      /** the remote port. */
      unsigned int remote_port;
      /** state of the socket, one of the Linux TCP_ defines. @see getTCPStateString. */
      unsigned int tcp_state;
      /** entries in transmit queue. */
      unsigned long tx_queue;
      /** entries in receive queue. */
      unsigned long rx_queue;
      /** uid owning the socket. @see leanux::system::getUserName(). */
      unsigned int uid;
      /** the inode of the socket. */
      ino_t inode;
    };

    /**
     * Dump a TCP4Socket to stream.
     */
    std::ostream& operator<<( std::ostream& os, const TCP4SocketInfo &inf );

    /**
     * Enumerate network devices.
     * @param devices the list of device names to fill.
     */
    void enumDevices( std::list<std::string> &devices );

    /**
     * Enumerate TCP4 sockets from /proc/net/tcp
     * @param sockets the list of TCP4SocketInfo to fill.
     */
    void enumTCP4Sockets( std::list<TCP4SocketInfo> &sockets );

    /**
     * Enumerate TCP6 sockets from /proc/net/tcp6
     * @param sockets the list of TCP6SocketInfo to fill.
     */
    void enumTCP6Sockets( std::list<TCP6SocketInfo> &sockets );

    /**
     * Get human readable name for a tcp state.
     * @param tcp_state the tcp state.
     * @return a human readable std::string.
     */
    std::string getTCPStateString( int tcp_state );

    /**
     * convert IPv4 network IP representation to human readable format.
     * @param addr the IPv4 address (in network format) to convert.
     * @return a std::string with the IPv4 address.
     */
    std::string convertAF_INET( unsigned long addr );

    /**
     * convert IPv6 network IP representation to human readable format.
     * @param addr the IPv6 address (in network format) to convert.
     * @return a std::string with the IPv6 address.
     */
    std::string convertAF_INET6( const struct in6_addr* addr );

    /**
     * Determine the type of IP address. If addr is not a valid address,
     * an Oops is thrown.
     * @param addr the IP address to inspect.
     * @return AF_INET or AF_INET6.
     */
    int whatAF_INET( const std::string &addr );

    /**
     * Try to resolve an IPv4 or IPv6 address.
     * If resolving fails, it resturns the input.
     * @param addr the address to resolve.
     * @return the resolved name for the adddress, or if the resolve failed, addr itself.
     */
    std::string resolveIP( const std::string &addr );

    /**
     * Get the service name, eg 'ssh' for port 22.
     * @param port the port number of the service
     * @return the service name associated with the port number
     */
    std::string getServiceName( int port );

    /**
     * Get the device sysfs path
     * @param device the network device
     * @return the sysfs path
     */
    std::string getSysPath( const std::string &device );

    /**
     * Search for a TCP4 socket by inode number.
     * @param inode the indoe to match.
     * @param info the TCP4SocketInfo to fill if an entry is found.
     * @return true when the inode is found.
     */
    bool findTCP4SocketByINode( ino_t inode, TCP4SocketInfo &info );

    /**
     * Unix domain socket.
     */
    struct UnixDomainSocketInfo {
      unsigned long refcount;
      unsigned long flags;
      unsigned int  state;
      ino_t inode;
      std::string path;
    };

    /**
     * Search for a Unix domain socket by inode number.
     * @param inode the indoe to match.
     * @param info the UnixDomainSocketInfo to fill if an entry is found.
     * @return true when the inode is found.
     */
    bool findUnixDomainSocketByINode( ino_t inode, UnixDomainSocketInfo &info );

    /**
     * Dump a UnixDomainSocketInfo to stream.
     */
    std::ostream& operator<<( std::ostream& os, const UnixDomainSocketInfo &inf );

    /**
     * UDP4 socket.
     */
    struct UDP4SocketInfo {
      unsigned long local_addr;
      unsigned int local_port;
      unsigned long remote_addr;
      unsigned int remote_port;
      unsigned int tcp_state;
      unsigned long tx_queue;
      unsigned long rx_queue;
      unsigned int uid;
      unsigned int inode;
    };

    /**
     * Search for a UDP4 socket by inode number.
     * @param inode the indoe to match.
     * @param info the UDP4SocketInfo to fill if an entry is found.
     * @return true when the inode is found.
     */
    bool findUDP4SocketByINode( ino_t inode, UDP4SocketInfo &info );

    /**
     * Dump a UDP4SocketInfo to stream.
     * @param os the stream to dump to.
     * @param inf the UDP4SocketInfo to dump.
     * @return reference to the stream.
     */
    std::ostream& operator<<( std::ostream& os, const UDP4SocketInfo &inf );

    /**
     * Network device statistics as in /proc/net/dev.
     */
    struct NetDeviceStat {
      std::string device;               /**< the name of the device. */
      unsigned long rx_bytes;      /**< the number of bytes received. */
      unsigned long rx_packets;    /**< the number of packets received. */
      unsigned long rx_errors;     /**< the number of packets received in error. */
      unsigned long rx_dropped;    /**< the number of packets recived but dropped. */
      unsigned long rx_fifo;       /**< the number of fifo buffer errors. */
      unsigned long rx_frame;      /**< the number of packet framing errors. */
      unsigned long rx_compressed; /**< the number of compressed packets received. */
      unsigned long rx_multicast;  /**< the number of multicast packets received. */

      unsigned long tx_bytes;      /**< the number of bytes transmitted. */
      unsigned long tx_packets;    /**< the number of packets transmitted. */
      unsigned long tx_errors;     /**< the number of transmit errors. */
      unsigned long tx_dropped;    /**< the number of packets dropped instead of transmitted. */
      unsigned long tx_fifo;       /**< the number of transmit fifo buffer errors. */
      unsigned long tx_collisions; /**< the number of transmit collisions. */
      unsigned long tx_carrier;    /**< the number of carrier losses during transmit. */
      unsigned long tx_compressed; /**< the number of compressed packets transmitted. */
    };

    /**
     * Compare two NetDeviceStat structs.
     * Criterium is bytes send+received, devicename.
     * @param n1 the first NetDeviceStat.
     * @param n2 the second NetDeviceStat.
     * @return 1 if n1 < n2.
     */
    inline int operator<( const NetDeviceStat& n1, const NetDeviceStat &n2 ) {
      return (n1.rx_bytes + n1.tx_bytes > n2.rx_bytes + n2.tx_bytes) ||
             (n1.rx_bytes + n1.tx_bytes == n2.rx_bytes + n2.tx_bytes && n1.device < n2.device );
    }

    /**
     * A map of network device name to NetDeviceStat statistics.
     */
    typedef std::map<std::string,NetDeviceStat> NetDeviceStatDeviceMap;

    /**
     * A vector of NetDeviceStat objects.
     */
    typedef std::vector<NetDeviceStat> NetDeviceStatDeviceVector;

    /**
     * Get network device statistics from /proc/net/dev.
     * @param stats the NetDeviceStatDeviceMap to fill with /proc/net/dev contents.
     */
    void getNetDeviceStat( NetDeviceStatDeviceMap &stats );

    /**
     * Get the delta of two NetDeviceStatDeviceMap objects, sorted
     * by using int operator<( const NetDeviceStat& n1, const NetDeviceStat &n2 ).
     * @param snap1 first NetDeviceStatDeviceMap.
     * @param snap2 second NetDeviceStatDeviceMap.
     * @param delta the NetDeviceStatDeviceVector to fill.
     */
    void getNetDeviceStatDelta( const NetDeviceStatDeviceMap& snap1, const NetDeviceStatDeviceMap& snap2, NetDeviceStatDeviceVector& delta );

      /**
       * Utility structure to key TCP connections. by (ip,port,user).
       * @internal
       */
      class TCPKey {
        public:
          /**
           * Default constructor.
           */
          TCPKey() { ip_ = ""; port_ = 0; uid_ = 0; };

          /**
           * Assignment constructor.
           */
          TCPKey( std::string ip, unsigned int port, unsigned int uid ) { ip_ = ip; port_ = port; uid_ = uid; };

          /**
           * Copy constructor.
           */
          TCPKey( const TCPKey &key ) { ip_ = key.ip_; port_ = key.port_; uid_ = key.uid_; };

          /**
           * compare two TCPKey objects.
           * @param s the TCPKey to compare to.
           * @return true if *this < s.
           */
          bool operator<( const TCPKey& s ) const { return ip_ < s.ip_ || (ip_ == s.ip_ && port_ < s.port_ ) || (ip_ == s.ip_ && port_ == s.port_ && uid_ < s.uid_); };

          std::string getIP() const { return ip_; };

          unsigned int getPort() const { return port_; };

          unsigned int getUID() const { return uid_; };

        private:
          /** the local IP used by the server process. */
          std::string ip_;

          /** the TCP port used by the server process. */
          unsigned int port_;

          /** the uid_ running the server process. */
          unsigned int uid_;
      };

      /**
       * TCP TCPKeyCounter status.
       */
      class TCPKeyCounter {
        public:
          /**
           * Constructor.
           * @param key the TCPKey.
           * @param esta the number of establised connections.
           */
          TCPKeyCounter( const TCPKey& key, unsigned int esta ) { key_ = key; esta_ = esta; };

          /**
           * Compare TCPKeyCounter entries on number of establised connections.
           * @param s TCPKeyCounter to compare to.
           * @return true if *this  < s.
           */
          bool operator<( const TCPKeyCounter& s ) const { return esta_ > s.esta_; };

          TCPKey getKey() const { return key_; };

          unsigned int getEsta() const { return esta_; };

        private:
          /** TCPKey of the connection. */
          TCPKey key_;
          /** number of established connections. */
          unsigned int esta_;
      };

      /**
       * Return the number of established TCP connections grouped by (ip,port,uid).
       * @param servers receives the server (local ip) TCPKeyCounter list.
       * @param clients receives the client (remote ip) TCPKeyCounter list.
       */
      void getTCPConnectionCounters( std::list<TCPKeyCounter> &servers, std::list<TCPKeyCounter> &clients );
      
      typedef enum {
         SyncookiesSent = 0,
         SyncookiesRecv,
         SyncookiesFailed,
         EmbryonicRsts,
         PruneCalled,
         RcvPruned,
         OfoPruned,
         OutOfWindowIcmps,
         LockDroppedIcmps,
         ArpFilter,
         TW,
         TWRecycled,
         TWKilled,
         PAWSActive,
         PAWSEstab,
         DelayedACKs,
         DelayedACKLocked,
         DelayedACKLost,
         ListenOverflows,
         ListenDrops,
         TCPHPHits,
         TCPPureAcks,
         TCPHPAcks,
         TCPRenoRecovery,
         TCPSackRecovery,
         TCPSACKReneging,
         TCPSACKReorder,
         TCPRenoReorder,
         TCPTSReorder,
         TCPFullUndo,
         TCPPartialUndo,
         TCPDSACKUndo,
         TCPLossUndo,
         TCPLostRetransmit,
         TCPRenoFailures,
         TCPSackFailures,
         TCPLossFailures,
         TCPFastRetrans,
         TCPSlowStartRetrans,
         TCPTimeouts,
         TCPLossProbes,
         TCPLossProbeRecovery,
         TCPRenoRecoveryFail,
         TCPSackRecoveryFail,
         TCPRcvCollapsed,
         TCPBacklogCoalesce,
         TCPDSACKOldSent,
         TCPDSACKOfoSent,
         TCPDSACKRecv,
         TCPDSACKOfoRecv,
         TCPAbortOnData,
         TCPAbortOnClose,
         TCPAbortOnMemory,
         TCPAbortOnTimeout,
         TCPAbortOnLinger,
         TCPAbortFailed,
         TCPMemoryPressures,
         TCPMemoryPressuresChrono,
         TCPSACKDiscard,
         TCPDSACKIgnoredOld,
         TCPDSACKIgnoredNoUndo,
         TCPSpuriousRTOs,
         TCPMD5NotFound,
         TCPMD5Unexpected,
         TCPMD5Failure,
         TCPSackShifted,
         TCPSackMerged,
         TCPSackShiftFallback,
         TCPBacklogDrop,
         PFMemallocDrop,
         TCPMinTTLDrop,
         TCPDeferAcceptDrop,
         IPReversePathFilter,
         TCPTimeWaitOverflow,
         TCPReqQFullDoCookies,
         TCPReqQFullDrop,
         TCPRetransFail,
         TCPRcvCoalesce,
         TCPOFOQueue,
         TCPOFODrop,
         TCPOFOMerge,
         TCPChallengeACK,
         TCPSYNChallenge,
         TCPFastOpenActive,
         TCPFastOpenActiveFail,
         TCPFastOpenPassive,
         TCPFastOpenPassiveFail,
         TCPFastOpenListenOverflow,
         TCPFastOpenCookieReqd,
         TCPFastOpenBlackhole,
         TCPSpuriousRtxHostQueues,
         BusyPollRxPackets,
         TCPAutoCorking,
         TCPFromZeroWindowAdv,
         TCPToZeroWindowAdv,
         TCPWantZeroWindowAdv,
         TCPSynRetrans,
         TCPOrigDataSent,
         TCPHystartTrainDetect,
         TCPHystartTrainCwnd,
         TCPHystartDelayDetect,
         TCPHystartDelayCwnd,
         TCPACKSkippedSynRecv,
         TCPACKSkippedPAWS,
         TCPACKSkippedSeq,
         TCPACKSkippedFinWait2,
         TCPACKSkippedTimeWait,
         TCPACKSkippedChallenge,
         TCPWinProbe,
         TCPKeepAlive,
         TCPMTUPFail,
         TCPMTUPSuccess,
         TCPDelivered,
         TCPDeliveredCE,
         TCPAckCompressed,
         TCPZeroWindowDrop,
         TCPRcvQDrop,
         TCPWqueueTooBig,
         TCPFastOpenPassiveAltKey,
         TcpTimeoutRehash,
         TcpDuplicateDataRehash,
         TCPDSACKRecvSegs,
         TCPDSACKIgnoredDubious,
         TCPMigrateReqSuccess,
         TCPMigrateReqFailure,
      } ProcNetDeviceStatEnums;
      
      extern std::vector<std::string> ProcNetDeviceStatNames;
      
      
      
      /**
       * @brief Info from /proc/net/netstat and /proc/net/snmp
       * 
       */
      struct TCPStat {
        
        // /proc/net/snmp
        long RtoAlgorithm;
        long RtoMin;
        long RtoMax;
        long MaxConn;
        long ActiveOpens;
        long PassiveOpens;
        long AttemptFails;
        long EstabResets;
        long CurrEstab;
        long InSegs;
        long OutSegs;
        long RetransSegs;
        long InErrs;
        long OutRsts;
        long InCsumErrors;
        
        // /proc/net/netstat
        long SyncookiesSent;
        long SyncookiesRecv;
        long SyncookiesFailed;
        long EmbryonicRsts;
        long PruneCalled;
        long RcvPruned;
        long OfoPruned;
        long OutOfWindowIcmps;
        long LockDroppedIcmps;
        long ArpFilter;
        long TW;
        long TWRecycled;
        long TWKilled;
        long PAWSActive;
        long PAWSEstab;
        long DelayedACKs;
        long DelayedACKLocked;
        long DelayedACKLost;
        long ListenOverflows;
        long ListenDrops;
        long TCPHPHits;
        long TCPPureAcks;
        long TCPHPAcks;
        long TCPRenoRecovery;
        long TCPSackRecovery;
        long TCPSACKReneging;
        long TCPSACKReorder;
        long TCPRenoReorder;
        long TCPTSReorder;
        long TCPFullUndo;
        long TCPPartialUndo;
        long TCPDSACKUndo;
        long TCPLossUndo;
        long TCPLostRetransmit;
        long TCPRenoFailures;
        long TCPSackFailures;
        long TCPLossFailures;
        long TCPFastRetrans;
        long TCPSlowStartRetrans;
        long TCPTimeouts;
        long TCPLossProbes;
        long TCPLossProbeRecovery;
        long TCPRenoRecoveryFail;
        long TCPSackRecoveryFail;
        long TCPRcvCollapsed;
        long TCPBacklogCoalesce;
        long TCPDSACKOldSent;
        long TCPDSACKOfoSent;
        long TCPDSACKRecv;
        long TCPDSACKOfoRecv;
        long TCPAbortOnData;
        long TCPAbortOnClose;
        long TCPAbortOnMemory;
        long TCPAbortOnTimeout;
        long TCPAbortOnLinger;
        long TCPAbortFailed;
        long TCPMemoryPressures;
        long TCPMemoryPressuresChrono;
        long TCPSACKDiscard;
        long TCPDSACKIgnoredOld;
        long TCPDSACKIgnoredNoUndo;
        long TCPSpuriousRTOs;
        long TCPMD5NotFound;
        long TCPMD5Unexpected;
        long TCPMD5Failure;
        long TCPSackShifted;
        long TCPSackMerged;
        long TCPSackShiftFallback;
        long TCPBacklogDrop;
        long PFMemallocDrop;
        long TCPMinTTLDrop;
        long TCPDeferAcceptDrop;
        long IPReversePathFilter;
        long TCPTimeWaitOverflow;
        long TCPReqQFullDoCookies;
        long TCPReqQFullDrop;
        long TCPRetransFail;
        long TCPRcvCoalesce;
        long TCPOFOQueue;
        long TCPOFODrop;
        long TCPOFOMerge;
        long TCPChallengeACK;
        long TCPSYNChallenge;
        long TCPFastOpenActive;
        long TCPFastOpenActiveFail;
        long TCPFastOpenPassive;
        long TCPFastOpenPassiveFail;
        long TCPFastOpenListenOverflow;
        long TCPFastOpenCookieReqd;
        long TCPFastOpenBlackhole;
        long TCPSpuriousRtxHostQueues;
        long BusyPollRxPackets;
        long TCPAutoCorking;
        long TCPFromZeroWindowAdv;
        long TCPToZeroWindowAdv;
        long TCPWantZeroWindowAdv;
        long TCPSynRetrans;
        long TCPOrigDataSent;
        long TCPHystartTrainDetect;
        long TCPHystartTrainCwnd;
        long TCPHystartDelayDetect;
        long TCPHystartDelayCwnd;
        long TCPACKSkippedSynRecv;
        long TCPACKSkippedPAWS;
        long TCPACKSkippedSeq;
        long TCPACKSkippedFinWait2;
        long TCPACKSkippedTimeWait;
        long TCPACKSkippedChallenge;
        long TCPWinProbe;
        long TCPKeepAlive;
        long TCPMTUPFail;
        long TCPMTUPSuccess;
        long TCPDelivered;
        long TCPDeliveredCE;
        long TCPAckCompressed;
        long TCPZeroWindowDrop;
        long TCPRcvQDrop;
        long TCPWqueueTooBig;
        long TCPFastOpenPassiveAltKey;
        long TcpTimeoutRehash;
        long TcpDuplicateDataRehash;
        long TCPDSACKRecvSegs;
        long TCPDSACKIgnoredDubious;
        long TCPMigrateReqSuccess;
        long TCPMigrateReqFailure;
      };     
      
      void discoverTCPStat();
      
      void getTCPStatNetstat( TCPStat &stats );
      
      void getTCPStatSnmp( TCPStat &stats );

  }
}

#endif
