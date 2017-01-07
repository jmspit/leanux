#include <algorithm>
#include <string>
#include <string.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <block.hpp>
#include <system.hpp>

using namespace leanux::block;
using namespace std;

string zabbifyId( const string& id ) {
  string res = id;
  if ( res.substr(0,2) == "0x" ) {
    res = res.substr(2);
  }
  std::replace( res.begin(), res.end(), '\\', 'a' );
  std::replace( res.begin(), res.end(), '\'', 'b' );
  std::replace( res.begin(), res.end(), '\"', 'c' );
  std::replace( res.begin(), res.end(), '`', 'd' );
  std::replace( res.begin(), res.end(), '*', 'e' );
  std::replace( res.begin(), res.end(), '?', 'f' );
  std::replace( res.begin(), res.end(), '[', 'g' );
  std::replace( res.begin(), res.end(), ']', 'h' );
  std::replace( res.begin(), res.end(), '{', 'i' );
  std::replace( res.begin(), res.end(), '}', 'j' );
  std::replace( res.begin(), res.end(), '~', 'k' );
  std::replace( res.begin(), res.end(), '$', 'l' );
  std::replace( res.begin(), res.end(), '!', 'm' );
  std::replace( res.begin(), res.end(), '&', 'n' );
  std::replace( res.begin(), res.end(), ';', 'o' );
  std::replace( res.begin(), res.end(), '(', 'p' );
  std::replace( res.begin(), res.end(), ')', 'q' );
  std::replace( res.begin(), res.end(), '<', 'r' );
  std::replace( res.begin(), res.end(), '>', 's' );
  std::replace( res.begin(), res.end(), '|', 't' );
  std::replace( res.begin(), res.end(), '#', 'u' );
  std::replace( res.begin(), res.end(), '@', 'v' );
  return res;
}

MajorMinor findByZabbix( const string& zabid ) {
  list<MajorMinor> disks;
  leanux::block::enumWholeDisks( disks );
  for ( std::list<MajorMinor>::const_iterator i = disks.begin(); i != disks.end(); i++ ) {
    if ( zabbifyId(getDiskId(*i)) == zabid ) {
      return *i;
    }
  }
  return MajorMinor::invalid;
}

/**
 * generate the JSON zabbix expects.
 */
void discovery( ostream &json ) {
  list<MajorMinor> disks;
  leanux::block::enumWholeDisks( disks );
  json << "{" << endl;
  json << "  \"data\": [" << endl;
  for ( std::list<MajorMinor>::const_iterator i = disks.begin(); i != disks.end(); i++ ) {
    if ( getSize(*i) > 0 ) {
      json << "    {" << endl;
      json << "      \"{#DISK}\":\"" << zabbifyId( getDiskId(*i) ) << "\"," << endl;
      json << "      \"{#KNAME}\":\"" << (*i).getName() << "\"," << endl;
      json << "      \"{#UNAME}\":\"" << (*i).getDeviceFile() << "\"," << endl;
      json << "      \"{#MAJMIN}\":\"" << *i << "\"," << endl;
      json << "      \"{#HCTL}\":\"" << getSCSIHCTL(*i) << "\"," << endl;
      json << "      \"{#CLASS}\":\"" << getClassStr(*i) << "\"," << endl;
      json << "      \"{#MODEL}\":\"" << getModel(*i) << "\"," << endl;
      json << "      \"{#SECTOR}\":\"" << getSectorSize(*i) << "\"," << endl;
      json << "      \"{#GIBIBYTES}\":\"" << fixed << setprecision(0) << getSize(*i)/1024.0/1024.0/1024.0 << "\"" << endl;
      if ( (*i) == disks.back() ) json << "    }" << endl; else json << "    }," << endl;
    }
  }
  json << "  ]" << endl;
  json << "}" << endl;
}

void getReads( const string& zabid, ostream &output ) {
  MajorMinor m = findByZabbix(zabid);
  if ( m != MajorMinor::invalid ) {
    DeviceStats stats;
    getStats( m, stats );
    output << std::fixed << stats.reads;
  } else output << "-1";
  output << endl;
}

void getReadSectors( const string& zabid, ostream &output ) {
  MajorMinor m = findByZabbix(zabid);
  if ( m != MajorMinor::invalid ) {
    DeviceStats stats;
    getStats( m, stats );
    output << std::fixed << stats.read_sectors;
  } else output << "-1";
  output << endl;
}

void getWrites( const string& zabid, ostream &output ) {
  MajorMinor m = findByZabbix(zabid);
  if ( m != MajorMinor::invalid ) {
    DeviceStats stats;
    getStats( m, stats );
    output << std::fixed << stats.writes;
  } else output << "-1";
  output << endl;
}

void getWriteSectors( const string& zabid, ostream &output ) {
  MajorMinor m = findByZabbix(zabid);
  if ( m != MajorMinor::invalid ) {
    DeviceStats stats;
    getStats( m, stats );
    output << std::fixed << stats.write_sectors;
  } else output << "-1";
  output << endl;
}

void getIOs( const string& zabid, ostream &output ) {
  MajorMinor m = findByZabbix(zabid);
  if ( m != MajorMinor::invalid ) {
    DeviceStats stats;
    getStats( m, stats );
    output << std::fixed << stats.reads + stats.writes;
  } else output << "-1";
  output << endl;
}

void getIOms( const string& zabid, ostream &output ) {
  MajorMinor m = findByZabbix(zabid);
  if ( m != MajorMinor::invalid ) {
    DeviceStats stats;
    getStats( m, stats );
    output << std::fixed << stats.io_ms;
  } else output << "-1";
  output << endl;
}

void getReadms( const string& zabid, ostream &output ) {
  MajorMinor m = findByZabbix(zabid);
  if ( m != MajorMinor::invalid ) {
    DeviceStats stats;
    getStats( m, stats );
    output << std::fixed << stats.read_ms;
  } else output << "-1";
  output << endl;
}

void getWritems( const string& zabid, ostream &output ) {
  MajorMinor m = findByZabbix(zabid);
  if ( m != MajorMinor::invalid ) {
    DeviceStats stats;
    getStats( m, stats );
    output << std::fixed << stats.write_ms;
  } else output << "-1";
  output << endl;
}

int printHelp(int r) {
  cout << "GNU/Linux disk monitoring extension for Zabbix." << endl << endl;
  cout << "usage:" << endl << endl;
  cout << "labbix discovery" << endl;
  cout << "  disk discovery in json format" << endl;
  cout << "  the disk identifiers for the value retrieval commands below are" << endl <<
          "  returned in the field {#DISK}. The disk sector size is returned" << endl <<
          "  in the field #SECTOR." << endl << endl;
  cout << "labbix diskid reads" << endl;
  cout << "  total diskid reads" << endl << endl;
  cout << "labbix diskid writes" << endl;
  cout <<"  total diskid writes" << endl << endl;
  cout << "labbix diskid read_sectors" << endl;
  cout << "  total diskid read sectors" << endl << endl;
  cout << "labbix diskid write_sectors" << endl;
  cout << "  total diskid write sectors" << endl << endl;
  cout << "labbix diskid io_ms" << endl;
  cout << "  total diskid time performing io (milliseconds)" << endl << endl;
  cout << "labbix diskid ios" << endl;
  cout << "  total diskid io's (reads+writes)" << endl << endl;
  cout << "labbix diskid read_ms" << endl;
  cout << "  total diskid times performing read io (milliseconds)" << endl << endl;
  cout << "labbix diskid write_ms" << endl;
  cout << "  total diskid time performing write io (milliseconds)" << endl << endl;
  return r;
}


int main( int argc, char* argv[] ) {
  leanux::init();
  if ( argc > 1 ) {
    if ( strncmp( argv[1], "discovery", 9 ) == 0 ) {
      discovery( cout );
    } else {
      if ( argc > 2 ) {
        if ( strcmp( argv[2], "read_sectors" ) == 0 ) {
          getReadSectors( argv[1], cout );
        } else if ( strcmp( argv[2], "write_sectors" ) == 0 ) {
          getWriteSectors( argv[1], cout );
        } else if ( strcmp( argv[2], "reads" ) == 0 ) {
          getReads( argv[1], cout );
        } else if ( strcmp( argv[2], "writes" ) == 0 ) {
          getWrites( argv[1], cout );
        } else if ( strcmp( argv[2], "io_ms" ) == 0 ) {
          getIOms( argv[1], cout );
        } else if ( strcmp( argv[2], "ios" ) == 0 ) {
          getIOs( argv[1], cout );
        } else if ( strcmp( argv[2], "read_ms" ) == 0 ) {
          getReadms( argv[1], cout );
        } else if ( strcmp( argv[2], "write_ms" ) == 0 ) {
          getWritems( argv[1], cout );
        } else printHelp(1);
      } else return printHelp(1);
    }
  } else printHelp(1);
  return 0;
}
