#! /usr/bin/python

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
	for _ in range(N):
		print 'starting run %d' % _
		s = subprocess.check_output(["run-client-milestone.sh"])
		score, latencies = process_std_out(s)
		avg_score += N
		print '  score  %d' % score

	print 'avg score: %d' % avg_score
