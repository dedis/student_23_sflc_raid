import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap
import itertools
import matplotlib.ticker as mtick

n_decoy_files = 25
size_decoy_files = 1

n_rounds = 30

n_tests = 5

total_slices = 890

shorted = False

for k in range(1, n_tests + 1):
    resultsfile = open("results" + str(k) + ".txt", "r")

    decoy_baseline = eval(resultsfile.readline().rstrip())
    hidden_baseline = eval(resultsfile.readline().rstrip())

    resultsfile.readline() # consume \n

    decoy_after_hidden = eval(resultsfile.readline().rstrip())
    hidden_after_hidden = eval(resultsfile.readline().rstrip())

    if decoy_after_hidden != decoy_baseline:
        print("decoy baseline changed")

    hidden_files = [i for i in hidden_after_hidden if i not in hidden_baseline]

    resultsfile.readline() # consume \n

    decoy_results = []
    hidden_results = []
    for i in range(n_rounds):
        decoy = [i for i in eval(resultsfile.readline().rstrip()) if i not in decoy_baseline]
        if len(decoy) != 0:
            decoy_results.append(decoy)
        hidden = [i for i in eval(resultsfile.readline().rstrip()) if i not in hidden_baseline and i not in hidden_files]
        if len(hidden) != 0:
            hidden_results.append(hidden)
        resultsfile.readline()  # consume \n

    resultsfile.close()

    # print('decoy baseline', decoy_baseline)
    # print('hidden baseline', hidden_baseline)
    #
    # print('hidden files', hidden_files)
    #
    # print('decoy results', decoy_results)
    # print('hidden results', hidden_results)

    encoded = []
    fill = []

    start = []
    for i in hidden_baseline:
        start.append(0.2)
    for i in hidden_files:
        start.append(0.4)
    if not shorted:
        for i in decoy_baseline:
             start.append(1.0)

    encoded.append(start)
    fill.append(100*(len(hidden_baseline)+len(hidden_files)+len(decoy_baseline))/total_slices)

    for i in range(n_rounds):
        current = []
        results = decoy_results[i]
        for i in hidden_baseline:
            if i not in results:
                current.append(0.2)
            else:
                results.remove(i)
                current.append(0.6)
        for i in hidden_files:
            if i not in results:
                current.append(0.4)
            else:
                results.remove(i)
                current.append(0.6)
        if not shorted:
            for i in decoy_baseline:
                current.append(0.8)
            for i in results:
                current.append(0.6)
        encoded.append(current)
        fill.append(100*(len(hidden_baseline)+len(hidden_files)+len(decoy_baseline)+len(results))/total_slices)

    pad_token = 0.0
    padded = zip(*itertools.zip_longest(*encoded, fillvalue=pad_token))
    encoded = list(padded)

    fig, ax1 = plt.subplots(figsize=(15, 5))
    if shorted:
        fig, ax1 = plt.subplots(figsize=(8, 5))

    nodes = [0.0, 0.2, 0.4, 0.6, 0.8]
    colors = ['white', 'green', 'red', 'blue', 'yellow']
    if shorted:
        nodes = [0.0, 0.2, 0.4, 0.6]
        colors = ['green', 'green', 'red', 'blue']
    cmap1 = ListedColormap(colors, name="cmap1")

    ax1.pcolormesh(encoded, cmap=cmap1)

    ax1.set_xlabel("Slices", fontsize=12)
    ax1.xaxis.set_label_position('top')
    ax1.set_ylabel("Rounds", fontsize=12)

    ax1.invert_yaxis()
    ax1.xaxis.tick_top()

    ax1.tick_params(labelsize=12)

    ax2 = ax1.twinx()
    ax2.plot(fill, linestyle='None')

    ax2.tick_params(labelsize=12)

    ax2.set_ylabel("Percentage of space filled", fontsize=12)
    #ax2.yaxis.set_major_formatter(mtick.PercentFormatter())
    ax2.invert_yaxis()

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

    fs.legend(prop={'size': 7})

    fs.set_xlabel("Filesystem events")
    fs.xaxis.set_label_position('top')

    if not shorted :
        fig.suptitle("100 * 1MB hidden files, "+str(n_decoy_files)+" * "+str(size_decoy_files)+"MB decoy files per round")
    else:
        fig.suptitle("100 * 1MB hidden files, "+str(n_decoy_files)+" * "+str(size_decoy_files)+"MB decoy files per round")
    '''

    fig.tight_layout()
    fig.savefig("graphs/test"+str(k)+".png")
