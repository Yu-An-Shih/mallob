import matplotlib.pyplot as plt
from functools import partial
import numpy as np

import re

import math

def fetch_values(directory, cases, num_pe, total_times, comm_num_epochs, comm_avg_times):
    
    comm_num_epochs_debug = [0 for i in range(len(comm_num_epochs))]

    for i in range(len(num_pe)):
        for case in cases:
            found_result = False
            comm_num_epochs_debug[i] = comm_num_epochs[i]
            
            try:
                f = open(directory + case + "-" + str(num_pe[i]) + ".txt", "r")
            except:
                print("WARN: " + directory + case + "-" + str(num_pe[i]) + ".txt does not exist!!!")
                continue
        
            while True:
                line = f.readline()

                #if re.search("TIMEOUT", line):
                #    print("WARN: " + directory + case + " timed out with " + str(num_pe[i]) + " PEs!!!")
                
                if not line:
                    if found_result == False:
                        print("WARN: " + directory + case + " timed out with " + str(num_pe[i]) + " PEs!!!")
                    break
                
                if re.search("Communication", line) != None:
                    digits = re.findall("\s\d+\s", line)
                    assert len(digits) == 2
                    assert int(digits[1]) == comm_num_epochs[i] + 1 - comm_num_epochs_debug[i]
                    comm_num_epochs[i] += 1
                    
                    digits = re.findall("\d+\.\d+", line)
                    assert len(digits) == 2
                    comm_avg_times[i] += float(digits[1])

                
                if re.fullmatch("c\s\d+\.\d+\s0\sMono\sjob\sdone\.\n", line) != None:
                    found_result = True
                    digits = re.findall("\d+\.\d+", line)
                    assert len(digits) == 1
                    total_times[i] += float(digits[0])
            
            f.close()
    
    for i in range(len(total_times)):
        total_times[i] /= len(cases)
    print(total_times)

    for i in range(len(comm_num_epochs)):
        if comm_num_epochs[i] != 0:
            comm_avg_times[i] /= comm_num_epochs[i]
            comm_num_epochs[i] /= len(cases)
    print(comm_num_epochs)
    print(comm_avg_times)


def main():
    old_dir = "original/results/"
    new_dir = "local_clause_sharing/results/"
    new_fast_dir = "local_clause_sharing/results-fast_share/"
    
    cases = ["sat_case_1", "sat_case_3", "unsat_case_1", "unsat_case_3"]

    num_pe = [1, 2, 4, 8, 16, 32, 48]
    
    old_total_times = [0, 0, 0, 0, 0, 0, 0]
    new_total_times = [0, 0, 0, 0, 0, 0, 0]
    new_fast_total_times = [0, 0, 0, 0, 0, 0, 0]
    
    old_comm_num_epochs = [0, 0, 0, 0, 0, 0, 0]
    old_comm_avg_times = [0, 0, 0, 0, 0, 0, 0]
    new_comm_num_epochs = [0, 0, 0, 0, 0, 0, 0]
    new_comm_avg_times = [0, 0, 0, 0, 0, 0, 0]
    new_fast_comm_num_epochs = [0, 0, 0, 0, 0, 0, 0]
    new_fast_comm_avg_times = [0, 0, 0, 0, 0, 0, 0]
    
    fetch_values(old_dir, cases, num_pe, old_total_times, old_comm_num_epochs, old_comm_avg_times)
    fetch_values(new_dir, cases, num_pe, new_total_times, new_comm_num_epochs, new_comm_avg_times)
    fetch_values(new_fast_dir, cases, num_pe, new_fast_total_times, new_fast_comm_num_epochs, new_fast_comm_avg_times)


    # plot
    fig, ax = plt.subplots()
    
    x_index = [0, 1, 2, 3, 4, 5, math.log2(48)]
    ax.set_xticks(x_index)
    ax.set_xticklabels(num_pe)

    ax.set_xlabel('# PEs')
    ax.set_ylabel('avg. runtime (s)')

    ax.plot(x_index, old_total_times, marker = 'o', linestyle = '--', label = 'original approach')
    ax.plot(x_index[1:], new_total_times[1:], marker = '^', linestyle = '--', label = 'local clause sharing')
    ax.plot(x_index[1:], new_fast_total_times[1:], marker = 's', linestyle = '--', label = 'local clause sharing w/ fast sharing')
    ax.legend()

    fig.savefig('timing.png')
    plt.close(fig)

    
    #fig, (ax1, ax2) = plt.subplots(1, 2, sharex=True)
    fig = plt.figure(figsize=(10,4))
    ax1 = fig.add_subplot(121)
    ax2 = fig.add_subplot(122)
    
    ax1.set_xticks(x_index[1:])
    ax1.set_xticklabels(num_pe[1:])

    ax2.set_xticks(x_index[1:])
    ax2.set_xticklabels(num_pe[1:])
    
    ax1.set_xlabel('# PEs')
    ax1.set_ylabel('avg. # communication sessions')
    ax2.set_xlabel('# PEs')
    ax2.set_ylabel('avg. session time (s)')

    ax1.plot(x_index[1:], old_comm_num_epochs[1:], marker = 'o', linestyle = '--', label = 'original approach')
    ax1.plot(x_index[1:], new_comm_num_epochs[1:], marker = '^', linestyle = '--', label = 'local clause sharing')
    ax1.plot(x_index[1:], new_fast_comm_num_epochs[1:], marker = 's', linestyle = '--', label = 'local clause sharing w/ fast sharing')
    ax1.legend()

    ax2.plot(x_index[1:], old_comm_avg_times[1:], marker = 'o', linestyle = '--', label = 'original approach')
    ax2.plot(x_index[1:], new_comm_avg_times[1:], marker = '^', linestyle = '--', label = 'local clause sharing')
    ax2.plot(x_index[1:], new_fast_comm_avg_times[1:], marker = 's', linestyle = '--', label = 'local clause sharing w/ fast sharing')
    ax2.legend()

    fig.savefig('comm.png')
    plt.close(fig)

if __name__ == "__main__":
    main()