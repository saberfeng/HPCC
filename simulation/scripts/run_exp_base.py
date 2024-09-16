import pandas as pd
import helper

EXP_DISABLE_STATUS = 0
EXP_UNRUN_STATUS = -1
EXP_RUN_STATUS = 1

class ExperimentRunnerBase:
    def __init__(self, blueprint_path, status_col_name):
        self.blueprint_path = blueprint_path
        self.status_col_name = status_col_name
    
    
    def read_blueprint(self):
        return pd.read_csv(self.blueprint_path)
    
    def save_blueprint(self, blueprint:pd.DataFrame):
        blueprint.to_csv(self.blueprint_path, index=False)

    
    def _find_unrun_row(self):
        blueprint = self.read_blueprint()
        row_num = blueprint.shape[0]
        for i in range(row_num):
            row_copy = blueprint.loc[i]
            if row_copy.get(self.status_col_name) == EXP_UNRUN_STATUS: 
                return row_copy, i
        return False, False

