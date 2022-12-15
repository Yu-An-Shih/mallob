#!/bin/bash
#SBATCH --job-name=run_original_1         # create a short name for your job
#SBATCH --partition=malik
#SBATCH --nodes=1                         # node count
#SBATCH --ntasks=1                        # total number of tasks across all nodes
#SBATCH --cpus-per-task=4                 # cpu-cores per task (>1 if multi-threaded tasks)
#SBATCH --mem-per-cpu=4G                  # memory per cpu-core (4G is default)
#SBATCH --time=02:30:00                   # total run time limit (HH:MM:SS)
#SBATCH --mail-type=all                   # send email when job starts, ends and faults
#SBATCH --mail-user=yashih@princeton.edu

module purge
module load openmpi/gcc/4.1.2

case_dir="instances/ece585_instances"

sat_case_1="${case_dir}/ssAES_4-4-8_round_7-10_faultAt_8_fault_injections_2_seed_1579630418.cnf.xz"
sat_case_2="${case_dir}/preimage_80r_491m_160h_seed_71.cnf.xz"
sat_case_3="${case_dir}/size_5_5_5_i053_r12.cnf.xz"

unsat_case_1="${case_dir}/crafted_n11_d6_c4_num19.cnf.xz"
unsat_case_2="${case_dir}/size_4_5_5_i031_w31_r9.cnf.xz"
unsat_case_3="${case_dir}/w16-5.cnf.xz"

# test
#RDMAV_FORK_SAFE=1 mpirun -np 1 --bind-to core --map-by ppr:1:node:pe=4 build/mallob -t=4 -mono="instances/r3sat_300.cnf" -satsolver=l -jwl=1000 2>&1 > test.txt

# satisfiable
RDMAV_FORK_SAFE=1 mpirun -np 1 --bind-to core --map-by ppr:1:node:pe=4 build/mallob -t=4 -mono=$sat_case_1 -satsolver=l -v=0 2>&1 > sat_case_1-1.txt
#RDMAV_FORK_SAFE=1 mpirun -np 1 --bind-to core --map-by ppr:1:node:pe=4 build/mallob -t=4 -mono=$sat_case_2 -satsolver=l -v=0 2>&1 > sat_case_2-1.txt
RDMAV_FORK_SAFE=1 mpirun -np 1 --bind-to core --map-by ppr:1:node:pe=4 build/mallob -t=4 -mono=$sat_case_3 -satsolver=l -v=0 2>&1 > sat_case_3-1.txt

# unsatisfiable
#RDMAV_FORK_SAFE=1 mpirun -np 1 --bind-to core --map-by ppr:1:node:pe=4 build/mallob -t=4 -mono=$unsat_case_1 -satsolver=l -v=0 2>&1 > unsat_case_1-1.txt
#RDMAV_FORK_SAFE=1 mpirun -np 1 --bind-to core --map-by ppr:1:node:pe=4 build/mallob -t=4 -mono=$unsat_case_2 -satsolver=l -v=0 2>&1 > unsat_case_2-1.txt
RDMAV_FORK_SAFE=1 mpirun -np 1 --bind-to core --map-by ppr:1:node:pe=4 build/mallob -t=4 -mono=$unsat_case_3 -satsolver=l -v=0 2>&1 > unsat_case_3-1.txt