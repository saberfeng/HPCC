# Copyright (c) 2017-2021 Universidade de Brasília
#
# This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public
# License version 2 as published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with this program; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>

# Set module name
set(name core)

#Set lib core link dependencies
set(libraries_to_link)

#Check libraries used for stack tracing
if(${NS3_STACKTRACE})
  find_library(DL dl)
  check_include_file_cxx(boost/stacktrace.hpp HAVE_BACKTRACE_H)
  if(NOT ("${DL}" STREQUAL "DL-NOTFOUND") AND HAVE_BACKTRACE_H)
    set(libraries_to_link ${DL})
    add_definitions(-DSTACK_TRACE_ENABLED)
  else()
    message(WARNING "LibDL and Boost stacktrace.hpp are required for stack-based event tracing" )
  endif()
endif()

set(gsl_test_sources)
if(${GSL_FOUND})
  include_directories(${GSL_INCLUDE_DIRS})
  set(libraries_to_link ${libraries_to_link} ${GSL_LIBRARIES})
  set(gsl_test_sources test/rng-test-suite.cc test/random-variable-stream-test-suite.cc)
endif()

# Check for dependencies and add sources accordingly
check_include_file_cxx("boost/units/quantity.hpp" HAVE_BOOST_UNITS_QUANTITY)
check_include_file_cxx("boost/units/systems/si.hpp" HAVE_BOOST_UNITS_SI)
if((NOT HAVE_BOOST_UNITS_QUANTITY) OR (NOT HAVE_BOOST_UNITS_SI))
  message(WARNING "Boost Units are an optional feature of length.cc."
                  " Ubuntu ships it within the libboost-dev package."
                  " You may need to clean up the CMake cache after installing it to pass this check."
  )
else()
  add_definitions(-DHAVE_BOOST -DHAVE_BOOST_UNITS)
endif()

set(rt_sources)
set(rt_headers)
if(${ENABLE_REALTIME})
  set(rt_sources model/realtime-simulator-impl.cc model/wall-clock-synchronizer.cc)
  set(rt_headers model/realtime-simulator-impl.h model/wall-clock-synchronizer.h)
  set(libraries_to_link ${libraries_to_link} ${LIBRT})
endif()

set(osclock_sources)
if(WIN32)
  set(osclock_sources model/win32-system-wall-clock-ms.cc)
else()
  set(osclock_sources model/unix-system-wall-clock-ms.cc)
endif()

set(int64x64_sources)
set(int64x64_headers)

if(${INT64X64} MATCHES "INT128")
  set(int64x64_sources model/int64x64-128.cc)
  set(int64x64_headers model/int64x64-128.h)
elseif(${INT64X64} MATCHES "DOUBLE")
  set(int64x64_headers model/int64x64-double.h)
elseif(${INT64X64} MATCHES "CAIRO")
  set(int64x64_sources model/int64x64-cairo.cc)
  set(int64x64_headers model/cairo-wideint-private.h model/int64x64-cairo.h)
endif()

set(thread_sources)
set(thread_headers)
set(thread_test_sources)

if(${THREADS_ENABLED})
  if(WIN32)
      set(thread_sources model/win32-fd-reader.cc)
      set(thread_headers model/win32-fd-reader.h)
    else()
      set(thread_sources model/unix-fd-reader.cc)
      set(thread_headers model/unix-fd-reader.h)
    endif()

    if (${PTHREADS_FOUND})
      if(WIN32)
        set(libraries_to_link ${libraries_to_link} wsock32 ws2_32)
      endif()

      set(libraries_to_link ${libraries_to_link} pthread)
      set(thread_sources
              ${thread_sources}
              model/system-thread.cc
              model/unix-system-mutex.cc
              model/unix-system-condition.cc
              )
    else()
      set(thread_sources
              ${thread_sources}
              model/cpp-system-thread.cc
              model/cpp-system-mutex.cc
              model/cpp-system-condition.cc
              )
    endif()
    set(thread_headers ${thread_headers} model/system-mutex.h model/system-thread.h model/system-condition.h)
    set(thread_test_sources test/threaded-test-suite.cc)
endif()

if(WSLv1)
  set(wslv1_sources ${PROJECT_SOURCE_DIR}/buildsupport/wslv1_workaround.cc)
endif()

if(${NS3_EXAMPLES} AND (NOT WIN32))
  set(example_as_test_sources model/example-as-test.cc)
  set(example_as_test_headers model/example-as-test.h)
  set(example_as_test_suite test/examples-as-tests-test-suite.cc)
endif()

# Embedded version support
set(embedded_version_sources)
set(embedded_version_headers)
if(${NS3_ENABLE_BUILD_VERSION})
  set(embedded_version_sources model/version.cc)
  set(embedded_version_headers model/version.h)
endif()

# Define core lib sources
set(source_files
    ${wslv1_sources}
    ${rt_sources}
    ${osclock_sources}
    ${int64x64_sources}
    ${thread_sources}
    ${valgrind_sources}
    ${example_as_test_sources}
    ${embedded_version_sources}
    model/attribute-construction-list.cc
    model/attribute.cc
    model/boolean.cc
    model/breakpoint.cc
    model/calendar-scheduler.cc
    model/callback.cc
    model/command-line.cc
    model/config.cc
    model/default-simulator-impl.cc
    model/double.cc
    model/enum.cc
    model/event-id.cc
    model/event-impl.cc
    model/fatal-impl.cc
    model/global-value.cc
    model/hash-fnv.cc
    model/hash-function.cc
    model/hash-murmur3.cc
    model/hash.cc
    model/heap-scheduler.cc
    model/int64x64.cc
    model/integer.cc
    model/list-scheduler.cc
    model/log.cc
    model/make-event.cc
    model/map-scheduler.cc
    model/names.cc
    model/object-base.cc
    model/object-factory.cc
    model/object-ptr-container.cc
    model/object.cc
    model/pointer.cc
    model/random-variable-stream-helper.cc
    model/random-variable-stream.cc
    model/random-variable.cc
    model/realtime-simulator-impl.cc
    model/ref-count-base.cc
    model/rng-seed-manager.cc
    model/rng-stream.cc
    model/scheduler.cc
    model/simulator-impl.cc
    model/simulator.cc
    model/string.cc
    model/synchronizer.cc
    model/system-path.cc
    model/system-thread.cc
    model/test.cc
    model/time.cc
    model/timer.cc
    model/trace-source-accessor.cc
    model/type-id.cc
    model/type-name.cc
    model/uinteger.cc
    model/unix-fd-reader.cc
    model/unix-system-condition.cc
    model/unix-system-mutex.cc
    model/vector.cc
    model/wall-clock-synchronizer.cc
    model/watchdog.cc
)

# Define core lib headers
set(header_files
    ${rt_headers}
    ${int64x64_headers}
    ${thread_headers}
    ${example_as_test_headers}
    ${embedded_version_headers}
    model/abort.h
    model/assert.h
    model/attribute-accessor-helper.h
    model/attribute-construction-list.h
    model/attribute-helper.h
    model/attribute.h
    model/boolean.h
    model/breakpoint.h
    model/cairo-wideint-private.h
    model/calendar-scheduler.h
    model/callback.h
    model/command-line.h
    model/config.h
    model/default-deleter.h
    model/default-simulator-impl.h
    model/deprecated.h
    model/double.h
    model/empty.h
    model/enum.h
    model/event-id.h
    model/event-impl.h
    model/fatal-error.h
    model/fatal-impl.h
    model/global-value.h
    model/hash-fnv.h
    model/hash-function.h
    model/hash-murmur3.h
    model/hash.h
    model/heap-scheduler.h
    model/int-to-type.h
    model/int64x64.h
    model/integer.h
    model/list-scheduler.h
    model/log.h
    model/make-event.h
    model/map-scheduler.h
    model/math.h
    model/names.h
    model/nstime.h
    model/object-base.h
    model/object-factory.h
    model/object-map.h
    model/object-ptr-container.h
    model/object-vector.h
    model/object.h
    model/pointer.h
    model/ptr.h
    model/random-variable-stream-helper.h
    model/random-variable-stream.h
    model/random-variable.h
    model/realtime-simulator-impl.h
    model/ref-count-base.h
    model/rng-seed-manager.h
    model/rng-stream.h
    model/scheduler.h
    model/simple-ref-count.h
    model/simulation-singleton.h
    model/simulator-impl.h
    model/simulator.h
    model/singleton.h
    model/string.h
    model/synchronizer.h
    model/system-condition.h
    model/system-mutex.h
    model/system-path.h
    model/system-thread.h
    model/system-wall-clock-ms.h
    model/test.h
    model/timer-impl.h
    model/timer.h
    model/trace-source-accessor.h
    model/traced-callback.h
    model/traced-value.h
    model/type-id.h
    model/type-name.h
    model/type-traits.h
    model/uinteger.h
    model/unix-fd-reader.h
    model/unused.h
    model/vector.h
    model/wall-clock-synchronizer.h
    model/watchdog.h
)

set(test_sources
    ${example_as_test_suite}
    ${gsl_test_sources}
    ${thread_test_sources}
    test/attribute-test-suite.cc
    test/callback-test-suite.cc
    test/command-line-test-suite.cc
    test/config-test-suite.cc
    test/global-value-test-suite.cc
    test/int64x64-test-suite.cc
    test/names-test-suite.cc
    test/object-test-suite.cc
    test/ptr-test-suite.cc
    test/random-variable-stream-test-suite.cc
    test/random-variable-test-suite.cc
    test/rng-test-suite.cc
    test/sample-test-suite.cc
    test/simulator-test-suite.cc
    test/threaded-test-suite.cc
    test/time-test-suite.cc
    test/timer-test-suite.cc
    test/traced-callback-test-suite.cc
    test/type-traits-test-suite.cc
    test/watchdog-test-suite.cc
)

# Build core lib
build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")

if(WSLv1)
  target_compile_definitions(${libcore-obj} PRIVATE -D__WSLv1__)
endif()
