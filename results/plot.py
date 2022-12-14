import matplotlib.pyplot as plt
import re

def fetch_values(directory, cases, num_pe, total_times, comm_mum_epochs, comm_avg_times):
    
    comm_num_epochs_debug = [0 for i in range(len(comm_mum_epochs))]

    for i in range(len(num_pe)):
        for case in cases:
            found_result = False
            comm_num_epochs_debug[i] = comm_mum_epochs[i]
            
            f = open(directory + case + "-" + str(num_pe[i]) + ".txt", "r")
        
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
                    assert int(digits[1]) == comm_mum_epochs[i] + 1 - comm_num_epochs_debug[i]
                    comm_mum_epochs[i] += 1
                    
                    digits = re.findall("\d+\.\d+", line)
                    assert len(digits) == 2
                    comm_avg_times[i] += float(digits[1])

                
                if re.fullmatch("c\s\d+\.\d+\s0\sMono\sjob\sdone\.\n", line) != None:
                    found_result = True
                    digits = re.findall("\d+\.\d+", line)
                    assert len(digits) == 1
                    total_times[i] += float(digits[0])
            
            f.close()
    
    total_times = [x / len(cases) for x in total_times]
    print(total_times)

    for i in range(len(comm_mum_epochs)):
        if comm_mum_epochs[i] != 0:
            comm_avg_times[i] /= comm_mum_epochs[i]
    print(comm_mum_epochs)
    print(comm_avg_times)


def main():
    old_dir = "original/"
    new_dir = "local_clause_sharing/"
    
    cases = ["sat_case_1", "sat_case_3", "unsat_case_1", "unsat_case_3"]

    num_pe = [1, 2, 4, 8, 16, 32, 48]
    
    old_total_times = [0, 0, 0, 0, 0, 0, 0]
    new_total_times = [0, 0, 0, 0, 0, 0, 0]
    
    old_comm_mum_epochs = [0, 0, 0, 0, 0, 0, 0]
    old_comm_avg_times = [0, 0, 0, 0, 0, 0, 0]
    new_comm_mum_epochs = [0, 0, 0, 0, 0, 0, 0]
    new_comm_avg_times = [0, 0, 0, 0, 0, 0, 0]
    
    #fetch_values(old_dir, cases, num_pe, old_total_times, old_comm_mum_epochs, old_comm_avg_times)
    fetch_values(new_dir, cases, num_pe, new_total_times, new_comm_mum_epochs, new_comm_avg_times)


    # plot
    x_index = list(range(len(num_pe)))
    plt.xticks(x_index, num_pe)

    #plt.title('')

    plt.xlabel('# PEs')
    plt.ylabel('avg. runtime (s)')

    plt.plot(x_index, old_total_times, marker = 'o', linestyle = '--', label = 'original approach')
    plt.plot(x_index, new_total_times, marker = 'o', linestyle = '--', label = 'local clause sharing')
    plt.legend()
    
    plt.savefig('timing.png')
    plt.close()

    
    #fig, (ax1, ax2) = plt.subplots(1, 2, sharex=True)
    fig = plt.figure(figsize=(10,4))
    ax1 = fig.add_subplot(121)
    ax2 = fig.add_subplot(122)
    
    ax1.set_xticks(x_index[1:])
    ax1.set_xticklabels(num_pe[1:])

    ax2.set_xticks(x_index[1:])
    ax2.set_xticklabels(num_pe[1:])
    
    ax1.set_xlabel('# PEs')
    ax1.set_ylabel('# communication sessions')
    ax2.set_xlabel('# PEs')
    ax2.set_ylabel('avg. session time (s)')

    ax1.plot(x_index[1:], old_comm_mum_epochs[1:], marker = 'o', linestyle = '--', label = 'original approach')
    ax1.plot(x_index[1:], new_comm_mum_epochs[1:], marker = 'o', linestyle = '--', label = 'local clause sharing')
    ax1.legend()

    ax2.plot(x_index[1:], old_comm_avg_times[1:], marker = 'o', linestyle = '--', label = 'original approach')
    ax2.plot(x_index[1:], new_comm_avg_times[1:], marker = 'o', linestyle = '--', label = 'local clause sharing')
    ax2.legend()

    fig.savefig('comm.png')
    plt.close(fig)

if __name__ == "__main__":
    main()