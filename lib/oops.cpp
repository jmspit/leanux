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
 * leanux::Oops c++ source file.
 */
#include "oops.hpp"
#include "leanux-config.hpp"

#include <string.h>
#include <sstream>

namespace leanux {

  Oops::Oops( const char* file, int line, const std::string &msg ) {
      std::stringstream ss;
      ss << stripSourceDir( file ) << ":" << line << " " << msg;
      msg_ = ss.str();
  }

  Oops::Oops( const char* file, int line, int err ) {
      std::stringstream ss;
      ss << stripSourceDir( file ) << ":" << line << " " << strerror( err );
      msg_ = ss.str();
  }

  std::string Oops::stripSourceDir( const std::string &srcdir ) {
      return srcdir.substr( strlen(LEANUX_SRC_DIR) + 1 );
  }

}
