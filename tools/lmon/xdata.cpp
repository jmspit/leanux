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
 * presentation data definiton.
 */

#include <algorithm>
#include <string>
#include <vector>
#include <math.h>
#include "cpu.hpp"

namespace leanux {

  namespace tools {

    namespace lmon {

      /**
       * Utility class for SysView::makeCPUBar.
       * The CPU character or symbol is the unique key.
       * The remain_ attribute, initially equal to value_,
       * is decreased  when the symbol's appearence (height_)
       * in the std::string is increased, The operator<( const CPUBarDigit& s )
       * allows for a sort that present the CPUDigit with the biggest remain_
       * to (fairly) distribute characters based on contribution.
       */
      class CPUBarDigit {
        public:
          /**
           * Construct from symbol and value.
           * @param symbol the cpu character.
           * @param value the (initial) cpu value.
           */
          CPUBarDigit( char symbol, double value ) { symbol_ = symbol; value_ = value; remain_ = value; height_ = 0; };

          /**
           * Copy constructor.
           * @param s the CPUBarDigit to copy from.
           */
          CPUBarDigit( const CPUBarDigit& s ) { symbol_ = s.symbol_; value_ = s.value_; remain_ = s.remain_; height_ = s.height_; };

          /**
           * Compare on remainder,
           * @param s CPUBarDigit to compare to.
           * @return true if *this < s.
           */
          bool operator<( const CPUBarDigit& s ) const { return s.remain_ < remain_; };

          /** the cpu character. */
          char symbol_;

          /** initial value. */
          double value_;

          /** remaining value. */
          double remain_;

          /** number of attributed cpu characters. */
          int height_;
      };

      /**
       * create a (vertical) CPU bar string from the CPUStat for terminal-based output.
       * @param stat the CPUStat statistics
       * @param maxvalue the maximum possible value
       * @param maxlines the maximum number of characters (lines) in the string
       */
      std::string makeCPUBar( const cpu::CPUStat &stat, double maxvalue, int maxlines ) {
        std::string s = "";

        std::vector<CPUBarDigit> vdigits;
        vdigits.push_back( CPUBarDigit( 'u', stat.user ) );
        vdigits.push_back( CPUBarDigit( 'n', stat.nice ) );
        vdigits.push_back( CPUBarDigit( 's', stat.system ) );
        vdigits.push_back( CPUBarDigit( 'w', stat.iowait ) );
        vdigits.push_back( CPUBarDigit( 'i', stat.irq ) );
        vdigits.push_back( CPUBarDigit( 'o', stat.softirq ) );
        std::sort( vdigits.begin(), vdigits.end() );

        double total = getCPUUsageTotal( stat );

        double line_frac = maxvalue/ (double)maxlines;

        int lines = std::min( maxlines, (int)round( total / line_frac ) );
        for ( int i = 0; i < lines; i++ ) {
          vdigits[0].remain_ -= line_frac;
          vdigits[0].height_++;
          s += vdigits[0].symbol_;
          sort( vdigits.begin(), vdigits.end() );
        };
        sort( s.begin(), s.end() );
        return s;
      }


    }; //namespace lmon

  }; //namespace tools

}; //namespace leanux

