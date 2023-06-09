import statistics

import matplotlib.pyplot as plt

decoy_file_size=1
rounds = 40
volume_size=1000

data = []

for i in range(1,31):
    f = open("backup_"+str(decoy_file_size)+"MB/results"+str(i)+".txt","r")
    subdata = []
    subdata.append(0)
    for l in f:
        subdata.append(int(l.strip()))
    data.append(subdata)
    f.close()

counpounded = zip(*data)
averages = [statistics.mean(list(x)) for x in counpounded]

fig,ax1 = plt.subplots()

#ax1.set_title("100 * 1MB hidden files, 20 * "+str(decoy_file_size)+"MB decoy files per round")

ax1.set_xlabel("Rounds", fontsize=20)
ax1.set_xlim([0,rounds])
ax1.set_ylabel("Files corrupted",color="blue", fontsize=20)
ax1.set_ylim([0,100])
ax1.plot(averages)
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
plt.savefig("graphs_average/average_checksum_corrupt_"+str(decoy_file_size)+"MB.png")
