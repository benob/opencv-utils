# -*- coding: utf-8
import json, sys

allowed = {
        'split': set(['1-full', '2-big-left', '2-horizontal', '3-big-left', '3-even', '4-big-right', '2-big-right', '2-vertical', '3-big-right', '4-big-left', '4-even', '1-other']),
        'role': set(['Presentateur', 'Journaliste', 'Invité/inteviewé', 'Personnesd\'intêret', 'Figurants', 'Autre']),
        'pose': set(['Face', 'Profil->', '<-Profil', 'Dos', 'Autre']),
        'location': set(['Centre', 'Gauche', 'Droite', 'Autre']),
        'type': set(['Plateau', 'Plateau(reportageincrusté)', 'Plateau(webcam)', 'Plateau(autreemission)', 'Reportage', 'Reportage(foule)', 'Photo', 'Infographie', 'Jingle', 'Autre'])
    }

def verify(annotation):
    if annotation['split'] not in allowed['split']:
        return False
    for subshot in annotation['subshots']:
        if subshot['type'] not in allowed['type']:
            return False
        for person in subshot['persons']:
            if person['role'] not in allowed['role']:
                return False
            if person['location'] not in allowed['location']:
                return False
            if person['pose'] not in allowed['pose']:
                return False
    return True

def trim_annotation(annotation):
    for subshot in annotation['subshots']:
        for person in subshot['persons']:
            if person['role'] in ["Personnesd'intêret", "Figurants"]:
                person['pose'] = None
                person['location'] = None
    del annotation['annotator']
    del annotation['date']
    del annotation['label']

shots = {}

for filename in sys.argv[1:]:
    fp = open(filename, 'r')
    sys.stderr.write(filename + ' ')
    annotations = json.loads(fp.read())
    sys.stderr.write(str(len(annotations)) + '\n')
    for annotation in annotations:
        name = annotation['name']
        if name not in shots or shots[name]['date'] < annotation['date']:
            if not verify(annotation):
                sys.stderr.write('ERROR: non conformant: ' + name +'\n' + json.dumps(annotation) + '\n')
            else:
                shots[name] = annotation

for annotation in shots.values():
    trim_annotation(annotation)

sys.stdout.write(json.dumps(sorted(shots.values(), key=lambda x: x['name']), sort_keys=True, indent=4,))

