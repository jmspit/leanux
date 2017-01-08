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

#ifndef LEANUX_PERSIST_HPP
#define LEANUX_PERSIST_HPP

#include "oops.hpp"

#include <string>
#include <sqlite3.h>

namespace leanux {

  /**
   * Persistent SQL storage, uses the great sqlite3.
   */
  namespace persist {

    typedef int(*WaitHandler)(void*,int);

    /**
     * A STL friendly wrapper around a sqlite3 database.
     * Additionally, a few extension functions are added
     * for use in SQL
     * - power(double x,double y) gives base x to the power y (x^y)
     * - floor(double x) gives the nearest integer downwards
     * - ceil(double x) gives the nearest integer upwards
     */
    class Database {
      public:

        /**
         * Constructor with explicit wait handler.
         * @param filename the database filename
         * @param handler the wait handler
         */
        Database( const std::string &filename, WaitHandler handler = 0 );

        /**
         * Destructor.
         */
        ~Database();

        /**
         * Enable foreign key constraints.
         */
        void enableForeignKeys() throw(Oops);

        void disableForeignKeys() throw(Oops);

        /**
         * Enable triggers.
         */
        void enableTriggers() throw(Oops);

        /**
         * Return database handle.
         * @return the sqlite3 database handle.
         */
        sqlite3* getDB() const { return database_; };

        /** Begin a transaction. */
        void begin() throw(Oops);

        /** Begin an immediate transaction. */
        void beginImmediate() throw(Oops);

        /** Begin an exclusive transaction. */
        void beginExclusive() throw(Oops);

        /** Commit a transaction. */
        void commit() throw(Oops);

        /** Rollback a transaction. */
        void rollback() throw(Oops);

        /**
         * Create a named savepoint.
         * @param sp the savepoint name
         * @see release, rollback
         */
        void savepoint( const std::string &sp ) throw(Oops);

        /**
         * Release a savepoint.
         * @param sp the savepoint name
         */
        void release( const std::string &sp ) throw(Oops);

        /**
         * Rollback to a savepoint.
         * @param sp the savepoint name
         */
        void rollback( const std::string &sp ) throw(Oops);

        /**
         * Issue a passive checpoint.
         */
        void checkPointPassive() throw(Oops);

        /**
         * Issue a (WAL) truncate checpoint.
         */
        void checkPointTruncate() throw(Oops);

        /**
         * Get the rowid of the last inserted row.
         */
        long lastInsertRowid() const throw(Oops);

        /**
         * set the user_version pragma
         */
        void setUserVersion( int version );

        /**
         * get the current user_version pragma
         */
        int getUserVersion();

        void releaseMemory();

        static unsigned long memUsed() {
          return sqlite3_memory_used();
        }

        static unsigned long memHighWater() {
          return sqlite3_memory_highwater(0);
        }

        static long softHeapLimit( long limit ) {
          return sqlite3_soft_heap_limit64( limit );
        }

        std::string fileName() const { return sqlite3_db_filename( database_, "main" ); };

      protected:
        /** Database handle. */
        sqlite3 *database_;
    };

    /**
     * Generic SQL Statement.
     */
    class Statement {
      public:

        /**
         * Constructor.
         */
        Statement( const Database& db );

        /**
         * Destructor.
         * Virtual so that this destructor will be called when
         * descendants are deleted.
         */
        virtual ~Statement();

        /**
         * Prepare a SQL statement.
         * @param sql the SQL statement.
         */
        void prepare( const std::string &sql ) throw(Oops);

        /**
         * Reset a SQL statement for rexecute or even re-prepare.
         */
        void reset() throw(Oops);

        /**
         * A statement handle can be explicitly closed without deleting
         * the Statement object itself. This frees the resources in SQLite,
         * . it's allowed to call prepare() again.
         */
        void close() throw(Oops);

      protected:

        /** statement handle. */
        sqlite3_stmt *stmt_;

        /** database handle on which the stmt_ is created. */
        sqlite3      *database_;

    };

    /**
     * Data Definition Language, SQL that takes no parameters,
     * returns no data such as CREATE TABLE.
     */
    class DDL : public Statement {
      public:

        /** Constructor. */
        DDL( const Database& db ) : Statement( db ) {};

        /** Destructor. */
        virtual ~DDL() {};

        /**
         * execute, throws Oops on error.
         * @see execute_r
         */
        void execute() throw(Oops);

        /**
         * execute and return result code.
         * @see execute
         */
        int execute_r();
    };

    /**
     * Data Modification Language statements can take bind values.
     * Note that calling reset leaves set bind values intact.
     */
    class DML : public Statement {
      public:

        /** Constructor. */
        DML( const Database& db );

        /** Destructor. */
        virtual ~DML() {};

        /**
         * Execute.
         */
        void execute() throw(Oops);

        /**
         * Bind a double value to the bind at position.
         * @param position the bind position in the SQL (start with 1)
         * @param value the value to bind.
         */
        void bind( int position, double value ) throw(Oops);

        /**
         * Bind an int value to the bind at position.
         * @param position the bind position in the SQL (start with 1)
         * @param value the value to bind.
         */
        void bind( int position, int value ) throw(Oops);

        /**
         * Bind a long value to the bind at position.
         * @param position the bind position in the SQL (start with 1)
         * @param value the value to bind.
         */
        void bind( int position, long value ) throw(Oops);

        /**
         * Bind a string value to the bind at position.
         * @param position the bind position in the SQL (start with 1)
         * @param value the value to bind.
         */
        void bind( int position, const std::string &value ) throw(Oops);
    };

    /**
     * Queries can take bind values and return select lists.
     */
    class Query : public DML {
      public:
        /** Constructor. */
        Query( const Database& db ) : DML( db ) {};

        /** Destructor. */
        virtual ~Query() {};

        /**
         * Step the result list, end of list returns false.
         * @return true as long as there are more rows.
         */
        bool step() throw(Oops);

        /**
         * Test if the result is NULL
         * @param col the select list column (start with 0).
         * @return true if the col holds a NULL value
         */
        bool isNull( int col ) const;

        /**
         * Get int value from select list.
         * @param col the select list column (start with 0).
         * @return the select list value.
         */
        int getInt( int col ) const;

        /**
         * Get long value from select list.
         * @param col the select list column (start with 0).
         * @return the select list value.
         */
        long getLong( int col ) const;

        /**
         * Get double value from select list.
         * @param col the select list column (start with 0).
         * @return the select list value.
         */
        double getDouble( int col ) const;

        /**
         * Get string value from select list.
         * @param col the select list column (start with 0).
         * @return the select list value.
         */
        std::string getText( int col ) const;
    };


  }; // namespace persist

}; // namespace leanux

#endif
