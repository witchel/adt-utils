#!/usr/local/bin/python3
import sys

lines = [line.split() for line in open(sys.argv[1])]

allocs = {pointer:0 for action, *_, pointer in lines}
frees = {pointer:0 for action, *_, pointer in lines}

for (action, *_, pointer) in lines:
	if action in ('malloc\'d', 'calloc\'d'):
		allocs[pointer] += 1;
	elif action == 'freed':
		frees[pointer] += 1
	else:
		raise Exception("Error on line:" + str((action, _, pointer)))

for pointer in allocs:
	alloc_count = allocs[pointer]
	free_count = frees[pointer]
	if alloc_count != free_count:
		print("Alloc/free mismatch for {}: {} allocs and {} frees.".format(pointer, alloc_count, free_count))