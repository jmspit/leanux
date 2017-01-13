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
 * leanux utility c++ header file.
 */
#include "util.hpp"

#include "oops.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <errno.h>

#include <limits.h>
#include <stdlib.h>


#include <fstream>

#include <iostream>
#include <sstream>

namespace leanux {

  namespace util {

    std::string leafDir( const std::string &dir ) {
      size_t p = dir.rfind( '/' );
      if ( p != std::string::npos ) {
        return dir.substr(p+1);
      }
      return "";
    }

    void tokenize( const std::string &str, std::list<std::string> &tokens, char sep, const std::set<std::string> &omit, bool reverse ) {
      tokens.clear();
      std::istringstream iss(str);
      std::string token;
      std::set<std::string>::const_iterator io;
      while ( std::getline( iss, token, sep ) ) {
        io = omit.find( token );
        if ( token != "" && io == omit.end() ) {
          if ( reverse ) tokens.push_front(token); else tokens.push_back(token);
        }
      }
    }

    std::string realPath( const std::string path ) {
      char rp[PATH_MAX];
      if ( realpath( path.c_str(), rp ) )
        return rp;
      else
        return path;
    }

    std::string shortenString( const std::string &src, size_t maxsz, char filler ) {
      if ( src.length() < maxsz ) return src; else {
        size_t sz = (maxsz-3) / 2;
        return src.substr(0, sz ) + filler + filler + filler + src.substr( src.length() - sz );
      }
    }

    std::string fileReadString( const std::string &filename ) {
      std::ifstream i( filename.c_str() );
      std::string result = "";
      if ( i.good() ) {
        getline( i, result );
      } else throw Oops( __FILE__, __LINE__, "failed to open '" + filename + "'" );
      return result;
    }

    long fileReadHexString( const std::string &filename ) {
      std::ifstream i( filename.c_str() );
      long result = -1;
      i >> std::hex;
      if ( i.good() ) {
        i >> result;
      } else throw Oops( __FILE__, __LINE__, "failed to open '" + filename + "'" );
      return result;
    }

    int fileReadInt( const std::string &filename ) {
      std::ifstream i( filename.c_str() );
      int result = -1;
      if ( i.good() ) {
        i >> result;
      } else throw Oops( __FILE__, __LINE__, "failed to open '" + filename + "'" );
      return result;
    }

    unsigned long fileReadUL( const std::string &filename ) {
      std::ifstream i( filename.c_str() );
      unsigned long result = 0;
      if ( i.good() ) {
        i >> result;
      } else throw Oops( __FILE__, __LINE__, "failed to open '" + filename + "'" );
      return result;
    }

    void Sleep( time_t seconds, long nanoseconds ) {
      struct timespec ts;
      ts.tv_sec = seconds;
      ts.tv_nsec =  nanoseconds;
      nanosleep( &ts, 0 );
    }

    bool directoryExists( const std::string &path ) {
      bool result = true;
      struct stat buf;
      int r = stat( path.c_str(), &buf );
      if ( r != 0 ) return false;
      result = S_ISDIR(buf.st_mode);
      return result;
    }

    bool fileReadAccess( const std::string& path ) {
      bool result = true;
      struct stat buf;
      int r = stat( path.c_str(), &buf );
      if ( r != 0 ) return false;
      result = S_ISREG(buf.st_mode);
      result = result && ( !access( path.c_str(), F_OK | R_OK ) );
      return result;
    }

    /**
     * maximum width a std::string returned by ByteStr.
     */
    const size_t bytestr_max_width = 64;

    /** pow(1024.0,1.0/3.0) */
    const long double base_1024 = 10.0793683991589837489755;

    /** log ( pow(1024.0,1.0/3.0) */
    const long double base_1024_log = 2.31049060186648436143741;

    std::string ByteStr( double bytes, int prec ) {
      char bytestrbuf[bytestr_max_width];
      bytestrbuf[0] = 0;
      char format[20];
      std::string suffix;
      if ( isnan( bytes ) ) {
        strncat( bytestrbuf, "nan", bytestr_max_width );
      } else
      if ( bytes != 0.0 ) {
        // the order of the number
        double baseorder = floor( log( fabs(bytes) )/base_1024_log );
        // the scale multiplier
        double scale = floor( baseorder / 3.0 );
        // the order of the number remaining after eating away scale powers of 1024^(1/3)
        double remainorder = floor( log(bytes/pow(base_1024,scale*3))/log(10) );
        double p = 0;
        if ( remainorder == 0 ) p = prec-1;
        else if ( remainorder == 1 ) p = prec-2;
        else if ( remainorder == 2 ) p = prec-3;
        else if ( remainorder == 3 ) p = 0;
        if ( p < 0 )
          snprintf( format, 20, "%%.0f%%s" );
        else
          snprintf( format, 20, "%%.%.0ff%%s", p );

        if ( scale < 0 ) {
          suffix = "";
           if ( prec-1 < 0 )
            snprintf( format, 20, "%%.0%%s" );
          else
            snprintf( format, 20, "%%.%if%%s", prec-1 );
          scale = 0;
        }
        else if ( scale < 1 ) suffix = "";
        else if ( scale < 2 ) suffix = "K";
        else if ( scale < 3 ) suffix = "M";
        else if ( scale < 4 ) suffix = "G";
        else if ( scale < 5 ) suffix = "T";
        else if ( scale < 6 ) suffix = "P";
        else if ( scale < 7 ) suffix = "E";
        else if ( scale < 8 ) suffix = "Z";
        else if ( scale < 9 ) suffix = "Y";
        snprintf( bytestrbuf, bytestr_max_width, format, bytes/pow(base_1024,scale*3.0), suffix.c_str() );
      } else strncat( bytestrbuf, "0", bytestr_max_width );
      return bytestrbuf;
    }

    std::string TimeStrSec( double time ) {
      return TimeStrMicro( time * 1000000.0 );
    }

    std::string TimeStrMicro( double time ) {
      char buffer[128];
      char mu = 'u';
      if ( fabs(time) == 0.0 )
        return "0s";
      else if ( fabs(time) < 1000.0 )
        snprintf( buffer, 128, "%0.f%cs", time, mu );
      else if ( fabs(time) < 100000.0 )
        snprintf( buffer, 128, "%.1fms", time / 1000.0 );
      else if ( fabs(time) < 1000000.0 )
        snprintf( buffer, 128, "%.0fms", time / 1000.0 );
      else if ( fabs(time) < 1000000.0*60.0 )
        snprintf( buffer, 128, "%.2fs", time / 1000000.0 );
      else if ( fabs(time) < 1000000.0*60.0*60.0 )
        if ( fmod( time, 60.0*1000000.0 )/1000000.0 >= 0.5 )
          snprintf( buffer, 128, "%0.fm%0.fs", floor( time / (60.0*1000000.0) ), fmod( time, 60.0*1000000.0 )/1000000.0 );
        else
          snprintf( buffer, 128, "%0.fm", floor( time / (60.0*1000000.0) ) );
      else if ( fabs(time) < 1000000.0*60.0*60.0*24.0 )
      {
        double h = floor( time / (60.0*60.0*1000000.0) );
        double m = floor( ( time - h * 60.0*60.0*1000000.0 ) / (60.0*1000000.0) );
        double s = ( time - h * 60.0*60.0*1000000.0 - m * 60.0*1000000.0 )  / 1000000.0;
        if ( s >= 0.5  )
          snprintf( buffer, 128, "%0.fh%0.fm%0.fs", h, m, s );
        else
          snprintf( buffer, 128, "%0.fh%0.fm", h, m );
      } else {
        double d = floor( time / (24.0*60.0*60.0*1000000.0) );
        double ts = d * (24.0*60.0*60.0*1000000.0);
        double h = floor( (time-ts) / (60.0*60.0*1000000.0) );
        ts += h * (60.0*60.0*1000000.0);
        double m = floor( ( time - ts ) / (60.0*1000000.0) );
        ts += m * (60.0*1000000.0);
        double s = ( time - ts )  / 1000000.0;
        snprintf( buffer, 128, "%0.fd%0.fh%0.fm%0.fs", d, h, m, s );
      }
      return buffer;
    }

    std::string NumStr( double num, int prec ) {
      char numstrbuf[32];
      numstrbuf[0] = 0;
      char format[20];
      std::string suffix;
      if ( isnan( num ) ) {
        strncat( numstrbuf, "nan", sizeof(numstrbuf) );
      } else
      if ( num != 0.0 ) {
        // the order of the number
        double baseorder = floor( log10( fabs(num) ) );
        // the scale multiplier, 3=k=1E3, 6=M=1E6, 9=G=1E9, 12=T=1E12
        double scale = floor( baseorder / 3.0 );
        // the order of the number remaining after eating away scale powers of 10
        double remainorder = floor( log10(num/pow(10.0,scale*3)) );
        if ( prec > fabs(remainorder) + 1 )
            snprintf( format, 20, "%%.%.0ff%%s", prec-(remainorder+1) );
        else
            snprintf( format, 20, "%%.0f%%s" );

        if ( scale < 0 ) {
          suffix = "";
          snprintf( format, 20, "%%.%if%%s", prec-1 );
          scale = 0;
        }
        else if ( scale < 1 ) suffix = "";
        else if ( scale < 2 ) suffix = "k";
        else if ( scale < 3 ) suffix = "M";
        else if ( scale < 4 ) suffix = "G";
        else if ( scale < 5 ) suffix = "T";
        else if ( scale < 6 ) suffix = "P";
        else if ( scale < 7 ) suffix = "E";
        else if ( scale < 8 ) suffix = "Z";
        snprintf( numstrbuf, sizeof(numstrbuf), format, num/pow(10.0,scale*3.0), suffix.c_str() );
      } else strncat( numstrbuf, "0", sizeof(numstrbuf) );
      return numstrbuf;
    }


    double deltaTime( const struct timeval &t1, const struct timeval &t2 ) {
      double dt = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) /  1.0E6;
      return dt;
    }

    std::string getUserConfigDir() {
      char *env = getenv("XDG_CONFIG_HOME");
      if ( env ) return env;
      env = getenv("HOME");
      if ( env ) return env;
      struct passwd *pwd;
      pwd = getpwuid(getuid());
      if ( pwd ) return pwd->pw_dir;
      else throw Oops( __FILE__, __LINE__, "cannot determine user configuration directory" );
    }

    void listDir( const std::string& path, const std::string& match, std::list< std::string > &result ) {
      result.clear();
      DIR *d;
      struct dirent *dir;
      d = opendir( path.c_str() );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( strncmp( dir->d_name, match.c_str(), match.length() ) == 0 ) {
            result.push_back( dir->d_name );
          }
        }
      } else throw Oops( __FILE__, __LINE__, errno );
      closedir( d );
    }

    std::string findDir( const std::string& path, const std::string& match ) {
      std::string result = "";
      DIR *d;
      struct dirent *dir;
      d = opendir( path.c_str() );
      if ( d ) {
        while ( (dir = readdir(d)) != NULL ) {
          if ( strncmp( dir->d_name, ".", 1 ) != 0 ) {
            if ( match.length() == 0 || strncmp( dir->d_name, match.c_str(), match.length() ) == 0 ) {
              result = dir->d_name;
              break;
            }
          }
        }
      }
      closedir( d );
      return result;
    }

    std::string localStrISODateTime() {
      time_t t;
      char out[200];
      memset( out, 0, 200 );
      struct tm *tmp;
      t = time(NULL);
      tmp = localtime( &t );
      strftime( out, sizeof(out), "%F %T", tmp );
      return out;
    }

    std::string localStrISODateTime( time_t t ) {
      char out[200];
      memset( out, 0, 200 );
      struct tm *tmp;
      tmp = localtime( &t );
      strftime( out, sizeof(out), "%F %T", tmp );
      return out;
    }

    std::string localStrISOTime() {
      time_t t;
      char out[200];
      memset( out, 0, 200 );
      struct tm *tmp;
      t = time(NULL);
      tmp = localtime( &t );
      strftime( out, sizeof(out), "%T", tmp );
      return out;
    }

    std::string localStrISOTime( time_t t ) {
      char out[200];
      memset( out, 0, 200 );
      struct tm *tmp;
      tmp = localtime( &t );
      strftime( out, sizeof(out), "%T", tmp );
      return out;
    }

    Tracer* Tracer::tracer_ = 0;

    Tracer::Tracer( const std::string& filename ) {
      of_ = new std::ofstream( filename.c_str() );
    }

    void Tracer::Trace( const std::stringstream& s ) {
      Tracer::Trace( s.str() );
    }

    void Tracer::Trace( const std::string& s ) {
      time_t t;
      char out[200];
      struct tm *tmp;
      t = time(NULL);
      tmp = localtime( &t );
      strftime( out, sizeof(out), "%F %T", tmp );
      (*tracer_->of_) << out << " " << s << std::endl;
    }

    Tracer* Tracer::setTracer( const std::string& filename ) {
      if ( !tracer_ ) {
        tracer_ = new Tracer( filename );
      }
      return tracer_;
    };

    void RegExp::set( const std::string& expr ) {
      expr_ = expr;
      int r = regcomp( &regex_, expr_.c_str(), REG_EXTENDED );
      if ( r ) {
        throw Oops( __FILE__, __LINE__, "invalid regex : '" + expr + "'" );
      }
    }

    double Stopwatch::getElapsedSeconds() const {
      struct timeval tt;
      gettimeofday( &tt, NULL );
      return double(tt.tv_sec - t1_.tv_sec) + double(tt.tv_usec - t1_.tv_usec)/1.0E6;
    }

  }



}
