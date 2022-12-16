#!/bin/bash

#sbatch run_case_48.slurm
#sbatch run_case_32.slurm

#module purge
module load openmpi/gcc/4.1.2

case_dir="instances/ece585_instances"

sat_case_1="${case_dir}/ssAES_4-4-8_round_7-10_faultAt_8_fault_injections_2_seed_1579630418.cnf.xz"
sat_case_3="${case_dir}/size_5_5_5_i053_r12.cnf.xz"

unsat_case_1="${case_dir}/crafted_n11_d6_c4_num19.cnf.xz"
unsat_case_3="${case_dir}/w16-5.cnf.xz"

I_MPI_OFI_PROVIDER=tcp srun --partition=malik --nodes=3 --ntasks=48 --cpus-per-task=4 --time=00:10:00 build/mallob -t=4 -s=0.4 -mono=$sat_case_1 -satsolver=l -jwl=500 -v=0 2>&1 > sat_case_1-48.txt
I_MPI_OFI_PROVIDER=tcp srun --partition=malik --nodes=3 --ntasks=48 --cpus-per-task=4 --time=00:10:00 build/mallob -t=4 -s=0.4 -mono=$sat_case_3 -satsolver=l -jwl=500 -v=0 2>&1 > sat_case_3-48.txt
I_MPI_OFI_PROVIDER=tcp srun --partition=malik --nodes=3 --ntasks=48 --cpus-per-task=4 --time=00:10:00 build/mallob -t=4 -s=0.4 -mono=$unsat_case_1 -satsolver=l -jwl=500 -v=0 2>&1 > unsat_case_1-48.txt
I_MPI_OFI_PROVIDER=tcp srun --partition=malik --nodes=3 --ntasks=48 --cpus-per-task=4 --time=00:10:00 build/mallob -t=4 -s=0.4 -mono=$unsat_case_3 -satsolver=l -jwl=1000 -v=0 2>&1 > unsat_case_3-48.txt

sbatch run_case_16.slurm
sbatch run_case_8.slurm
sbatch run_case_4.slurm
sbatch run_case_2.slurm
#sbatch run_case_1.slurm

I_MPI_OFI_PROVIDER=tcp srun --partition=malik --nodes=2 --ntasks=32 --cpus-per-task=4 --time=00:10:00 build/mallob -t=4 -s=0.4 -mono=$sat_case_1 -satsolver=l -jwl=500 -v=0 2>&1 > sat_case_1-32.txt
I_MPI_OFI_PROVIDER=tcp srun --partition=malik --nodes=2 --ntasks=32 --cpus-per-task=4 --time=00:10:00 build/mallob -t=4 -s=0.4 -mono=$sat_case_3 -satsolver=l -jwl=500 -v=0 2>&1 > sat_case_3-32.txt
I_MPI_OFI_PROVIDER=tcp srun --partition=malik --nodes=2 --ntasks=32 --cpus-per-task=4 --time=00:10:00 build/mallob -t=4 -s=0.4 -mono=$unsat_case_1 -satsolver=l -jwl=500 -v=0 2>&1 > unsat_case_1-32.txt
I_MPI_OFI_PROVIDER=tcp srun --partition=malik --nodes=2 --ntasks=32 --cpus-per-task=4 --time=00:10:00 build/mallob -t=4 -s=0.4 -mono=$unsat_case_3 -satsolver=l -jwl=1000 -v=0 2>&1 > unsat_case_3-32.txt
