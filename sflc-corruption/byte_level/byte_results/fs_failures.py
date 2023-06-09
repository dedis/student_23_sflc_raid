import matplotlib.pyplot as plt
import numpy as np

decoy_file_size=1
rounds=40

journal=[]
superblock=[]
directory=[]

for i in range(1,31):
    f = open("backup_"+str(decoy_file_size)+"MB/fserr"+str(i)+".txt","r")
    for l in f:
        components = l.split(',')
        if len(components)==1:
            continue

        if components[0]=="invalid journal":
            for c in components[1:]:
                journal.append(int(c.strip()))
        elif components[0]=="invalid superblock":
            for c in components[1:]:
                superblock.append(int(c.strip()))
        elif components[0]=="corrupted directory":
            for c in components[1:]:
                directory.append(int(c.strip()))
    f.close()

max = 0
x = np.arange(1,rounds+1)
j = []
for i in x:
    count = journal.count(i)
    j.append(count)
    if count>max:
        max=count
s = []
for i in x:
    count = superblock.count(i)
    s.append(count)
    if count>max:
        max=count
d = []
for i in x:
    count = directory.count(i)
    d.append(count)
    if count>max:
        max=count

plt.bar(x-0.2,j,0.2,label="Invalid journal")
plt.bar(x,s,0.2,label="Invalid superblock")
plt.bar(x+0.2,j,0.2,label="Corrupted directory")

plt.title("1MB hidden files, 20*"+str(decoy_file_size)+"MB decoy files per round")

plt.xlabel("Rounds")
plt.xticks(range(1,rounds+1))
plt.yticks(range(1,max+1))
plt.legend()

plt.savefig("graphs_fs/fs_failures_"+str(decoy_file_size)+"MB.png")
