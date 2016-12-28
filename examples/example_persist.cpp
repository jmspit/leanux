//========================================================================
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

//========================================================================
//  Author: Jan-Marten Spit
//========================================================================
#include "oops.hpp"
#include "persist.hpp"
#include "system.hpp"
#include <iostream>

using namespace leanux;
using namespace std;

const string test_db_name = "lard-test.db";

int main( int argc, char* argv[] ) {

  try {
    init();
    {
      persist::Database db( test_db_name );
      db.enableForeignKeys();

      persist::DDL ddl( db );
      persist::DML dml( db );
      ddl.prepare( "create table if not exists cmd ( cmd text not null, cmdline text not null );" );
      ddl.execute();
      db.begin();
      dml.prepare( "insert into cmd (cmd,cmdline) values (:b1, :b2)" );
      dml.bind( 1, "sshd" );
      dml.bind( 2, "sshd: spjm [priv]" );
      dml.execute();
      db.commit();

      persist::Query qry( db );
      qry.prepare( "select * from cmd order by cmd" );
      while ( qry.step() ) {
        cout << ';' << qry.getText( 0 );
        cout << ';' << qry.getText( 1 );
        cout << endl;
      }
    }
    unlink( test_db_name.c_str() );
  }
  catch ( const leanux::Oops &oops ) {
    unlink( test_db_name.c_str() );
    cerr << oops << endl;
    return 1;
  }
  return 0;
};
