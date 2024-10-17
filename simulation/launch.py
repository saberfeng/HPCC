import sys
from scripts.run_experiment import HPCCExperiment, BlueprintGenerator
import scripts.helper as helper

def main():
    proj_dir = 'simulation/mix/rand_offset/preliminary'
    app_path = 'simulation/build/scratch/third'
    if len(sys.argv) < 2:
        print("need params")
    else:
        if sys.argv[1] == "preliminary":
            blueprint_name = sys.argv[2]
            if len(sys.argv) > 3:
                proc_num = int(sys.argv[3])
            else:
                proc_num = 2
            blueprint_path = f'{proj_dir}/{blueprint_name}'
            experiment = HPCCExperiment(
                blueprint_path=blueprint_path,
                status_col_name='state',
                proj_dir=proj_dir,
                app_path=app_path,
                proc_num=proc_num,
            )
            # experiment.run_by_blueprint_parallel()
            # experiment.run_by_blueprint()
            # experiment.run_by_blueprint_proc_pool()
            experiment.run_by_blueprint_proc_pool_que_msg()
        elif sys.argv[1] == "test_algo":
            bp2_test_algo_path = 'simulation/mix/rand_offset/preliminary/bp4.csv'
            proc_num = 3
            experiment = HPCCExperiment(
                blueprint_path=bp2_test_algo_path,
                status_col_name='state',
                proj_dir=proj_dir,
                app_path=app_path,
                proc_num=proc_num,
            )
            experiment.run_by_blueprint_proc_pool_que_msg()
        elif sys.argv[1] == "manage_blueprint":
            pass
            # mgr = helper.BlueprintManagerBase(status_col_name='state')
            # path = 'simulation/mix/rand_offset/preliminary/exp_blueprint.csv'
            # mgr.insert_column(path, 'runtimeS', -1)
            # mgr.reorder_one_column(path, 'state', 0)
            # mgr.reset_col_values_by_idx(path, idx_range=[87, 100], col_name='cc', col_val='dcqcn')

            # tmp_blueprint_path = 'simulation/mix/rand_offset/preliminary/tmp_exp_blueprint.csv'
            # bp2_test_algo_path = 'simulation/mix/rand_offset/preliminary/bp2.csv'
            # bp_generator = BlueprintGenerator()
            # bp_generator.gen_test_blueprint(bp2_test_algo_path)

            # bp3_test_algo_path = 'simulation/mix/rand_offset/preliminary/bp3.csv'
            # mgr = helper.BlueprintManagerBase(status_col_name='state', path=bp3_test_algo_path)
            # mgr.insert_column(column_name='mkspanAllNs', column_value=-1)
            # mgr.reorder_one_column(column_name='mkspanAllNs', position=11)
        else:
            print("unknown param")


if __name__ == '__main__':
    main()