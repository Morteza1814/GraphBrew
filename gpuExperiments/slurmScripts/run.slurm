#!/bin/bash
#SBATCH --job-name="run"
#SBATCH --error="run.err"
#SBATCH --output="run.out"
#SBATCH --gpus=1
#SBATCH --constraint=a100_80gb
#SBATCH --time=4-00:00:00
#SBATCH --nodelist=cheetah05 
#SBATCH --partition=gpu
#SBATCH --reservation=cmv6ru_96
#SBATCH --mem=400GB

module load cuda-toolkit-12.3.2
module load cmake-3.24.0
module load python3

export CUDA_HOME=/sw/ubuntu-22.04/cuda/12.3.2/bin/

# GPUGPUSim
export CUDA_INSTALL_PATH=${CUDA_HOME}
export PTXAS_CUDA_INSTALL_PATH=${CUDA_HOME}

export PATH=${CUDA_HOME}/bin:${PATH}
export LD_LIBRARY_PATH=${CUDA_HOME}/lib64:$LD_LIBRARY_PATH

export PATH=/usr/bin:${PATH}
# export LD_LIBRARY_PATH=/tools/Xilinx/Vitis/2023.1/tps/lnx64/cmake-3.24.2/libs/Ubuntu:$LD_LIBRARY_PATH
export PATH=/usr/local/bin:$PATH

python3 /u/rgq5aw/GIT/GraphBrew/gpuExperiments/gpuExperiments_gunrock.py