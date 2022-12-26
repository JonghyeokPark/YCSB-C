set(YCSBC_CORE
    core_workload.cc
    )

# Form the full path to the source files...
PREPEND(YCSBC_CORE)

# ... and pass the variable to the parent scope.
set(YCSBC_CORE ${YCSBC_CORE}  PARENT_SCOPE)
