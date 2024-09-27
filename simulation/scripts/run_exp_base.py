import pandas as pd
import scripts.helper as helper


class ExperimentRunnerBase:
    def __init__(self, blueprint_path, status_col_name, dtype:dict):
        self.blueprint_path = blueprint_path
        self.status_col_name = status_col_name
        self.blueprint_mgr = helper.BlueprintManagerBase(
            status_col_name=status_col_name, 
            path=blueprint_path,)
    
    def read_blueprint(self):
        return pd.read_csv(self.blueprint_path)
    
    def save_blueprint(self, blueprint:pd.DataFrame):
        blueprint.to_csv(self.blueprint_path, index=False)

    
    def _find_unrun_row(self):
        blueprint = self.read_blueprint()
        row_num = blueprint.shape[0]
        for i in range(row_num):
            row_copy = blueprint.loc[i]
            if row_copy.get(self.status_col_name) == self.blueprint_mgr.EXP_UNRUN_STATUS: 
                return row_copy, i
        return False, False
    
    # length == -1 when we want to find all unrun rows
    def _find_unrun_row_list(self, length):
        blueprint = self.read_blueprint()
        row_copy_li = []
        row_id_li = []
        row_num = blueprint.shape[0]
        for i in range(row_num):
            row_copy = blueprint.loc[i]
            if row_copy.get(self.status_col_name) == self.blueprint_mgr.EXP_UNRUN_STATUS: 
                row_copy_li.append(row_copy)    
                row_id_li.append(i)
                if len(row_copy_li) == length:
                    break
        return row_copy_li, row_id_li

