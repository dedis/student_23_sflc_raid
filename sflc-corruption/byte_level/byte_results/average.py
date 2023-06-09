import statistics

import matplotlib.pyplot as plt

decoy_file_size=1
rounds = 40
volume_size=1000

summed = []
averaged = []

for i in range(1,31):
    f = open("backup_"+str(decoy_file_size)+"MB/summed"+str(i)+".txt","r")
    subdata = []
    subdata.append(0)
    for l in f:
        corrupted_bytes = int(l.strip())
        corrupted_kbytes = corrupted_bytes/1024
        corrupted_mbytes = corrupted_kbytes/1024
        subdata.append(corrupted_mbytes)
    summed.append(subdata)
    f.close()

    f = open("backup_" + str(decoy_file_size) + "MB/averaged" + str(i) + ".txt", "r")
    subdata = []
    subdata.append(0)
    for l in f:
        corrupted_bytes = int(l.strip())
        corrupted_kbytes = corrupted_bytes / 1024
        corrupted_mbytes = corrupted_kbytes / 1024
        subdata.append(corrupted_mbytes)
    averaged.append(subdata)
    f.close()

counpound_sum = zip(*summed)
sum_averages = [statistics.mean(list(x)) for x in counpound_sum]

counpound_avg = zip(*averaged)
avg_averages = [statistics.mean(list(x)) for x in counpound_avg]

fig,ax1 = plt.subplots()

#ax1.set_title("100 * 1MB hidden files, 20 * "+str(decoy_file_size)+"MB decoy files per round")

ax1.set_xlabel("Rounds", fontsize=20)
ax1.set_xlim([0,rounds])
ax1.set_ylabel("Average MB corrupted",color="blue", fontsize=20)
#ax1.set_ylim([0,100])
ax1.plot(avg_averages)
ax1.tick_params(axis='y', labelcolor="blue")
ax1.tick_params(labelsize=20)

space = []
fill = 100
space.append(round(fill*100/volume_size))

for i in range(1,rounds+1):
    fill+=20*decoy_file_size
    space.append(round(fill*100/volume_size))

ax2 = ax1.twinx()

ax2.set_ylabel('Percentage of space filled', color="orange", fontsize=20)
ax2.set_ylim([0,100])
ax2.plot(space, color="orange")
ax2.tick_params(axis='y', labelcolor="orange", labelsize=20)

fig.tight_layout()
plt.savefig("graphs_average/average_byte_corrupt_"+str(decoy_file_size)+"MB.png")

fig,ax1 = plt.subplots()

#ax1.set_title("100 * 1MB hidden files, 20 * "+str(decoy_file_size)+"MB decoy files per round")

ax1.set_xlabel("Rounds", fontsize=20)
ax1.set_xlim([0,rounds])
ax1.set_ylabel("Total MB corrupted", color="blue", fontsize=20)
#ax1.set_ylim([0,100])
ax1.plot(sum_averages)
ax1.tick_params(axis='y', labelcolor="blue")
ax1.tick_params(labelsize=20)

space = []
fill = 100
space.append(round(fill*100/volume_size))

for i in range(1,rounds+1):
    fill+=20*decoy_file_size
    space.append(round(fill*100/volume_size))

ax2 = ax1.twinx()

ax2.set_ylabel('Percentage of space filled', color="orange", fontsize=20)
ax2.set_ylim([0,100])
ax2.plot(space, color="orange")
ax2.tick_params(axis='y', labelcolor="orange", labelsize=20)

fig.tight_layout()
plt.savefig("graphs_summed/sum_byte_corrupt_"+str(decoy_file_size)+"MB.png")
