import sys, re, os

if len(sys.argv) != 2:
    sys.stderr.write('usage: %s <mapping-file>\n' % sys.argv[0])
    sys.exit(1)

mapping_file = sys.argv[1]
mapping = {}

if not os.path.exists(mapping_file):
    sys.stderr.write('usage: %s <mapping-file>\n' % sys.argv[0])
    sys.exit(1)

with open(mapping_file) as fp:
    for line in fp:
        index, label = line.strip().split()
        mapping[index] = label

for line in sys.stdin:
    found = re.search(r'^\S+', line).group(0)
    if found not in mapping:
        sys.stderr.write('error: no mapping found for "%s"\n' + found)
        sys.exit(1)
    sys.stdout.write(mapping[found] + line[len(found):])
