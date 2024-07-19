#!/bin/bash
#SBATCH --job-name="gapExperiments"
#SBATCH --error="gapExperiments.err"
#SBATCH --output="gapExperiments.out"
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=16
#SBATCH --threads-per-core=2
#SBATCH --time=4-00:00:00
#SBATCH --partition=gpu
#SBATCH --nodelist=cheetah01
#SBATCH --reservation=cmv6ru_100
#SBATCH --mem=120G

module load python3
python3 -u ../gapExperiments.py "/bigtemp/rgq5aw/graphDatasets_nested/" "-n100" 