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
 * ncurses based real time linux performance monitoring tool - c++ header file.
 * lmon ncurses screen implementation.
 */
#ifndef LEANUX_CURSES_HPP
#define LEANUX_CURSES_HPP

#include "block.hpp"
#include "cpu.hpp"
#include "net.hpp"
#include "process.hpp"
#include "vmem.hpp"
#include "persist.hpp"
#include "realtime.hpp"

#include <ncurses.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include "termkey.h"
#include <poll.h>

#include <iomanip>
#include <list>
#include <queue>
#include <sstream>

namespace leanux {

  namespace tools {

    namespace lmon {

      class Screen;

      /**
       * A reactangular area on the ncurses Screen as a window of
       * information.
       */
      class View {
        public:
          /**
           * The constructor specifies location and size of the View on the Screen.
           * @see View::resize.
           * @param width the width (number of columns) of the View.
           * @param height the height (number of lines) of the View.
           * @param x the column of left-top.
           * @param y the line of left-top.
           * @param screen the Screen object holding the View.
           *
           */
          View ( int width, int height, int x, int y, Screen* screen );

          /**
           * Destroy the View.
           */
          virtual ~View();

          /**
           * Resize the View.
           */
          virtual void resize( int width, int height, int x, int y );

          /**
           * The minimum height (lines) required by the View.
           */
          static int getMinHeight() { return 0; };

          int getHeight() const { return height_; }

        protected:

          /**
           * Overload to take a sample.
           */
          virtual void doSample() {};

          /**
           * Write text at x,y with display attrs.
           * @param x the x (column) position.
           * @param y the y (line) position.
           * @param attrs the ncurses display attributes to use.
           * @param text the text to display.
           */
          inline void textOut( int x, int y, int attrs, std::string text );

          /**
           * Write integer as std::string at x,y with display attrs.
           * @param x the x (column) position.
           * @param y the y (line) position.
           * @param attrs the ncurses display attributes to use.
           * @param i the integer to display.
           */
          inline void textOut( int x, int y, int attrs, int i );

          /**
           * Write character ch at x,y with display attrs.
           * @param x the x (column) position.
           * @param y the y (line) position.
           * @param attrs the ncurses display attributes to use.
           * @param ch the char to display.
           */
          inline void charOut( int x, int y, int attrs, const chtype ch );

          /**
           * Write double at x,y with display attrs.
           * @param x the x (column) position.
           * @param y the y (line) position.
           * @param attrs the ncurses display attributes to use.
           * @param value the double to display.
           */
          inline void doubleOut( int x, int y, int attrs, double value );

          /**
           * Write double as a percentage x,y with display attrs.
           * @param x the x (column) position.
           * @param y the y (line) position.
           * @param attrs the ncurses display attributes to use.
           * @param value the double to display.
           */
          inline void doubleOutPct( int x, int y, int attrs, double value );

          /**
           * Draw text right-adjusted.
           * @param x x position, set x=x+w+1 on finish.
           * @param y y position
           * @param w width of the displayed value.
           * @param attrs the ncurses display attributes to use.
           * @param text the text to display.
           */
          inline void textOutRA( int x, int y, int w, int attrs, std::string text );

          /**
           * Draw integer right-adjusted.
           * @param x x position, set x=x+w+1 on finish.
           * @param y y position
           * @param w width of the displayed value.
           * @param attrs the ncurses display attributes to use.
           * @param i the integer to display.
           */
          inline void textOutRA( int x, int y, int w, int attrs, int i );

          /**
           * Draw text right-adjusted and progress x with w+1.
           * @param x x position, set x=x+w+1 on finish.
           * @param y y position
           * @param w width of the displayed value.
           * @param attrs the ncurses display attributes to use.
           * @param text the text to display.
           */
          inline void textOutMoveXRA( int &x, int y, int w, int attrs, std::string text );

          /**
           * Draw an integer right-adjusted and progress x with w+1.
           * @param x x position, set x=x+w+1 on finish.
           * @param y y position
           * @param w width of the displayed value.
           * @param attrs the ncurses display attributes to use.
           * @param i the integer value to display.
           */
          inline void textOutMoveXRA( int &x, int y, int w, int attrs, int i );

          /**
           * Draw a horizontal line.
           * @param x1 line left.
           * @param x2 line right.
           * @param y line.
           * @param attrs draw attributes.
           * @param token token to use.
           */
          inline void hLine( int x1, int x2, int y, int attrs, chtype token = ACS_HLINE );

          /**
           * Draw a vertical line.
           * @param y1 line top.
           * @param y2 line bottom.
           * @param x column.
           * @param attrs draw attributes.
           * @param token token to use.
           */
          inline void vLine( int y1, int y2, int x, int attrs, chtype token = ACS_VLINE );

          /**
           * Draw a box.
           * @param x1 top-left column.
           * @param y1 top-left line.
           * @param x2 right-bottom column.
           * @param y2 right-bottom line.
           */
          void box( int x1, int y1, int x2, int y2 );

          /**
           * The height (lines) of the View.
           */
          int height_;

          /**
           * The width (columns) of the View.
           */
          int width_;

          /**
           * The absolute x (column) position of left-top.
           */
          int absx_;

          /**
           * The absolute y (line) position of left-top.
           */
          int absy_;

          /**
           * The ncurses window.
           */
          WINDOW *window_;

          /**
           * The Screen.
           */
          Screen* screen_;

          /**
           * attributes for drawing normal text.
           */
          int attr_normal_text_;

          /**
           * attributes for drawing bold text.
           */
          int attr_bold_text_;

          /**
           * attributes for drawing lines.
           */
          int attr_line_;

          /**
           * attributes for (default) plotting characters.
           */
          int attr_plot_element_;

          /**
           * attributes for drawing alert text.
           */
          int attr_alert_text_;

          /**
           * total number of samples taken.
           */
          unsigned long sample_count_;
      };

      /**
       * The lmon Header is a View shown at the top.
       */
      class Header : public View {
        public:

          /**
           * Construct a Header.
           * @param width the width.
           * @param height the height.
           * @param x the top-left x.
           * @param y the top-left y.
           * @param screen the Screen to sit on.
           */
          Header( int width, int height, int x, int y, Screen* screen );

          /** Destructor. */
          virtual ~Header() {};

          /**
           * Draw the header, with msg right aligned at right top
           */
          virtual void xrefresh( const std::string &caption, const std::string &msg );

          /**
           * The minimum height (lines) required by the Header.
           */
          static int getMinHeight()  { return 1; };

          /**
           * provide the optimal height for the Header.
           * @return the optimal height in lines.
           */
          static int getOptimalHeight() { return 1; };

      protected:
        /** The header informational std::string. */
        std::string header_;

      };

      /**
       * The lmon Footer is a View shown at the bottom.
       */
      class Footer : public View {
        public:
          /**
           * The Footer can rotate-display messages, if they exist.
           */
          class Message {
            public:
              Message() { key_ = 0; prio_ = 0; message_ = ""; first_seen_ = 0; repeat_ = 0; display_count_ = 0; };
              /**
               * Message constructor.
               * @param key the Message key.
               * @param prio the Message priority.
               * @param message the Message text.
               */
              Message( unsigned int key, unsigned int prio, std::string message ) { key_ = key; prio_ = prio; message_ = message; first_seen_ = time(NULL); repeat_ = 0; };
              /** The Message (unique) key. */
              unsigned int key_;
              /** the Message priority. */
              unsigned int prio_;
              /** the Message text. */
              std::string message_;
              /** time when first reported. */
              time_t first_seen_;
              /** number of message repeats. */
              unsigned long repeat_;
              /** display count - used for priority ordering. */
              unsigned long display_count_;

              int operator<( const Message &msg ) const { return prio_ > msg.prio_ || ( prio_ == msg.prio_ && display_count_ < msg.display_count_ ); };
          };

          /**
           * Construct a Footer.
           * @param width the width.
           * @param height the height.
           * @param x the top-left x.
           * @param y the top-left y.
           * @param screen the Screen to sit on.
           */
          Footer( int width, int height, int x, int y, Screen* screen );

          /**
           * refresh (redraw) the Footer.
           */
          virtual void xrefresh();

          /**
           * The minimum height (lines) required by the Footer.
           */
          static int getMinHeight()  { return 2; };

          virtual ~Footer() {};

          /**
           * provide the optimal height for the Header.
           * @return the optimal height in lines.
           */
          static int getOptimalHeight() { return getMinHeight(); };

          /**
           * Report a Message.
           * @param key the Message key.
           * @param prio the Message priority.
           * @param message the Message text.
           */
          void reportMessage( unsigned int key, unsigned int prio, const std::string & message );

          /**
           * Clear a Message.
           * @param key the Message key.
           */
          void clearMessage( unsigned int key );

      protected:

        /**
         * map of Message objects keyed by Message.key_.
         */
        std::map<unsigned int,Message> messages_;

        /**
         * current index in messages_.
         */
        size_t msgidx_;

        /** ncurses attributes for prio 0 */
        int attr_prio_0_;

        /** ncurses attributes for prio 1 */
        int attr_prio_1_;

        /** ncurses attributes for prio 2 */
        int attr_prio_2_;

      };

      /**
       * System view.
       */
      class SysView : public View {
        public:

          /**
           * Construct a SysView.
           * @param width the width.
           * @param height the height.
           * @param x the top-left x.
           * @param y the top-left y.
           * @param screen the Screen to sit on.
           */
          SysView( int width, int height, int x, int y, Screen* screen );

          /** Destructor. */
          virtual ~SysView() {};

          /**
           * refresh (redraw) the View.
           */
          void xrefresh( const XSysView &data, bool history );

          /**
           * The minimum height (lines) required by the Footer.
           */
          static int getMinHeight()  { return 13; };

          /**
           * provide the optimal height for the Header.
           * @return the optimal height in lines.
           */
          static int getOptimalHeight() { return 13; };

        protected:

          /**
           * Get ncurses display attributes depending on the type of CPU chararcter.
           * @param c the CPU character.
           * @return the ncurses display attributes.
           */
          int attrFromCPUChar( std::string c );


      };

      /**
       * IO view.
       */
      class IOView : public View {
        public:

          /**
           * Construct a IOView.
           * @param width the width.
           * @param height the height.
           * @param x the top-left x.
           * @param y the top-left y.
           * @param screen the Screen to sit on.
           */
          IOView( int width, int height, int x, int y, Screen* screen );

          /** Destructor. */
          virtual ~IOView() {};

          /**
           * refresh (redraw) the View.
           */
          void xrefresh( const XIOView &data );

          /**
           * The minimum height (lines) required by the Footer.
           */
          static int getMinHeight();

          /**
           * provide the optimal height for the Header.
           * @return the optimal height in lines.
           */
          static int getOptimalHeight();

        protected:

          /**
           * Overloaded by descendent classes to take a sample.
           */
          virtual void doSample();

          /**
           * Get sector size through sector_size_cache_.
           * @param m the MajorMinor of the block device.
           * @return the device sector size in bytes.
           */
          unsigned long getSectorSize( const block::MajorMinor& m );

          /**
           * Get device name through name_cache_.
           */
          std::string getDeviceName( const block::MajorMinor& m );

          /** Cache of MajorMinor to sector size, cache miss will force read through. */
          std::map<block::MajorMinor,unsigned int> sector_size_cache_;

          /** Cache of device names to MajorMinor, cache miss will force read through. */
          std::map<block::MajorMinor,std::string> name_cache_;

          /** Maximum width of the filesystem mountpoint column. */
          static unsigned int max_mountpoint_width_;

          /** cache of device special files to MajorMinor */
          static std::map<std::string,block::MajorMinor> devicefilecache_;
      };

      /**
       * View of top processes.
       */
      class ProcessView : public View {
        public:

          /**
           * Construct a ProcessView.
           * @param width the width.
           * @param height the height.
           * @param x the top-left x.
           * @param y the top-left y.
           * @param screen the Screen to sit on.
           */
          ProcessView( int width, int height, int x, int y, Screen* screen );

          /** Destructor. */
          virtual ~ProcessView();

          /**
           * Refresh/redraw the ProcessView.
           */
          void xrefresh( const XProcView& data );

          /**
           * Resize the ProcessView.
           * @param width new width
           * @param height new height.
           * @param x new top-left x.
           * @param y new top-left y.
           */
          virtual void resize( int width, int height, int x, int y );

          /**
           * Get the minimal height for the ProcessView.
           * @return the minimal height in lines.
           */
          static int getMinHeight() { return 4; };

          /**
           * provide the optimal height for the ProcessView,
           * for this View as many as it can get.
           * @return the optimal height in lines.
           */
          static int getOptimalHeight() { return 10000; };
        protected:

          /** width of the wchan column. */
          static int width_wchan_;

          /** number of procs on the system when sampling disabled. */
          unsigned long disabled_procs_;

          /** maximum sample duration in seconds before being disabled. */
          static const double max_sample_time;

      };

      /**
       * View of top network devices.
       */
      class NetView : public View {
        public:
          /**
           * Construct a NetView.
           * @param width the width.
           * @param height the height.
           * @param x the top-left x.
           * @param y the top-left y.
           * @param screen the Screen to sit on.
           */
          NetView( int width, int height, int x, int y, Screen* screen );

          /** Destructor. */
          virtual ~NetView();

          /**
           * Refresh/redraw the ProcessView.
           */
          virtual void xrefresh( const XNetView& );

          /**
           * Take a NetView sample.
           */
          virtual void doSample();

          /**
           * Get the minimal height for the NetView.
           * @return the minimal height in lines.
           */
          static int getMinHeight();

          /**
           * provide the optimal height for the NetView,
           * @return the optimal height in lines.
           */
          static int getOptimalHeight();
        protected:
      };

      /**
       * The Palette provides reasonable colors within terminal capabilities,
       * hiding the details of such capabilities behind standardized calls.
       */
      class Palette {
        public:

          /**
           * Constructor.
           */
          Palette();

          /**
           * setup the Palette.
           */
          void setup();

          /**
           * get normal text color.
           * @return normal text color.
           */
          int getColorText() const { return idx_color_text_; };

          /**
           * get bold text color.
           * @return normal text color.
           */
          int getColorBoldText() const { return idx_color_bold_text_; };

          /**
           * get normal text color.
           * @return normal text color.
           */
          int getColorLine() const { return idx_color_line_; };

          /**
           * get running process color.
           * @return runinng process color.
           */
          int getColorRunningProc() const { return idx_color_running_proc_; };

          /**
           * get blocked process color.
           * @return blocked process color.
           */
          int getColorBlockedProc() const { return idx_color_blocked_proc_; };

          /**
           * get system cpu color.
           * @return system cpu color.
           */
          int getColorSystemCPU() const { return idx_system_mode_cpu_; };

          /**
           * get user cpu color.
           * @return user cpu color.
           */
          int getColorUserCPU() const { return idx_user_mode_cpu_; };

          /**
           * get nice cpu color.
           * @return nice cpu color.
           */
          int getColorNiceCPU() const { return idx_nice_mode_cpu_; };

          /**
           * get iowait color.
           * @return iowait color.
           */
          int getColorIOWaitCPU() const { return idx_iowait_mode_cpu_; };

          /**
           * get irq color.
           * @return irq color.
           */
          int getColorIRQCPU() const { return idx_irq_mode_cpu_; };

          /**
           * get softirq color.
           * @return softirq color.
           */
          int getColorSoftIRQCPU() const { return idx_softirq_mode_cpu_; };
        private:
          /** palette index entry for normal text. */
          int idx_color_text_;
          /** palette index entry for bold text. */
          int idx_color_bold_text_;
          /** palette index entry for horizontal and vertical lines. */
          int idx_color_line_;
          /** palette index entry for running processes. */
          int idx_color_running_proc_;
          /** palette index entry for blocked processes. */
          int idx_color_blocked_proc_;
          /** palette index entry for system mode CPU. */
          int idx_system_mode_cpu_;
          /** palette index entry for user mode CPU. */
          int idx_user_mode_cpu_;
          /** palette index entry for nice mode CPU. */
          int idx_nice_mode_cpu_;
          /** palette index entry for iowait mode CPU. */
          int idx_iowait_mode_cpu_;
          /** palette index entry for irq mode CPU. */
          int idx_irq_mode_cpu_;
          /** palette index entry for sortirq mode CPU. */
          int idx_softirq_mode_cpu_;
      };

      /**
       * The curses screen.
       */
      class Screen {
        public:

          /** Constructor. */
          Screen();

          /** Destructor. */
          ~Screen();

          /**
           * Stop the Screen.
           */
          void Stop() { stopped_  = true; };

          /**
           * Run the Screen in realtime mode.
           */
          void runRealtime();

          /**
           * Run the Screen in history mode.
           */
          void runHistory( persist::Database *db );

          /**
           * Report a message.
           * @param key key of the message.
           * @param prio priority of the message.
           * @param message the message text.
           */
          void reportMessage( unsigned int key, unsigned int prio, const std::string & message );

          /**
           * Clear a message.
           * @param key key of the message.
           */
          void clearMessage( unsigned int key );

          /**
           * The Screen palette.
           */
          Palette palette_;

        private:

          /**
           * Recalculate the window sizes.
           */
          void screenResize();

          /**
           * Initialize and start the ncurses Screen.
           */
          void initTerminal();

          /**
           * Deinitialize and stop the ncurses Screen.
           */
          void resetTerminal();

          /**
           * Determine if terminal size has changed.
           * @return true when the terminal size has changed.
           */
          bool sizeChanged();

          /**
           * ncurses main window.
           */
          WINDOW* wmain_;

          /** Header View. */
          View* vheader_;

          /** Footer View. */
          View* vfooter_;

          /** SysView. */
          View* vsys_;

          /** Disk/Filesystem View. */
          View* vio_;

          /** Process View. */
          View* vprocess_;

          /** Network View. */
          View* vnetwork_;

          /**
           * Structure to hold the terminal size (columns,lines).
           */
          struct winsize winsz_;

          /**
           * If true, the Screen is requested to stop.
           */
          bool stopped_;

          /**
           * The amount of time to sleep between wakeups.
           */
          unsigned int sleep_duration_ms_;

          /**
           * The sample interval.
           */
          unsigned int sample_interval_s_;

          /**
           * Footer scrolling interval.
           */
          unsigned int footer_scroll_interval_s_;

          /**
           * Termkey pointer for keyboard input.
           */
          TermKey *tk_;

          /**
           * Polling structure for keyboard input.
           */
          struct pollfd fd_;

      };

      void View::textOut( int x, int y, int attrs, std::string text ) {
        wattrset( window_, attrs );
        mvwprintw( window_, y, x, "%s", text.c_str() );
      }

      void View::textOut( int x, int y, int attrs, int i ) {
        std::stringstream ss;
        ss << i;
        wattrset( window_, attrs );
        mvwprintw( window_, y, x, "%s", ss.str().c_str() );
      }

      void View::charOut( int x, int y, int attrs, const chtype ch ) {
        wattrset( window_, attrs );
        mvwaddch( window_, y, x, ch );
      }

      void View::textOutRA( int x, int y, int w, int attrs, std::string text ) {
        wattrset( window_, attrs );
        std::stringstream ss;
        ss << std::setw(w) << std::right << text.substr(0,w);
        mvwprintw( window_, y, x, "%s", ss.str().c_str() );
      }

      void View::textOutRA( int x, int y, int w, int attrs, int i ) {
        wattrset( window_, attrs );
        std::stringstream ss;
        ss << std::setw(w) << std::right << i;
        mvwprintw( window_, y, x, "%s", ss.str().c_str() );
      }

      void View::textOutMoveXRA( int &x, int y, int w, int attrs, std::string text ) {
        wattrset( window_, attrs );
        std::stringstream ss;
        ss << std::setw(w) << std::right << text.substr(0,w);
        mvwprintw( window_, y, x, "%s", ss.str().c_str() );
        x = x + w + 1;
      }

      void View::textOutMoveXRA( int &x, int y, int w, int attrs, int i ) {
        wattrset( window_, attrs );
        std::stringstream ss;
        ss << std::setw(w) << std::right << i;
        mvwprintw( window_, y, x, "%s", ss.str().c_str() );
        x = x + w + 1;
      }

      void View::doubleOut( int x, int y, int attrs, double value ) {
        wattrset( window_, attrs );
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << value;
        mvwprintw( window_, y, x, "%s", ss.str().c_str() );
      }

      void View::doubleOutPct( int x, int y, int attrs, double value ) {
        wattrset( window_, attrs );
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << value << "%";
        mvwprintw( window_, y, x, "%s", ss.str().c_str() );
      }

      void View::hLine( int x1, int x2, int y, int attrs, chtype token ) {
        wattrset( window_, attrs );
        mvwhline( window_, y, x1, token, x2-x1 );
      }

      void View::vLine( int y1, int y2, int x, int attrs, chtype token ) {
        wattrset( window_, attrs );
        mvwvline( window_, y1, x, token, y2-y1 );
      }

    }; //namespace lmon

  }; //namespace tools

}; //namespace leanux

#endif
