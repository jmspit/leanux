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
 * utility class for tabular screen output - c++ source file.
 */
#ifndef LEANUX_TABULAR
#define LEANUX_TABULAR


#include "oops.hpp"
#include <iostream>
#include <map>
#include <vector>

namespace leanux {

  namespace tools {

    /**
     * Produce tabular output on terminal easily.
     * Use addcolumn to setup the columns, each column keyed
     * width the unique caption.
     * Use appendString and appendUL to add row cells.
     * Use dump to write the table to an ostream.
     */
    class Tabular {
      public:
        /** Constructor. */
        Tabular() {};

        /** Destructor. */
        ~Tabular() {};

        /**
         * Add a column.
         * @param caption the column caption.
         * @param alignright if true, alignright else alignleft.
         */
        void addColumn( const std::string& caption, bool alignright = true );

        /**
         * Append a std::string to the Tabular.
         * @param caption caption of the column.
         * @param value the std::string value for the cell.
         */
        void appendString( const std::string& caption, const std::string& value );

        /**
         * Append an unsigned long to the Tabular.
         * @param caption caption of the column.
         * @param value the unsigned long value for the cell.
         */
        void appendUL( const std::string& caption, const unsigned long& value );

        /**
         * Dump or 'draw' the contents to a stream.
         */
        void dump( std::ostream& os );

        /**
         * Clear all contents, including columns.
         */
        void clear();

        /**
         * Get number of columns in the Tabular.
         * @return number of columns.
         */
        size_t columnCount() const { return colidx_.size(); };

        /**
         * Get number of rows in the Tabular.
         * @return number of rows.
         */
        size_t rowCount() const { if ( columns_.size() == 0 ) return 0; else return columns_[0].size(); };

      private:
        /** map of column names to column indexes. */
        std::map< std::string, unsigned int > colidx_;

        /** map of column indexes to column widths. */
        std::map< unsigned int, unsigned int > widths_;

        /** map of column indexes to column align. */
        std::map< unsigned int, bool > align_;

        /** outer vector by column index, inner vector column cells. */
        std::vector< std::vector<std::string> > columns_;
    };

  }; //namespace tools

}; //namespace leanux

#endif
