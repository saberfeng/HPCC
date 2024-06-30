set(name tools)

set(source_files
    model/delay-jitter-estimation.cc
    model/event-garbage-collector.cc
    model/gnuplot.cc
)

set(header_files
    model/average.h
    model/delay-jitter-estimation.h
    model/event-garbage-collector.h
    model/gnuplot.h
)

set(libraries_to_link  ${network}  ${stats} )

set(test_sources
    test/average-test-suite.cc
    test/event-garbage-collector-test-suite.cc
)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")