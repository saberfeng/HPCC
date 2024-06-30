set(name energy)

set(source_files
    model/basic-energy-source.cc
    model/device-energy-model-container.cc
    model/device-energy-model.cc
    model/energy-source.cc
    model/li-ion-energy-source.cc
    model/rv-battery-model.cc
    model/simple-device-energy-model.cc
    model/wifi-radio-energy-model.cc
    helper/basic-energy-source-helper.cc
    helper/energy-model-helper.cc
    helper/energy-source-container.cc
    helper/rv-battery-model-helper.cc
    helper/wifi-radio-energy-model-helper.cc
)

set(header_files
    model/basic-energy-source.h
    model/device-energy-model-container.h
    model/device-energy-model.h
    model/energy-source.h
    model/li-ion-energy-source.h
    model/rv-battery-model.h
    model/simple-device-energy-model.h
    model/wifi-radio-energy-model.h
    helper/basic-energy-source-helper.h
    helper/energy-model-helper.h
    helper/energy-source-container.h
    helper/rv-battery-model-helper.h
    helper/wifi-radio-energy-model-helper.h
)

set(libraries_to_link ${libnetwork})

set(test_sources
    test/basic-energy-model-test.cc
    test/li-ion-energy-source-test.cc
    test/rv-battery-model-test.cc
)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")
