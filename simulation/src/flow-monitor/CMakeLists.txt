set(name flow-monitor)

set(source_files
    model/flow-classifier.cc
    model/flow-monitor.cc
    model/flow-probe.cc
    model/histogram.cc
    model/ipv4-flow-classifier.cc
    model/ipv4-flow-probe.cc
    helper/flow-monitor-helper.cc
)

set(header_files
    model/flow-classifier.h
    model/flow-monitor.h
    model/flow-probe.h
    model/histogram.h
    model/ipv4-flow-classifier.h
    model/ipv4-flow-probe.h
    helper/flow-monitor-helper.h
)

set(libraries_to_link ${libinternet} ${libconfigstore})

set(test_sources
    test/histogram-test-suite.cc
)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")
