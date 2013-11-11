# -*- coding: utf-8
import json, sys

images = {}
for line in open('shots.list.dev2'):
    name = line.strip().split('/')[-1]
    images[name] = line.strip()

data = {}

def protect(text):
    text = text.replace('&', '&amp;')
    text = text.replace('<', '&lt;')
    text = text.replace('>', '&gt;')
    text = text.replace('"', '&quot;')
    return text

def make_label(annotation):
    subshots = []
    for subshot in annotation['subshots']:
        persons = []
        for person in subshot['persons']:
            if person['role'] in ["Personnesd'intêret", "Figurants"]:
                persons.append(person['role'])
            else:
                persons.append('+'.join([person[x] for x in ['role', 'pose', 'location']]))
        subshots.append(subshot['type'] + ':' + ','.join(persons))
    return annotation['split'] + '=' + '|'.join(subshots)

from collections import defaultdict

labels = defaultdict(lambda: defaultdict(list))
label_as_annotation = {}

for filename in sys.argv[1:]:
    sys.stderr.write(filename + ' ')
    fp = open(filename, 'r')
    annotations = json.loads(fp.read())
    sys.stderr.write(str(len(annotations)) + '\n')
    for annotation in annotations:
        name = annotation['name']
        del annotation['label']
        #print(annotation)
        if name not in data or data[name]['date'] < annotation['date']:
            #if name == 'LCP_TopQuestions_2011-07-13_213800.MPG_0000012391.png':
            #    sys.stderr.write(str(annotation) + '\n')
            data[name] = annotation
        else:
            #print('skipping', name, annotation['date'], data[name]['date'])
            pass
for name in data:
    label = make_label(data[name])
    if label not in label_as_annotation:
        label_as_annotation[label] = data[name]
    show = '_'.join(name.split('.')[0].split('_')[:-2])
    labels[label][show].append(name)

def label_to_html(annotation):
    output = annotation['name'] + '<br>'
    output += '<img src="../splits/%s.png">' % annotation['split'] + '<ol>'
    for subshot in annotation['subshots']:
        output += '<li>' + subshot['type'] + '<ul>'
        for person in subshot['persons']:
            output += '<li>' + person['role'] + ' ' + person['location'] + ' ' + person['pose'] + '</li>'
        output += '</ul></li>'
    output += '</ol>'
    return output

unlabeled = []
for name in sorted(images.keys()):
    if name not in data:
        unlabeled.append(name)

from subprocess import call as system

system('mkdir -p viewer.dev2', shell=True)
print('<html><head><meta http-equiv="Content-Type" content="text/html; charset=utf-8"></head><body>')
label_id = 0
sorted_labels = sorted(labels.keys(), key=lambda x: -sum([len(y) for y in labels[x]]))
for label in sorted_labels:
    fp = open('viewer.dev2/%d.html' % label_id, 'w')
    print('<a href="viewer.dev2/%d.html">%s</a><br>\n' % (label_id, protect(label)))
    label_id += 1
    fp.write('<html><head><meta http-equiv="Content-Type" content="text/html; charset=utf-8"></head><body>\n')
    fp.write('<h1>' + protect(label) + '</h1>\n')
    fp.write(label_to_html(label_as_annotation[label]))
    for show in sorted(labels[label]):
        fp.write('<hr><h3>' + show + '</h3>\n')
        for name in labels[label][show]:
            fp.write('<a href="%s" target="annotator"><img src="%s" width="192" height="108" title="%s"></a>\n' % ('../' + '/'.join(images[name].split('/')[:-3]) + '/index.html?' + name, 'http://talep.lif.univ-mrs.fr/percol/' + images[name] + '.192', name))
    fp.write('</body></html>')
    fp.close()
print('</body></html>')
