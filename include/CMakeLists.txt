set(YCSBC_INC
	basic_db.h
	client.h
	const_generator.h
	core_workload.h
	counter_generator.h
	db_factory.h
	db.h
	discrete_generator.h
	generator.h
	hashtable_db.h
	lock_stl_db.h
	lock_stl_hashtable.h
	mem_alloc.h
	properties.h
	scrambled_zipfian_generator.h
	skewed_latest_generator.h
	stl_hashtable.h
	string_hashtable.h
	tbb_rand_db.h
	tbb_rand_hashtable.h
	tbb_scan_db.h
	tbb_scan_hashtable.h
	timer.h
	uniform_generator.h
	utils.h
	zipfian_generator.h    
	my_string.h
	rocksdb_db.h
	coding.h
    )

# Form the full path to the source files...
PREPEND(YCSBC_INC)

# ... and pass the variable to the parent scope.
set(YCSBC_INC ${YCSBC_INC}  PARENT_SCOPE)
