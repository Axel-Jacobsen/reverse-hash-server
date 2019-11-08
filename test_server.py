#! /usr/bin/python

# USAGE: ./test_server.py
# Put this file in your client ssh. Run your server in your server ssh.
# Run this code and read the outputs
# Change N to change the number of trials, and change the command to 
# control what client test you run (e.g. "run-client-final.sh")

import subprocess

def process_std_out(s):
	latencies = []
	for line in s:
		if line.startswith('['):
			latency = line.split(']')[-1].strip()
			latencies.append(latency)
		if line.startswith('Results'):
			score = line.split(' ')[-1]
	return score, latencies

if __name__ == '__main__':
	N = 2
	avg_score = 0
	ls = []
	command = "run-client-milestone.sh"
	for _ in range(N):
		print 'starting run %d' % _
		s = subprocess.check_output([command])
		score, latencies = process_std_out(s)
		avg_score += N
		print '  score  %d' % score

	print 'avg score: %d' % avg_score
