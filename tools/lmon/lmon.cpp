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
 * ncurses based real time linux performance monitoring tool - c++ source file.
 * entry point, exception handling, main loop, help.
 */
#include <iostream>
#include <iomanip>
#include <sstream>

#include "system.hpp"
#include "util.hpp"
#include "lmon_curses.hpp"
#include "leanux-config.hpp"
#include "configfile.hpp"
#include "lard-config.hpp"

#include <signal.h>
#include <sys/time.h>
#include <algorithm>
#include <string.h>

using namespace leanux;

/**
 * The global Screen.
 */
tools::lmon::Screen *screen = 0;

/**
 * handle signals such a CTRL-C.
 */
void  sig_handler(int sig) {
  signal(sig, SIG_IGN);
  if ( screen ) screen->Stop();
}

void printHelp() {
  std::cout << "lmon - Linux performance viewer." << std::endl;
  std::cout << "usage: lmon [OPTIONS]" << std::endl;
  std::cout << "  -b      : browse historic data from system lard database " << LARD_SYSDB_FILE << " (if present)" << std::endl;
  std::cout << "  -f file : browse historic data from a lard database file" << std::endl;
  std::cout << "  -g      : (re)write the configuration file with default values" << std::endl;
  std::cout << "  -h      : show this help" << std::endl;
  std::cout << "  -v      : show version" << std::endl;
  std::cout << std::endl;
}

/**
 * lmon entry point.
 * @param argc number of arguments in argv.
 * @param argv arguments.
 * @return 0 on success, 1 on Oops, 2 on other errors.
 */
int main( int argc, char* argv[] ) {

  #ifdef LEANUX_DEBUG
  util::Tracer::setTracer( "/tmp/lmon.trc" );
  #endif

  char* cur_term = getenv("TERM");
  std::stringstream s;
  s << "initial TERM value=" << cur_term;
  TRACE( s );

  signal( SIGINT, sig_handler );
  screen = 0;
  try {
    leanux::init();

    leanux::util::ConfigFile::setDefault( "COLOR_BACKGROUND", "(10,10,12)", "Color for background (only effective for 256color terminals)" );
    leanux::util::ConfigFile::setDefault( "COLOR_TEXT", "(80,110,160)", "Color for text (only effective for 256color terminals)" );
    leanux::util::ConfigFile::setDefault( "COLOR_BOLD_TEXT", "(110,130,170)", "Color for bold text (only effective for 256color terminals)" );
    leanux::util::ConfigFile::setDefault( "COLOR_LINE", "(50,50,53)", "Color for lines (only effective for 256color terminals)" );
    leanux::util::ConfigFile::setDefault( "COLOR_RUNNING_PROC", "(1,234,109)", "Color for processes in R state (only effective for 256color terminals)" );
    leanux::util::ConfigFile::setDefault( "COLOR_BLOCKED_PROC", "(239,40,40)", "Color for processes in D state (only effective for 256color terminals)" );
    leanux::util::ConfigFile::setDefault( "COLOR_USER_CPU", "(200,234,109)", "Color for user mode CPU (only effective for 256color terminals)" );
    leanux::util::ConfigFile::setDefault( "COLOR_NICE_CPU", "(70,234,70)", "Color for nice mode CPU (only effective for 256color terminals)" );
    leanux::util::ConfigFile::setDefault( "COLOR_SYSTEM_CPU", "(237,89,29)", "Color for system mode CPU (only effective for 256color terminals)" );
    leanux::util::ConfigFile::setDefault( "COLOR_WAIT_CPU", "(239,40,40)", "Color for wait mode CPU (only effective for 256color terminals)" );
    leanux::util::ConfigFile::setDefault( "COLOR_IRQ_CPU", "(49,130,253)", "Color for irq mode CPU (only effective for 256color terminals)" );
    leanux::util::ConfigFile::setDefault( "COLOR_SOFTIRQ_CPU", "(117,164,234)", "Color for softirq mode CPU (only effective for 256color terminals)" );
    leanux::util::ConfigFile::setDefault( "SAMPLE_INTERVAL", "4", "seconds between samples" );
    leanux::util::ConfigFile::setDefault( "MAX_MOUNTPOINT_WIDTH", "22", "maximum characters used by a mountpoint" );

    leanux::util::ConfigFile::setConfig( "lmon", leanux::util::getUserConfigDir() + "/.leanux-lmon" );

    if ( argc > 1 ) {
      if ( strncmp( argv[1], "-h", 2 ) == 0 ) {
        printHelp();
      } else if ( strncmp( argv[1], "-v", 2 ) == 0 ) {
        std::cout <<  LEANUX_VERSION << std::endl;
      } else if ( strncmp( argv[1], "-g", 2 ) == 0 ) {
        leanux::util::ConfigFile::getConfig()->write(true);
      } else if ( strncmp( argv[1], "-f", 2 ) == 0 ) {
        if ( argc > 2 ) {
          if ( !util::fileReadAccess( argv[2] ) ) {
            std::cerr << "database file '" << argv[2] << "' cannot be read" << std::endl;
            return 1;
          } else {
            screen = new tools::lmon::Screen();
            screen->runHistory( argv[2] );
            delete screen;
          }
        } else printHelp();
      } else if ( strncmp( argv[1], "-b", 2 ) == 0 ) {
        if ( !util::fileReadAccess( LARD_SYSDB_FILE ) ) {
          std::cerr << "database file '" << LARD_SYSDB_FILE << "' cannot be read" << std::endl;
          return 1;
        } else {
          screen = new tools::lmon::Screen();
          screen->runHistory( LARD_SYSDB_FILE );
          delete screen;
        }
      } else printHelp();
    } else {

      screen = new tools::lmon::Screen();
      screen->runRealtime();
      delete screen;
    }
  }
  catch ( Oops &oops ) {
    try {
      if ( screen ) {
        screen->Stop();
        delete screen;
        std::cerr << "Oops:" << oops.getMessage() << std::endl;
        std::flush( std::cerr );
        screen = 0;
      }
    }
    catch ( ... ) {
      std::cerr << "unhandled exception during handling of Oops: " << oops.getMessage() << std::endl;
    }
    return 1;
  }
  catch ( ... ) {
    std::cerr << "unhandled exception" << std::endl;
    std::flush( std::cerr );
    if ( screen ) {
      screen->Stop();
      delete screen;
      screen = 0;
    }
    return 2;
  }

  return 0;
}
