all:
	$(CC) -DSELF_TEST -o test dbase.c -lsqlite3

clean:
	rm -f test *.o
