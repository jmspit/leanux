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

#include "tabular.hpp"
#include <iomanip>
#include <sstream>

namespace leanux {

  namespace tools {

  void Tabular::clear() {
    colidx_.clear();
    widths_.clear();
    align_.clear();
    columns_.clear();
  }

  void Tabular::addColumn( const std::string& caption, bool alignright ) {
    unsigned int newidx = colidx_.size();
    widths_[ newidx ] = caption.length() + (newidx != 0);
    align_[ newidx ] = alignright;
    colidx_[ caption ] = newidx;
    std::vector<std::string> tmp;
    tmp.push_back( caption );
    columns_.reserve( newidx+1 );
    columns_.push_back( tmp );
  }

  void Tabular::appendString( const std::string& caption, const std::string& value ) {
    if ( colidx_.find( caption ) == colidx_.end() ) {
      throw Oops( __FILE__, __LINE__, "adding data tot non-existing column" );
    }
    if ( value.length() + 1 > widths_[ colidx_[caption] ] ) widths_[ colidx_[caption] ] = value.length() + 1;
    columns_[ colidx_[caption ] ].push_back( value );
  }

  void Tabular::appendUL( const std::string& caption, const unsigned long& value ) {
    std::stringstream ss;
    ss << value;
    appendString( caption, ss.str() );
  }

  void Tabular::dump( std::ostream& os ) {
    if ( columns_.size() > 0 ) {
      unsigned int rows = columns_[0].size();
      for ( unsigned int r = 0; r < rows; r++ ) {
        if ( r == 1 ) {
          for ( unsigned int c = 0; c < columns_.size(); c++ ) {
            if ( align_[c] ) {
              os << std::setfill('-');
              os << std::right << ' ' << std::setw( widths_[c] -1 ) << '-';
            } else {
              os << std::setfill('-');
              if ( c != 0 ) os << " ";
              os << std::setw( widths_[c]-(c!=0)) << std::left << '-' << std::right;
            }
          }
          os << std::setfill(' ') << std::endl;
        }
        for ( unsigned int c = 0; c < columns_.size(); c++ ) {
          if ( align_[c] ) {
            os << std::setw( widths_[c] ) << (columns_[c])[r];
          } else {
            if ( c != 0 ) os << " ";
            os << std::setw( widths_[c]-(c!=0)) << std::left << (columns_[c])[r] << std::right;
          }
        }
        os << std::endl;
      }
    }
  }

  }; //namespace tools

}; //namespace leanux
