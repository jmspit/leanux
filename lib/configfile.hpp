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
 * @file configfile.hpp
 * leanux::util::ConfigFile c++ header file.
 */

#ifndef LEANUX_CONFIG_FILE
#define LEANUX_CONFIG_FILE

#include <string>
#include <iostream>
#include <map>
#include <list>

namespace leanux {

  namespace util {

    /**
     * A configuration file of name=value pairs.
     * File format:
     *   - empty lines or lines with first non-whitespace character='#' are ignored.
     *   - name=value<eol>
     * note that any whitespace in front of value is ignored.
     */
    class ConfigFile {
      public:

        /**
         * A name-value pair as configuration parameter.
         */
        struct Parameter {
          /** Default constructor. */
          Parameter() { name_ = ""; value_ = ""; };
          /** Initializer constructor. */
          Parameter( const std::string &name, const std::string &value ) { name_ = name; value_ = value; };
          /** configuration parameter name. */
          std::string name_;
          /** configuration parameter value. */
          std::string value_;
        };

        /**
         * RGB color.
         */
        struct RGB {
          /** red value. */
          unsigned short red;
          /** green value. */
          unsigned short green;
          /** blue value. */
          unsigned short blue;
        };

        /**
         * return the configuration parameter as a string.
         * @param name the configuration parameter name.
         * @return the configuration parameter value.
         */
        std::string getValue( const std::string &name ) const;

        /**
         * return the configuration parameter as a RGB struct.
         * @param name the configuration parameter name.
         * @return the configuration parameter value.
         */
        RGB getRGBValue( const std::string &name ) const;

        /**
         * return the configuration parameter as an int.
         * @param name the configuration parameter name.
         * @return the configuration parameter value.
         */
        int getIntValue( const std::string &name ) const;

        /**
         * return the configuration parameter as an int.
         * @param name the configuration parameter name.
         * @return the configuration parameter value.
         */
        std::list<std::string> getStringListValue( const std::string &name ) const;

        /**
         * Set the configuration parameter to an int value.
         * @param name the configuration parameter name.
         * @param value the integer value to set.
         */
        void setValue( const std::string name, int value );

        /**
         * Set the configuration parameter to a string value.
         * @param name the configuration parameter name.
         * @param value the string value to set.
         */
        void setValue( const std::string name, const std::string &value );

        /**
         * Set the configuration parameter to a RGB value.
         * @param name the configuration parameter name.
         * @param value the RGB value to set.
         */
        void setValue( const std::string name, const RGB &value );

        /**
         * Set the configuration parameter to a stringlist value.
         * @param name the configuration parameter name.
         * @param value the stringlist value to set.
         */
        void setValue( const std::string name, const std::list<std::string> value );

        /**
         * declare the configuration paramater and set a hardcoded default.
         * @param name the parameter name
         * @param value the parameter default value
         * @param description the parameter description
         * @param comment parameter comment
         */
        static void declareParameter( const std::string &name, const std::string &value, const std::string &description, const std::string &comment = "" );

        /**
         * read and interpret the configuration file.
         */
        void read();

        /**
         * write out the configuration file, including runtime modifications.
         * @param defaults if set, overwrite with default values
         */
        void write( bool defaults = false );

        /**
         * cleanup objects in configfiles_;
         */
        void cleanup();

        /**
         * Get the ConfigurationFile object for the appname, or
         * construct one if it does not exist.
         * setConfig( const std::string &appname, const std::string &filename )
         * must be called first.
         * @return pointer to the ConfigFile, or 0 if setConfig was not called.
         */
        static ConfigFile* getConfig();

        /**
         * set the application name and config file name.
         */
        static ConfigFile* setConfig( const std::string &appname, const std::string &filename );

        /**
         * Release (delete) the configFile.
         */
        static void releaseConfig() { if ( config_ ) { delete config_; config_ = 0; } };

      private:
        /**
         * Constructor, should not be called directly, use setConfig and getConfig to
         * create the ConfigFile.
         * @see getConfig.
         */
        ConfigFile( const std::string &appname, const std::string &filename );

        /**
         * Destructor, should not be called directly.
         * @see releaseConfig
         */
        ~ConfigFile() {};

        /**
         * name of application owning the ConfigFile
         */
        std::string appname_;

        /**
         * The filename of the ConfigFile
         */
        std::string filename_;

        /**
         * Current values
         */
        std::map<std::string,Parameter> values_;

        /**
         * Configuration parameter description.
         */
        static std::map<std::string,std::string> descriptions_;

        /**
         * Configuration parameter comments.
         */
        static std::map<std::string,std::string> comments_;

        /**
         * Configuration parameter defaults.
         */
        static std::map<std::string,Parameter> defaults_;

        /**
         * The singleton ConfigFile.
         */
        static ConfigFile* config_;

    };


  };  //namespace leanux


}; // namespace leanux

#endif
