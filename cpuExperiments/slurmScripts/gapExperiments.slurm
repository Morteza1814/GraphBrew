#!/bin/bash
#SBATCH --job-name="gapExperiments"
#SBATCH --error="gapExperiments.err"
#SBATCH --output="gapExperiments.out"
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=32
#SBATCH --threads-per-core=2
#SBATCH --mem=400G
#SBATCH --time=4-00:00:00
#SBATCH --nodelist=cheetah05 
#SBATCH --partition=gpu
#SBATCH --reservation=cmv6ru_96

module load python3
python3 -u ../gapExperiments.py