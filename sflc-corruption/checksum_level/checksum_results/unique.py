import matplotlib.pyplot as plt

f = open("backup_1MB/results1.txt","r")

data = []
for l in f:
    data.append(int(l.strip()))

decoy_file_size="1"
fs_failures=[]
image="1"

plt.plot(data)
plt.suptitle("Shufflecake file corruption")

plt.title("1MB hidden files, 20*"+decoy_file_size+"MB decoy files per round")

plt.xlabel("Rounds")
plt.ylabel("Files corrupted")

for i in fs_failures:
    plt.axvline(i-1,color="red",label="Filesystem failure(s)")

handles, labels = plt.gca().get_legend_handles_labels()
by_label = dict(zip(labels, handles))

plt.legend(by_label.values(), by_label.keys())

plt.savefig("graphs_unique/checksum_corrupt_"+decoy_file_size+"MB_"+image+".png")

f.close()
