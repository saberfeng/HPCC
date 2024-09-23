import sys
from scripts.run_experiment import HPCCExperiment
import scripts.helper as helper

def main():
    proj_dir = 'simulation/mix/rand_offset/preliminary'
    app_path = 'simulation/build/scratch/third'
    if len(sys.argv) < 2:
        print("need params")
    else:
        if sys.argv[1] == "preliminary":
            if sys.argc > 2:
                proc_num = int(sys.argv[2])
            else:
                proc_num = 4
            blueprint_name = 'exp_blueprint.csv'
            blueprint_path = f'{proj_dir}/{blueprint_name}'
            experiment = HPCCExperiment(
                blueprint_path=blueprint_path,
                status_col_name='state',
                proj_dir=proj_dir,
                app_path=app_path,
                proc_num=proc_num,
            )
            experiment.run_by_blueprint_parallel()

        elif sys.argv[1] == "manage_blueprint":
            mgr = helper.BlueprintManagerBase(status_col_name='state')
            path = 'simulation/mix/rand_offset/preliminary/exp_blueprint.csv'
            # mgr.insert_column(path, 'runtimeS', -1)
            # mgr.reorder_one_column(path, 'state', 0)
        else:
            print("unknown param")


if __name__ == '__main__':
    main()