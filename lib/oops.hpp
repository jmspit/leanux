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
 * leanux::Oops c++ header file.
 */
#ifndef LEANUX_OOPS_HPP
#define LEANUX_OOPS_HPP

#include <string>

namespace leanux {

  /**
   * leanux exception class.
   */
  class Oops {
    public:
      /**
       * construct Oops from source file name, line and with msg.
       * @param file the source file throwing the Oops.
       * @param line the line on which the Oops is thrown.
       * @param msg the Oops message.
       */
      Oops( const char* file, int line, const std::string &msg );

      /**
       * construct from source file name, line and system error.
       * @param file the source file throwing the Oops.
       * @param line the line on which the Oops is thrown.
       * @param err the errno converted to error std::string with strerror.
       */
      Oops( const char* file, int line, int err );

      /**
       * Get the Oops message.
       * @return the Oops message.
       */
      const std::string& getMessage() const { return msg_; };

    protected:

      static std::string stripSourceDir( const std::string &srcdir );

      /** The Oops message. */
      std::string msg_;
  };

  /**
   * Write an Oops to a stream.
   * @param os the stream to write to.
   * @param oops the Oops to write.
   * @return reference to the stream.
   */
  inline std::ostream& operator<<( std::ostream &os, const Oops &oops ) {
    return (os << oops.getMessage());
  }

}

#endif
