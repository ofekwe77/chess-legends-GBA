"""Turns the balance study logs (docs/balance_logs/*.txt, written by tools/balance/run_experiments.sh) into the
Markdown tables of docs/BALANCE_REPORT.md. Usage: py -3.12 tools/balance/summarize.py > build_host/balance_tables.md
Every number comes from the logs; win shares carry their standard error (binomial, decisive and drawn games).
"""
import glob
import math
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
LOGS = os.path.join(ROOT, 'docs', 'balance_logs')
ABILITIES = ['ss', 'ps', 'gw', 'cs', 'rc', 're']


def parse(path):
    text = open(path, encoding='utf-8').read()
    data = {'name': os.path.splitext(os.path.basename(path))[0]}
    data['command'] = text.splitlines()[0].lstrip('# ').strip()
    m = re.search(r'White wins (\d+)\s+Black wins (\d+)\s+Draws (\d+) \(of which ply-capped (\d+)\)', text)
    data['white'], data['black'], data['draws'], data['capped'] = map(int, m.groups())
    data['games'] = data['white'] + data['black'] + data['draws']
    m = re.search(r'Config A wins (\d+)\s+Config B wins (\d+)', text)
    data['a'], data['b'] = map(int, m.groups()) if m else (None, None)
    data['length'] = float(re.search(r'Average length ([\d.]+) plies', text).group(1))
    m = re.search(r'Searches (\d+) \(book moves (\d+)\), nodes (\d+), avg nodes (\d+), avg completed depth ([\d.]+)', text)
    data['avg_nodes'], data['depth'] = int(m.group(4)), float(m.group(5))
    reasons = re.search(r'Terminal reasons:(.*)', text).group(1)
    data['reasons'] = dict((k.strip(), int(v)) for k, v in re.findall(r'([A-Z0-9 \-]+)=(\d+)', reasons))
    uses = re.search(r'Ability uses \(white/black\):(.*)', text).group(1)
    data['uses'] = dict((k, int(v)) for k, v in re.findall(r'(\w\w)=(\d+) \(', uses))
    gains = re.search(r'Material balance change 4 plies after a cast[^:]*:(.*)', text).group(1)
    data['gains'] = dict((k, v) for k, v in re.findall(r'(\w\w)=([+\-]?\d+|n/a)', gains))
    m = re.search(r'Decisive leads \(3\+ pawns\): (\d+) games; converted (\d+), drawn (\d+), reversed (\d+)', text)
    data['leads'], data['converted'], data['leads_drawn'], data['reversed'] = map(int, m.groups())
    data['mana_stored'] = float(re.search(r'average stored mana at decision time ([\d.]+)', text).group(1))
    return data


def share(wins, games):
    p = wins / games
    return '%.1f%% (±%.1f)' % (100 * p, 100 * math.sqrt(p * (1 - p) / games))


def main():
    sys.stdout.reconfigure(encoding='utf-8')
    runs ={d['name']: d for d in (parse(p) for p in sorted(glob.glob(os.path.join(LOGS, '*.txt'))))}
    total = sum(d['games'] for d in runs.values())
    print('Games simulated in total: %d (%d experiments)\n' % (total, len(runs)))

    print('| Experiment | Games | White / Black / Draw | Config A / B | Plies | Checkmates | Draws |')
    print('|---|---|---|---|---|---|---|')
    for name, d in runs.items():
        mates = d['reasons'].get('CHECKMATE', 0)
        ab = '%d / %d' % (d['a'], d['b']) if d['a'] is not None else '-'
        print('| %s | %d | %d / %d / %d | %s | %.1f | %s | %s |' % (
            name, d['games'], d['white'], d['black'], d['draws'], ab, d['length'], share(mates, d['games']),
            share(d['draws'], d['games'])))

    print('\n| Experiment | Config A score | ss | ps | gw | cs | rc | re | Stored mana |')
    print('|---|---|---|---|---|---|---|---|---|')
    for name, d in runs.items():
        score = (d['a'] + 0.5 * d['draws']) / d['games'] if d['a'] is not None else None
        uses = ' | '.join('%d' % d['uses'].get(a, 0) for a in ABILITIES)
        print('| %s | %s | %s | %.2f |' % (name, '%.1f%%' % (100 * score) if score is not None else '-', uses,
                                          d['mana_stored']))

    print('\n| Experiment | Material change 4 plies after a cast (cp): ss / ps / gw / cs / rc / re | Decisive leads: '
          'converted / drawn / reversed |')
    print('|---|---|---|')
    for name, d in runs.items():
        gains = ' / '.join(d['gains'].get(a, 'n/a') for a in ABILITIES)
        print('| %s | %s | %d of %d / %d / %d |' % (name, gains, d['converted'], d['leads'], d['leads_drawn'],
                                                   d['reversed']))

    print('\nCommands:\n')
    for name, d in runs.items():
        print('- `%s`: `%s`' % (name, d['command']))


if __name__ == '__main__':
    main()
