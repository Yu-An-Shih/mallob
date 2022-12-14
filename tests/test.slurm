#!/bin/bash
#SBATCH --job-name=test_cpu      # create a short name for your job
#SBATCH --partition=malik
#SBATCH --nodes=1                # node count
#SBATCH --ntasks=1               # total number of tasks across all nodes
#SBATCH --cpus-per-task=1        # cpu-cores per task (>1 if multi-threaded tasks)
#SBATCH --mem-per-cpu=4G         # memory per cpu-core (4G is default)
#SBATCH --time=00:01:00          # total run time limit (HH:MM:SS)
# #SBATCH --mail-type=all          # send email when job starts, ends and faults
# #SBATCH --mail-user=yashih@princeton.edu

module purge
module load openmpi/gcc/4.1.2

#lscpu
#echo "---------------------------------------------------------------------------------------------------------------------------------"
#cat /proc/cpuinfo
#echo "---------------------------------------------------------------------------------------------------------------------------------"
#nproc

# RDMAV_FORK_SAFE=1 NPROCS="$(($(nproc)/4))" mpirun -np $NPROCS --bind-to core --map-by ppr:${NPROCS}:node:pe=4 build/mallob -t=4 -mono="instances/r3sat_300.cnf" -satsolver=llck 2>&1 > test.txt
RDMAV_FORK_SAFE=1 mpirun -np 16 --bind-to core --map-by ppr:8:node:pe=4 build/mallob -t=4 -mono="instances/r3sat_300.cnf" -satsolver=llck 2>&1 > test.txt

# RDMAV_FORK_SAFE=1 PATH=build/:$PATH mpirun -np 4 --oversubscribe build/mallob -t=4 -mono="instances/r3sat_300.cnf" -satsolver=llck -appmode=fork -assertresult=SAT 2>&1 > test.txt