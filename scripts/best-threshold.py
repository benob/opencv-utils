import sys

if len(sys.argv) != 2:
    print >>sys.stderr, "usage: %s <label>" % sys.argv[0]
    sys.exit(1)

label = sys.argv[1]
examples = []

num_ref = 0.0
for line in sys.stdin:
    tokens = line.strip().split()
    if tokens[0] == label:
        examples.append([True, float(tokens[1])])
        num_ref += 1
    else:
        examples.append([False, float(tokens[1])])

examples.sort(key=lambda x: x[1])

num_hyp = len(examples)
num_correct = num_ref

max_f = 0
threshold = float("-inf")

for i in range(len(examples) - 1):
    num_hyp -= 1
    if examples[i][0] == True:
        num_correct -= 1
    p = num_correct / num_hyp
    r = num_correct / num_ref
    f = 2 * p * r / (p + r)
    #print examples[i][1], f, p, r
    if f > max_f:
        max_f = f
        threshold = (examples[i][1] + examples[i + 1][1]) / 2

print "threshold: %f f-score: %f" % (threshold, max_f)
