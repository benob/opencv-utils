import json, sys
from collections import defaultdict

images = defaultdict(set)

for line in open('shots.list'):
    name = line.strip().split('/')[-1]
    show = name.split('.')[0]
    images[show].add(name)

segments = defaultdict(list)

for line in open('../../../data/anais/all-segments'):
    line = line.strip()
    if line.endswith('.png'):
        show = '_'.join(line.split('.')[:-2])
        segments[show].append(int(line[:-4].split('_')[-1]))

data = defaultdict(set)

for filename in sys.argv[1:]:
    fp = open(filename, 'r')
    annotations = json.loads(fp.read())
    for annotation in annotations:
        name = annotation['name']
        show = name.split('.')[0]
        data[show].add(name)

unlabeled = {}
for show in sorted(images):
    num = 0
    labeled = 0
    for name in images[show]:
        image = int(name[:-4].split('_')[-1])
        keep = True
        if show in segments:
            keep = False
            for i in range(0, len(segments[show]), 2):
                if image >= segments[show][i] and image <= segments[show][i + 1]:
                    keep = True
                    break
        if keep:
            num += 1
            if name in data[show]:
                labeled += 1
            else:
                print('   ' + name)

    if num != labeled:
        print(show, num - labeled, '/', num)

