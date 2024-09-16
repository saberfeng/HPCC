import sys
import scripts.run_experiment as run_experiment

def main():
    proj_dir = 'simulation/mix/rand_offset/preliminary'
    app_path = 'simulation/build/scratch/third'
    if len(sys.argv) < 2:
        print("need params")
    else:
        if sys.argv[1] == "preliminary":
            blueprint_name = 'exp_blueprint.csv'
            blueprint_path = f'{proj_dir}/{blueprint_name}'
            experiment = run_experiment.HPCCExperiment(
                blueprint_path=blueprint_path,
                status_col_name='state',
                proj_dir=proj_dir,
                app_path=app_path,
            )
            experiment.run_by_blueprint()
        else:
            print("unknown param")


if __name__ == '__main__':
    main()