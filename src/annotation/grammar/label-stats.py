import json, sys
from collections import defaultdict

annotations = json.loads(sys.stdin.read())

stats = defaultdict(lambda: defaultdict(float))

for annotation in annotations:
    show = '_'.join(annotation['name'].split('_')[:2])
    stats['show'][show] += 1
    stats['shots']['num'] += 1
    stats['split'][annotation['split']] += 1
    for subshot in annotation['subshots']:
        stats['subshots']['num'] += 1
        stats['subshots'][subshot['type']] += 1
        stats['persons']['num'] += len(subshot['persons'])
        for person in subshot['persons']:
            stats['role'][person['role']] += 1
            if person['pose'] != None:
                stats['pose'][person['pose']] += 1
                stats['pose']['all'] += 1
            if person['location'] != None:
                stats['location'][person['location']] += 1
                stats['location']['all'] += 1

for show in sorted(stats['show'], key=lambda x: -stats['show'][x]):
    print('%s: %.2f%%' % (show, 100.0 * stats['show'][show] / stats['shots']['num']))
print()

for split in sorted(stats['split'], key=lambda x: -stats['split'][x]):
    print('%s: %.2f%%' % (split, 100.0 * stats['split'][split] / stats['shots']['num']))
print()

print('avg num subshots: %.4f' % (stats['subshots']['num'] / stats['shots']['num']))
print('avg num persons: %.4f' % (stats['persons']['num'] / stats['shots']['num']))
print()

for subshot in sorted(stats['subshots'], key=lambda x: -stats['subshots'][x]):
    if subshot != 'num':
        print('%s: %.2f%%' % (subshot, 100.0 * stats['subshots'][subshot] / stats['shots']['num']))
print()

for role in sorted(stats['role'], key=lambda x: -stats['role'][x]):
    print('%s: %.2f%%' % (role, 100.0 * stats['role'][role] / stats['persons']['num']))
print()

for pose in sorted(stats['pose'], key=lambda x: -stats['pose'][x]):
    if pose != 'all':
        print('%s: %.2f%%' % (pose, 100.0 * stats['pose'][pose] / stats['pose']['all']))
print()

for location in sorted(stats['location'], key=lambda x: -stats['location'][x]):
    if location != 'all':
        print('%s: %.2f%%' % (location, 100.0 * stats['location'][location] / stats['location']['all']))
print()
