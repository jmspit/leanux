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
 * @file configfile.cpp
 * leanux::util::ConfigFile c++ source file.
 */


#include "configfile.hpp"
#include "oops.hpp"
#include "util.hpp"
#include <fstream>
#include <stdlib.h>

namespace leanux {

  namespace util {

    std::map<std::string,ConfigFile::Parameter> ConfigFile::defaults_;
    std::map<std::string,std::string> ConfigFile::descriptions_;
    std::map<std::string,std::string> ConfigFile::comments_;
    ConfigFile* ConfigFile::config_ = 0;

    ConfigFile::ConfigFile( const std::string &appname, const std::string &filename ) {
      appname_ = appname;
      filename_ = filename;
      read();
    }

    void ConfigFile::read() {
      std::ifstream ifs( filename_.c_str() );
      while( ifs.good() ) {
        std::string line;
        getline( ifs, line );
        if ( ifs.good() ) {
          unsigned int state = 0;
          const unsigned int state_line_start = 0;
          const unsigned int state_line_type = 1;
          const unsigned int state_read_name = 2;
          const unsigned int state_trimws = 3;
          const unsigned int state_read_value = 4;
          const unsigned int state_finish = 100;
          std::string name = "";
          std::string value = "";
          size_t pos = 0;
          while ( state != state_finish && pos < line.length() ) {
            switch ( state ) {
              case state_line_start: //initial for each line
                if ( !std::isspace( line[pos] ) ) state = state_line_type; else pos++;
                break;
              case state_line_type: //what sort of line?
                if ( line[pos] == '#' ) state = state_finish;
                else if ( !std::isspace(line[pos]) ) {
                  state = state_read_name;
                }
                break;
              case state_read_name: //read the name
                if ( line[pos] == '=' ) {
                  state = state_trimws;
                } else if ( !std::isspace(line[pos]) ) {
                  name.push_back( line[pos] );
                }
                pos++;
                break;
              case state_trimws: //trim ws before value
                if ( !std::isspace(line[pos]) ) state = state_read_value; else pos++;
                break;
              case state_read_value: //read the value
                if ( pos < line.length() ) {
                  value+= line[pos++];
                  if ( pos >= line.length() ) state = state_finish;
                } else state = state_finish;
                break;
            }
          }
          if ( name != "" && value != "" ) {
            std::map<std::string,Parameter>::const_iterator def = defaults_.find( name );
            if ( def == defaults_.end() ) throw Oops( __FILE__, __LINE__, "invalid configuration parameter '" + name + "'" );
            Parameter param( name, value );
            values_[name] = param;
          }
        }
      }
    }

    void ConfigFile::write( bool defaults) {
      std::ofstream ofs( filename_.c_str() );
      if ( ofs.good() ) {
        ofs << "# " << appname_ << " configuration file" << std::endl << std::endl;
        //ofs << "# specify RGB values as (RED,GREEN,BLUE), each scalar >=0 and <=255" << std::endl << std::endl;
        for ( std::map<std::string,Parameter>::const_iterator d = defaults_.begin(); d != defaults_.end(); ++d ) {
          std::string write_value;
          if ( defaults ) {
            write_value = d->second.value_;
          } else {
            std::map<std::string,Parameter>::const_iterator user = values_.find( d->first );
            if ( user == values_.end() ) write_value = d->second.value_; else write_value = user->second.value_;
          }
          ofs << "# " << d->first << ": " << descriptions_[d->first] << std::endl;
          //
          std::list<std::string> tokens;
          std::set<std::string> omit;
          util::tokenize( comments_[d->first], tokens, ' ', omit );
          int cr = 2;
          if ( tokens.size() > 0 ) {
            ofs << "# ";
            for ( std::list<std::string>::const_iterator t = tokens.begin(); t != tokens.end(); t++ ) {
              if ( cr + (*t).length() > 80 ) {
                ofs << std::endl << "# " << *t << " ";
                cr = 2;
              } else {
                ofs << *t << " ";
                cr += (*t).length();
              }
            }
            ofs << std::endl;
          }
          //
          ofs << "# default " << d->first << "=" << d->second.value_ << std::endl;
          ofs << d->first << "=" << write_value << std::endl;
          ofs << std::endl;
        }
      } else throw Oops( __FILE__, __LINE__, "unable to write to '" + filename_ + "'" );
    }

    std::string ConfigFile::getValue( const std::string &name ) const {
      std::map<std::string,Parameter>::const_iterator user = values_.find( name );
      std::map<std::string,Parameter>::const_iterator def = defaults_.find( name );
      if ( def == defaults_.end() ) throw Oops( __FILE__, __LINE__, "invalid configration parameter '" + name + "'" );
      if ( user == values_.end() ) return def->second.value_; else return user->second.value_;
    }

    std::list<std::string> ConfigFile::getStringListValue( const std::string &name ) const {
      std::string value = getValue( name );
      std::stringstream iss(value);
      std::list<std::string> result;
      for (std::string token; std::getline(iss, token, ',' ); ) {
        result.push_back( token );
      }
      return result;
    }

    ConfigFile::RGB ConfigFile::getRGBValue( const std::string &name ) const {
      ConfigFile::RGB rgb;
      std::string s = getValue( name );
      const unsigned int state_initial = 0;
      const unsigned int state_red = 1;
      const unsigned int state_green = 2;
      const unsigned int state_blue = 3;
      const unsigned int state_finished = 100;
      const unsigned int state_invalid = 101;
      unsigned int state = state_initial;
      std::string red = "";
      std::string green = "";
      std::string blue = "";
      unsigned int pos = 0;
      while ( pos < s.length() ) {
        switch ( state ) {
          case state_initial:
            if ( s[pos] == '(' ) state = state_red; else state = state_invalid;
            pos++;
            break;
          case state_red:
            if ( s[pos] != ',' ) {
              red += s[pos];
            } else state = state_green;
            pos++;
            break;
          case state_green:
            if ( s[pos] != ',' ) {
              green += s[pos];
            } else state = state_blue;
            pos++;
            break;
          case state_blue:
            if ( s[pos] != ')' ) {
              blue += s[pos];
            } else state = state_finished;
            pos++;
            break;
        }
      }
      if ( state != state_finished ) throw Oops( __FILE__, __LINE__, "invalid RGB value for '" + name + "'" );
      else {
        rgb.red = atoi( red.c_str() );
        rgb.green = atoi( green.c_str() );
        rgb.blue = atoi( blue.c_str() );
      }
      return rgb;
    }

    int ConfigFile::getIntValue( const std::string &name ) const {
      std::string v = getValue( name );
      return atoi( v.c_str() );
    }

    void ConfigFile::setValue( const std::string name, int value ) {
      std::map<std::string,Parameter>::const_iterator def = defaults_.find( name );
      if ( def == defaults_.end() ) throw Oops( __FILE__, __LINE__, "invalid configuration parameter '" + name + "'" );
      std::stringstream ss;
      ss << std::fixed << value;
      values_[name].value_ = ss.str();
    }

    void ConfigFile::setValue( const std::string name, const std::string &value ) {
      std::map<std::string,Parameter>::const_iterator def = defaults_.find( name );
      if ( def == defaults_.end() ) throw Oops( __FILE__, __LINE__, "invalid configuration parameter '" + name + "'" );
      values_[name].value_ = value;
    }

    void ConfigFile::setValue( const std::string name, const RGB &value ) {
      std::map<std::string,Parameter>::const_iterator def = defaults_.find( name );
      if ( def == defaults_.end() ) throw Oops( __FILE__, __LINE__, "invalid configuration parameter '" + name + "'" );
      std::stringstream ss;
      ss << std::fixed << "(" << value.red << "," << value.green << "," << value.blue << ")";
      values_[name].value_ = ss.str();
    }

    void ConfigFile::setValue( const std::string name, const std::list<std::string> value ) {
      std::map<std::string,Parameter>::const_iterator def = defaults_.find( name );
      if ( def == defaults_.end() ) throw Oops( __FILE__, __LINE__, "invalid configuration parameter '" + name + "'" );
      std::stringstream ss;
      for ( std::list<std::string>::const_iterator i = value.begin(); i != value.end(); i++ ) {
        if ( i != value.begin() ) ss << ",";
        ss << (*i);
      }
      values_[name].value_ = ss.str();
    }

    void ConfigFile::declareParameter( const std::string &name, const std::string &value, const std::string &description, const std::string &comment ) {
      ConfigFile::Parameter param;
      param.name_ = name;
      param.value_ = value;
      defaults_[name] = param;
      descriptions_[name] = description;
      comments_[name] = comment;
    }

    ConfigFile* ConfigFile::getConfig() {
      return config_;
    }

    ConfigFile* ConfigFile::setConfig( const std::string &appname, const std::string &filename ) {
      config_ = new ConfigFile( appname, filename );
      return config_;
    }


  };  //namespace leanux


}; // namespace leanux
