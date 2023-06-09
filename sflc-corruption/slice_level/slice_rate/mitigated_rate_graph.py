import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap
import itertools
import matplotlib.ticker as mtick

def sumup(a):
    count = 0
    for i in range(len(a)):
        count += a[i]
        a[i] = count

among_corr_300kb = [0,0,2,2,2,5,1,3,4,2,2,2,3,1,3,3,2,4,2,4,3,5,2,3,4,1,1,3,2,3,3,4,3,3,5,4,5,1,4,2,3,2,0,2,4,3,7,4,4,2,2]
among_fix_300kb = [0,0,2,2,2,5,1,3,4,2,2,2,3,1,3,3,2,4,2,4,3,5,2,3,4,1,1,3,2,3,3,2,3,3,5,4,5,1,4,2,3,2,0,2,4,3,7,4,4,2,2]

sumup(among_corr_300kb)
sumup(among_fix_300kb)
among_remain_300kb = []
for i in range(len(among_corr_300kb)):
    among_remain_300kb.append(among_corr_300kb[i]-among_fix_300kb[i])
print(among_corr_300kb)
print(among_remain_300kb)

among_corr_1mb = [0,7,9,8,8,5,8,8,9,9,8,7]
among_fix_1mb = [0,7,9,8,4,5,8,6,9,7,8,5]

sumup(among_corr_1mb)
sumup(among_fix_1mb)
among_remain_1mb = []
for i in range(len(among_corr_1mb)):
    among_remain_1mb.append(among_corr_1mb[i]-among_fix_1mb[i])
print(among_corr_1mb)
print(among_remain_1mb)

among_corr_3mb = [0,23,28,25,30,27,40]
among_fix_3mb = [0,21,28,23,30,23,34]

sumup(among_corr_3mb)
sumup(among_fix_3mb)
among_remain_3mb = []
for i in range(len(among_corr_3mb)):
    among_remain_3mb.append(among_corr_3mb[i]-among_fix_3mb[i])
print(among_corr_3mb)
print(among_remain_3mb)

fig,ax1 = plt.subplots()

ax1.set_xlabel("Rounds", fontsize=20)
ax1.set_ylabel("Corrupted slices", color="blue", fontsize=20)
ax1.set_ylim([0,among_corr_300kb[-1]])
ax1.plot(among_corr_300kb)
ax1.tick_params(axis='y', labelcolor="blue")
ax1.tick_params(labelsize=20)

ax2 = ax1.twinx()

ax2.set_ylabel('Unresolved corrupted slices', color="red", fontsize=20)
ax2.set_ylim([0,among_corr_300kb[-1]])
ax2.plot(among_remain_300kb, color="red")
ax2.tick_params(axis='y', labelcolor="red", labelsize=20)

fig.tight_layout()
plt.savefig("special_graphs/mitigated_corruption_rate_300KB.png")

fig,ax1 = plt.subplots()

ax1.set_xlabel("Rounds", fontsize=20)
ax1.set_ylabel("Corrupted slices", color="blue", fontsize=20)
ax1.set_ylim([0,among_corr_1mb[-1]])
ax1.plot(among_corr_1mb)
ax1.tick_params(axis='y', labelcolor="blue")
ax1.tick_params(labelsize=20)

ax2 = ax1.twinx()

ax2.set_ylabel('Unresolved corrupted slices', color="red", fontsize=20)
ax2.set_ylim([0,among_corr_1mb[-1]])
ax2.plot(among_remain_1mb, color="red")
ax2.tick_params(axis='y', labelcolor="red", labelsize=20)

fig.tight_layout()
plt.savefig("special_graphs/mitigated_corruption_rate_1MB.png")

fig,ax1 = plt.subplots()

ax1.set_xlabel("Rounds", fontsize=20)
ax1.set_ylabel("Corrupted slices", color="blue", fontsize=20)
ax1.set_ylim([0,among_corr_3mb[-1]])
ax1.plot(among_corr_3mb)
ax1.tick_params(axis='y', labelcolor="blue")
ax1.tick_params(labelsize=20)

ax2 = ax1.twinx()

ax2.set_ylabel('Unresolved corrupted slices', color="red", fontsize=20)
ax2.set_ylim([0,among_corr_3mb[-1]])
ax2.plot(among_remain_3mb, color="red")
ax2.tick_params(axis='y', labelcolor="red", labelsize=20)

fig.tight_layout()
plt.savefig("special_graphs/mitigated_corruption_rate_3MB.png")
