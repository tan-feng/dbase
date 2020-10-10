/* @file dbase.h
 * @brief A simple wrapper of squlite3 APIs
 * @date 2020-08-22 <Tan Feng> Created
 * @date 2020-09-15 <Tan Feng> Added support of precompiled procedures
 */
#include "ez_debug.h"
#include "dbase.h"

int db_init(dbase_t* db);
int db_reset(dbase_t* db);

/*************************************************/
/*       Database Operations    */
/*************************************************/
int db_init(dbase_t* db)
{
	memset(db, 0, sizeof(*db));
	return 0;
}

int db_open(dbase_t* db, char* filename)
{
	if(db->inst)
		db_close(db);
	if(sqlite3_open(filename, &db->inst) != SQLITE_OK) {
		ez_debug("Error: [%s] %s", filename, sqlite3_errmsg(db->inst));
		sqlite3_close(db->inst);
		return -1;
	}
	db->dbname = filename;
	return 0;
}

int db_close(dbase_t* db)
{
        db_reset(db);
	if(db->inst) 
		sqlite3_close(db->inst);
	return 0;
}

int db_reset(dbase_t* db)
{	
	if(db->stmt) {
		sqlite3_finalize(db->stmt);
//		sqlite3_free(db->stmt);
	        db->stmt = NULL;
	}
	if(db->result) {
		sqlite3_free_table(db->result);
	        db->result = NULL;
	}
	if(db->errmsg) {
		sqlite3_free(db->errmsg);
	        db->errmsg = NULL;
	}
	db->i = db->nrow = db->ncol = 0;
	return 0; 
}

dbase_t *db_new(char* filename)
{
        dbase_t*  db = (dbase_t*) malloc(sizeof(dbase_t)); 
        db_init(db);	
	db_open(db, filename);
	return db; 
}

int db_free(dbase_t* db)
{
	db_close(db);
	free(db);
	return 0; 
}

/*************************************************/
/*       Table Operations    */
/*************************************************/
int db_get_table(dbase_t* db, char* sqlfmt, ... ) 
{
        va_list ap;

        va_start(ap, sqlfmt);
        db->sql = sqlite3_vmprintf(sqlfmt, ap);
        va_end(ap);

        db_reset(db);
        db->rc = sqlite3_get_table(db->inst, db->sql, &db->result, &db->nrow, &db->ncol, &db->errmsg); 

        if (db->rc != SQLITE_OK) {
                 ez_debug("Failed to fetch data: [%s] %s", db->dbname, sqlite3_errmsg(db->inst));
                 return -1;
	}

	if (db->nrow > 0) {
	         db->i = 1;
	}
	return db->rc;
}


int db_query(dbase_t* db, char* sqlfmt, ... ) 
{
        va_list ap;

        va_start(ap, sqlfmt);
        db->sql = sqlite3_vmprintf(sqlfmt, ap);
        va_end(ap);


        db_reset(db);
	db->rc = sqlite3_prepare_v2(db->inst, db->sql,  -1, &db->stmt, 0);

        if (db->rc != SQLITE_OK) {
             ez_debug("Failed to fetch data: [%s] %s", db->dbname, sqlite3_errmsg(db->inst));
             return -1;
	}
	db->ncol = sqlite3_column_count(db->stmt);

	db->stepwise = true;
        db_step(db);
	return db->rc;
}

int db_step(dbase_t* db)
{

        if(!db->stepwise || !db->stmt) {
		ez_debug("db_step() can only be called after db_query()");
		return -2;
	}

        db->nrow = 0;
	int r = sqlite3_step(db->stmt);
        
	switch( r ) {
		case SQLITE_ROW:
                        db->i++;
			db->nrow = 1;
			db->rc = SQLITE_OK;
		        break;
		case SQLITE_DONE:
			//ez_debug("stmt finalized");
                        sqlite3_finalize(db->stmt);
			db->stmt = NULL;
			db->rc = SQLITE_OK;
		        break;
                default: 
                        ez_debug("Failed to fetch data: [%s] %s", db->dbname, sqlite3_errmsg(db->inst));
                        sqlite3_finalize(db->stmt);
			db->stmt = NULL;
			db->rc = r;
		        break;
	}
        return db->rc; 
}

int db_seek(dbase_t* db, int n)
{
	int i = n;
        
	if( n < 0 ) i = n + db->nrow + 1;

	if(i < 1) i = 1;
	if(i > db->nrow) i = db->nrow; 

        db->i = i;
	return 0;
}

int db_exec(dbase_t* db, char* sqlfmt, ... ) 
{
        va_list ap;

        va_start(ap, sqlfmt);
        db->sql = sqlite3_vmprintf(sqlfmt, ap);
        va_end(ap);

        db_reset(db);
	db->rc = sqlite3_exec(db->inst, db->sql, 0, 0, &db->errmsg);

        if (db->rc != SQLITE_OK) {
             ez_debug("Failed to execute: [%s] %s", db->dbname, sqlite3_errmsg(db->inst));
             return -1;
	}
	db->ncol = sqlite3_column_count(db->stmt);
	db->nrow = 0;
	return db->rc;
}


/*************************************************/
/*       Column Getters    */
/*************************************************/
int db_col_n2i(dbase_t* db, const char* name)
{
	int i;

	if(db->result) {
	        for(i=0; i< db->ncol; i++) {
			if(!strcmp(name, db->result[i])) return i;
                }
		return -1;
	}
	
        if(!db->stmt) {
	        ez_debug("No query performed");
		return -2;
	}

	for(i=0; i< db->ncol; i++) {
		if(!strcmp(name, sqlite3_column_name(db->stmt, i)))
			return i;
	}
	ez_debug("Unknown collumn name: %s", name);
	return -1;
}


const char* db_col(dbase_t* db, int col)
{
	if(db->result) {
		return db->result[db->i * db->ncol + col]; 
	}
        return (const char*) sqlite3_column_text(db->stmt, col);
}

int db_col_int(dbase_t* db, const char* name)
{
	if(db->result) {
		return atoi(db->result[db->i * db->ncol + db_col_n2i(db, name)]);
	}
        return sqlite3_column_int(db->stmt, db_col_n2i(db, name));
}

int64_t db_col_int64(dbase_t* db, const char* name)
{
	if(db->result) {
		return atoll(db->result[db->i * db->ncol + db_col_n2i(db, name)]);
	}
        return sqlite3_column_int64(db->stmt, db_col_n2i(db, name));
}

bool db_col_bool(dbase_t* db, const char* name)
{
	if(db->result) {
		return atoi(db->result[db->i * db->ncol + db_col_n2i(db, name)]);
	}
        return sqlite3_column_int(db->stmt, db_col_n2i(db, name));
}

const char* db_col_str(dbase_t* db, const char* name)
{
	if(db->result) {
		return db->result[db->i * db->ncol + db_col_n2i(db, name)];
	}
        return (const char*) sqlite3_column_text(db->stmt, db_col_n2i(db, name));
}

double db_col_double(dbase_t* db, const char* name)
{
	if(db->result) {
		return atof(db->result[db->i * db->ncol + db_col_n2i(db, name)]);
	}
        return sqlite3_column_double(db->stmt, db_col_n2i(db, name));
}

/*************************************************/
/*       Recompiled Procedures    */
/*************************************************/
int db_proc_compile(dbase_t* db, char* sqlfmt, ... )
{
        db_reset(db);

        va_list ap;
        va_start(ap, sqlfmt);
        db->sql = sqlite3_vmprintf(sqlfmt, ap);
        va_end(ap);

        db_reset(db);
	db->rc = sqlite3_prepare_v2(db->inst, db->sql,  -1, &db->stmt, 0);

        if (db->rc != SQLITE_OK) {
             ez_debug("Failed to compile: [%s] %s", db->dbname, sqlite3_errmsg(db->inst));
             return -1;
	}
	return db->rc;
}

int db_proc_run(dbase_t* db, db_var_t vars[])
{

	if(!db->stmt) {
		ez_debug("Procedure not compiled yet");
		return -1;
	}
        
	sqlite3_reset(db->stmt);
	sqlite3_clear_bindings(db->stmt);

	int i;
	for (i=0; i < sqlite3_bind_parameter_count(db->stmt); i++) {
		switch (vars[i].typ) {
		case VAR_NULL:
			db->rc = sqlite3_bind_null(db->stmt, sqlite3_bind_parameter_index(db->stmt, vars[i].nam)); 
			break;
		case VAR_INT:
			db->rc = sqlite3_bind_int(db->stmt, sqlite3_bind_parameter_index(db->stmt, vars[i].nam),  vars[i].val.i); 
			break;
		case VAR_INT64:
			db->rc = sqlite3_bind_int64(db->stmt, sqlite3_bind_parameter_index(db->stmt, vars[i].nam),  vars[i].val.l); 
			break;
		case VAR_BOOL:
			db->rc = sqlite3_bind_int(db->stmt, sqlite3_bind_parameter_index(db->stmt, vars[i].nam),  vars[i].val.b); 
			break;
		case VAR_DOUBLE:
			db->rc = sqlite3_bind_double(db->stmt, sqlite3_bind_parameter_index(db->stmt, vars[i].nam),  vars[i].val.d); 
			break;
		case VAR_STR:
			db->rc = sqlite3_bind_text(db->stmt, sqlite3_bind_parameter_index(db->stmt, vars[i].nam), vars[i].val.s, strlen(vars[i].val.s), NULL); 
			break;
		case VAR_VALUE:
			db->rc = sqlite3_bind_value(db->stmt, sqlite3_bind_parameter_index(db->stmt, vars[i].nam),  vars[i].val.v); 
			break;
		default:
                        ez_debug("Failed to bind vars { %s }: <%s>", sqlite3_sql(db->stmt), "Unknown type of value");
                        return -1;
                } //switch
                if ( db->rc != SQLITE_OK) {
                        ez_debug("Failed to bind vars { %s }: <%s>", sqlite3_sql(db->stmt), sqlite3_errmsg(db->inst));
                        return -1;
	        }
	}
	
        db->sql = sqlite3_expanded_sql(db->stmt);

        db->rc = sqlite3_step(db->stmt);
	if(db->rc != SQLITE_ROW && db->rc != SQLITE_DONE) {
                ez_debug("Failed to run: { %s } %s", db->sql, sqlite3_errmsg(db->inst));
	}
	return db->rc;
}

int db_proc_destroy(dbase_t* db)
{
	if(!db->stmt) {
	        sqlite3_clear_bindings(db->stmt);
	        sqlite3_reset(db->stmt);
                sqlite3_finalize(db->stmt);
	}
	db_reset(db);
	return 0;
}

/* Param Variable Setters */
db_var_t db_var_null(const char* nam)
{ 
	db_var_t x = { .nam = nam, .typ = VAR_NULL }; 
	return x; 
} 

db_var_t db_var_str(const char* nam, char* v)
{ 
	db_var_t x = { .nam = nam, .typ = VAR_STR, .val.s = v }; 
	return x; 
} 

db_var_t db_var_int(const char* nam, int v)
{ 
	db_var_t x = { .nam = nam, .typ = VAR_INT, .val.i = v }; 
	return x; 
} 

db_var_t db_var_int64(const char* nam, int64_t v)
{ 
	db_var_t x = { .nam = nam, .typ = VAR_INT64, .val.l = v }; 
	return x; 
} 

db_var_t db_var_bool(const char* nam, bool v)
{ 
	db_var_t x = { .nam = nam, .typ = VAR_BOOL, .val.b = v }; 
	return x; 
} 

db_var_t db_var_double(const char* nam, double v)
{ 
	db_var_t x = { .nam = nam, .typ = VAR_DOUBLE, .val.d = v }; 
	return x; 
} 

db_var_t db_var_value(const char* nam, db_value* v)     
{ 
	db_var_t x = { .nam = nam, .typ = VAR_VALUE, .val.v = v }; 
	return x; 
} 


/***************************************************************************************************************/
/***************************************************************************************************************/
/***************************************************************************************************************/
/***************************************************************************************************************/
/* Run 
 * $CC -DSELF_TEST dbase.c -lsqlite3
 * to build a self tester */
#ifdef SELF_TEST
int main()
{
	dbase_t* db = db_new("test.db");

	/* Old style simple CREATE/UPDATE/DELETE/... */
        db_exec(db, "\
CREATE TABLE IF NOT EXISTS students \n\
( \n\
    id  INT, \n\
    name CHAR(32), \n\
    male BOOL, \n\
    age  INT, \n\
    weight REAL \n\
); \n\
");
	/* Old style simple CREATE/UPDATE/DELETE/... */
        db_exec(db, "INSERT INTO students VALUES(0, %Q, %d, %d, %f)", "Girl-Maria", false, 18, 50.321); 

	/* New style INSERT/UPDATE/DELETE operations:
	 * Reusable precomiled procedure with params, 
	 * RECOMMENDED for repetitive updates and insertions */
        //
        db_proc_compile(db, "INSERT INTO students VALUES(@id, @name, %d, @age, @weight)", true); 
	int i;
        for(i=1; i<20; i++) {
		char name[64];
		sprintf(name, "Boy-Tony%02d", i);
	        db_var_t vars[] = { db_var_int("@id", 100+i), 
			            db_var_str("@name", name), 
				    db_var_int("@age", 5+i),
				    db_var_double("@weight", i*5.110),
		};
		db_proc_run(db, vars);
        }
	db_proc_destroy(db);

	/* SEELCT */
        //the NEW on-demand fetch mode, RECOMMENDED
        db_query(db, "SELECT * FROM students WHERE age > %d ORDER by weight", 10); 
        for(; db->nrow > 0; db_step(db)) {
	        printf("[%d] name=%s age=%d weight=%f\n", db->i, 
				db_col_str(db, "name"), db_col_int(db, "age"), db_col_double(db, "weight"));
	}

	/* SEELCT */
        //the OLD whole fetch mode, fully navigatible but resource comsuming, DEPRECATED 
        db_get_table(db, "SELECT * FROM students WHERE age > %d ORDER by weight", 10);
	db_seek(db, 3);  //the 3rd row
        printf("[%d] name=%s age=%d\n", db->i, db_col_str(db, "name"), db_col_int(db, "age")); 
	db_seek(db, -2);  //the 2nd last row
        printf("[%d] name=%s age=%d\n", db->i, db_col_str(db, "name"), db_col_int(db, "age")); 
        
	//cleanup all boys 
        db_exec(db, "DELETE FROM students WHERE male == 1");
        ///
	db_free(db);
	return 0;
}
/* Run 
 * $CC -DSELF_TEST dbase.c -lsqlite3
 * to build a self tester */
#endif

