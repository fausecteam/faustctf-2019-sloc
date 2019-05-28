import subprocess
import os

tests = [
	{"file": "strncmp.sl", "code": "SUCC", "expected": "Y"},
	{"file": "strncmp2.sl", "code": "SUCC", "expected": "N"},
	# test some invalid programs
	{"file": "doubledeclare.sl", "code": "CERR", "expected": "Variable already declared"},
	{"file": "unknownfunc.sl", "code": "CERR", "expected": "function bar does not exist"},
	{"file": "forbiddenfunc.sl", "code": "CERR", "expected": "Experimental features disabled"}
]

for test in tests:
	print("Running test " + test["file"])
	try:
		r = subprocess.check_output(["./Compiler", "checker/sloc/" + test["file"], "asmFile.s"])
		r = r.decode('ascii')
		assert test["code"] == "SUCC"
		assert r == "", "Output: " + r
		r = subprocess.check_output(["g++", "-no-pie", "asmFile.s", "-o", "binFile"])
		r = r.decode('ascii')
		assert r == "", r
		r = subprocess.check_output(["./binFile"])
		r = r.decode('ascii')
		assert r == test["expected"], r
	except subprocess.CalledProcessError as e:
		r = e.output.decode('ascii')
		assert test["code"] == "CERR", "Compilation failed unexpectedly with: " + r
		assert r == test["expected"], "Expected: " + test["expected"] + ", but got: " + r

os.unlink("binFile")
os.unlink("asmFile.s")
