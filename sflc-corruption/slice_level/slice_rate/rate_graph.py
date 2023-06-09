import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap
import itertools
import matplotlib.ticker as mtick

n_hidden_files = 100
size_hidden_files = 1
metric_hidden_files = "M"

n_decoy_files = 25
size_decoy_files = 300
metric_decoy_files = "K"

n_rounds = 100

folder = "backup_"+str(size_decoy_files)+metric_decoy_files+"B/"

n_tests = 30

total_space = 1024
advertised_space = 954
size_slice = 1
total_slices = 890

total_expected = 0
total_actual = 0
largest_pos_deviation = 0
largest_neg_deviation = 0

expected_list = [0]*n_rounds
actual_list = [0]*n_rounds

CDF = True

for k in range(1, n_tests + 1):
    resultsfile = open(folder+"results" + str(k) + ".txt", "r")

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

    # print('hidden baseline', hidden_baseline)
    # print('hidden files', hidden_files)
    # print('decoy results', decoy_results)

    advertised_ratio = advertised_space/total_space
    print("total to advertised", total_space, "->", advertised_space, "=>", advertised_ratio)
    sflc_overhead = total_slices*size_slice/advertised_space
    print("size slice", size_slice, "slices", total_slices, "after overhead", sflc_overhead)

    decoy_slices = decoy_baseline
    print("decoy slices", len(decoy_slices), len(decoy_slices)/total_slices)

    hidden_slices = hidden_baseline+hidden_files
    print("hidden slices", len(hidden_slices), len(hidden_slices)/total_slices)

    print("usable slices", total_slices-len(decoy_slices)-len(hidden_slices), "/", total_slices)

    print("probability to pick hidden slice", len(hidden_slices)/(total_slices-len(decoy_slices)))

    test_expected = 0
    test_actual = 0
    for i in range(n_rounds):
        expected = 0
        actual = 0
        for j in decoy_results[i]:
            if j not in decoy_slices:
                expected += len(hidden_slices)/(total_slices-len(decoy_slices))
            if j in hidden_slices:
                if j not in decoy_slices:
                    hidden_slices.remove(j)
                    decoy_slices.append(j)
                    actual += 1
            else:
                if j not in decoy_slices:
                    decoy_slices.append(j)
        if expected > actual and expected-actual > largest_pos_deviation:
            largest_pos_deviation = expected-actual
        if expected < actual and expected-actual < largest_neg_deviation:
            largest_neg_deviation = expected-actual
        test_expected += expected
        test_actual += actual
        expected_list[i] += expected
        actual_list[i] += actual

    total_expected += test_expected / n_rounds
    total_actual += test_actual / n_rounds

total_expected /= n_tests
total_actual /= n_tests

expected_list = [i/n_tests for i in expected_list]
actual_list = [i/n_tests for i in actual_list]

if CDF:
    temp_total = 0
    for i in range(len(expected_list)):
        expected_list[i]+=temp_total
        temp_total+=expected_list[i]-temp_total
    temp_total = 0
    for i in range(len(actual_list)):
        actual_list[i]+=temp_total
        temp_total+=actual_list[i]-temp_total

print(expected_list)
print(actual_list)

print("expected average", total_expected)
print("actual average", total_actual)
print("largest positive deviation", largest_pos_deviation)
print("largest negative deviation", largest_neg_deviation)

deviation_range = largest_pos_deviation - largest_neg_deviation

fig,ax1 = plt.subplots()

ax1.set_xlabel("Rounds", fontsize=20)
ax1.set_ylabel("Corrupted slices", color="blue", fontsize=20)
if not CDF:
    ax1.set_ylim([0,deviation_range])
ax1.plot(actual_list)
ax1.tick_params(axis='y', labelcolor="blue")
ax1.tick_params(labelsize=20)

ax2 = ax1.twinx()

ax2.set_ylabel('Expected corrupted slices', color="orange", fontsize=20)
if not CDF:
    ax2.set_ylim([0,deviation_range])
ax2.plot(expected_list, color="orange")
ax2.tick_params(axis='y', labelcolor="orange", labelsize=20)

#fig.suptitle("100 * 1MB hidden files, " + str(n_decoy_files) + " * " + str(
#    size_decoy_files) + metric_decoy_files + "B decoy files per round")

fig.tight_layout()
plt.savefig("graphs/corruption_rate"+str(size_decoy_files)+metric_decoy_files+"B.png")

