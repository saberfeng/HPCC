set(name spectrum)

set(source_files
    model/aloha-noack-mac-header.cc
    model/aloha-noack-net-device.cc
    model/constant-spectrum-propagation-loss.cc
    model/friis-spectrum-propagation-loss.cc
    model/half-duplex-ideal-phy-signal-parameters.cc
    model/half-duplex-ideal-phy.cc
    model/microwave-oven-spectrum-value-helper.cc
    model/multi-model-spectrum-channel.cc
    model/non-communicating-net-device.cc
    model/single-model-spectrum-channel.cc
    model/spectrum-analyzer.cc
    model/spectrum-channel.cc
    model/spectrum-converter.cc
    model/spectrum-error-model.cc
    model/spectrum-interference.cc
    model/spectrum-model-300kHz-300GHz-log.cc
    model/spectrum-model-ism2400MHz-res1MHz.cc
    model/spectrum-model.cc
    model/spectrum-phy.cc
    model/spectrum-propagation-loss-model.cc
    model/spectrum-signal-parameters.cc
    model/spectrum-value.cc
    model/waveform-generator.cc
    model/wifi-spectrum-value-helper.cc
    helper/adhoc-aloha-noack-ideal-phy-helper.cc
    helper/spectrum-analyzer-helper.cc
    helper/spectrum-helper.cc
    helper/waveform-generator-helper.cc
)

set(header_files
    model/aloha-noack-mac-header.h
    model/aloha-noack-net-device.h
    model/constant-spectrum-propagation-loss.h
    model/friis-spectrum-propagation-loss.h
    model/half-duplex-ideal-phy-signal-parameters.h
    model/half-duplex-ideal-phy.h
    model/microwave-oven-spectrum-value-helper.h
    model/multi-model-spectrum-channel.h
    model/non-communicating-net-device.h
    model/single-model-spectrum-channel.h
    model/spectrum-analyzer.h
    model/spectrum-channel.h
    model/spectrum-converter.h
    model/spectrum-error-model.h
    model/spectrum-interference.h
    model/spectrum-model-300kHz-300GHz-log.h
    model/spectrum-model-ism2400MHz-res1MHz.h
    model/spectrum-model.h
    model/spectrum-phy.h
    model/spectrum-propagation-loss-model.h
    model/spectrum-signal-parameters.h
    model/spectrum-value.h
    model/waveform-generator.h
    model/wifi-spectrum-value-helper.h
    helper/adhoc-aloha-noack-ideal-phy-helper.h
    helper/spectrum-analyzer-helper.h
    helper/spectrum-helper.h
    helper/waveform-generator-helper.h
)

set(libraries_to_link ${libpropagation} ${libantenna})

set(test_sources
    test/spectrum-ideal-phy-test.cc
    test/spectrum-interference-test.cc
    test/spectrum-value-test.cc
)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")
