# -*- coding: utf-8
import json, sys

images = {}
for line in open('shots.list'):
    name = line.strip().split('/')[-1]
    images[name] = line.strip()

data = {}

def protect(text):
    text = text.replace('&', '&amp;')
    text = text.replace('<', '&lt;')
    text = text.replace('>', '&gt;')
    text = text.replace('"', '&quot;')
    return text

from collections import defaultdict
data = defaultdict(list)

fp = open(sys.argv[1], 'r')
annotations = json.loads(fp.read())
for annotation in annotations:
    name = annotation['name']
    split = annotation['split']
    data[split].append(name)

from subprocess import call as system
system('mkdir -p subshot-annotator', shell=True)
print('<html><head><meta http-equiv="Content-Type" content="text/html; charset=utf-8"></head><body>')
sorted_splits = sorted(data.keys(), key=lambda x: -len(data[x]))
for split in sorted_splits:
    fp = open('subshot-annotator/%s.html' % split, 'w')
    print('<a href="subshot-annotator/%s.html"><img src="splits/%s.png"> %s</a><br>\n' % (split, split, split))

    fp.write('<html><head><meta http-equiv="Content-Type" content="text/html; charset=utf-8">'
            #+ '<script lang="javascript" src="../jquery-1.10.2.min.js"></script>'
            #+ '<script lang="javascript" src="../filter.js"></script>'
            #+ '<script lang="javascript" src="../subshot-annotator.js"></script>'
            + '</head><body>\n')
    fp.write('<h1><img src="../splits/%s.png"> %s</h1>\n' % (split, split))
    fp.write('<hr>')
    for name in sorted(data[split]):
        fp.write('<a href="../subshot-annotator.html?split=%s&img=%s"><img src="../%s.192" width="192" height="108"></a>\n' % (split, images[name], images[name]))
    fp.write('</body></html>')
    fp.close()
print('</body></html>')
