set(YCSBC_DB
    db_factory.cc
	hashtable_db.cc
	rocksdb_db.cc
    )

# Form the full path to the source files...
PREPEND(YCSBC_DB)

# ... and pass the variable to the parent scope.
set(YCSBC_DB ${YCSBC_DB}  PARENT_SCOPE)
