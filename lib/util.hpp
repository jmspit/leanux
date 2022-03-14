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
 * leanux utility c++ header file.
 */
#ifndef LEANUX_UTIL_HPP
#define LEANUX_UTIL_HPP

#include <string>
#include <sstream>
#include <fstream>
#include <set>
#include <list>
#include <sys/time.h>
#include <regex.h>
#include <string.h>
#include "leanux-config.hpp"

namespace leanux {

  /**
   * Utility classes and functions.
   */
  namespace util {

    /**
     * return the build leanux version
     */
    inline std::string leanuxVersion() { return LEANUX_VERSION; };

    /**
     * returns the leaf of the tree - that string after the last '/'
     */
    std::string leafDir( const std::string &dir );

    void tokenize( const std::string &str, std::list<std::string> &tokens, char sep, const std::set<std::string> &omit, bool reverse = false );

    /**
     * Convert a path on a filesystem to the realpath
     * if the path is a link.
     * @param path the path to find the realpath for.
     * @return the realpath corresponding to path. If path is not a
     * link, the original path is returned.
     */
    std::string realPath( const std::string path );

    /**
     * Shorten a string by replacing characters from the middle with
     * three filler charachters. If maxsz >= src.lenght(), src is returned as is.
     * @param src the (constant) source string.
     * @param maxsz the maximum size of the returned string.
     * @param filler the maximum size of the returned string.
     * @return the shortened string.
     */
    std::string shortenString( const std::string &src, size_t maxsz, char filler = '.' );

    /**
     * read the file as a single string.
     * @throw Oops when the file cannot be read.
     * @param filename the file to read from.
     * @return the file contents as a string.
     */
    std::string fileReadString( const std::string &filename );

    /**
     * read the first data in the file as a hexadecimal string representation
     * of a signed long.
     * @throw Oops when the file cannot be read.
     * @param filename the file to read from.
     * @return the file contents as a long.
     */
    long fileReadHexString( const std::string &filename );

    /**
     * read the first data in the file as a string representation
     * of a signed int.
     * @throw Oops when the file cannot be read.
     * @param filename the file to read from.
     * @return the file contents as an int.
     */
    int fileReadInt( const std::string &filename );

    /**
     * read the first data in the file as a string representation
     * of a unsigned long.
     * @throw Oops when the file cannot be read.
     * @param filename the file to read from.
     * @return the file contents as an unsigned long.
     */
    unsigned long fileReadUL( const std::string &filename );

    /**
     * Test if the path is an existing directory.
     * @param path the directory to test for existence.
     * @return true when the directory exists.
     */
    bool directoryExists( const std::string &path );

    /**
     * Test if a file exists and can be read.
     * @param path the file to test for read access.
     * @return true when the path is a file and can be read.
     */
    bool fileReadAccess( const std::string& path );

    /**
     * Sleep seconds+nanoseconds.
     */
    void Sleep( time_t seconds, long nanoseconds );

    /**
     * convert a byte value to pretty print string.
     */
    std::string ByteStr( double bytes, int prec );

    /**
     * convert a time duration in microseconds to a pretty print string.
     */
    std::string TimeStrMicro( double time );

    /**
     * convert a time duration in seconds to a pretty print string.
     */
    std::string TimeStrSec( double time );

    /**
     * Convert a double (real number) to a pretty print string.
     * @param num the number to pretty print.
     * @param prec the number of digits after decimal to display.
     * @return the pretty print string.
     */
    std::string NumStr( double num, int prec = 2 );

    /**
     * Return the deltaof two timevals in seconds.
     */
    double deltaTime( const struct timeval &t1, const struct timeval &t2 );

    /**
     * Return the config dir for the current user.
     * It checks $XDG_CONFIG_HOME, HOME and getpwuid->pw_dir
     * in that order, first match returned.
     * @return the user's configuration directory.
     */
    std::string getUserConfigDir();

    /**
     * std::list subdirectories that match into result, if any.
     * @param path base directory to search.
     * @param match pattern to match.
     * @param result std::list of subdirectories matching.
     */
    void listDir( const std::string& path, const std::string& match, std::list< std::string > &result );

    /**
     * Find subdirectories, first match is returned, if any.
     * Any directory that starts with a '.' is not considered.
     * @param path base path / parent directory.
     * @param match the search pattern.
     * @return subdirectory or empty std::string if no match.
     */
    std::string findDir( const std::string& path, const std::string& match );

    /**
     * Return current local time in ISO format; YYYY-MM-DD HH24:MI:SS
     * @return a string with the local time
     */
    std::string localStrISODateTime();
    std::string localStrISODateTime(time_t);
    std::string localStrISOTime();
    std::string localStrISOTime(time_t);

    /**
     * Simple singleton tracing class.
     */
    class Tracer {
      public:

        /**
         * Destructor.
         */
        ~Tracer() { delete of_; of_ = NULL; };

        /**
         * Dump the contents of the stringstream.
         * @param s the stringstream to dump.
         */
        static void Trace( const std::stringstream& s );

        /**
         * Dump the contents of the stringstream.
         * @param s the string to dump.
         */
        static void Trace( const std::string& s );

        /**
         * Return the singleton.
         */
        static Tracer* getTracer() { return tracer_; };

        /**
         * Initialize the singleton.
         * @param filename the file to trace to.
         * @return the Tracer singleton.
         * @see Tracer::Tracer
         */
        static Tracer* setTracer( const std::string& filename );

      private:

        /**
         * Setup the Tracer.
         * Constructor is private as the singleton should be
         * realized with Tracer::setTracer and accessed with Tracer::getTracer.
         * @param filename file to trace to.
         * @see Tracer::setTracer, Tracer::getTracer
         */
        Tracer( const std::string& filename );

        /**
         * The tracing file stream.
         */
        std::ofstream *of_;

        /**
         * The singleton Tracer object.
         */
        static Tracer* tracer_;
    };

    /**
     * When defined, all TRACE macro invocations
     * will be honoured and written to the trace file.
     */
    #ifdef LEANUX_DEBUG
      #define TRACEON
    #endif

    /**
     * If not TRACEON, ignore TRACE macro invocations.
     */
    #ifdef TRACEON
      #define TRACE( s ) leanux::util::Tracer::Trace( s );
    #else
      #define TRACE( s )
    #endif

    /**
     * Stopwatch to time durations with microsecond precision.
     */
    class Stopwatch {
      public:

        Stopwatch() { gettimeofday( &t1_, NULL ); gettimeofday( &t2_, NULL ); };

        ~Stopwatch() {};

        /**
         * Start the Stopwatch.
         */
        void start() { gettimeofday( &t1_, NULL ); };

        /**
         * stop the Stopwatch.
         * @return number of seconds since last call to start.
         */
        double stop() { gettimeofday( &t2_, NULL ); return double(t2_.tv_sec - t1_.tv_sec) + double(t2_.tv_usec - t1_.tv_usec)/1.0E6; };

        double getElapsedSeconds() const;

      private:
        /** start time */
        struct timeval t1_;

        /** stop time */
        struct timeval t2_;

    };

    /**
     * POSIX regular expression wrapper.
     */
    class RegExp {
      public:
        RegExp() { memset( &regex_, 0, sizeof(regex_) ); expr_ = ""; };
        ~RegExp() { regfree( &regex_ ); };
        void set( const std::string& expr );
        bool match( const std::string& str ) const;
      private:
        regex_t regex_;
        std::string expr_;
    };

    inline bool RegExp::match( const std::string& str ) const {
      int r = regexec( &regex_, str.c_str(), 0, NULL, 0 );
      return r == 0;
    }

  } //namespace util

} //namespace leanux

#endif
