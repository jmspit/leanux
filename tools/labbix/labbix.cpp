#include <algorithm>
#include <string>
#include <string.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <block.hpp>
#include <system.hpp>

namespace leanux {
  namespace tools {
    namespace labbix {

      std::string zabbifyId( const std::string& id ) {
        std::string res = id;
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

      block::MajorMinor findByZabbix( const std::string& zabid ) {
        std::list<block::MajorMinor> disks;
        leanux::block::enumWholeDisks( disks );
        for ( std::list<block::MajorMinor>::const_iterator i = disks.begin(); i != disks.end(); i++ ) {
          if ( zabbifyId((*i).getDiskId()) == zabid ) {
            return *i;
          }
        }
        return block::MajorMinor::invalid;
      }

      /**
       * generate the JSON zabbix expects.
       */
      void discovery( std::ostream &json ) {
        std::list<block::MajorMinor> disks;
        leanux::block::enumWholeDisks( disks );
        json << "{" << std::endl;
        json << "  \"data\": [" << std::endl;
        for ( std::list<block::MajorMinor>::const_iterator i = disks.begin(); i != disks.end(); i++ ) {
          if ( (*i).getSize() > 0 ) {
            json << "    {" << std::endl;
            json << "      \"{#DISK}\":\"" << zabbifyId( (*i).getDiskId() ) << "\"," << std::endl;
            json << "      \"{#KNAME}\":\"" << (*i).getName() << "\"," << std::endl;
            json << "      \"{#UNAME}\":\"" << (*i).getDeviceFile() << "\"," << std::endl;
            json << "      \"{#MAJMIN}\":\"" << *i << "\"," << std::endl;
            json << "      \"{#HCTL}\":\"" << (*i).getSCSIHCTL() << "\"," << std::endl;
            json << "      \"{#CLASS}\":\"" << (*i).getClassStr() << "\"," << std::endl;
            json << "      \"{#MODEL}\":\"" << (*i).getModel() << "\"," << std::endl;
            json << "      \"{#SECTOR}\":\"" << (*i).getSectorSize() << "\"," << std::endl;
            json << "      \"{#GIBIBYTES}\":\"" << std::fixed << std::setprecision(0) << (*i).getSize()/1024.0/1024.0/1024.0 << "\"" << std::endl;
            if ( (*i) == disks.back() ) json << "    }" << std::endl; else json << "    }," << std::endl;
          }
        }
        json << "  ]" << std::endl;
        json << "}" << std::endl;
      }

      void getReads( const std::string& zabid, std::ostream &output ) {
        block::MajorMinor m = findByZabbix(zabid);
        if ( m != block::MajorMinor::invalid ) {
          block::DeviceStats stats;
          m.getStats( stats );
          output << std::fixed << stats.reads;
        } else output << "-1";
        output << std::endl;
      }

      void getReadSectors( const std::string& zabid, std::ostream &output ) {
        block::MajorMinor m = findByZabbix(zabid);
        if ( m != block::MajorMinor::invalid ) {
          block::DeviceStats stats;
          m.getStats( stats );
          output << std::fixed << stats.read_sectors;
        } else output << "-1";
        output << std::endl;
      }

      void getWrites( const std::string& zabid, std::ostream &output ) {
        block::MajorMinor m = findByZabbix(zabid);
        if ( m != block::MajorMinor::invalid ) {
          block::DeviceStats stats;
          m.getStats( stats );
          output << std::fixed << stats.writes;
        } else output << "-1";
        output << std::endl;
      }

      void getWriteSectors( const std::string& zabid, std::ostream &output ) {
        block::MajorMinor m = findByZabbix(zabid);
        if ( m != block::MajorMinor::invalid ) {
          block::DeviceStats stats;
          m.getStats( stats );
          output << std::fixed << stats.write_sectors;
        } else output << "-1";
        output << std::endl;
      }

      void getIOs( const std::string& zabid, std::ostream &output ) {
        block::MajorMinor m = findByZabbix(zabid);
        if ( m != block::MajorMinor::invalid ) {
          block::DeviceStats stats;
          m.getStats( stats );
          output << std::fixed << stats.reads + stats.writes;
        } else output << "-1";
        output << std::endl;
      }

      void getIOms( const std::string& zabid, std::ostream &output ) {
        block::MajorMinor m = findByZabbix(zabid);
        if ( m != block::MajorMinor::invalid ) {
          block::DeviceStats stats;
          m.getStats( stats );
          output << std::fixed << stats.io_ms;
        } else output << "-1";
        output << std::endl;
      }

      void getReadms( const std::string& zabid,std:: ostream &output ) {
        block::MajorMinor m = findByZabbix(zabid);
        if ( m != block::MajorMinor::invalid ) {
          block::DeviceStats stats;
          m.getStats( stats );
          output << std::fixed << stats.read_ms;
        } else output << "-1";
        output << std::endl;
      }

      void getWritems( const std::string& zabid, std::ostream &output ) {
        block::MajorMinor m = findByZabbix(zabid);
        if ( m != block::MajorMinor::invalid ) {
          block::DeviceStats stats;
          m.getStats( stats );
          output << std::fixed << stats.write_ms;
        } else output << "-1";
        output << std::endl;
      }

      int printHelp(int r) {
        std::cout << "GNU/Linux disk monitoring extension for Zabbix." << std::endl << std::endl;
        std::cout << "usage:" << std::endl << std::endl;
        std::cout << "labbix discovery" << std::endl;
        std::cout << "  disk discovery in json format" << std::endl;
        std::cout << "  the disk identifiers for the value retrieval commands below are" << std::endl <<
                "  returned in the field {#DISK}. The disk sector size is returned" << std::endl <<
                "  in the field #SECTOR." << std::endl << std::endl;
        std::cout << "labbix diskid reads" << std::endl;
        std::cout << "  total diskid reads" << std::endl << std::endl;
        std::cout << "labbix diskid writes" << std::endl;
        std::cout <<"  total diskid writes" << std::endl << std::endl;
        std::cout << "labbix diskid read_sectors" << std::endl;
        std::cout << "  total diskid read sectors" << std::endl << std::endl;
        std::cout << "labbix diskid write_sectors" << std::endl;
        std::cout << "  total diskid write sectors" << std::endl << std::endl;
        std::cout << "labbix diskid io_ms" << std::endl;
        std::cout << "  total diskid time performing io (milliseconds)" << std::endl << std::endl;
        std::cout << "labbix diskid ios" << std::endl;
        std::cout << "  total diskid io's (reads+writes)" << std::endl << std::endl;
        std::cout << "labbix diskid read_ms" << std::endl;
        std::cout << "  total diskid times performing read io (milliseconds)" << std::endl << std::endl;
        std::cout << "labbix diskid write_ms" << std::endl;
        std::cout << "  total diskid time performing write io (milliseconds)" << std::endl << std::endl;
        return r;
      }


      int main( int argc, char* argv[] ) {
        leanux::init();
        if ( argc > 1 ) {
          if ( strncmp( argv[1], "discovery", 9 ) == 0 ) {
            discovery( std::cout );
          } else {
            if ( argc > 2 ) {
              if ( strcmp( argv[2], "read_sectors" ) == 0 ) {
                getReadSectors( argv[1], std::cout );
              } else if ( strcmp( argv[2], "write_sectors" ) == 0 ) {
                getWriteSectors( argv[1], std::cout );
              } else if ( strcmp( argv[2], "reads" ) == 0 ) {
                getReads( argv[1], std::cout );
              } else if ( strcmp( argv[2], "writes" ) == 0 ) {
                getWrites( argv[1], std::cout );
              } else if ( strcmp( argv[2], "io_ms" ) == 0 ) {
                getIOms( argv[1], std::cout );
              } else if ( strcmp( argv[2], "ios" ) == 0 ) {
                getIOs( argv[1], std::cout );
              } else if ( strcmp( argv[2], "read_ms" ) == 0 ) {
                getReadms( argv[1], std::cout );
              } else if ( strcmp( argv[2], "write_ms" ) == 0 ) {
                getWritems( argv[1], std::cout );
              } else printHelp(1);
            } else return printHelp(1);
          }
        } else printHelp(1);
        return 0;
      }


    }; // namespace labbix
  }; // namespace tools
}; // namespace leanux

int main( int argc, char* argv[] ) {
  leanux::tools::labbix::main( argc, argv );
}
