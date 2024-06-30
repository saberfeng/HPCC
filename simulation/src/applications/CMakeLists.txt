set(name applications)

set(source_files
    model/bulk-send-application.cc
    model/onoff-application.cc
    model/packet-loss-counter.cc
    model/packet-sink.cc
    model/ping6.cc
    model/radvd-interface.cc
    model/radvd-prefix.cc
    model/radvd.cc
    model/rdma-client.cc
    model/udp-client.cc
    model/udp-echo-client.cc
    model/udp-echo-server.cc
    model/udp-server.cc
    model/udp-trace-client.cc
    model/v4ping.cc
    helper/bulk-send-helper.cc
    helper/on-off-helper.cc
    helper/packet-sink-helper.cc
    helper/ping6-helper.cc
    helper/rdma-client-helper.cc
    helper/udp-client-server-helper.cc
    helper/udp-echo-helper.cc
    helper/v4ping-helper.cc
)

set(header_files
    model/bulk-send-application.h
    model/onoff-application.h
    model/packet-loss-counter.h
    model/packet-sink.h
    model/ping6.h
    model/radvd-interface.h
    model/radvd-prefix.h
    model/radvd.h
    model/rdma-client.h
    model/udp-client.h
    model/udp-echo-client.h
    model/udp-echo-server.h
    model/udp-server.h
    model/udp-trace-client.h
    model/v4ping.h
    helper/bulk-send-helper.h
    helper/on-off-helper.h
    helper/packet-sink-helper.h
    helper/ping6-helper.h
    helper/rdma-client-helper.h
    helper/udp-client-server-helper.h
    helper/udp-echo-helper.h
    helper/v4ping-helper.h
)

# link to dependencies
set(libraries_to_link ${libinternet} ${libstats} ${libconfig-store} ${libtools} ${libpoint-to-point})

set(test_sources
    test/udp-client-server-test.cc
)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")
