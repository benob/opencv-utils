import sys
from collections import defaultdict

keep = False
b = 0
weights = defaultdict(float)

for line in sys.stdin:
    if line.find('# threshold b') != -1:
        b = float(line.split()[0])
        keep = True
    elif keep == True:
        tokens = line.strip().split()
        factor = float(tokens[0])
        for token in tokens[1:-1]:
            index, value = token.split(":")
            weights[int(index) - 1] += float(value) * factor

print "w"
for index in range(len(weights)):
    print weights[index]
print -b
