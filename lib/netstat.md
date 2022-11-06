# TCP netstat

## TCP internals

| layer | terminology | max user data size | description |
|-------|-------------|-------------|-|
|application   | data        | none | say a JSON document |
|transport   | TCP segment | MSS = MDDS - TCPHdrLen | The segment may be up to MSS (max segment size) bytes. The MSS must at least be 536 bytes, and ither TCP direction may have its own MSS. Application layer data may be fragmented into multiple TCP segments |
|internet   | IP datagram | MDDS = MTU - IPHdrLen | The MDDS (maximum datagram data size) is the payload of an IP datagram, so exluding the IP header |
|data link | packet      | MTU | The MTU is the maximum transmission unit |

### Segment

### ack

### retransmit

When a peer sends data, a timer is set. If this times out before the send data is acknowledged, the data is retransmitted assuming it has been lost.

### windowing

A peer will ACK with its window size, advertising what size packet would fit in its RX buffer. As a 2 byte value, this has a limit of 65536, but *window scaling* scales this number.

## TCP quantities

### Errors

Quantities that represent unrecoverable errors.

| quantity | description |
|----------|-------------|
|OutOfWindowIcmps| ICMP packets dropped because they were out-of-window |
|PAWSEstab| packets rejected in established connections because of timestamp |
|PruneCalled| packets pruned from receive queue because of socket buffer overrun |
|TCPAbortOnClose| connections reset due to early user close |
|TCPAbortOnData| connections reset due to unexpected data |
|TCPAbortOnTimeout| connections aborted due to timeout |
|TCPLossFailures| timeouts in loss state |
|TCPSackFailures| ? |

### latency

Quantities that indicate performance loss


### recovery

Succesfull atemmpts to work around infra problems.


## All quantities

| quantity | category | description |
|----------|----------|-------------|
|DelayedACKLocked| latency | delayed acks further delayed because of locked socket |
|DelayedACKLost| ? | times quick ack mode was activated (?) |
|DelayedACKs| latency | delayed acks sent |
|IPReversePathFilter| ? | ? |
|OutOfWindowIcmps| error | ICMP packets dropped because they were out-of-window |
|PAWSEstab| error | packets rejected in established connections because of timestamp |
|PruneCalled| error | packets pruned from receive queue because of socket buffer overrun |
|TCPAbortOnClose| error | connections reset due to early user close |
|TCPAbortOnData| error | connections reset due to unexpected data |
|TCPAbortOnTimeout| error | connections aborted due to timeout |
|TCPAckCompressed| ? | ? |
|TCPACKSkippedSeq| ? | ? |
|TCPAutoCorking| ? | ? |
|TCPBacklogCoalesce| ? | ? |
|TCPChallengeACK| ? | ? |
|TCPDelivered| ? | ? |
|TCPDSACKIgnoredNoUndo| ? | ? |
|TCPDSACKOfoRecv| ? | ? |
|TCPDSACKOldSent| ? | ? |
|TCPDSACKRecv| ? | ? |
|TCPDSACKRecvSegs| ? | ? |
|TCPDSACKUndo| ? | ? |
|TcpDuplicateDataRehash| ? | ? |
|TCPFastOpenActive| ? | ? |
|TCPFastRetrans| recovery | fast retransmits |
|TCPFullUndo| recovery | congestion windows fully recovered without slow start |
|TCPHPAcks| optimization | predicted acknowledgments |
|TCPHPHits| optimization | packet headers predicted |
|TCPHystartTrainCwnd| ? | ? |
|TCPHystartTrainDetect| ? | ? |
|TCPKeepAlive| ? | ? |
|TCPLossFailures| error | timeouts in loss state |
|TCPLossProbeRecovery| ? | ? |
|TCPLossProbes| ? | ? |
|TCPLossUndo| recovery | congestion windows recovered without slow start after partial ack |
|TCPLostRetransmit| ? | ? |
|TCPOFOMerge| ? | ? |
|TCPOFOQueue| ? | ? |
|TCPOrigDataSent| ? | ? |
|TCPPureAcks| latency | acknowledgments not containing data payload received |
|TCPRcvCoalesce| latency | ? |
|TCPSackFailures| error | ? |
|TCPSackMerged| ? | ? |
|TCPSackRecovery| ? | ? |
|TCPSackRecoveryFail| ? | ? |
|TCPSACKReorder| recovery | detected reordering using SACK |
|TCPSackShifted| ? | ? |
|TCPSackShiftFallback| ? | ? |
|TCPSlowStartRetrans| recovery | retransmits in slow start |
|TCPSpuriousRTOs| ? | ? |
|TCPSpuriousRtxHostQueues| ? | ? |
|TCPSYNChallenge| ? | ? |
|TCPSynRetrans| ? | ? |
|TcpTimeoutRehash| ? | ? |
|TCPTimeouts| ? | ? |
|TCPTSReorder| recovery | detected reordering using time stamp |
|TCPWantZeroWindowAdv| ? | ? |
|TW| ? | TCP sockets finished time wait in fast timer |
