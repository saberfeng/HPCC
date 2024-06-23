set(name emu)

#set(source_files
#    model/emu-encode-decode.cc
#    model/emu-net-device.cc
#    model/emu-sock-creator.cc
#    helper/emu-helper.cc
#)
#
#set(header_files
#    model/emu-encode-decode.h
#    model/emu-net-device.h
#    helper/emu-helper.h
#)
#
#set(libraries_to_link  ${network} )
#
#set(test_sources
#)
#
#build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")