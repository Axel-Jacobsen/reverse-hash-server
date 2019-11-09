#! /usr/bin/python

# USAGE: ./test_server.py
# Put this file in your client ssh. Run your server in your server ssh.
# Run this code and read the outputs
# Change N to change the number of trials, and change the command to 
# control what client test you run (e.g. "run-client-final.sh")

import subprocess

def process_std_out(s):
	pls = []
	import time

	line = ''

	for c in s:
		if c == '\n':
			if line.startswith('['):
				bracket_split = line.split(']')
				priority = int(bracket_split[0].split(' ')[-1])
				latency  = int(bracket_split[-1].strip())
				pls.append((priority, latency))

			if line.startswith('Results'):
				score = int(line.split(' ')[-1].strip())

			line = ''

		else:
			line += c

	return (score, pls)

if __name__ == '__main__':
	N = 2
	avg_score = 0

	scores = []
	priority_latency_ls = []

	for _ in range(N):
		print 'starting run %d' % _
		commands = ["./run-client.sh"]
		s = subprocess.check_output(commands)
		score, pls = process_std_out(s)

		scores.append(score)
		priority_latency_ls.append(pls)

		avg_score += score
		print '\tscore  %d\n' % score

	avg_score /= N

	with open('res.txt', 'w') as f:
		f.write('average score: %d\n' % avg_score)
		for i in xrange(len(scores)):
			score = scores[i]
			pls = priority_latency_ls[i][0]

			f.write('run %d score %d\n' % (i, score))
			for pl in pls:
				f.write('\tpriority (p): %d\tlatency (l): %d\tp*l: %d\n' % (pls[0], pls[1], pls[0] * pls[1]))
			f.write('\n')

	print 'avg score: %d' % avg_score
