set(name point-to-point)

set(mpi_sources)
set(mpi_headers)
set(mpi_libraries)
set(mpi_include_directories)

if(${ENABLE_MPI})
  set(mpi_sources model/point-to-point-remote-channel.cc)
  set(mpi_headers model/point-to-point-remote-channel.h)
  set(mpi_libraries ${libmpi} ${MPI_CXX_LIBRARIES})
  set(mpi_include_directories ${MPI_CXX_INCLUDE_DIRS})
endif()

set(source_files
    model/cn-header.cc
    model/open_jackson_model.cpp
    model/pause-header.cc
    model/pint.cc
    model/point-to-point-channel.cc
    model/point-to-point-net-device.cc
    model/point-to-point-remote-channel.cc
    model/ppp-header.cc
    model/qbb-channel.cc
    model/qbb-header.cc
    model/qbb-net-device.cc
    model/qbb-remote-channel.cc
    model/rand_offset_injector.cpp
    model/rdma-driver.cc
    model/rdma-hw.cc
    model/rdma-queue-pair.cc
    model/switch-mmu.cc
    model/switch-node.cc
    model/net_model.cc
    model/netcalc_model.cc
    model/plain_random_model.cc
    model/monitor.cc
    helper/point-to-point-helper.cc
    helper/qbb-helper.cc
)

set(header_files
    model/cn-header.h
    model/open_jackson_model.h
    model/pause-header.h
    model/pint.h
    model/point-to-point-channel.h
    model/point-to-point-net-device.h
    model/point-to-point-remote-channel.h
    model/ppp-header.h
    model/qbb-channel.h
    model/qbb-header.h
    model/qbb-net-device.h
    model/qbb-remote-channel.h
    model/rand_offset_injector.h
    model/rdma-driver.h
    model/rdma-hw.h
    model/rdma-queue-pair.h
    model/switch-mmu.h
    model/switch-node.h
    model/trace-format.h
    model/net_model.h
    model/netcalc_model.h
    model/plain_random_model.h
    model/monitor.h
    helper/point-to-point-helper.h
    helper/qbb-helper.h
    helper/sim-setting.h
)

set(libraries_to_link ${libnetwork} ${mpi_libraries})

set(test_sources
    test/point-to-point-test.cc
)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")

target_include_directories(${libpoint-to-point-obj} PUBLIC ${mpi_include_directories})
