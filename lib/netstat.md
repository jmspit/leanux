| quantity | category | description |
|----------|----------|-------------|
|DelayedACKLocked| latency | delayed acks further delayed because of locked socket |
|DelayedACKLost| ? | times quick ack mode was activated (eh?) |
|DelayedACKs| latency | delayed acks sent |
|OutOfWindowIcmps| error | ICMP packets dropped because they were out-of-window |
|PAWSEstab| error | packets rejected in established connections because of timestamp |
|PruneCalled| error | packets pruned from receive queue because of socket buffer overrun |
|TCPDSACKUndo| ? | ? |
|TCPFastRetrans| recovery | fast retransmits |
|TCPFullUndo| recovery | congestion windows fully recovered without slow start |
|TCPHPAcks| optimization | predicted acknowledgments |
|TCPHPHits| optimization | packet headers predicted |
|TCPLossFailures| error | timeouts in loss state |
|TCPLossUndo| recovery | congestion windows recovered without slow start after partial ack |
|TCPLostRetransmit| ? | ? |
|TCPPureAcks| latency | acknowledgments not containing data payload received |
|TCPSackFailures| error | ? |
|TCPSackRecovery| ? | ? |
|TCPSACKReorder| recovery | detected reordering using SACK |
|TCPSlowStartRetrans| recovery | retransmits in slow start |
|TCPTSReorder| recovery | detected reordering using time stamp |
|TW| ? | TCP sockets finished time wait in fast timer |


idx 40 token TCPTimeouts value 848              # 
idx 41 token TCPLossProbes value 1089
idx 42 token TCPLossProbeRecovery value 435
idx 44 token TCPSackRecoveryFail value 5
idx 46 token TCPBacklogCoalesce value 68471
idx 47 token TCPDSACKOldSent value 1597
idx 48 token TCPDSACKOfoSent value 2
idx 49 token TCPDSACKRecv value 62
idx 50 token TCPDSACKOfoRecv value 2
idx 51 token TCPAbortOnData value 601           # connections reset due to unexpected data
idx 52 token TCPAbortOnClose value 14           # connections reset due to early user close
idx 54 token TCPAbortOnTimeout value 3          # connections aborted due to timeout
idx 61 token TCPDSACKIgnoredNoUndo value 27
idx 62 token TCPSpuriousRTOs value 7
idx 66 token TCPSackShifted value 3
idx 67 token TCPSackMerged value 3
idx 68 token TCPSackShiftFallback value 243
idx 73 token IPReversePathFilter value 1
idx 78 token TCPRcvCoalesce value 1824274
idx 79 token TCPOFOQueue value 4140
idx 81 token TCPOFOMerge value 2
idx 82 token TCPChallengeACK value 7
idx 83 token TCPSYNChallenge value 7
idx 84 token TCPFastOpenActive value 1
idx 91 token TCPSpuriousRtxHostQueues value 104
idx 93 token TCPAutoCorking value 1268
idx 96 token TCPWantZeroWindowAdv value 12
idx 97 token TCPSynRetrans value 404
idx 98 token TCPOrigDataSent value 90116
idx 99 token TCPHystartTrainDetect value 7
idx 100 token TCPHystartTrainCwnd value 211
idx 105 token TCPACKSkippedSeq value 3
idx 110 token TCPKeepAlive value 33733
idx 113 token TCPDelivered value 91460
idx 115 token TCPAckCompressed value 2178
idx 120 token TcpTimeoutRehash value 848
idx 121 token TcpDuplicateDataRehash value 90
idx 122 token TCPDSACKRecvSegs value 64
