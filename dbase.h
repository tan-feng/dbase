/* @file dbase.h
 * @brief A simple wrapper of squlite3 APIs
 * @date 2020-08-22 <Tan Feng> Created
 * @date 2020-09-15 <Tan Feng> Added support of precompiled procedures
 */
#ifndef _DBASE_H_
#define _DBASE_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <sqlite3.h>

typedef struct 
{
	sqlite3* inst;
	const char* dbname;
	sqlite3_stmt* stmt;
	char* sql;
	char** result;
	int i, ncol, nrow;
	bool stepwise;
	int rc;
	char* errmsg;
} dbase_t;

/* Database Operations */
dbase_t *db_new(char* filename);
int db_free(dbase_t* db);
int db_open(dbase_t* db, char* filename);
int db_close(dbase_t* db);

/* Table Operations */
int db_get_table(dbase_t* db, char* sqlfmt, ... );
int db_seek(dbase_t* db, int n);
int db_query(dbase_t* db, char* sqlfmt, ... );
int db_step(dbase_t* db);
int db_exec(dbase_t* db, char* sqlfmt, ... );

/* Column Getters */
int db_col_n2i(dbase_t* db, const char* name);
const char* db_col(dbase_t* db, int col);
int db_col_int(dbase_t* db, const char* name);
int64_t db_col_int64(dbase_t* db, const char* name);
bool db_col_bool(dbase_t* db, const char* name);
const char* db_col_str(dbase_t* db, const char* name);
double db_col_double(dbase_t* db, const char* name);


/* Precompiled Procedures */
enum {
	VAR_NULL = 1,
	VAR_STR,
	VAR_INT,
	VAR_INT64,
	VAR_BOOL,
	VAR_DOUBLE,
	VAR_VALUE,
};

typedef sqlite3_value db_value;

typedef struct
{
	const char* nam;
	int typ; 
	union {
		int i;
		int64_t l;
		bool b;
		double d;
                char* s;
		db_value* v;
	} val;
} db_var_t;


int db_proc_compile(dbase_t* db, char* sqlfmt, ... );
int db_proc_run(dbase_t* db, db_var_t vars[]);
int db_proc_destroy(dbase_t* db); 

/* Param Variable Setters */
db_var_t db_var_null(const char* nam);
db_var_t db_var_str(const char* nam, char* v);     
db_var_t db_var_int(const char* nam, int v);     
db_var_t db_var_int64(const char* nam, int64_t v);     
db_var_t db_var_bool(const char* nam, bool v);     
db_var_t db_var_double(const char* nam, double v);     
db_var_t db_var_value(const char* nam, db_value* v);     

#endif
