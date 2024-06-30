set(name stats)

find_package(SQLite3)
check_include_file_cxx(semaphore.h HAVE_SEMAPHORE_H)
if(${SQLite3_FOUND})
  set(sqlite_sources model/sqlite-data-output.cc)
  set(sqlite_headers model/sqlite-data-output.h )

  include_directories(${SQLite3_INCLUDE_DIRS})
  set(sqlite_libraries ${SQLite3_LIBRARIES})
endif()

set(source_files
    ${sqlite_sources}
    model/data-calculator.cc
    model/data-collector.cc
    model/data-output-interface.cc
    model/omnet-data-output.cc
    model/packet-data-calculators.cc
    model/sqlite-data-output.cc
    model/time-data-calculators.cc
)

set(header_files
    ${sqlite_headers}
    model/basic-data-calculators.h
    model/data-calculator.h
    model/data-collector.h
    model/data-output-interface.h
    model/omnet-data-output.h
    model/packet-data-calculators.h
    model/sqlite-data-output.h
    model/time-data-calculators.h
)

set(libraries_to_link ${libcore} ${sqlite_libraries})

set(test_sources
    test/basic-data-calculators-test-suite.cc
)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")
