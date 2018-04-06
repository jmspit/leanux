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
 * lmon ncurses screen implementation.
 */
#include "lmon_curses.hpp"
#include "block.hpp"
#include "block.hpp"
#include "history.hpp"
#include "oops.hpp"
#include "persist.hpp"
#include "system.hpp"
#include "leanux-config.hpp"
#include "configfile.hpp"
#include "util.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <set>

#include <errno.h>
#include <math.h>
#include <string.h>

#include <termios.h>

namespace leanux {

  namespace tools {

    namespace lmon {

      /** Alert message id for significant amount of unused hugepages. */
      const unsigned int ALERT_HUGEPAGES_UNUSED = 1;

      /** Alert message id for significant and sustained amount of major faults. */
      const unsigned int ALERT_MAJORFAULTS = 2;
      const unsigned int MSG_DISTRO = 3;
      const unsigned int MSG_MACHINE = 4;
      const unsigned int MSG_CPUMODEL = 5;
      const unsigned int MSG_STORAGE = 6;
      const unsigned int HLP_QUIT = 60;
      const unsigned int HLP_BROWSE_ARROW = 61;
      const unsigned int HLP_BROWSE_HOUR = 62;
      const unsigned int HLP_BROWSE_DAY = 63;
      const unsigned int HLP_BROWSE_WEEK = 64;
      const unsigned int HLP_BROWSE_HOMEND = 65;

      Palette::Palette() {
      }

      void Palette::setup() {
        init_pair(1, COLOR_WHITE, COLOR_BLACK );
        idx_color_running_proc_ = 1;
        idx_color_text_ = 1;
        idx_color_bold_text_ = 1;
        idx_color_line_ = 1;
        idx_system_mode_cpu_ = 1;
        idx_user_mode_cpu_ = 1;
        idx_nice_mode_cpu_ = 1;
        idx_iowait_mode_cpu_ = 1;
        idx_color_blocked_proc_ = 1;
        idx_irq_mode_cpu_ = 1;
        idx_softirq_mode_cpu_ = 1;
        if ( has_colors() ) {
          #ifdef TRACEON
          std::stringstream s;
          s << "terminal has " << COLORS << " colors";
          TRACE( s );
          #endif
          if ( can_change_color() ) {
            TRACE( "terminal can change colors" );
            idx_color_text_ = 134;
            idx_color_bold_text_ = 135;
            idx_color_line_ = 136;
            idx_color_running_proc_ = 2;
            idx_system_mode_cpu_ = 13;
            idx_user_mode_cpu_ = 126;
            idx_nice_mode_cpu_ = 127;
            idx_iowait_mode_cpu_ = 128;
            idx_color_blocked_proc_ = 129;
            idx_irq_mode_cpu_ = 130;
            idx_softirq_mode_cpu_ = 131;

            leanux::util::ConfigFile::RGB rgb = leanux::util::ConfigFile::getConfig()->getRGBValue( "COLOR_BACKGROUND" );
            init_color(COLOR_BLACK, rgb.red*1000/255, rgb.green*1000/255, rgb.blue*1000/255 );

            rgb = leanux::util::ConfigFile::getConfig()->getRGBValue( "COLOR_TEXT" );
            init_color(idx_color_text_, rgb.red*1000/255, rgb.green*1000/255, rgb.blue*1000/255 );
            init_pair(idx_color_text_, idx_color_text_, COLOR_BLACK );

            rgb = leanux::util::ConfigFile::getConfig()->getRGBValue( "COLOR_BOLD_TEXT" );
            init_color(idx_color_bold_text_, rgb.red*1000/255, rgb.green*1000/255, rgb.blue*1000/255 );
            init_pair(idx_color_bold_text_, idx_color_bold_text_, COLOR_BLACK );

            rgb = leanux::util::ConfigFile::getConfig()->getRGBValue( "COLOR_LINE" );
            init_color(idx_color_line_, rgb.red*1000/255, rgb.green*1000/255, rgb.blue*1000/255 );
            init_pair(idx_color_line_, idx_color_line_, COLOR_BLACK );

            rgb = leanux::util::ConfigFile::getConfig()->getRGBValue( "COLOR_RUNNING_PROC" );
            init_color(idx_color_running_proc_, rgb.red*1000/255, rgb.green*1000/255, rgb.blue*1000/255 );
            init_pair(idx_color_running_proc_, idx_color_running_proc_, COLOR_BLACK );

            rgb = leanux::util::ConfigFile::getConfig()->getRGBValue( "COLOR_SYSTEM_CPU" );
            init_color(idx_system_mode_cpu_, rgb.red*1000/255, rgb.green*1000/255, rgb.blue*1000/255 );
            init_pair(idx_system_mode_cpu_, idx_system_mode_cpu_, COLOR_BLACK );

            rgb = leanux::util::ConfigFile::getConfig()->getRGBValue( "COLOR_USER_CPU" );
            init_color(idx_user_mode_cpu_, rgb.red*1000/255, rgb.green*1000/255, rgb.blue*1000/255 );
            init_pair(idx_user_mode_cpu_, idx_user_mode_cpu_, COLOR_BLACK );

            rgb = leanux::util::ConfigFile::getConfig()->getRGBValue( "COLOR_NICE_CPU" );
            init_color(idx_nice_mode_cpu_, rgb.red*1000/255, rgb.green*1000/255, rgb.blue*1000/255 );
            init_pair(idx_nice_mode_cpu_, idx_nice_mode_cpu_, COLOR_BLACK );

            rgb = leanux::util::ConfigFile::getConfig()->getRGBValue( "COLOR_WAIT_CPU" );
            init_color(idx_iowait_mode_cpu_, rgb.red*1000/255, rgb.green*1000/255, rgb.blue*1000/255 );
            init_pair(idx_iowait_mode_cpu_, idx_iowait_mode_cpu_, COLOR_BLACK );

            rgb = leanux::util::ConfigFile::getConfig()->getRGBValue( "COLOR_BLOCKED_PROC" );
            init_color(idx_color_blocked_proc_, rgb.red*1000/255, rgb.green*1000/255, rgb.blue*1000/255 );
            init_pair(idx_color_blocked_proc_, idx_color_blocked_proc_, COLOR_BLACK );

            rgb = leanux::util::ConfigFile::getConfig()->getRGBValue( "COLOR_IRQ_CPU" );
            init_color(idx_irq_mode_cpu_, rgb.red*1000/255, rgb.green*1000/255, rgb.blue*1000/255 );
            init_pair(idx_irq_mode_cpu_, idx_irq_mode_cpu_, COLOR_BLACK );

            rgb = leanux::util::ConfigFile::getConfig()->getRGBValue( "COLOR_SOFTIRQ_CPU" );
            init_color(idx_softirq_mode_cpu_, rgb.red*1000/255, rgb.green*1000/255, rgb.blue*1000/255 );
            init_pair(idx_softirq_mode_cpu_, idx_softirq_mode_cpu_, COLOR_BLACK );

          } else {
            idx_color_text_ = 1;          // WHITE
            idx_color_bold_text_ = 1;     // WHITE
            idx_color_line_ = 1;          // WHITE
            idx_color_running_proc_ = 2;  // GREEN
            idx_color_blocked_proc_ = 3;  // RED
            idx_system_mode_cpu_ = 3;     // RED
            idx_user_mode_cpu_ = 2;       // GREEN
            idx_nice_mode_cpu_ = 2;       // GREEN
            idx_iowait_mode_cpu_ = 3;     // RED
            idx_irq_mode_cpu_ = 4;        // BLUE
            idx_softirq_mode_cpu_ = 4;    // BLUE

            init_pair( idx_color_text_, COLOR_WHITE, COLOR_BLACK );
            init_pair( idx_color_bold_text_, COLOR_WHITE, COLOR_BLACK );
            init_pair( idx_color_line_, COLOR_WHITE, COLOR_BLACK );
            init_pair( idx_color_running_proc_, COLOR_GREEN, COLOR_BLACK );
            init_pair( idx_color_blocked_proc_, COLOR_RED, COLOR_BLACK );
            init_pair( idx_system_mode_cpu_, COLOR_RED, COLOR_BLACK );
            init_pair( idx_irq_mode_cpu_, COLOR_BLUE, COLOR_BLACK );
          }
        } else { // no color capability
          TRACE( "terminal does not support colors" );
        }
      }

      Screen::Screen() {
        stopped_ = false;
        sleep_duration_ms_ = 200;
        sample_interval_s_ = leanux::util::ConfigFile::getConfig()->getIntValue( "SAMPLE_INTERVAL" );
        footer_scroll_interval_s_ = 4;
        memset( &winsz_, 0, sizeof(winsz_) );
        vheader_ = 0;
        vfooter_ = 0;
        vsys_ = 0;
        vio_ = 0;
        vprocess_ = 0;
        vnetwork_ = 0;
        fd_.fd = 0; /* the file descriptor we passed to termkey_new() */
        fd_.events = POLLIN;

        tk_ = termkey_new(0, 0);
        initTerminal();
        screenResize();
        doupdate();
      }

      Screen::~Screen() {
        try {
          leanux::util::ConfigFile::getConfig()->read();
          leanux::util::ConfigFile::getConfig()->setValue( "SAMPLE_INTERVAL", sample_interval_s_ );
          leanux::util::ConfigFile::getConfig()->write();
        }
        catch ( ... ) {
        }
        delete vprocess_;
        delete vnetwork_;
        delete vio_;
        delete vsys_;
        delete vheader_;
        delete vfooter_;
        start_color();
        Screen::resetTerminal();
        termkey_destroy(tk_);
      }

      void Screen::initTerminal() {
        wmain_ = initscr();
        if ( has_colors() ) {
          start_color();
        };
        palette_.setup();
        curs_set(0);
        cbreak();
        noecho();
        timeout(0);
        leaveok( wmain_, TRUE );
        if ( ioctl( 0, TIOCGWINSZ, &winsz_ ) == -1 ) throw Oops( __FILE__, __LINE__, errno );
        resize_term(winsz_.ws_row,winsz_.ws_col);
        #ifdef TRACEON
        std::stringstream s;
        s << "initial window size col=" << winsz_.ws_col << " row=" << winsz_.ws_row << std::endl;
        TRACE( s );
        #endif
      }

      void Screen::screenResize() {

        const int idx_header = 0;
        const int idx_cpu = 1;
        const int idx_disk = 2;
        const int idx_net = 3;
        const int idx_proc = 4;
        const int idx_foot = 5;
        int lines[6];
        lines[idx_header] = Header::getMinHeight();
        lines[idx_cpu] = SysView::getMinHeight();
        lines[idx_disk] = IOView::getMinHeight();
        lines[idx_net] = NetView::getMinHeight();
        lines[idx_proc] = ProcessView::getMinHeight();
        lines[idx_foot] = Footer::getMinHeight();
        int surplus = winsz_.ws_row;
        for ( int i = 0; i < 6; i++ ) surplus -= lines[i];
        int cur_idx = idx_header;
        while ( surplus > 0 ) {
          switch ( cur_idx ) {
            case idx_header:
              if ( lines[idx_header] < Header::getOptimalHeight() ) {
                lines[idx_header]++;
                surplus--;
              }
              break;
            case idx_cpu:
              if ( lines[idx_cpu] < SysView::getOptimalHeight() ) {
                lines[idx_cpu]++;
                surplus--;
              }
              break;
            case idx_disk:
              if ( lines[idx_disk] < IOView::getOptimalHeight() ) {
                lines[idx_disk]++;
                surplus--;
              }
              break;
            case idx_net:
              if ( lines[idx_net] < NetView::getOptimalHeight() ) {
                lines[idx_net]++;
                surplus--;
              }
              break;
            case idx_proc:
              if ( lines[idx_proc] < ProcessView::getOptimalHeight() ) {
                lines[idx_proc]++;
                surplus--;
              }
              break;
            case idx_foot:
              if ( lines[idx_foot] < Footer::getOptimalHeight() ) {
                lines[idx_foot]++;
                surplus--;
              }
              break;
          }
          cur_idx++;
          if ( cur_idx >= 6 ) cur_idx = 0;
        }

        #ifdef TRACEON
        std::stringstream s;
        s.str("");
        s << "lines=" << lines[idx_header] << " "
          << lines[idx_cpu] << " "
          << lines[idx_disk] << " "
          << lines[idx_net] << " "
          << lines[idx_proc] << " "
          << lines[idx_foot] << " total "
          << lines[idx_header]+lines[idx_cpu]+lines[idx_disk]+lines[idx_cpu]+lines[idx_net] + lines[idx_proc] + lines[idx_foot]<< std::endl;
        TRACE( s );
        #endif

        if ( !vheader_ )
          vheader_ = new Header( winsz_.ws_col, Header::getMinHeight(), 0, 0, this );
        else
          vheader_->resize( winsz_.ws_col, Header::getMinHeight(), 0, 0 );
        if ( !vsys_ )
          vsys_ = new SysView( winsz_.ws_col, lines[idx_cpu], 0, Header::getMinHeight(), this );
        else
          vsys_->resize( winsz_.ws_col, lines[idx_cpu], 0, lines[idx_header] );
        if ( !vio_ )
          vio_ = new IOView( winsz_.ws_col, lines[idx_disk], 0, lines[idx_header] + lines[idx_cpu], this );
        else
          vio_->resize( winsz_.ws_col, lines[idx_disk], 0, lines[idx_header] + lines[idx_cpu] );
        if ( !vnetwork_ )
          vnetwork_ = new NetView( winsz_.ws_col, lines[idx_net], 0, lines[idx_header] + lines[idx_cpu] + lines[idx_disk], this );
        else
          vnetwork_->resize( winsz_.ws_col, lines[idx_net], 0, lines[idx_header] + lines[idx_cpu] + lines[idx_disk] );
        if ( !vprocess_ )
          vprocess_ = new ProcessView( winsz_.ws_col, lines[idx_proc], 0, lines[idx_header] + lines[idx_cpu] + lines[idx_disk] + lines[idx_net], this );
        else
          vprocess_->resize( winsz_.ws_col, lines[idx_proc], 0, lines[idx_header] + lines[idx_cpu] + lines[idx_disk] + lines[idx_net] );
        if ( !vfooter_ )
          vfooter_ = new Footer( winsz_.ws_col, Footer::getMinHeight(), 0, lines[idx_header] + lines[idx_cpu] + lines[idx_disk] + lines[idx_net] + lines[idx_proc], this );
        else {
          vfooter_->resize( winsz_.ws_col, Footer::getMinHeight(), 0, lines[idx_header] + lines[idx_cpu] + lines[idx_disk] + lines[idx_net] + lines[idx_proc] );
        }
      }

      void Screen::resetTerminal() {
        endwin();
        curs_set(1);
        nocbreak();
        echo();
      }

      void Screen::runHistory( persist::Database *db ) {
        reportMessage( HLP_QUIT, 0, "press q or ^C to quit" );
        reportMessage( HLP_BROWSE_ARROW, 0, "move back/forward leftarrow/rightarrow or ,/. " );
        reportMessage( HLP_BROWSE_HOUR, 0, "move back/forward one hour h/H" );
        reportMessage( HLP_BROWSE_DAY, 0, "move back/forward one day d/D" );
        reportMessage( HLP_BROWSE_WEEK, 0, "move back/forward one week w/W" );
        reportMessage( HLP_BROWSE_HOMEND, 0, "move to front/end of history HOME/END" );
        sleep_duration_ms_ = 20;

        time_t cur_zoom = 300;

        LardHistory history( db, cur_zoom );
        XSysView sysview;
        XIOView ioview;
        XNetView netview;
        XProcView procview;
        stopped_ = false;
        bool update_required = true;
        bool footer_refresh = true;
        struct timeval samplet1, samplet2, scrollt1, scrollt2, last_size_check;
        gettimeofday( &samplet1, 0 );
        gettimeofday( &samplet2, 0 );
        scrollt1 = samplet1;
        scrollt2 = samplet2;

        gettimeofday( &last_size_check, 0 );

        TermKeyResult ret;
        TermKeyKey key;
        int nextwait = 10;

        while ( !stopped_ ) {

          if ( poll( &fd_, 1, nextwait ) == 0 ) {
            // Timed out
            //if ( termkey_getkey_force( tk_, &key ) == TERMKEY_RES_KEY )
            //  has_key = true;
          }
          if( fd_.revents & ( POLLIN | POLLHUP | POLLERR ) )
            termkey_advisereadable( tk_ );

          // any recognized user input?
          if ( ( ret = termkey_getkey( tk_, &key ) ) == TERMKEY_RES_KEY ) {
            if ( key.code.codepoint == 'q' ) {
              stopped_ = true;
            } else if ( key.code.sym == 10 || key.code.codepoint == '.' ) { // RIGHT ARROW
              history.rangeUp();
              update_required = true;
            } else if ( key.code.sym == 9  || key.code.codepoint == ',' ) {  // LEFT ARROW
              history.rangeDown();
              update_required = true;
            } else if ( key.code.sym == 18 ) { // HOME key
              history.rangeStart();
              update_required = true;
            } else if ( key.code.sym == 19 ) { // END key
              history.rangeEnd();
              update_required = true;
            } else if ( key.code.codepoint == '-' ) {
              cur_zoom = history.zoomOut();
              update_required = true;
            } else if ( key.code.codepoint == '+' ) {
              cur_zoom = history.zoomIn();
              update_required = true;
            } else if ( key.code.codepoint == 'h' ) {
              history.hourDown();
              update_required = true;
            } else if ( key.code.codepoint == 'H' ) {
              history.hourUp();
              update_required = true;
            } else if ( key.code.codepoint == 'd' ) {
              history.dayDown();
              update_required = true;
            } else if ( key.code.codepoint == 'D' ) {
              history.dayUp();
              update_required = true;
            } else if ( key.code.codepoint == 'w' ) {
              history.weekDown();
              update_required = true;
            } else if ( key.code.codepoint == 'W' ) {
              history.weekUp();
              update_required = true;
            }

          }

          if ( update_required ) {
            history.fetchXSysView( sysview );
            history.fetchXIOView( ioview );
            history.fetchXNetView( netview );
            history.fetchXProcView( procview );
          }

          gettimeofday( &samplet2, 0 );
          if ( util::deltaTime( last_size_check, samplet2 ) >= 0.1 ) {
            last_size_check = samplet2;
            if ( sizeChanged() ) {
              if ( ioctl( 0, TIOCGWINSZ, &winsz_ ) == -1 ) throw Oops( __FILE__, __LINE__, errno );
              werase(wmain_);
              wrefresh(wmain_);
              resize_term(winsz_.ws_row,winsz_.ws_col);
              screenResize();
              update_required = true;
              footer_refresh = true;
              doupdate();
              refresh();

              #ifdef TRACEON
              std::stringstream s;
              s << "resized window size col=" << winsz_.ws_col << " row=" << winsz_.ws_row << " " << COLS << "/" << LINES << std::endl;
              TRACE( s );
              #endif
            }
          }

          gettimeofday( &scrollt2, 0 );
          if ( util::deltaTime( scrollt1, scrollt2 ) > footer_scroll_interval_s_ ) {
            scrollt1 = scrollt2;
            footer_refresh = true;
          }

          if ( update_required || footer_refresh ) {
            if ( update_required ) {
              std::stringstream ss;
              ss << "zoom " << util::TimeStrSec( util::deltaTime( sysview.t1, sysview.t2 ) ) << " "
                 //<< history.getStartSnap() << "/" << history.getEndSnap()
                 //<< " = "
                 << util::localStrISODateTime( history.getStartTime() )
                 << " - " << util::localStrISOTime( history.getEndTime() );
              ((Header*)vheader_)->xrefresh( "browse " + db->fileName(), ss.str() );
              ((SysView*)vsys_)->xrefresh( sysview, true );
              ((IOView*)vio_)->xrefresh( ioview );
              ((NetView*)vnetwork_)->xrefresh( netview );
              ((ProcessView*)vprocess_)->xrefresh( procview );
              update_required = false;
            }
            if ( footer_refresh ) {
              ((Footer*)vfooter_)->xrefresh();
              footer_refresh = false;
            }
            doupdate();
            // eat away all further input to prevent key/refresh queuing
            while ( ( ret = termkey_getkey( tk_, &key ) ) == TERMKEY_RES_KEY );
          }

          if ( !stopped_ ) util::Sleep( 0, (long int)(sleep_duration_ms_ * 1.0E6) );
        }
      }

      void Screen::runRealtime() {
        reportMessage( HLP_QUIT, 0, "press q or ^C to quit" );
        reportMessage( HLP_BROWSE_ARROW, 0, "increase/decrease sampling interval -/+" );
        RealtimeSampler realtimesampler;
        stopped_ = false;
        bool update_required = true;
        bool footer_refresh = true;
        struct timeval samplet1, samplet2, scrollt1, scrollt2, last_size_check;
        gettimeofday( &samplet1, 0 );
        gettimeofday( &samplet2, 0 );
        scrollt1 = samplet1;
        scrollt2 = samplet2;

        gettimeofday( &last_size_check, 0 );

        TermKeyResult ret;
        TermKeyKey key;
        int nextwait = 0;

        while ( !stopped_ ) {

          if ( poll( &fd_, 1, nextwait ) == 0 ) {
            // Timed out
            //if ( termkey_getkey_force( tk_, &key ) == TERMKEY_RES_KEY )
            //  has_key = true;
          }
          if( fd_.revents & ( POLLIN | POLLHUP | POLLERR ) )
            termkey_advisereadable( tk_ );

          // any recognized user input?
          if ( ( ret = termkey_getkey( tk_, &key ) ) == TERMKEY_RES_KEY ) {
            if ( key.code.codepoint == 'q' ) {
              stopped_ = true;
            } else if ( key.code.codepoint == '+' ) {
              sample_interval_s_ += 1;
              realtimesampler.resetCPUTrail();
              update_required = true;
            } else if ( key.code.codepoint == '-' ) {
              if ( sample_interval_s_ > 1 ) {
                sample_interval_s_ -= 1;
                realtimesampler.resetCPUTrail();
                update_required = true;
              }
            }
          }

          gettimeofday( &samplet2, 0 );
          if ( util::deltaTime( last_size_check, samplet2 ) >= 0.1 ) {
            last_size_check = samplet2;
            if ( sizeChanged() ) {
              if ( ioctl( 0, TIOCGWINSZ, &winsz_ ) == -1 ) throw Oops( __FILE__, __LINE__, errno );
              werase(wmain_);
              wrefresh(wmain_);
              resize_term(winsz_.ws_row,winsz_.ws_col);
              screenResize();
              update_required = true;
              footer_refresh = true;
              doupdate();
              refresh();

              #ifdef TRACEON
              std::stringstream s;
              s << "resized window size col=" << winsz_.ws_col << " row=" << winsz_.ws_row << " " << COLS << "/" << LINES << std::endl;
              TRACE( s );
              #endif
            }
          }
          if ( util::deltaTime( samplet1, samplet2 ) > sample_interval_s_ ) {
            samplet1 = samplet2;
            realtimesampler.sample( vsys_->getHeight() );
            update_required = true;
          }

          gettimeofday( &scrollt2, 0 );
          if ( util::deltaTime( scrollt1, scrollt2 ) > footer_scroll_interval_s_ ) {
            scrollt1 = scrollt2;
            footer_refresh = true;
          }

          if ( update_required || footer_refresh ) {
            if ( update_required ) {
              std::stringstream ss;
              ss << "each " << sample_interval_s_ << "s " << util::localStrISODateTime();
              ((Header*)vheader_)->xrefresh( "realtime", ss.str() );
              ((SysView*)vsys_)->xrefresh( realtimesampler.getXSysView(), false );
              ((IOView*)vio_)->xrefresh( realtimesampler.getXIOView() );
              ((NetView*)vnetwork_)->xrefresh( realtimesampler.getXNetView() );
              ((ProcessView*)vprocess_)->xrefresh( realtimesampler.getXProcView() );
              update_required = false;
            }
            if ( footer_refresh ) {
              ((Footer*)vfooter_)->xrefresh();
              footer_refresh = false;
            }
            doupdate();
          }

          if ( !stopped_ ) util::Sleep( 0, (long int)(sleep_duration_ms_ * 1.0E6) );
        }
      }

      bool Screen::sizeChanged() {
        struct winsize winsz;
        if ( ioctl( 0, TIOCGWINSZ, &winsz ) == -1 ) throw Oops( __FILE__, __LINE__, errno );
        if ( winsz.ws_row != winsz_.ws_row || winsz.ws_col != winsz_.ws_col ) {
          return true;
        } else return false;
      }

      void Screen::reportMessage( unsigned int key, unsigned int prio, const std::string & message ) {
        ((Footer*)vfooter_)->reportMessage( key, prio, message );
      }

      void Screen::clearMessage( unsigned int key ) {
        ((Footer*)vfooter_)->clearMessage( key );
      }


      View::View ( int width, int height, int x, int y, Screen* screen ) {
        sample_count_ = 0;
        screen_ = screen;
        width_ = width;
        height_ = height;
        absx_ = x;
        absy_ = y;
        window_ = newwin( height, width, y, x );
        if ( window_ == NULL ) throw Oops( __FILE__, __LINE__, "newwin failed" );
        attr_normal_text_ = COLOR_PAIR(screen_->palette_.getColorText() ) | A_NORMAL;
        attr_bold_text_ = COLOR_PAIR( screen_->palette_.getColorBoldText() ) | A_BOLD;
        attr_alert_text_ = COLOR_PAIR( screen_->palette_.getColorBlockedProc() ) | A_NORMAL;
        attr_plot_element_ = COLOR_PAIR( screen_->palette_.getColorRunningProc() ) | A_BOLD;
        attr_line_ = COLOR_PAIR( screen_->palette_.getColorLine() ) | A_BOLD;
      }

      void View::resize( int width, int height, int x, int y ) {
        delwin( window_ );
        width_ = width;
        height_ = height;
        absx_ = x;
        absy_ = y;
        window_ = newwin( height_, width_, absy_, absx_ );
        if ( window_ == NULL ) throw Oops( __FILE__, __LINE__, "newwin failed" );
      }

      View::~View() {
        delwin( window_ );
      }

      void View::box( int x1, int y1, int x2, int y2 ) {
        hLine( x1, x2, y1, attr_line_ );
        hLine( x1, x2, y2, attr_line_ );
        vLine( y1, y2, x1, attr_line_ );
        vLine( y1, y2, x2, attr_line_ );
        mvwaddch( window_, y1, x1, ACS_ULCORNER );
        mvwaddch( window_, y1, x2, ACS_URCORNER );
        mvwaddch( window_, y2, x1, ACS_LLCORNER );
        mvwaddch( window_, y2, x2, ACS_LRCORNER );
      }

      Header::Header( int width, int height, int x, int y, Screen* screen ) :
        View( width, height, x, y, screen ) {
        std::stringstream ss;
        ss << "lmon " << LEANUX_VERSION << " on " << system::getNodeName();
        header_ = ss.str();
      };

      void Header::xrefresh(  const std::string &caption, const std::string &msg ) {
        werase( window_ );
        std::stringstream ss;
        ss << header_ << " " << caption;
        textOut( 0, 0, attr_plot_element_, ss.str().substr( 0, width_ - 20 ) );

        textOut( width_ - msg.length() - 1, 0, attr_bold_text_, msg );
        wnoutrefresh( window_ );
      }

      Footer::Footer( int width, int height, int x, int y, Screen* screen ) :
        View( width, height, x, y, screen ) {
        msgidx_ = 0;

        attr_prio_0_ = COLOR_PAIR(screen_->palette_.getColorNiceCPU() ) | A_NORMAL;
        attr_prio_1_ = COLOR_PAIR( screen_->palette_.getColorSystemCPU() ) | A_NORMAL;
        attr_prio_2_ = COLOR_PAIR( screen_->palette_.getColorBlockedProc() ) | A_NORMAL;

        std::stringstream ss;
        system::Distribution dist;
        dist = system::getDistribution();
        ss << dist.release;
        ss << ", kernel " << system::getKernelVersion();
        ss << ", " << system::getArchitecture();
        reportMessage( MSG_DISTRO, 0, ss.str() );

        cpu::CPUInfo info;
        cpu::getCPUInfo( info );
        reportMessage( MSG_CPUMODEL, 0, info.model );

        ss.str("");
        ss << system::getChassisTypeString();
        if ( system::getBoardName() != "" ) ss << ", " << system::getBoardName();
        if ( system::getBoardVendor() != "" ) ss << ", " << system::getBoardVendor();
        reportMessage( MSG_MACHINE, 0, ss.str() );

        std::list<block::MajorMinor> devices;
        block::enumWholeDisks( devices );
        ss.str("");
        ss << devices.size() << " disks, total " << util::ByteStr( block::getAttachedStorageSize(), 3 );
        reportMessage( MSG_STORAGE, 0, ss.str() );
      };

      void Footer::reportMessage( unsigned int key, unsigned int prio, const std::string & message ) {
        std::map<unsigned int, Message>::iterator found = messages_.find( key );
        if ( found != messages_.end() ) {
          found->second.repeat_++;
          found->second.message_ = message;
        } else {
          messages_[key].key_ = key;
          messages_[key].repeat_ = 1;
          messages_[key].prio_ = prio;
          messages_[key].message_ = message;
          messages_[key].display_count_ = 0;
          messages_[key].first_seen_ = time(0);
        }
      }

      void Footer::clearMessage( unsigned int key ) {
        messages_.erase( key );
      }

      void Footer::xrefresh() {
        werase( window_ );
        hLine( 0, width_, 0, attr_line_ );
        if ( messages_.size() > 0 ) {
          std::vector<Message> sorted;
          for ( std::map<unsigned int, Message>::const_iterator i = messages_.begin(); i != messages_.end(); ++i ) {
            sorted.push_back( i->second );
          }
          std::sort( sorted.begin(), sorted.end() );

          std::stringstream ss;

          if ( sorted.front().prio_ > 0 ) {
            char out[200];
            struct tm *tmp;
            tmp = localtime( &sorted.front().first_seen_ );
            strftime( out, sizeof(out), "%F %T", tmp );
            ss << out << " : ";
          }

          ss << sorted.front().message_;

          int attr = attr_prio_0_;
          if ( sorted.front().prio_ == 1 ) attr = attr_prio_1_;
          else if ( sorted.front().prio_ >= 1 ) attr = attr_prio_2_;

          textOut( 0, 1, attr, ss.str() );
          messages_[ sorted.front().key_].display_count_++;
        }

        wnoutrefresh( window_ );
      }

      SysView::SysView( int width, int height, int x, int y, Screen* screen ) :
        View( width, height, x, y, screen ) {
      };

     void SysView::xrefresh( const XSysView &data, bool history ) {
        werase( window_ );
        std::stringstream ss;
        ss << "CPU " << data.cpu_topo.physical << "/" << data.cpu_topo.cores << "/" << data.cpu_topo.logical;
        hLine( 0, width_, 0, attr_line_ );
        textOut( 1, 0, attr_bold_text_, ss.str() );
        //double dt = util::deltaTime( data.t1, data.t2 );
        int xs = data.cpu_topo.logical+2;

        if ( data.cpu_topo.logical > 32 ) {
          //just draw the total over all cpu's
          xs = 3;
          box( 0, 1, 1, height_ - 1 );
          std::string bar = makeCPUBar( data.cpu_total, data.cpu_topo.logical, height_ - 2 );
          for ( int i = 0; i < (int)bar.length() && i < height_ - 3; i++ ) {
            textOut( 1, height_ - 2 - i, attrFromCPUChar(  bar.substr(i,1) ), bar.substr(i,1) );
          }
        } else {
          //one bar per cpu
          box( 0, 1, data.cpu_topo.logical+1, height_ - 1 );
          int c = 0;
          for ( cpu::CPUStatsMap::const_iterator i = data.cpu_delta.begin(); i != data.cpu_delta.end(); ++i, c++ ) {
            std::string bar = makeCPUBar( i->second, 1, height_ - 2 );
            for ( int i = 0; i < (int)bar.length() && i < height_ - 3; i++ ) {
              textOut( 1+c, height_ - 2 - i, attrFromCPUChar(  bar.substr(i,1) ), bar.substr(i,1) );
            }
          }
        }

        textOutRA( xs, 1, 8, attr_normal_text_, "user" );
        textOut( xs+4, 1, attrFromCPUChar("u"), "u" );
        textOutRA( xs, 2, 8, attr_normal_text_, "nice" );
        textOut( xs+4, 2, attrFromCPUChar("n"), "n" );
        textOutRA( xs, 3, 8, attr_normal_text_, "system" );
        textOut( xs+2, 3, attrFromCPUChar("s"), "s" );
        textOutRA( xs, 4, 8, attr_normal_text_, "iowait" );
        textOut( xs+4, 4, attrFromCPUChar("w"), "w" );
        textOutRA( xs, 5, 8, attr_normal_text_, "irq" );
        textOut( xs+5, 5, attrFromCPUChar("i"), "i" );
        textOutRA( xs, 6, 8, attr_normal_text_, "softirq" );
        textOut( xs+2, 6, attrFromCPUChar("o"), "o" );
        textOutRA( xs, 7, 8, attr_bold_text_, "total" );
        textOutRA( xs, 8, 8, attr_normal_text_, "slice" );
        textOutRA( xs, 9, 8, attr_normal_text_, "ctxsw/s" );
        textOutRA( xs, 10, 8, attr_normal_text_, "forks/s" );
        textOutRA( xs, 11, 8, attr_normal_text_, "loadavg" );
        textOutRA( xs, 12, 8, attr_normal_text_, "runq" );
        xs += 9;

        if ( data.sample_count > 1 )  {
          textOut( xs, 1, attr_normal_text_, util::NumStr( data.cpu_total.user, 3 ) );
          textOut( xs, 2, attr_normal_text_, util::NumStr( data.cpu_total.nice, 3 ) );
          textOut( xs, 3, attr_normal_text_, util::NumStr( data.cpu_total.system, 3 ) );
          textOut( xs, 4, attr_normal_text_, util::NumStr( data.cpu_total.iowait, 3 ) );
          textOut( xs, 5, attr_normal_text_, util::NumStr( data.cpu_total.irq, 3 ) );
          textOut( xs, 6, attr_normal_text_, util::NumStr( data.cpu_total.softirq, 3 ) );
          textOut( xs, 7, attr_bold_text_, util::NumStr( data.cpu_seconds, 3 ) );
          textOut( xs, 8, attr_normal_text_, util::TimeStrSec( data.time_slice ) );
          textOut( xs, 9, attr_normal_text_, util::NumStr( data.ctxsws ) );
          textOut( xs, 10, attr_normal_text_, util::NumStr( data.forks ) );
          ss.str("");
          ss << std::fixed << std::setprecision(2) << data.loadavg.avg5_ << "/" << data.loadavg.avg10_;
          textOut( xs, 11, attr_normal_text_, ss.str() );
          ss.str("");
          ss << data.runq << "/" << data.blockq;
          textOut( xs, 12, attr_normal_text_, ss.str() );
        }
        // memory section
        xs += 10;
        vLine( 1, height_, xs, attr_line_ );
        xs += 2;
        ss.str("");
        ss << "Memory " << util::ByteStr( data.mem_total, 3 ) << " RAM";
        textOut( xs, 0, attr_bold_text_, ss.str() );
        textOutRA( xs, 1, 8, attr_normal_text_, "unused" );
        textOutRA( xs, 2, 8, attr_normal_text_, "commitas" );
        textOutRA( xs, 3, 8, attr_normal_text_, "anon" );
        textOutRA( xs, 4, 8, attr_normal_text_, "file" );
        textOutRA( xs, 5, 8, attr_normal_text_, "shmem" );
        textOutRA( xs, 6, 8, attr_normal_text_, "slab" );
        textOutRA( xs, 7, 8, attr_normal_text_, "pagetbls" );
        textOutRA( xs, 8, 8, attr_normal_text_, "dirty" );
        textOutRA( xs, 9, 8, attr_normal_text_, "pgin/s" );
        textOutRA( xs, 10, 8, attr_normal_text_, "pgout/s" );
        textOutRA( xs, 11, 8, attr_normal_text_, "swpin/s" );
        textOutRA( xs, 12, 8, attr_normal_text_, "swpout/s" );
        xs += 8;
        textOutRA( xs, 1, 6, attr_normal_text_, util::ByteStr( data.mem_unused, 3 ) );
        textOutRA( xs, 2, 6, attr_bold_text_, util::ByteStr( data.mem_commitas, 3 ) );
        textOutRA( xs, 3, 6, attr_normal_text_, util::ByteStr( data.mem_anon, 3 ) );
        textOutRA( xs, 4, 6, attr_normal_text_, util::ByteStr( data.mem_file, 3 ) );
        textOutRA( xs, 5, 6, attr_normal_text_, util::ByteStr( data.mem_shmem, 3 ) );
        textOutRA( xs, 6, 6, attr_normal_text_, util::ByteStr( data.mem_slab, 3 ) );
        textOutRA( xs, 7, 6, attr_normal_text_, util::ByteStr( data.mem_pagetbls, 3 ) );
        textOutRA( xs, 8, 6, attr_normal_text_, util::ByteStr( data.mem_dirty, 3 ) );
        if ( data.sample_count > 1 ) {
          textOutRA( xs, 9, 6, attr_normal_text_, util::NumStr( data.mem_pageins ) );
          textOutRA( xs, 10, 6, attr_normal_text_, util::NumStr( data.mem_pageouts ) );
          textOutRA( xs, 11, 6, attr_normal_text_, util::NumStr( data.mem_swapins ) );
          textOutRA( xs, 12, 6, attr_normal_text_, util::NumStr( data.mem_swapouts ) );
        }
        xs += 7;
        doubleOutPct( xs, 1, attr_normal_text_, (double)data.mem_unused / data.mem_total * 100.0 );
        doubleOutPct( xs, 2, attr_bold_text_, (double)data.mem_commitas / data.mem_total * 100.0 );
        doubleOutPct( xs, 3, attr_normal_text_, (double)data.mem_anon / data.mem_total * 100.0 );
        doubleOutPct( xs, 4, attr_normal_text_, (double)data.mem_file / data.mem_total * 100.0 );
        doubleOutPct( xs, 5, attr_normal_text_, (double)data.mem_shmem / data.mem_total * 100.0 );
        doubleOutPct( xs, 6, attr_normal_text_, (double)data.mem_slab / data.mem_total * 100.0 );
        doubleOutPct( xs, 7, attr_normal_text_, (double)data.mem_pagetbls / data.mem_total * 100.0 );
        doubleOutPct( xs, 8, attr_normal_text_, (double)data.mem_dirty / data.mem_total * 100.0 );
        xs += 7;
        textOutRA( xs, 1, 8, attr_normal_text_, "hp total" );
        textOutRA( xs, 2, 8, attr_normal_text_, "hp rsvd" );
        textOutRA( xs, 3, 8, attr_normal_text_, "hp free" );
        textOutRA( xs, 4, 8, attr_normal_text_, "thp anon" );
        textOutRA( xs, 5, 8, attr_normal_text_, "mlock" );
        textOutRA( xs, 6, 8, attr_normal_text_, "mapped" );
        textOutRA( xs, 7, 8, attr_normal_text_, "swp used" );
        textOutRA( xs, 8, 8, attr_normal_text_, "swp size" );
        textOutRA( xs, 9, 8, attr_normal_text_, "minflt/s" );
        textOutRA( xs, 10, 8, attr_normal_text_, "majflt/s" );
        textOutRA( xs, 11, 8, attr_normal_text_, "alloc/s" );
        textOutRA( xs, 12, 8, attr_normal_text_, "free/s" );
        xs += 9;
        textOutRA( xs, 1, 6, attr_normal_text_, util::ByteStr( data.mem_hptotal, 3 ) );
        textOutRA( xs, 2, 6, attr_normal_text_, util::ByteStr( data.mem_hprsvd, 3 ) );
        textOutRA( xs, 3, 6, attr_normal_text_, util::ByteStr( data.mem_hpfree, 3 ) );
        textOutRA( xs, 4, 6, attr_normal_text_, util::ByteStr( data.mem_thpanon, 3 ) );
        textOutRA( xs, 5, 6, attr_normal_text_, util::ByteStr( data.mem_mlock, 3 ) );
        textOutRA( xs, 6, 6, attr_normal_text_, util::ByteStr( data.mem_mapped, 3 ) );
        textOutRA( xs, 7, 6, attr_normal_text_, util::ByteStr( data.mem_swpused, 3 ) );
        textOutRA( xs, 8, 6, attr_normal_text_, util::ByteStr( data.mem_swpsize, 3 ) );
        if ( data.sample_count > 1 ) {
          textOutRA( xs, 9, 6, attr_normal_text_, util::NumStr( data.mem_minflts ) );
          textOutRA( xs, 10, 6, attr_normal_text_, util::NumStr( data.mem_majflts ) );
          textOutRA( xs, 11, 6, attr_normal_text_, util::NumStr( data.mem_allocs ) );
          textOutRA( xs, 12, 6, attr_normal_text_, util::NumStr( data.mem_frees ) );
        }

        //system resources
        xs += 7;
        if( xs + 20 < width_ ) {
          vLine( 1, height_, xs, attr_line_ );
          charOut( xs, 0, attr_line_, ACS_TTEE );
          xs += 1;
          textOut( xs, 0, attr_bold_text_, "System resources" );
          textOutRA( xs, 1, 11, attr_normal_text_, "files open" );
          textOut( xs+12, 1, attr_normal_text_, util::NumStr( data.res_filesopen ) );
          textOutRA( xs, 2, 11, attr_normal_text_, "files max" );
          textOut( xs+12, 2, attr_normal_text_, util::NumStr( data.res_filesmax ) );
          textOutRA( xs, 3, 11, attr_normal_text_, "inodes open" );
          textOut( xs+12, 3, attr_normal_text_, util::NumStr( data.res_inodesopen ) );
          textOutRA( xs, 4, 11, attr_normal_text_, "inodes free" );
          textOut( xs+12, 4, attr_normal_text_, util::NumStr( data.res_inodesfree ) );
          textOutRA( xs, 5, 11, attr_normal_text_, "processes" );
          textOut( xs+12, 5, attr_normal_text_, util::NumStr( data.res_processes ) );
          textOutRA( xs, 6, 11, attr_normal_text_, "users" );
          textOut( xs+12, 6, attr_normal_text_, data.res_users );
          textOutRA( xs, 7, 11, attr_normal_text_, "logins" );
          textOut( xs+12, 7, attr_normal_text_, data.res_logins );
          if ( data.sample_count > 1 ) {
            textOutRA( xs, 8, 11, attr_normal_text_, "fs growth/s" );
            textOut( xs+12, 8, attr_normal_text_, util::ByteStr( data.fs_growths, 2 ) );
          }
        }
        xs += 18;
        if ( xs + 10 < width_ ) {

          //cpu trail

          //box( xs, 1, width_ - 1, height_ - 1 );
          box( xs, 0, width_ - 1, height_ -1 );
          textOut( xs+1, 0, attr_bold_text_, "CPU trail" );
          charOut( xs, 0, attr_line_, ACS_TTEE );

          if ( history ) {
            int mid = xs+(width_-xs)/2;
            charOut( mid, 0, attr_plot_element_, ACS_TTEE );
            for ( int i = 1; i < height_-1; i++ ) {
              charOut( mid, i, attr_plot_element_, ACS_VLINE );
            }
            ss.str("");
            ss << util::localStrISODateTime(data.t2.tv_sec);
            textOut( mid-ss.str().length()/2, height_ - 1, attr_normal_text_, ss.str() );

            std::string bar = makeCPUBar( data.cpu_total, data.cpu_topo.logical, height_ - 2 );
            for ( int i = 0; i < (int)bar.length() && i < height_ - 1; i++ ) {
              textOut( mid, height_ - 2 - i, attrFromCPUChar(  bar.substr(i,1) ), bar.substr(i,1) );
            }

            if ( mid - xs > 50 ) {
              ss.str("");
              ss << util::localStrISOTime(data.t2.tv_sec-util::deltaTime(data.t1,data.t2)*40);
              textOut( mid-40-ss.str().length()/2, height_ - 1, attr_normal_text_, ss.str() );
              charOut( mid-40, 0, attr_line_, ACS_TTEE );
              for ( int i = 1; i < height_-1; i++ ) {
                charOut( mid-40, i, attr_line_, ACS_VLINE );
              }
            }

            if ( width_ - mid > 50 ) {
              ss.str("");
              ss << util::localStrISOTime(data.t2.tv_sec+util::deltaTime(data.t1,data.t2)*40);
              textOut( mid+40-ss.str().length()/2, height_ - 1, attr_normal_text_, ss.str() );
              charOut( mid+40, 0, attr_line_, ACS_TTEE );
              for ( int i = 1; i < height_-1; i++ ) {
                charOut( mid+40, i, attr_line_, ACS_VLINE );
              }
            }

            int c = 0;
            for ( std::list<cpu::CPUStat>::const_iterator i = data.cpupast.begin(); i != data.cpupast.end() && mid-1-c > xs; i++,c++ ) {
              bar = makeCPUBar( *i, 1.0, height_ - 2 );
              for ( int i = 0; i < (int)bar.length() && i < height_ - 1; i++ ) {
                textOut( mid-1-c, height_ - 2 - i, attrFromCPUChar(  bar.substr(i,1) ), bar.substr(i,1) );
              }
            }

            c = 0;
            for ( std::list<cpu::CPUStat>::const_iterator i = data.cpufuture.begin(); i != data.cpufuture.end(); i++,c++ ) {
              bar = makeCPUBar( *i, 1.0, height_ - 2 );
              for ( int i = 0; i < (int)bar.length() && i < height_ - 1; i++ ) {
                textOut( mid+1+c, height_ - 2 - i, attrFromCPUChar(  bar.substr(i,1) ), bar.substr(i,1) );
              }
            }

          } else {
            if ( data.sample_count > 1 ) {
              int mark = width_ - 11;
              size_t lp = width_ - 1;
              int tc = 10;
              while ( mark > xs + 1 ) {
                charOut( mark, height_ - 1, attr_line_, ACS_TTEE );
                std::stringstream ss;
                ss << "-" << util::TimeStrSec( util::deltaTime(data.t1,data.t2)*tc);
                if ( ss.str().length() < lp - mark ) {
                  textOut( mark+1, height_ - 1, attr_normal_text_, ss.str() );
                  lp = mark;
                }
                mark -= 10;
                tc += 10;
              }

              std::list<std::string>::const_iterator q = data.cpurtpast.end();
              q--;
              for ( int p = width_ - 2; p > xs && q != data.cpurtpast.begin(); p-- ) {
                for ( int i = 0; i < (int)(*q).length() && i < height_ - 1; i++ ) {
                  textOut( p, height_ - 2 - i, attrFromCPUChar( (*q).substr(i,1) ), (*q).substr(i,1) );
                }
                q--;
              }
            }
          }
        }

        wnoutrefresh( window_ );
      }

      int SysView::attrFromCPUChar( std::string c ) {
        int result = COLOR_PAIR( screen_->palette_.getColorText() );
        if ( c == "s" ) result = COLOR_PAIR( screen_->palette_.getColorSystemCPU() );
        else if ( c == "u" ) result = COLOR_PAIR( screen_->palette_.getColorUserCPU() );
        else if ( c == "n" ) result = COLOR_PAIR( screen_->palette_.getColorNiceCPU() );
        else if ( c == "w" ) result = COLOR_PAIR( screen_->palette_.getColorIOWaitCPU() );
        else if ( c == "i" ) result = COLOR_PAIR( screen_->palette_.getColorIRQCPU() );
        else if ( c == "o" ) result = COLOR_PAIR( screen_->palette_.getColorSoftIRQCPU() );
        result |= A_BOLD;
        return result;
      }

      int IOView::getMinHeight()  {
        return leanux::util::ConfigFile::getConfig()->getIntValue( "IOVIEW_MIN_HEIGHT" );
      }

      unsigned int IOView::max_mountpoint_width_;
      std::map<std::string,block::MajorMinor> IOView::devicefilecache_;

      int IOView::getOptimalHeight()  {
        std::list<vmem::SwapInfo> swaps;
        vmem::getSwapInfo( swaps );
        std::map<block::MajorMinor,block::MountInfo> mounts;
        block::enumMounts( mounts, devicefilecache_ );
        return
          std::max( (size_t)leanux::util::ConfigFile::getConfig()->getIntValue( "IOVIEW_MIN_HEIGHT" ),
                    std::min( (size_t)leanux::util::ConfigFile::getConfig()->getIntValue( "IOVIEW_MAX_HEIGHT" ),
                              std::max( (size_t)block::getAttachedWholeDisks(), mounts.size() + swaps.size() )
                              + 3
                            )
                  );
      }

      IOView::IOView( int width, int height, int x, int y, Screen* screen ) :
        View( width, height, x, y, screen ) {
      }

      unsigned long IOView::getSectorSize( const block::MajorMinor& m ) {
        std::map<block::MajorMinor,unsigned int>::const_iterator i = sector_size_cache_.find( m );
        if ( i == sector_size_cache_.end() ) {
          unsigned long size = m.getSectorSize();
          sector_size_cache_[ m ] = size;
          return size;
        } else return i->second;
      }

      std::string IOView::getDeviceName( const block::MajorMinor& m ) {
        std::map<block::MajorMinor,std::string>::const_iterator i = name_cache_.find( m );
        if ( i == name_cache_.end() ) {
          std::string name = block::MajorMinor::getNameByMajorMinor( m );
          name_cache_[ m ] = name;
          return name;
        } else return i->second;
      }

      void IOView::xrefresh( const XIOView &data ) {
        const unsigned int device_width = 8;
        const unsigned int util_width = 7;
        const unsigned int svct_width = 6;
        const unsigned int rs_width = 5;
        const unsigned int ws_width = 5;
        const unsigned int rbs_width = 5;
        const unsigned int wbs_width = 5;
        const unsigned int artm_width = 6;
        const unsigned int awtm_width = 6;
        const unsigned int rsz_width = 5;
        const unsigned int wsz_width = 5;
        const unsigned int qsz_width = 5;
        const unsigned int iodone_width = 5;
        const unsigned int ioreq_width = 5;
        const unsigned int ioerr_width = 5;
        const unsigned int fsg_width = 7;
        werase( window_ );
        std::stringstream ss;
        ss << "Disk IO";
        hLine( 0, width_, 0, attr_line_ );
        textOut( 1, 0, attr_bold_text_, ss.str() );

        int x = 0;
        textOutMoveXRA( x, 1, device_width, attr_bold_text_, "device" );
        textOutMoveXRA( x, 1, util_width, attr_bold_text_, "util" );
        textOutMoveXRA( x, 1, svct_width,  attr_bold_text_, "svct" );
        textOutMoveXRA( x, 1, rs_width, attr_bold_text_, "r/s" );
        textOutMoveXRA( x, 1, ws_width, attr_bold_text_, "w/s" );
        textOutMoveXRA( x, 1, rbs_width, attr_bold_text_, "rb/s" );
        textOutMoveXRA( x, 1, wbs_width, attr_bold_text_, "wb/s" );
        textOutMoveXRA( x, 1, artm_width, attr_bold_text_, "artm" );
        textOutMoveXRA( x, 1, awtm_width, attr_bold_text_, "awtm" );
        textOutMoveXRA( x, 1, rsz_width, attr_bold_text_, "rsz" );
        textOutMoveXRA( x, 1, wsz_width, attr_bold_text_, "wsz" );
        textOutMoveXRA( x, 1, qsz_width, attr_bold_text_, "qsz" );
        textOutMoveXRA( x, 1, iodone_width, attr_bold_text_, "dones" );
        textOutMoveXRA( x, 1, ioreq_width, attr_bold_text_, "reqs" );
        textOutMoveXRA( x, 1, ioerr_width, attr_bold_text_, "errs" );

        unsigned int fs_start = x;

        double s_disk_util = 0.0;
        double s_disk_read = 0.0;
        double s_disk_write = 0.0;
        double s_disk_rb = 0.0;
        double s_disk_wb = 0.0;
        int y = 2;
        unsigned int disk_count = 0;
        for ( std::vector<std::string>::const_iterator d = data.iosorted.begin(); d != data.iosorted.end(); d++ ) {
          IORecMap::const_iterator dstat = data.iostats.find(*d);
          if ( dstat != data.iostats.end() ) {
            disk_count ++;
            if ( y < height_ - 1 ) {
              x = 0;
              textOutMoveXRA( x, y, device_width, attr_normal_text_, *d );
              textOutMoveXRA( x, y, util_width, attr_normal_text_, util::NumStr( dstat->second.util, 3 ) );
              textOutMoveXRA( x, y, svct_width, attr_normal_text_, util::TimeStrSec( dstat->second.svctm ) );
              textOutMoveXRA( x, y, rs_width, attr_normal_text_, util::NumStr( dstat->second.rs ) );
              textOutMoveXRA( x, y, ws_width, attr_normal_text_, util::NumStr( dstat->second.ws ) );
              textOutMoveXRA( x, y, rbs_width, attr_normal_text_, util::ByteStr(dstat->second.rbs, 2) );
              textOutMoveXRA( x, y, wbs_width, attr_normal_text_, util::ByteStr(dstat->second.wbs, 2) );
              textOutMoveXRA( x, y, artm_width, attr_normal_text_, util::TimeStrSec(dstat->second.artm ) );
              textOutMoveXRA( x, y, awtm_width, attr_normal_text_, util::TimeStrSec(dstat->second.awtm ) );
              if ( dstat->second.rs != 0 )
                textOutMoveXRA( x, y, rsz_width, attr_normal_text_, util::ByteStr(dstat->second.rbs/dstat->second.rs, 3 ) );
              else x += rsz_width + 1;
              if ( dstat->second.ws != 0 )
                textOutMoveXRA( x, y, wsz_width, attr_normal_text_, util::ByteStr(dstat->second.wbs/dstat->second.ws, 3 ) );
              else x += wsz_width + 1;
              if ( dstat->second.svctm != 0 )
                textOutMoveXRA( x, y, qsz_width, attr_normal_text_, util::NumStr( (dstat->second.artm+dstat->second.awtm)/dstat->second.svctm - 1.0 ) );
              else x += qsz_width + 1;
              textOutMoveXRA( x, y, iodone_width, attr_normal_text_, util::NumStr(dstat->second.iodone_cnt ) );
              textOutMoveXRA( x, y, ioreq_width, attr_normal_text_, util::NumStr(dstat->second.iorequest_cnt ) );
              if ( dstat->second.ioerr_cnt > 0 )
                textOutMoveXRA( x, y, ioerr_width, COLOR_PAIR( screen_->palette_.getColorBlockedProc() ), util::NumStr(dstat->second.ioerr_cnt ) );
              else
                textOutMoveXRA( x, y, ioerr_width, attr_normal_text_, util::NumStr(dstat->second.ioerr_cnt ) );
              y++;
            }
            s_disk_util += (double)dstat->second.util;
            s_disk_read += dstat->second.rs;
            s_disk_write += dstat->second.ws;
            s_disk_rb += dstat->second.rbs;
            s_disk_wb += dstat->second.wbs;
          }
        }
        x = 0;
        if ( disk_count > 1 ) {
          textOutMoveXRA( x, y, device_width, attr_bold_text_, "total" );
          textOutMoveXRA( x, y, util_width, attr_bold_text_, util::NumStr( s_disk_util, 3 ) );
          x += svct_width + 1;
          textOutMoveXRA( x, y, rs_width, attr_bold_text_, util::NumStr( s_disk_read ) );
          textOutMoveXRA( x, y, ws_width, attr_bold_text_, util::NumStr( s_disk_write ) );
          textOutMoveXRA( x, y, rbs_width, attr_bold_text_, util::ByteStr( s_disk_rb, 2 ) );
          textOutMoveXRA( x, y, wbs_width, attr_bold_text_, util::ByteStr( s_disk_wb, 2 ) );
        }

        unsigned int right_required = fs_start + max_mountpoint_width_ + util_width + svct_width + rs_width + ws_width + rbs_width + wbs_width + 7;
        unsigned int right_optional = right_required + artm_width + awtm_width + rsz_width + wsz_width + 3;

        if ( right_required < (unsigned int)width_ ) {

          vLine( 1, height_, fs_start, attr_line_ );
          textOut( fs_start + 1, 0, attr_bold_text_, "Filesystem IO" );
          max_mountpoint_width_ = 0;
          unsigned int mmpw = leanux::util::ConfigFile::getConfig()->getIntValue("MAX_MOUNTPOINT_WIDTH");
          for ( std::vector<std::string>::const_iterator m = data.mountsorted.begin(); m != data.mountsorted.end(); ++m ) {
            if ( util::shortenString(*m,mmpw).length() > max_mountpoint_width_ ) max_mountpoint_width_ = util::shortenString(*m,mmpw).length();
          }
          double s_fs_util = 0.0;
          double s_fs_read = 0.0;
          double s_fs_write = 0.0;
          double s_fs_rb = 0.0;
          double s_fs_wb = 0.0;
          x = fs_start + 1;
          textOutMoveXRA( x, 1, max_mountpoint_width_, attr_bold_text_, "mountpoint" );
          textOutMoveXRA( x, 1, util_width, attr_bold_text_, "util" );
          textOutMoveXRA( x, 1, svct_width, attr_bold_text_, "svct" );
          textOutMoveXRA( x, 1, rs_width, attr_bold_text_, "r/s" );
          textOutMoveXRA( x, 1, ws_width, attr_bold_text_, "w/s" );
          textOutMoveXRA( x, 1, rbs_width, attr_bold_text_, "rb/s" );
          textOutMoveXRA( x, 1, wbs_width, attr_bold_text_, "wb/s" );
          if ( right_optional < (unsigned int)width_ + fsg_width ) {
            textOutMoveXRA( x, 1, artm_width, attr_bold_text_, "artm" );
            textOutMoveXRA( x, 1, awtm_width, attr_bold_text_, "awtm" );
            textOutMoveXRA( x, 1, rsz_width, attr_bold_text_, "rsz" );
            textOutMoveXRA( x, 1, wsz_width, attr_bold_text_, "wsz" );
            textOutMoveXRA( x, 1, fsg_width, attr_bold_text_, "grow/s" );
          }
          y = 2;
          unsigned int mount_count = 0;
          for ( std::vector<std::string>::const_iterator d = data.mountsorted.begin(); d != data.mountsorted.end(); d++ ) {
            x = fs_start+1;
            IORecMap::const_iterator dstat = data.mountstats.find(*d);
            if ( dstat != data.mountstats.end() ) {
              mount_count++;
              if ( y < height_ - 1 ) {
                textOutMoveXRA( x, y, max_mountpoint_width_, attr_normal_text_, util::shortenString(*d, mmpw) );
                textOutMoveXRA( x, y, util_width, attr_normal_text_, util::NumStr( dstat->second.util, 3 ) );
                textOutMoveXRA( x, y, svct_width, attr_normal_text_, util::TimeStrSec( dstat->second.svctm ) );
                textOutMoveXRA( x, y, rs_width, attr_normal_text_, util::NumStr(dstat->second.rs ) );
                textOutMoveXRA( x, y, ws_width, attr_normal_text_, util::NumStr(dstat->second.ws ) );
                textOutMoveXRA( x, y, rbs_width, attr_normal_text_, util::ByteStr(dstat->second.rbs, 2) );
                textOutMoveXRA( x, y, wbs_width, attr_normal_text_, util::ByteStr(dstat->second.wbs, 2) );
                if ( right_optional < (unsigned int)width_ ) {
                  textOutMoveXRA( x, y, artm_width, attr_normal_text_, util::TimeStrSec(dstat->second.artm ) );
                  textOutMoveXRA( x, y, awtm_width, attr_normal_text_, util::TimeStrSec(dstat->second.awtm ) );
                  if ( dstat->second.rs != 0 )
                    textOutMoveXRA( x, y, rsz_width, attr_normal_text_, util::ByteStr(dstat->second.rbs/dstat->second.rs, 3 ) );
                  else x += rsz_width + 1;
                  if ( dstat->second.ws > 0 )
                    textOutMoveXRA( x, y, wsz_width, attr_normal_text_, util::ByteStr((double)dstat->second.wbs / dstat->second.ws, 3 ) );
                  else x += wsz_width + 1;
                  textOutMoveXRA( x, y, fsg_width, attr_normal_text_, util::ByteStr( dstat->second.growths, 3 ) );
                }

                y++;
              }
              s_fs_util += (double)dstat->second.util;
              s_fs_read += dstat->second.rs;
              s_fs_write += dstat->second.ws;
              s_fs_rb += dstat->second.rbs;
              s_fs_wb += dstat->second.wbs;
            }
          }
          if ( mount_count > 1 ) {
            x = fs_start + 1;
            textOutMoveXRA( x, y, max_mountpoint_width_, attr_bold_text_, "total" );
            textOutMoveXRA( x, y, util_width, attr_bold_text_, util::NumStr( s_fs_util, 3 ) );
            x += svct_width + 1;
            textOutMoveXRA( x, y, rs_width, attr_bold_text_, util::NumStr( s_fs_read ) );
            textOutMoveXRA( x, y, ws_width, attr_bold_text_, util::NumStr( s_fs_write ) );
            textOutMoveXRA( x, y, rbs_width, attr_bold_text_, util::ByteStr( s_fs_rb, 2 ) );
            textOutMoveXRA( x, y, wbs_width, attr_bold_text_, util::ByteStr( s_fs_wb, 2 ) );
          }
        }

        wnoutrefresh( window_ );
      }

      void IOView::doSample() {
      }

      int ProcessView::width_wchan_ = 16;

      const double ProcessView::max_sample_time = 0.12;

      ProcessView::ProcessView( int width, int height, int x, int y, Screen* screen )
        : View( width, height, x, y, screen) {
      }

      ProcessView::~ProcessView() {
      }

      void ProcessView::resize( int width, int height, int x, int y ) {
        View::resize( width, height, x, y );
      }

      void ProcessView::xrefresh( const XProcView& data ) {
        const int width_pid = 6;
        const int width_pgrp = 6;
        const int width_q = 2;
        const int width_user = 8;
        const int width_comm = 16;
        const int width_time = 6;
        const int width_utime = 6;
        const int width_stime = 6;
        const int width_iotime = 6;
        const int width_minflt = 7;
        const int width_majflt = 7;
        const int width_rss = 6;
        const int width_vsz = 6;
        const int width_fixed = width_pid + width_pgrp + width_q + width_user + width_comm + width_time + width_utime + width_stime
          + width_iotime + width_minflt + width_majflt + width_rss + width_vsz + 11;

        width_wchan_ = std::max( width_wchan_, (int)16 );
        //if ( width_wchan_ < 5 ) width_wchan_ = 5;
        int width_arg0 = width_ - width_wchan_ - width_fixed - 2;
        if ( width_arg0 < 0 ) width_arg0 = 0;

        werase( window_ );
        hLine( 0, width_, 0, attr_line_ );
        if ( data.disabled )
          textOut( 1, 0, attr_bold_text_, "Process - too many processes" );
        else {

          std::stringstream ff;
          ff << "Process";
          textOut( 1, 0, attr_bold_text_, ff.str() );
          ff.str("");
          ff << "(" << data.delta.size() << " total)";
          textOut( 9, 0, attr_normal_text_, ff.str() );

          int x = 0;
          textOutMoveXRA( x, 1, width_pid, attr_bold_text_, "pid" );
          textOutMoveXRA( x, 1, width_pgrp, attr_bold_text_, "pgrp" );
          textOutMoveXRA( x, 1, width_q, attr_bold_text_, "S" );
          textOutMoveXRA( x, 1, width_user, attr_bold_text_, "user" );
          textOutMoveXRA( x, 1, width_comm, attr_bold_text_, "comm" );
          textOutMoveXRA( x, 1, width_time, attr_bold_text_, "time" );
          textOutMoveXRA( x, 1, width_utime, attr_bold_text_, "utime" );
          textOutMoveXRA( x, 1, width_stime, attr_bold_text_, "stime" );
          textOutMoveXRA( x, 1, width_iotime, attr_bold_text_, "iotime" );
          textOutMoveXRA( x, 1, width_minflt, attr_bold_text_, "minflt" );
          textOutMoveXRA( x, 1, width_majflt, attr_bold_text_, "majflt" );
          textOutMoveXRA( x, 1, width_rss, attr_bold_text_, "rss" );
          textOutMoveXRA( x, 1, width_vsz, attr_bold_text_, "vsz" );
          if (width_arg0  > 4 ) {
            textOut( x, 1,  attr_bold_text_, "args" );
          }
          x += width_arg0 + 1;
          if (  width_+ 2 > x + width_wchan_ + 1 ) textOut( x, 1,  attr_bold_text_, "wchan" );
          if ( data.sample_count > 1 ) {
            double s_utime = 0;
            double s_stime = 0;
            double s_iotime = 0;
            double s_time = 0;
            double s_minflt = 0;
            double s_majflt = 0;
            int y = 2;

            for ( process::ProcPidStatDeltaVector::const_iterator i = data.delta.begin(); i != data.delta.end(); i++ ) {
              if ( y < height_ - 1 ) {
                x = 0;
                textOutMoveXRA( x, y, width_pid, attr_normal_text_, (*i).pid );
                textOutMoveXRA( x, y, width_pgrp, attr_normal_text_, (*i).pgrp );
                std::stringstream ss;
                int text_attr = attr_normal_text_;
                if ( (*i).state == 'D' ) {
                  text_attr = COLOR_PAIR( screen_->palette_.getColorBlockedProc() );
                } else if ( (*i).state == 'R' ) {
                  text_attr = COLOR_PAIR( screen_->palette_.getColorRunningProc() );
                }
                ss <<  (*i).state;
                textOutMoveXRA( x, y, width_q, text_attr, ss.str() );
                std::map<pid_t,uid_t>::const_iterator u = data.piduids.find((*i).pid);
                textOutMoveXRA( x, y, width_user, text_attr, system::getUserName(u->second).substr( 0, width_user ) );
                textOutMoveXRA( x, y, width_comm, text_attr,  (*i).comm );
                textOutMoveXRA( x, y, width_time, text_attr, util::NumStr( ( (*i).utime + (*i).stime  + (*i).delayacct_blkio_ticks ), 3 ) );
                textOutMoveXRA( x, y, width_utime, text_attr, util::NumStr( (*i).utime, 3 ) );
                textOutMoveXRA( x, y, width_stime, text_attr, util::NumStr( (*i).stime, 3 ) );
                textOutMoveXRA( x, y, width_iotime, text_attr, util::NumStr( (*i).delayacct_blkio_ticks, 3 ) );
                textOutMoveXRA( x, y, width_minflt, text_attr, util::NumStr( (*i).minflt, 3 ) );
                textOutMoveXRA( x, y, width_majflt, text_attr, util::NumStr( (*i).majflt, 3 ) );
                textOutMoveXRA( x, y, width_rss, text_attr, util::ByteStr( (*i).rss * leanux::system::getPageSize() , 3 ) );
                textOutMoveXRA( x, y, width_vsz, text_attr, util::ByteStr( (*i).vsize, 3 ) );
                if ( width_ > x + width_arg0 ) {
                  std::map<pid_t,std::string>::const_iterator a = data.pidargs.find((*i).pid);
                  std::string args = a->second;
                  if ( args.size() > 0 && width_arg0  > 7 ) {
                    textOut( x, y, text_attr, args.substr(0, width_arg0) );
                  }
                  x += width_arg0+1;

                  if ( (width_+ 2 > x + width_wchan_ + 1) && (*i).state == 'D' ) {
                    textOut( x, y, text_attr, (*i).wchan.substr(0, width_wchan_ ) );
                    width_wchan_ = std::max( width_wchan_ , (int)(*i).wchan.length() );
                  }
                }
                y++;
              }
              s_utime += (*i).utime;
              s_stime += (*i).stime;
              s_iotime += (*i).delayacct_blkio_ticks;
              s_time += ((*i).utime+(*i).stime+(*i).delayacct_blkio_ticks);
              s_minflt += (*i).minflt;
              s_majflt += (*i).majflt;
            }
            x = width_pid + width_pgrp + width_q + width_user + 4;
            textOutMoveXRA( x, y, width_comm, attr_bold_text_, "total" );
            textOutMoveXRA( x, y, width_time, attr_bold_text_, util::NumStr( s_time, 3 ) );
            textOutMoveXRA( x, y, width_utime, attr_bold_text_, util::NumStr( s_utime, 3 ) );
            textOutMoveXRA( x, y, width_stime, attr_bold_text_, util::NumStr( s_stime, 3 ) );
            textOutMoveXRA( x, y, width_iotime, attr_bold_text_, util::NumStr( s_iotime, 3 ) );
            textOutMoveXRA( x, y, width_minflt, attr_bold_text_, util::NumStr( s_minflt, 3 ) );
            textOutMoveXRA( x, y, width_majflt, attr_bold_text_, util::NumStr( s_majflt, 3 ) );
          }
        }

        wnoutrefresh( window_ );
      }

      int NetView::getOptimalHeight() {
        net::NetStatDeviceMap stat;
        net::getNetStat( stat );
        return std::max( (size_t)getMinHeight(), stat.size()+3 );
      }

      NetView::NetView( int width, int height, int x, int y, Screen* screen )
        : View( width, height, x, y, screen ) {
      }

      NetView::~NetView() {
      }

      void NetView::doSample() {
      }

      int NetView::getMinHeight() {
        return leanux::util::ConfigFile::getConfig()->getIntValue( "NETVIEW_MIN_HEIGHT" );
      }

      void NetView::xrefresh( const XNetView &data ) {
        const unsigned int device_width = 8;
        const unsigned int rxb_width = 6;
        const unsigned int txb_width = 6;
        const unsigned int rxpkt_width = 7;
        const unsigned int txpkt_width = 7;
        const unsigned int rxsz_width = 6;
        const unsigned int txsz_width = 6;
        const unsigned int rxerr_width = 7;
        const unsigned int txerr_width = 7;
        int server_start = 0;
        werase( window_ );
        hLine( 0, width_, 0, attr_line_ );
        textOut( 1, 0, attr_bold_text_, "Network" );
        int x = 0;
        textOutMoveXRA( x, 1, device_width, attr_bold_text_, "device" );
        textOutMoveXRA( x, 1, rxb_width, attr_bold_text_, "rxb/s" );
        textOutMoveXRA( x, 1, txb_width, attr_bold_text_, "txb/s" );
        textOutMoveXRA( x, 1, rxpkt_width, attr_bold_text_, "rxpkt/s" );
        textOutMoveXRA( x, 1, txpkt_width, attr_bold_text_, "txpkt/s" );
        textOutMoveXRA( x, 1, rxsz_width, attr_bold_text_, "rxsz" );
        textOutMoveXRA( x, 1, txsz_width, attr_bold_text_, "txsz" );
        textOutMoveXRA( x, 1, rxerr_width, attr_bold_text_, "rxerr/s" );
        textOutMoveXRA( x, 1, txerr_width, attr_bold_text_, "txerr/s" );
        server_start = x;
        if ( data.sample_count > 1 ) {
          int y = 2;
          double s_rx_bytes = 0.0;
          double s_tx_bytes = 0.0;
          double s_rx_packets = 0.0;
          double s_tx_packets = 0.0;
          for ( net::NetStatDeviceVector::const_iterator i = data.delta.begin(); i != data.delta.end(); i++, y++ ) {
            if ( y < height_ ) {
              x = 0;
              textOutMoveXRA( x, y, device_width, attr_normal_text_, (*i).device );
              textOutMoveXRA( x, y, rxb_width, attr_normal_text_, util::ByteStr( (*i).rx_bytes, 3 ) );
              textOutMoveXRA( x, y, txb_width, attr_normal_text_, util::ByteStr( (*i).tx_bytes, 3 ) );
              textOutMoveXRA( x, y, rxpkt_width, attr_normal_text_, util::NumStr( (*i).rx_packets ) );
              textOutMoveXRA( x, y, txpkt_width, attr_normal_text_, util::NumStr( (*i).tx_packets ) );
              if ( (*i).rx_packets > 0 )
                textOutMoveXRA( x, y, rxsz_width, attr_normal_text_, util::ByteStr( (*i).rx_bytes/ (*i).rx_packets, 3 ) );
              else x += rxsz_width + 1;
              if ( (*i).tx_packets > 0 )
                textOutMoveXRA( x, y, txsz_width, attr_normal_text_, util::ByteStr( (*i).tx_bytes/ (*i).tx_packets, 3 ) );
              else x += txsz_width + 1;
              textOutMoveXRA( x, y, rxerr_width, attr_normal_text_, util::NumStr( (*i).rx_errors ) );
              textOutMoveXRA( x, y, txerr_width, attr_normal_text_, util::NumStr( (*i).tx_errors ) );
            }
            s_rx_bytes += (*i).rx_bytes;
            s_tx_bytes += (*i).tx_bytes;
            s_rx_packets += (*i).rx_packets;
            s_tx_packets += (*i).tx_packets;
          }
          x = 0;
          textOutMoveXRA( x, height_ - 1, device_width, attr_bold_text_, "total" );
          textOutMoveXRA( x, height_ - 1, rxb_width, attr_bold_text_, util::ByteStr( s_rx_bytes, 3 ) );
          textOutMoveXRA( x, height_ - 1, txb_width, attr_bold_text_, util::ByteStr( s_tx_bytes, 3 ) );
          textOutMoveXRA( x, height_ - 1, rxpkt_width, attr_bold_text_, util::NumStr( s_rx_packets ) );
          textOutMoveXRA( x, height_ - 1, txpkt_width, attr_bold_text_, util::NumStr( s_tx_packets ) );
        }
        // server analysis
        int server_width = 30;
        if ( width_ >= server_start + server_width ) {
          unsigned int max_server_ip_width_ = 0;
          for ( std::list<net::TCPKeyCounter>::const_iterator i = data.tcpserver.begin(); i != data.tcpserver.end(); i++ ) {
            if ( (*i).getKey().getIP().length() > max_server_ip_width_ ) max_server_ip_width_ = (*i).getKey().getIP().length();
          }

          unsigned int max_client_ip_width_ = 0;
          for ( std::list<net::TCPKeyCounter>::const_iterator i = data.tcpclient.begin(); i != data.tcpclient.end(); i++ ) {
            if ( (*i).getKey().getIP().length() > max_client_ip_width_ ) max_client_ip_width_ = (*i).getKey().getIP().length();
          }

          const unsigned int port_width = 6;
          const unsigned int uid_width = 9;
          const unsigned int esta_width = 6;

          int j = 2;
          int x = server_start+1;

          if ( server_start + max_server_ip_width_ + port_width + port_width + esta_width + 6 < (unsigned int)width_ ) {
            vLine( 1, height_ , server_start, attr_line_ );
            textOut( server_start+1, 0, attr_bold_text_, "TCP server" );
            for ( std::list<net::TCPKeyCounter>::const_iterator i = data.tcpserver.begin(); i != data.tcpserver.end() && j < height_; i++, j++ ) {
              x = server_start+1;
              textOutMoveXRA( x, j, max_server_ip_width_, attr_normal_text_, (*i).getKey().getIP() );
              textOutMoveXRA( x, j, port_width, attr_normal_text_, (*i).getKey().getPort() );
              textOutMoveXRA( x, j, uid_width, attr_normal_text_, system::getUserName( (*i).getKey().getUID() ) );
              textOutMoveXRA( x, j, esta_width, attr_normal_text_, (*i).getEsta() );
            }
            x = server_start + 1;
            textOutMoveXRA( x, 1, max_server_ip_width_, attr_bold_text_, "address" );
            textOutMoveXRA( x, 1, port_width, attr_bold_text_, "port" );
            textOutMoveXRA( x, 1, uid_width, attr_bold_text_, "user" );
            textOutMoveXRA( x, 1, esta_width, attr_bold_text_, "#conn" );

            int client_start = x;

            if ( client_start + max_client_ip_width_ + port_width + port_width + esta_width + 6 < (unsigned int)width_ ) {

              vLine( 1, height_ , client_start, attr_line_ );
              textOut( client_start+1, 0, attr_bold_text_, "TCP client" );
              j = 2;
              for ( std::list<net::TCPKeyCounter>::const_iterator i = data.tcpclient.begin(); i != data.tcpclient.end() && j < height_; i++, j++ ) {
                x = client_start + 1;
                textOutMoveXRA( x, j, max_client_ip_width_, attr_normal_text_, (*i).getKey().getIP() );
                textOutMoveXRA( x, j, port_width, attr_normal_text_, (*i).getKey().getPort() );
                textOutMoveXRA( x, j, uid_width, attr_normal_text_, system::getUserName( (*i).getKey().getUID() ) );
                textOutMoveXRA( x, j, esta_width, attr_normal_text_, (*i).getEsta() );
              }
              x = client_start + 1;
              textOutMoveXRA( x, 1, max_client_ip_width_, attr_bold_text_, "address" );
              textOutMoveXRA( x, 1, port_width, attr_bold_text_, "port" );
              textOutMoveXRA( x, 1, uid_width, attr_bold_text_, "user" );
              textOutMoveXRA( x, 1, esta_width, attr_bold_text_, "#conn" );
            }
          }
        }
        wnoutrefresh( window_ );
      }

    }; //namespace lmon

  }; //namespace tools

}; //namespace leanux
