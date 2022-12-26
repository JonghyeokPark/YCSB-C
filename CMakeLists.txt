cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_FLAGS "-std=c++17 -g -O2 -Wall -pthread -I./ -pthread")
set(CMAKE_CXX_FLAGS "-std=c++17 -g -O2 -Wall -pthread -I./ -pthread")

project(YCSBC VERSION 1.0)
set(YCSBC_VERSION 1.0)

# enable unit testing via "make test" once the code has been compiled.
include(CTest)

# Function to prepend the subdirectory to source files in subdirectories
FUNCTION(PREPEND var )
   SET(listVar "")
   FOREACH(f ${${var}})
       LIST(APPEND listVar "${CMAKE_CURRENT_SOURCE_DIR}/${f}")
   ENDFOREACH(f)
   SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

# Include source code and headers. This calls the CMakeLists.txt in each
# subdirectory. These can define their own libraries, executables, etc. as targets, 
# but here we define all exportable targets in the root CMakeLists.txt.
add_subdirectory(core)
add_subdirectory(db)
add_subdirectory(include)

# Add the test directory. It is optional and can be disabled during with
#   cmake -DBUILD_TESTING=OFF ..
# To run unit tests produced here, we only need to run:
#   make test
# or
#   ctest 
# In case your tests are printing to console, you can view their output to
# stdout with:
#   ctest -V

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Add the library YCSBC as a target, with the contents of src/ and include/
# as dependencies.
add_library(YCSBC STATIC ycsbc.cc ${YCSBC_CORE} ${YCSBC_DB} ${YCSBC_INC})

# These variables slightly modify the install location to allow for version
# specific installations.
set(YCSBC_INCLUDE_DEST "include/YCSBC-${YCSBC_VERSION}")
set(YCSBC_LIB_DEST "lib/YCSBC-${YCSBC_VERSION}")


# generator expressions are needed for the include directories, since installing headers changes the include path
# Specify that YCSBC requires the files located in the include/ directory at
# compile time. This would normally look like 
#   target_include_directories(YCSBC PUBLIC include/)
# PUBLIC means that other libraries including YCSBC should also include the
# directory include/.
# However, there is a catch. If we are installing the project in
# CMAKE_INSTALL_PREFIX, we can't specify include/ in the build directory: we have 
# copied the contents of include to CMAKE_INSTALL_PREFIX/include and we would
# like  other projects to include this directory instead of include/. The follow
# CMake command handles this. $<BUILD_INTERFACE:...> and
# $<INSTALL_INTERFACE:...> are macros whose values change depending on if we are
# simply building the code or if we are installing it.
target_include_directories(YCSBC PUBLIC
   # headers to include when building from source
   $<BUILD_INTERFACE:${YCSBC_SOURCE_DIR}/include> 
   $<BUILD_INTERFACE:${YCSBC_BINARY_DIR}/include> 

   # headers to include when installing  (implicitly prefixes with ${CMAKE_INSTALL_PREFIX}).
   $<INSTALL_INTERFACE:include> 
   )

# Specify that YCSBC requires LAPACK to link properly. Ideally, LAPACK would
# specify LAPACK::LAPACK for linking so that we can avoid using the variables.
# However, each package is different and one must check the documentation to 
# see what variables are defined.
target_link_libraries(YCSBC ${LAPACK_LIBRARIES} pthread tbb rocksdb z dl snappy)

# Install YCSBC in CMAKE_INSTALL_PREFIX (defaults to /usr/local on linux). 
# To change the install location, run 
#   cmake -DCMAKE_INSTALL_PREFIX=<desired-install-path> ..

# install(...) specifies installation rules for the project. It can specify
# location of installed files on the system, user permissions, build
# configurations, etc. Here, we are only copying files.
# install(TARGETS ...) specifies rules for installing targets. 
# Here, we are taking a target or list of targets (YCSBC) and telling CMake
# the following:
#   - put shared libraries associated with YCSBC in ${YCSBC_LIB_DEST}
#   - put static libraries associated with YCSBC in ${YCSBC_LIB_DEST}
#   - put include files associated with YCSBC in ${YCSBC_INCLUDE_DEST}
# We also need to specify the export that is associated with YCSBC; an export 
# is just a list of targets to be installed.
# So we are associating YCSBC with YCSBCTargets.
install(
    # targets to install
    TARGETS YCSBC 
    # name of the CMake "export group" containing the targets we want to install
    EXPORT YCSBCTargets
    # Dynamic, static library and include destination locations after running
    # "make install"
    LIBRARY DESTINATION ${YCSBC_LIB_DEST}
    ARCHIVE DESTINATION ${YCSBC_LIB_DEST} 
    INCLUDES DESTINATION ${YCSBC_INCLUDE_DEST}
    )

# We now need to install the export YCSBCTargets that we defined above. This
# is needed in order for another project to import YCSBC using 
#   find_package(YCSBC)
# find_package(YCSBC) will look for YCSBC-config.cmake to provide
# information about the targets contained in the project YCSBC. Fortunately,
# this is specified in the export YCSBCTargets, so we will install this too.
# install(EXPORT ...) will install the information about an export. Here, we
# save it to a file {$YCSBC_LIB_DEST}/YCSBCTargets.cmake and prepend 
# everything inside YCSBCTargets  with the namespace YCSBC::.
install(
    # The export we want to save (matches name defined above containing the
    # install targets)
    EXPORT YCSBCTargets
    # CMake file in which to store the export's information
    FILE  YCSBCTargets.cmake
    # Namespace prepends all targets in the export (when we import later, we
    # will use YCSBC::YCSBC)
    NAMESPACE YCSBC::
    # where to place the resulting file (here, we're putting it with the library)
    DESTINATION ${YCSBC_LIB_DEST}
    )

# install(FILES ...) simply puts files in a certain place with certain
# properties. We're just copying them to the desired place here.
install(FILES ${YCSBC_INC} DESTINATION ${YCSBC_INCLUDE_DEST})

# Write a "version file" in case some wants to only load a particular version of
# YCSBC @BUG this fails at the moment.
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    YCSBCConfigVersion.cmake
    VERSION ${YCSBC_VERSION}
    COMPATIBILITY AnyNewerVersion
    )

# Copies the resulting CMake config files to the installed library directory
install(
    FILES 
        "cmake/YCSBC-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/YCSBCConfigVersion.cmake"
    DESTINATION ${YCSBC_LIB_DEST}
    )

