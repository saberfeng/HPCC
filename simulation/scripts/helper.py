import re

def append_file(file_path, content):
    with open(file_path, "a+") as f:
        f.write(content)

def cycles_to_str(cycles):
    return " ".join([str(cycle) for cycle in cycles])

def str_to_cycles(cycles):
    return [int(cycle) for cycle in cycles.split(" ")]

def read_file(file_path):
    # try encodings: utf-8, windows-1252
    f = open(file_path, "r", encoding='utf-8')
    while True:
        try:
            line = f.readline()
        except UnicodeDecodeError:
            f.close()
            encoding = 'windows-1252'
            break
        if not line:
            f.close()
            encoding = 'utf-8'
            break
    with open(file_path, "r", encoding=encoding) as f:
        return f.read()
    

def write_file(file_path, content):
    with open(file_path, "w", encoding='utf-8') as f:
        return f.write(content)

def match_and_replace_in_file(file_path, pattern, replace):
    content = read_file(file_path)
    new_content = re.sub(pattern, replace, content)
    write_file(file_path, new_content)