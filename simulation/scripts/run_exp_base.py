import pandas as pd
import scripts.helper as helper


class ExperimentRunnerBase:
    def __init__(self, blueprint_path, status_col_name,
                EXP_DISABLE_STATUS = -2,
                EXP_UNRUN_STATUS = -1,
                EXP_DONE_STATUS = 1,
                EXP_RUNNING_STATUS = 0):
        self.blueprint_path = blueprint_path
        self.status_col_name = status_col_name
        self.EXP_DISABLE_STATUS = EXP_DISABLE_STATUS
        self.EXP_UNRUN_STATUS = EXP_UNRUN_STATUS
        self.EXP_DONE_STATUS = EXP_DONE_STATUS
        self.EXP_RUNNING_STATUS = EXP_RUNNING_STATUS 
    
    def read_blueprint(self):
        return pd.read_csv(self.blueprint_path)
    
    def save_blueprint(self, blueprint:pd.DataFrame):
        blueprint.to_csv(self.blueprint_path, index=False)

    
    def _find_unrun_row(self):
        blueprint = self.read_blueprint()
        row_num = blueprint.shape[0]
        for i in range(row_num):
            row_copy = blueprint.loc[i]
            if row_copy.get(self.status_col_name) == self.EXP_UNRUN_STATUS: 
                return row_copy, i
        return False, False

