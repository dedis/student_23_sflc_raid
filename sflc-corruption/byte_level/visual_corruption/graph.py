import matplotlib.pyplot as plt
import matplotlib.cm as cm

total_size = 1

n_hidden_files = 1
size_hidden_files = 1
metric_hidden_files = "K"

n_decoy_files = 20
size_decoy_files = 1
metric_decoy_files = "M"

n_rounds = 40

n_tests = 5

for k in range(1, n_tests + 1):
    resultsfile = open("results" + str(k) + ".txt", "r")
    results = []
    for j in range(n_hidden_files):
        results.append([])

    for i in range(n_rounds):
        for j in range(n_hidden_files):
            results[j].append([float(c) for c in resultsfile.readline().rstrip()])
        resultsfile.readline()  # consume \n
    resultsfile.readline()  # consume \n

    resultsfile.close()

    fig, (fs, data) = plt.subplots(1, 2, width_ratios=[1, 3])

    data.pcolormesh(results[0], cmap=cm.gray_r)

    data.set_xlabel("Bytes", fontsize=12)
    data.xaxis.set_label_position('top')
    data.set_ylabel("Rounds", fontsize=12)

    data.invert_yaxis()
    data.xaxis.tick_top()

    '''
    def flatten(l):
        return [item for sublist in l for item in sublist]

    fill = [[((i * n_decoy_files * size_decoy_files) * 100 / (total_size * 1024))] for i in range(n_rounds)]

    spaceplot = space.pcolormesh(fill,vmin=0,vmax=100)

    space.tick_params(
        axis='x',
        which='both',
        bottom=False,
        top=False,
        labelbottom=False)

    space.tick_params(
        axis='y',
        which='both',
        left=False,
        right=False,
        labelleft=False)

    space.invert_yaxis()

    space.set_xlabel("Space filled (%)")
    space.xaxis.set_label_position('top')

    colorbar = plt.colorbar(spaceplot, ax=space)
    colorbar.ax.invert_yaxis()
    '''

    fsfile = open("fserr"+str(k)+".txt","r")
    journal = fsfile.readline().rstrip().split(',')
    if len(journal) == 1:
        journal = []
    else:
        journal = [int(i) for i in journal[1:]]
    superblock = fsfile.readline().rstrip().split(',')
    if len(superblock) == 1:
        superblock = []
    else:
        superblock = [int(i) for i in superblock[1:]]
    directory = fsfile.readline().rstrip().split(',')
    if len(directory) == 1:
        directory = []
    else:
        directory = [int(i) for i in directory[1:]]

    fs.set_ylim([0,n_rounds])

    j_line = None
    for i in journal:
        j_line = fs.axhline(i, linewidth=2.0, color="blue", label="Corrupted\njournal")
    s_line = None
    for i in superblock:
        s_line = fs.axhline(i, linewidth=2.0, color="red", label="Corrupted\nsuperblock")
    d_line = None
    for i in directory:
        d_line = fs.axhline(i, linewidth=2.0, color="green", label="Corrupted\ndirectory")

    fs.tick_params(
        axis='x',
        which='both',
        bottom=False,
        top=False,
        labelbottom=False)

    fs.tick_params(
        axis='y',
        which='both',
        left=False,
        right=False,
        labelleft=False)

    fs.invert_yaxis()

    fs.legend(prop={'size': 10})

    fs.set_xlabel("Filesystem events", fontsize=12)
    fs.xaxis.set_label_position('top')

    #fig.suptitle("20 * 1MB decoy files per round")

    fig.tight_layout()
    fig.savefig("graphs/test"+str(k)+".png")
