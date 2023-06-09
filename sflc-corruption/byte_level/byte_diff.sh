target1="byte_corruption.sh"
target2="verify_hidden.sh"

count1=$(wc -c < $target1)
count2=$(wc -c < $target2)

initial=$(( count1-count2 ))

echo $initial

compared=$(cmp --verbose $target1 $target2 | wc -l)

echo $compared
