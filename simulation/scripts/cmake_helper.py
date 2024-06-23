import os
import helper
import re
import subprocess
from datetime import datetime

def get_files_in_dir(dir_path:str, sub_dirs:list[str], file_exts:list[str]):
    '''Returns a list of files with specified extensions in the 
    directory specified by dir_path and sub_dirs.
    '''   
    cxx_sources = []
    if sub_dirs is None or len(sub_dirs) == 0:
        # get all subdirs
        sub_dirs = [d for d in os.listdir(dir_path) if os.path.isdir(os.path.join(dir_path, d))]
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

def substitute_all_files_in_dir(dir_path:str, file_exts:list[str],
                                substitute:str, replace:str):
    '''Substitutes all files with specified extensions in the directory
    specified by dir_path.
    '''
    for root, dirs, files in os.walk(dir_path):
        for file in files:
            if file.endswith(tuple(file_exts)):
                file_path = os.path.join(root, file)
                helper.match_and_replace_in_file(file_path, substitute, replace)

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

def remove_NS_LOG_FUNCTION():
    dir = 'simulation/src/'
    pattern = r'\/*NS_LOG_FUNCTION\s*([^)]|\n)*?\)'
    substitute = '//NS_LOG_FUNCTION'
    file_exts = ['.cc'] 
    substitute_all_files_in_dir(dir, file_exts, pattern, substitute)

def file_restore(search_dir:str, target_datetime_str:str):
    def is_git_tracked(file_path):
        """ Check if the file is tracked by Git. """
        try:
            subprocess.run(['git', 'ls-files', '--error-unmatch', file_path], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            return True
        except subprocess.CalledProcessError:
            return False
    
    def reset_git_file(file_path):
        """ Reset the file to the last commit state. """
        subprocess.run(['git', 'checkout', '--', file_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        print(f"Reset: {file_path}")
    
    # datetime format: '2024-06-23 03:44:29.000000000'
    target_datetime = datetime.strptime(target_datetime_str, '%Y-%m-%d %H:%M:%S')\
                              .replace(microsecond=0)
    count = 0
    for root, dirs, files in os.walk(search_dir):
        for file in files:
            file_path = os.path.join(root, file)
            # Get file modification time and compare with the target time
            modify_time = datetime.fromtimestamp(os.path.getmtime(file_path))\
                                  .replace(microsecond=0)
            if modify_time == target_datetime:
                if is_git_tracked(file_path):
                    reset_git_file(file_path)
                    print(f"Restored: {file_path}")
                    count += 1
    print(f"Total {count} files restored.")
                # else:
                #     print(f"Not tracked by Git: {file_path}")

def get_file_modification_time(file_path):
    return datetime.fromtimestamp(os.path.getmtime(file_path))

def main():
    # assert working dir is simulation
    # print(os.getcwd())
    # get_all_cxx_files_in_dir('simulation/src/wifi')
    # dir_path = 'simulation/src/uan'
    file_restore(search_dir='simulation/src', target_datetime_str='2024-06-23 18:44:29')
    # file_path = 'simulation/src/wimax/test/phy-test.cc'
    # print(get_file_modification_time(file_path))
    # update_cmakelists(dir_path)

    # gen_cmakelists_for_dir(dir_path)

    # cmake_set_src, cmake_set_hdr, cmake_set_tst, test_src_varname = \
    #     get_all_cxx_files_in_dir(dir_path)
    # print(cmake_set_src)

    # remove_NS_LOG_FUNCTION()

if __name__ == "__main__":
    main()
    # test_replace()
    # test_get_linking_libs()