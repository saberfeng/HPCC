set(name test)

set(nsc_sources)
if(${NS3_NSC})
  set(nsc_sources ns3tcp/nsctcp-loss-test-suite.cc ns3tcp/ns3tcp-interop-test-suite.cc ns3tcp/ns3tcp-cwnd-test-suite.cc)
endif()

add_library(
  ${lib${name}} OBJECT
  ${nsc_sources}
  csma-system-test-suite.cc
  error-model-test-suite.cc
  global-routing-test-suite.cc
  mobility-test-suite.cc
  static-routing-test-suite.cc
  ns3tcp/ns3tcp-cwnd-test-suite.cc
  ns3tcp/ns3tcp-interop-test-suite.cc
  ns3tcp/ns3tcp-loss-test-suite.cc
  ns3tcp/ns3tcp-no-delay-test-suite.cc
  ns3tcp/ns3tcp-socket-test-suite.cc
  ns3tcp/ns3tcp-socket-writer.cc
  ns3tcp/ns3tcp-state-test-suite.cc
  ns3tcp/nsctcp-loss-test-suite.cc
  ns3wifi/wifi-interference-test-suite.cc
  ns3wifi/wifi-msdu-aggregator-test-suite.cc)
