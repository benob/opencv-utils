import sys, re, os

if len(sys.argv) != 2:
    sys.stderr.write('usage: %s <mapping-file> (created if not found)\n' % sys.argv[0])
    sys.exit(1)

mapping_file = sys.argv[1]
mapping = {}

if os.path.exists(mapping_file):
    with open(mapping_file) as fp:
        for line in fp:
            index, label = line.strip().split()
            mapping[label] = index

for line in sys.stdin:
    found = re.search(r'^\S+', line).group(0)
    if found not in mapping:
        mapping[found] = str(len(mapping))
    sys.stdout.write(mapping[found] + line[len(found):])

with open(mapping_file, 'w') as fp:
    for label, index in sorted(mapping.items(), key=lambda x: x[1]):
        fp.write(index + ' ' + label + '\n')
