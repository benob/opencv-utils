import sys

if len(sys.argv) != 2:
    print >>sys.stderr, 'usage: %s <showname>' % sys.argv[0]

show = sys.argv[1]

for line in sys.stdin:
    startframe, endframe, frame, starttime, endtime, time, score = line.strip().split()
    print show, starttime, endtime, 'shot', line.strip()
