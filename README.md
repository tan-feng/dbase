
This is a simple wrapper of libsqlite3 in C, which features:
1. Object-oriented design, all calling variables are contained in one single context structure, no aditional declarations are needed;
2. Light yet complete capsulation, all-in-one single file, no memory allocation, and no knowlege to sqlite3.h is needed;
3. Getters/setters of columns/parameters by names instead of by indices, helpful to reduce errors and to write more human-readable elegant code; 
4. Support of both the legacy single-step queries and the new precompiled multi-step ones;
5. Builtin debug capability with ez_debug (a runtime debug printing wrapper of printf, see my other project)


The following is a full demonstration:


	dbase_t* db = db_new("test.db");

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
	db_seek(db, 3);  //goto the 3rd row
        printf("[%d] name=%s age=%d\n", db->i, db_col_str(db, "name"), db_col_int(db, "age")); 
	db_seek(db, -2);  //goto the 2nd last row

        ///
	db_free(db);




Enjoy
Felix Tan

