import sys, shelve

if len(sys.argv) != 2:
    print >>sys.stderr, "usage: %s <label-mapping>" % sys.argv[0]
    sys.exit(1)

mapping = shelve.open(sys.argv[1])
labels = [x[0] for x in sorted(mapping.items(), key=lambda x: x[1])]
mapping.close()

print " ".join(labels)

in_model = False
for line in sys.stdin:
    if line.strip() == "w":
        in_model = True
    elif in_model:
        print line.strip()
