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
    cmake_set += ")\n\n"
    return cmake_set

def get_all_cxx_files_in_dir(dir_path):
    source_header_subdir = ['model', 'helper', 'utils']
    cxx_sources = get_files_in_dir(dir_path, source_header_subdir, 
                                   ['.cpp', '.cc', '.cxx'])
    cxx_headers = get_files_in_dir(dir_path, source_header_subdir, 
                                   ['.h', '.hpp'])
    cxx_tests = get_files_in_dir(dir_path, ['test'],['.cc', '.cpp', '.cxx'])

    cmake_set_src = wrap_files_in_cmake_set(cxx_sources, 'source_files')
    cmake_set_hdr = wrap_files_in_cmake_set(cxx_headers, 'header_files')
    cmake_set_tst = wrap_files_in_cmake_set(cxx_tests, 'test_files')

    cmake_set_src_pattern = r'set\(source_files\s*([^)]|\n)*?\)'
    cmake_set_hdr_pattern = r'set\(header_files\s*([^)]|\n)*?\)'
    cmake_set_tst_pattern = r'set\(test_files\s*([^)]|\n)*?\)' 

    helper.match_and_replace_in_file(dir_path+'/CMakeLists.txt',
                                     cmake_set_src_pattern, cmake_set_src)
    helper.match_and_replace_in_file(dir_path+'/CMakeLists.txt',
                                            cmake_set_hdr_pattern, cmake_set_hdr)
    helper.match_and_replace_in_file(dir_path+'/CMakeLists.txt',
                                     cmake_set_tst_pattern, cmake_set_tst)   
                                    
def main():
    # assert working dir is simulation
    print(os.getcwd())
    # get_all_cxx_files_in_dir('simulation/src/wifi')
    get_all_cxx_files_in_dir('simulation/src/propagation')

def test_replace():
    text = 'hello world'
    pattern = 'world'
    new_content = re.sub(pattern, 'there', text)
    print(new_content == 'hello there')

if __name__ == "__main__":
    main()
    # test_replace()