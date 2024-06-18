import os
import helper
import re

def get_files_in_dir(dir_path:str, sub_dirs:list[str], file_exts:list[str]):
    '''Returns a list of files with specified extensions in the 
    directory specified by dir_path and sub_dirs.
    '''   
    cxx_sources = []
    for sub_dir in sub_dirs:
        for root, dirs, files in os.walk(os.path.join(dir_path, sub_dir)):
            for file in files:
                if file.endswith(tuple(file_exts)):
                    path_from_proj = os.path.join(root, file)
                    path_from_module = os.path.relpath(path_from_proj, dir_path)
                    # change to forward slash
                    path_from_module = path_from_module.replace('\\', '/')
                    cxx_sources.append(path_from_module)
    return cxx_sources

def wrap_files_in_cmake_set(files:list[str], set_name:str):
    '''Returns a string that represents a CMake set containing the files in
    the list files.
    '''
    cmake_set = f"set({set_name}\n"
    for file in files:
        cmake_set += f"    {file}\n"
    cmake_set += ")"
    return cmake_set

def get_all_cxx_files_in_dir(dir_path):
    source_header_subdir = ['model', 'helper', 'utils', 'ns3tcp', 'ns3wifi']
    cxx_sources = get_files_in_dir(dir_path, source_header_subdir, 
                                   ['.cpp', '.cc', '.cxx'])
    cxx_headers = get_files_in_dir(dir_path, source_header_subdir, 
                                   ['.h', '.hpp'])
    cxx_tests = get_files_in_dir(dir_path, ['test'],['.cc', '.cpp', '.cxx'])

    test_src_varname = 'test_sources'
    cmake_set_src = wrap_files_in_cmake_set(cxx_sources, 'source_files')
    cmake_set_hdr = wrap_files_in_cmake_set(cxx_headers, 'header_files')
    cmake_set_tst = wrap_files_in_cmake_set(cxx_tests, test_src_varname)
    return cmake_set_src, cmake_set_hdr, cmake_set_tst, test_src_varname

# input: dir_path containing wscript with such a line:
# module = bld.create_ns3_module('applications', ['internet', 'config-store', 'tools', 'point-to-point'])
# output: ['internet', 'config-store', 'tools', 'point-to-point']
def get_linking_libs(dir_path):
    wscript = os.path.join(dir_path, 'wscript')
    wscript_content = helper.read_file(wscript)
    pattern = r"bld\.create_ns3_module\('[^']*', \[([^\]]*)\]\)"
    match = re.search(pattern, wscript_content)
    if match:
        linking_libs = match.group(1).strip().split(',')
        linking_libs = [lib.strip().replace('\'', '') for lib in linking_libs]
        print(linking_libs)
        return linking_libs


def gen_cmakelists_for_dir(dir_path):
    cmake_set_src, cmake_set_hdr, cmake_set_tst, _ = \
        get_all_cxx_files_in_dir(dir_path)
    model_name = os.path.basename(dir_path)

    # linking libs
    linking_libs = get_linking_libs(dir_path)
    set_libs_to_link = f"set(libraries_to_link "
    for lib in linking_libs:
        set_libs_to_link += " ${"+lib+"} "
    set_libs_to_link += ")"

    # build lib
    build_lib = 'build_lib("${name}" "${source_files}" '\
        '"${header_files}" "${libraries_to_link}" "${test_sources}")'

    CMakeLists = f"""set(name {model_name})

{cmake_set_src}

{cmake_set_hdr}

{set_libs_to_link}

{cmake_set_tst}

{build_lib}"""
    helper.write_file(os.path.join(dir_path, "CMakeLists.txt"), CMakeLists)

def replace_cmake_lists(dir_path, cmake_set_src, cmake_set_hdr, 
                        cmake_set_tst, test_src_varname):
    cmake_set_src_pattern = r'set\(source_files\s*([^)]|\n)*?\)'
    cmake_set_hdr_pattern = r'set\(header_files\s*([^)]|\n)*?\)'
    cmake_set_tst_pattern = r'set\(test_sources\s*([^)]|\n)*?\)' 
    cmake_test_files_pattern = r'test_files'

    # the test sources sometimes are called 'test_sources', sometimes 'test_files'
    # we first replace all 'test_files' with 'test_sources'
    helper.match_and_replace_in_file(dir_path+'/CMakeLists.txt',
                                        cmake_test_files_pattern, test_src_varname)
    helper.match_and_replace_in_file(dir_path+'/CMakeLists.txt',
                                     cmake_set_src_pattern, cmake_set_src)
    helper.match_and_replace_in_file(dir_path+'/CMakeLists.txt',
                                            cmake_set_hdr_pattern, cmake_set_hdr)
    helper.match_and_replace_in_file(dir_path+'/CMakeLists.txt',
                                     cmake_set_tst_pattern, cmake_set_tst)   
                                    
def test_replace():
    text = 'hello world\nxsdfsdfwe\nhello world'
    pattern = 'world'
    new_content = re.sub(pattern, 'there', text)
    print(new_content)

def test_get_linking_libs():
    dir_path = 'simulation/src/applications'
    get_linking_libs(dir_path)

def update_cmakelists(dir_path):
    cmake_set_src, cmake_set_hdr, cmake_set_tst, \
        test_src_varname = get_all_cxx_files_in_dir(dir_path)
    replace_cmake_lists(dir_path, cmake_set_src, cmake_set_hdr, 
        cmake_set_tst, test_src_varname)

def main():
    # assert working dir is simulation
    # print(os.getcwd())
    # get_all_cxx_files_in_dir('simulation/src/wifi')
    dir_path = 'simulation/src/propagation'

    update_cmakelists(dir_path)

    # gen_cmakelists_for_dir(dir_path)

    # cmake_set_src, cmake_set_hdr, cmake_set_tst, test_src_varname = \
    #     get_all_cxx_files_in_dir(dir_path)
    # print(cmake_set_src)


if __name__ == "__main__":
    main()
    # test_replace()
    # test_get_linking_libs()