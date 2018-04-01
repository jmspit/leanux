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

#include "persist.hpp"
#include "oops.hpp"
#include "util.hpp"
#include <map>
#include <sstream>
#include <iostream>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <string.h>

/**
 * @file
 * SQLite3 wrapper c++ source file.
 */

namespace leanux {

  namespace persist {

    static void sqlite_ext_pow(sqlite3_context *context, int argc, sqlite3_value **argv) {
      double r1 = 0.0;
      double r2 = 0.0;
      double val;

      assert( argc==2 );

      if( sqlite3_value_type(argv[0]) == SQLITE_NULL || sqlite3_value_type(argv[1]) == SQLITE_NULL ){
        sqlite3_result_null(context);
      } else {
        r1 = sqlite3_value_double(argv[0]);
        r2 = sqlite3_value_double(argv[1]);
        errno = 0;
        val = pow(r1,r2);
        if (errno == 0) {
          sqlite3_result_double(context, val);
        } else {
          sqlite3_result_error(context, strerror(errno), errno);
        }
      }
    }

    static void sqlite_ext_log2(sqlite3_context *context, int argc, sqlite3_value **argv) {
      double r1 = 0.0;
      double val;

      assert( argc==1 );

      if( sqlite3_value_type(argv[0]) == SQLITE_NULL ){
        sqlite3_result_null(context);
      } else {
        r1 = sqlite3_value_double(argv[0]);
        errno = 0;
        val = log2(r1);
        if (errno == 0) {
          sqlite3_result_double(context, val);
        } else {
          sqlite3_result_error(context, strerror(errno), errno);
        }
      }
    }

    static void sqlite_ext_ceil(sqlite3_context *context, int argc, sqlite3_value **argv) {
      double r1 = 0.0;
      double val;

      assert( argc==1 );

      if( sqlite3_value_type(argv[0]) == SQLITE_NULL ){
        sqlite3_result_null(context);
      } else {
        r1 = sqlite3_value_double(argv[0]);
        errno = 0;
        val = ceil(r1);
        if (errno == 0) {
          sqlite3_result_double(context, val);
        } else {
          sqlite3_result_error(context, strerror(errno), errno);
        }
      }
    }

    static void sqlite_ext_floor(sqlite3_context *context, int argc, sqlite3_value **argv) {
      double r1 = 0.0;
      double val;

      assert( argc==1 );

      if( sqlite3_value_type(argv[0]) == SQLITE_NULL ){
        sqlite3_result_null(context);
      } else {
        r1 = sqlite3_value_double(argv[0]);
        errno = 0;
        val = floor(r1);
        if (errno == 0) {
          sqlite3_result_double(context, val);
        } else {
          sqlite3_result_error(context, strerror(errno), errno);
        }
      }
    }

    Database::Database( const std::string &filename, WaitHandler handler ) {
      database_ = NULL;
      int r = sqlite3_open_v2( filename.c_str(), &database_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, NULL );
      if ( r != SQLITE_OK ) {
        throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      }
      int eTextRep = SQLITE_UTF8;
      #if SQLITE_VERSION_NUMBER > 3008003
      eTextRep |= SQLITE_DETERMINISTIC;
      #endif
      r = sqlite3_create_function_v2( database_,
                                      "power",
                                      2,
                                      eTextRep,
                                      NULL,
                                      sqlite_ext_pow,
                                      NULL,
                                      NULL,
                                      NULL );
      if ( r != SQLITE_OK ) {
        throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      }
      r = sqlite3_create_function_v2( database_,
                                      "ceil",
                                      1,
                                      eTextRep,
                                      NULL,
                                      sqlite_ext_ceil,
                                      NULL,
                                      NULL,
                                      NULL );
      if ( r != SQLITE_OK ) {
        throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      }
      r = sqlite3_create_function_v2( database_,
                                      "floor",
                                      1,
                                      eTextRep,
                                      NULL,
                                      sqlite_ext_floor,
                                      NULL,
                                      NULL,
                                      NULL );
      if ( r != SQLITE_OK ) {
        throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      }
      r = sqlite3_create_function_v2( database_,
                                      "log2",
                                      1,
                                      eTextRep,
                                      NULL,
                                      sqlite_ext_log2,
                                      NULL,
                                      NULL,
                                      NULL );
      if ( r != SQLITE_OK ) {
        throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      }


      if ( handler ) sqlite3_busy_handler( database_, handler, database_ );
    }

    Database::~Database() {
      int r = sqlite3_close_v2( database_ );
      if ( r != SQLITE_OK ) std::cout << "sqlite3_close returns " << r << std::endl;
    }

    void Database::enableForeignKeys() throw(Oops) {
      int succes = 0;
      int r = sqlite3_db_config( database_, SQLITE_DBCONFIG_ENABLE_FKEY, 1, &succes );
      if ( r != SQLITE_OK ) {
        throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      }
    }

    void Database::disableForeignKeys() throw(Oops) {
      int succes = 0;
      int r = sqlite3_db_config( database_, SQLITE_DBCONFIG_ENABLE_FKEY, 0, &succes );
      if ( r != SQLITE_OK ) {
        throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      }
    }

    void Database::enableTriggers() throw(Oops) {
      int succes = 0;
      int r = sqlite3_db_config( database_, SQLITE_DBCONFIG_ENABLE_TRIGGER, 1, &succes );
      if ( r != SQLITE_OK ) {
        throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      }
    }

    void Database::begin() throw(Oops) {
      std::stringstream ss;
      ss << "BEGIN";
      DDL ddl( *this );
      ddl.prepare( ss.str() );
      ddl.execute();
      ddl.close();
    }

    void Database::beginImmediate() throw(Oops) {
      std::stringstream ss;
      ss << "BEGIN IMMEDIATE";
      DDL ddl( *this );
      ddl.prepare( ss.str() );
      ddl.execute();
      ddl.close();
    }

    void Database::beginExclusive() throw(Oops) {
      std::stringstream ss;
      ss << "BEGIN EXCLUSIVE";
      DDL ddl( *this );
      ddl.prepare( ss.str() );
      ddl.execute();
      ddl.close();
    }

    void Database::commit() throw(Oops) {
      std::stringstream ss;
      ss << "COMMIT";
      DDL ddl( *this );
      ddl.prepare( ss.str() );
      ddl.execute();
      ddl.close();
    }

    void Database::rollback() throw(Oops) {
      std::stringstream ss;
      ss << "ROLLBACK";
      DDL ddl( *this );
      ddl.prepare( ss.str() );
      ddl.execute();
      ddl.close();
    }

    void Database::savepoint( const std::string &sp ) throw(Oops) {
      std::stringstream ss;
      ss << "SAVEPOINT " << sp;
      DDL ddl( *this );
      ddl.prepare( ss.str() );
      ddl.execute();
      ddl.close();
    }

    void Database::release( const std::string &sp ) throw(Oops) {
      std::stringstream ss;
      ss << "RELEASE " << sp;
      DDL ddl( *this );
      ddl.prepare( ss.str() );
      ddl.execute();
      ddl.close();
    }

    void Database::rollback( const std::string &sp ) throw(Oops) {
      std::stringstream ss;
      ss << "ROLLBACK TO SAVEPOINT " << sp;
      DDL ddl( *this );
      ddl.prepare( ss.str() );
      ddl.execute();
      ddl.close();
    }

    long Database::lastInsertRowid() const throw(Oops) {
      long r = sqlite3_last_insert_rowid( database_ );
      if ( !r ) {
        throw Oops( __FILE__, __LINE__, "lastInsertRowid failed" );
      }
      return r;
    }

    void Database::checkPointPassive() throw(Oops) {
      int nLog = 0;
      int nCkpt = 0;
      int r = sqlite3_wal_checkpoint_v2( database_, NULL, SQLITE_CHECKPOINT_PASSIVE, &nLog, &nCkpt );
      if ( r != SQLITE_OK ) {
        throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      }
    }

    void Database::checkPointTruncate() throw(Oops) {
      int nLog = 0;
      int nCkpt = 0;
      #if SQLITE_VERSION_NUMBERS > 30080303
      int r = sqlite3_wal_checkpoint_v2( database_, NULL, SQLITE_CHECKPOINT_TRUNCATE, &nLog, &nCkpt );
      #else
      int r = sqlite3_wal_checkpoint_v2( database_, NULL, SQLITE_CHECKPOINT_RESTART, &nLog, &nCkpt );
      #endif
      if ( r != SQLITE_OK ) {
        throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      }
    }

    void Database::setUserVersion( int version ) {
      std::stringstream ss;
      ss << "PRAGMA user_version=" << version;
      DDL ddl( *this );
      ddl.prepare( ss.str() );
      ddl.execute();
      ddl.close();
    }

    int Database::getUserVersion() {
      int r = 0;
      persist::Query query( *this );
      query.prepare( "PRAGMA user_version" );
      if ( query.step() ) {
        r = query.getInt( 0 );
      } else throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      query.close();
      return r;
    }

    void Database::releaseMemory() {
      sqlite3_db_release_memory(database_);
    }

    Statement::Statement( const Database& db ) {
      database_ = db.getDB();
      stmt_ = 0;
    }

    Statement::~Statement() {
      if ( stmt_ ) {
        sqlite3_finalize( stmt_ );
      }
    }

    void Statement::prepare( const std::string &sql ) throw(Oops) {
      const char *err = 0;
      if ( stmt_ ) close();
      int r = sqlite3_prepare_v2( database_, sql.c_str(), -1, &stmt_, &err );
      if ( r != SQLITE_OK ) {
        std::stringstream ss;
        ss << sqlite3_errmsg( database_ ) << " at '" << err << "' sql='" << sql << "'";
        throw Oops( __FILE__, __LINE__, ss.str() );
      }
    }

    void Statement::reset() throw(Oops) {
      int r = sqlite3_reset( stmt_ );
      if ( r != SQLITE_OK ) {
        throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      }
    }

    void Statement::close() throw(Oops) {
      int r = sqlite3_finalize( stmt_ );
      stmt_ = 0;
      if ( r != SQLITE_OK ) {
        throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      }
    }

    void DDL::execute() throw(Oops) {
      int r = sqlite3_step( stmt_ );
      if ( r != SQLITE_DONE ) {
        throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      }
    }

    int DDL::execute_r() {
      return sqlite3_step( stmt_ );
    }

    DML::DML( const Database& db ) : Statement( db ) {
    }

    void DML::execute() throw(Oops) {
      int r = sqlite3_step( stmt_ );
      if ( r != SQLITE_DONE ) {
        throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      }
    }

    void DML::bind( int position, double value ) throw(Oops) {
      int r = sqlite3_bind_double( stmt_, position, value );
      if ( r != SQLITE_OK ) {
        throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      }
    }

    void DML::bind( int position, int value ) throw(Oops) {
      int r = sqlite3_bind_int( stmt_, position, value );
      if ( r != SQLITE_OK ) {
        throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      }
    }

    void DML::bind( int position, long value ) throw(Oops) {
      int r = sqlite3_bind_int64( stmt_, position, value );
      if ( r != SQLITE_OK ) {
        throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      }
    }

    void DML::bind( int position, const std::string &value ) throw(Oops) {
      int r = sqlite3_bind_text( stmt_, position, value.c_str(), -1,  SQLITE_TRANSIENT );
      if ( r != SQLITE_OK ) {
        throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      }
    }



    bool Query::step() throw(Oops) {
      int r = sqlite3_step( stmt_ );
      if ( r != SQLITE_DONE && r != SQLITE_ROW ) {
        throw Oops( __FILE__, __LINE__, sqlite3_errmsg( database_ ) );
      }
      return r == SQLITE_ROW;
    }

    bool Query::isNull( int col ) const {
      return sqlite3_column_type( stmt_, col ) == SQLITE_NULL;
    }

    int Query::getInt( int col ) const {
      return sqlite3_column_int( stmt_, col );
    }

    long Query::getLong( int col ) const {
      return sqlite3_column_int64( stmt_, col );
    }

    double Query::getDouble( int col ) const {
      return sqlite3_column_double( stmt_, col );
    }

    std::string Query::getText( int col ) const {
      const char* c = (const char*)sqlite3_column_text( stmt_, col );
      if ( c ) return c; else return "";
    }

    int Query::getColumnCount() const {
      return sqlite3_column_count( stmt_ );
    }

  }; // namespace persist

}; // namespace leanux
