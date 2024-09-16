import re
import pandas as pd
import json
# from .data_structures import PathConfig, SchedConfig
import os

def append_file(file_path, content):
    with open(file_path, "a+") as f:
        f.write(content)


def cycles_to_str(cycles):
    return " ".join([str(cycle) for cycle in cycles])

def str_to_cycles(cycles):
    return [int(cycle) for cycle in cycles.split(" ")]

def write_file(file_path, content):
    with open(file_path, "w") as f:
        return f.write(content)

class BlueprintManagerBase:
    def __init__(self, status_col_name) -> None:
        self.status_col_name = status_col_name

    # blueprint is a csv file saving experiment configs and results, header:
    # topo,cycles,cycleIdx,macrotick,seed,status,runtime,flow_num
    # - successful experiments have status=1, 
    # - ready-to-run experiments have status=-1, 
    # - disabled experiments have status=0

    #TODO: list of conditions; blueprint = blueprint.loc[]; 
    def _disable_rows(self, blueprint, condition):
        blueprint.loc[condition, self.status_col_name] = 0
    
    def _get_rows_by_condition(self, blueprint, condition):
        return blueprint.loc[condition]
    
    def disable_rows(self, blueprint, con_and_list):
        for condition in con_and_list:
            blueprint = self._get_rows_by_condition(blueprint, condition)
        blueprint.loc[:,self.status_col_name] = 0

    
    def set_status_seed_range(self, path, seed_from:int, 
                                seed_to_notinclude:int, status_names:list,
                                status_val:int):
        blueprint = pd.read_csv(path)
        condition = (blueprint['seed'] >= seed_from) & (blueprint['seed'] < seed_to_notinclude)
        assert type(status_names) == list
        for status_name in status_names:
            blueprint.loc[condition, status_name] = status_val
        blueprint.to_csv(path, index=False)
    
    def move_columns_to_back(self, path, column_names):
        blueprint = pd.read_csv(path)
        blueprint = blueprint[[c for c in blueprint if c not in column_names] + column_names]
        blueprint.to_csv(path, index=False)

    def order_rows(self, path:str, priority_cols:list):
        blueprint = pd.read_csv(path)
        blueprint = blueprint.sort_values(by=priority_cols)
        blueprint.to_csv(path, index=False)

    def reset_status_by_col_value(self, path, con_col:str, con_val, status_names:list):
        blueprint = pd.read_csv(path)
        condition = (blueprint[con_col] == con_val)
        assert type(status_names) == list
        for status_name in status_names:
            blueprint.loc[condition, status_name] = -1
        blueprint.to_csv(path, index=False)
    
    def reset_col_values(self, path, con_col:str, con_val, reset_col_val_pair:dict):
        blueprint = pd.read_csv(path)
        condition = (blueprint[con_col] == con_val)
        for reset_col, reset_val in reset_col_val_pair.items():
            blueprint.loc[condition, reset_col] = reset_val
        blueprint.to_csv(path, index=False)
    
    def reset_val_by_func(self, path, conditions:dict, reset_col_func_pairs:dict):
        blueprint = pd.read_csv(path)
    
    def delete_column(self, path, column_names):
        blueprint = pd.read_csv(path)
        for column_name in column_names:
            blueprint = blueprint.drop(columns=[column_name])
        blueprint.to_csv(path, index=False)
 

class BlueprintManager:
    def __init__(self) -> None:
        pass

    def disable_large_hypercycle_rows(self, path):
        blueprint = pd.read_csv(path)

        condition = blueprint['cycles'] != "4000 16000 20000 10000 5000 8000 10000 20000"
        self._disable_rows(blueprint, condition)

        blueprint.to_csv(path, index=False)
    

    def disable_small_macrotick_rows(self, path):
        blueprint = pd.read_csv(path)

        condition = blueprint['macrotick'] < 1
        self._disable_rows(blueprint, condition)

        blueprint.to_csv(path, index=False)

  
    def reset_mt_var_blueprint_seed(self, path:str):
        # for the same topo, cycleIdx, instances share the same seed
        blueprint = pd.read_csv(path)
        row_num = blueprint.shape[0]

        last_topo = ''
        last_cycle_idx = ''
        cur_row_seed = 0
        for i in range(row_num):
            row_copy = blueprint.loc[i]
            cur_topo = row_copy.get('topo')
            cur_cycle_idx = row_copy.get('cycleIdx')
            if cur_topo != last_topo or cur_cycle_idx != last_cycle_idx:
                cur_row_seed = row_copy.get('seed')
                last_topo = cur_topo
                last_cycle_idx = cur_cycle_idx
            else:
                blueprint.loc[i, 'seed'] = cur_row_seed
                blueprint.loc[i, 'status'] = -1
        blueprint.to_csv(path, index=False)


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
    

def match_and_replace_in_file(file_path, pattern, replace):
    content = read_file(file_path)
    new_content = re.sub(pattern, replace, content)
    write_file(file_path, new_content)


# -------------- file helpers --------------------

def get_file_content(file_path):
    with open(file_path, "r") as f:
        content = f.read()
        return content


def write_json_to_file(content, path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(json.dumps(content, indent=4))


def read_json_from_file(path):
    with open(path, "r") as f:
        return json.loads(f.read())


def flow_periods_list_to_str(flow_periods: list):
    flow_str_periods = []
    for flow_period in flow_periods:
        flow_str_periods.append(str(flow_period))
    return ','.join(flow_str_periods)


def get_concise_num(number: int):
    if number % 1000 == 0 and number >= 1000:
        return str(int(number/1000)) + 'k'
    else:
        return str(number)


def get_dotted_concise_num_list(numbers):
    return '.'.join([get_concise_num(number) for number in numbers])


