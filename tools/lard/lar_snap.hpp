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

#ifndef LEANUX_LAR_SNAP
#define LEANUX_LAR_SNAP

#include <time.h>
#include "block.hpp"
#include "cpu.hpp"
#include "net.hpp"
#include "process.hpp"
#include "vmem.hpp"
#include "persist.hpp"

using namespace leanux;

void sysLog( unsigned short level, unsigned short limit, const std::string &msg );

const unsigned short LOG_ERR = 0;
const unsigned short LOG_WARN = 1;
const unsigned short LOG_STAT = 2;
const unsigned short LOG_INFO = 3;
const unsigned short LOG_DEBUG = 4;

typedef long snapid;

class Snapshot {
  public:
    Snapshot() {};
    virtual ~Snapshot() {};
    virtual void startSnap() = 0;
    virtual void stopSnap() = 0;
    virtual long storeSnap( const persist::Database &db, long snapid, double seconds ) = 0;
  protected:
};

class TimeSnap: public Snapshot {
  public:
    TimeSnap() : Snapshot() { istart_ = 0; istop_ = 0; };
    virtual ~TimeSnap() {};
    virtual void startSnap() { istart_ = time(0); };
    virtual void stopSnap()  { istop_ = time(0); };
    virtual long storeSnap( const persist::Database &db, long snapid, double seconds );

    long getSeconds() const { return istop_ - istart_; };
  protected:
    time_t istart_;
    time_t istop_;
};

class IOSnap : public Snapshot {
  public:
    IOSnap() : Snapshot() {};
    virtual ~IOSnap() {};

    virtual void startSnap();
    virtual void stopSnap();
    virtual long storeSnap( const persist::Database &db, long snapid, double seconds );

  protected:
    block::DeviceStatsMap stat1_;
    block::DeviceStatsMap stat2_;
};

class CPUSnap : public Snapshot {
  public:
    CPUSnap() : Snapshot() {};
    virtual ~CPUSnap() {};

    virtual void startSnap();
    virtual void stopSnap();
    virtual long storeSnap( const persist::Database &db, long snapid, double seconds );
  protected:
    cpu::CPUStatsMap stat1_;
    cpu::CPUStatsMap stat2_;

};

class SchedSnap : public Snapshot {
  public:
    SchedSnap() : Snapshot() {};
    virtual ~SchedSnap() {};

    virtual void startSnap();
    virtual void stopSnap();
    virtual long storeSnap( const persist::Database &db, long snapid, double seconds );
  protected:
    cpu::SchedInfo sched1_;
    cpu::SchedInfo sched2_;
    cpu::LoadAvg load1_;
    cpu::LoadAvg load2_;

};

class NetSnap : public Snapshot {
  public:
    NetSnap() : Snapshot() {};
    virtual ~NetSnap() {};

    virtual void startSnap();
    virtual void stopSnap();
    virtual long storeSnap( const persist::Database &db, long snapid, double seconds );
  protected:
    net::NetStatDeviceMap stat1_;
    net::NetStatDeviceMap stat2_;
};

class VMSnap : public Snapshot {
  public:
    VMSnap() : Snapshot() {};
    virtual ~VMSnap() {};

    virtual void startSnap();
    virtual void stopSnap();
    virtual long storeSnap( const persist::Database &db, long snapid, double seconds );
  protected:
    vmem::VMStat stat1_;
    vmem::VMStat stat2_;
};

class ProcSnap : public Snapshot {
  public:
    ProcSnap() : Snapshot() {};
    virtual ~ProcSnap() {};

    virtual void startSnap();
    virtual void stopSnap();
    virtual long storeSnap( const persist::Database &db, long snapid, double seconds );
  protected:
    process::ProcPidStatMap snap1_;
    process::ProcPidStatMap snap2_;
};

class ResSnap : public Snapshot {
  public:
    ResSnap() : Snapshot() {};
    virtual ~ResSnap() {};

    virtual void startSnap();
    virtual void stopSnap();
    virtual long storeSnap( const persist::Database &db, long snapid, double seconds );
  protected:
};

class MountSnap : public Snapshot {
  public:
    MountSnap() : Snapshot() {};
    virtual ~MountSnap() {};

    virtual void startSnap();
    virtual void stopSnap();
    virtual long storeSnap( const persist::Database &db, long snapid, double seconds );
  protected:
    static std::map<std::string,block::MajorMinor> devicefilecache_;
    block::DeviceStatsMap stat1_;
    block::DeviceStatsMap stat2_;
    std::map<std::string,unsigned long> fsbytes1_;
    std::map<std::string,unsigned long> fsbytes2_;
};

class TCPEstaSnap : public Snapshot {
  public:
    TCPEstaSnap() : Snapshot() {};
    virtual ~TCPEstaSnap() {};

    virtual void startSnap() {};
    virtual void stopSnap() {};
    virtual long storeSnap( const persist::Database &db, long snapid, double seconds );
  protected:
};

#endif
