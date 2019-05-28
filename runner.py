#!/usr/bin/python3 -u

import glob
import os
import time
import re
import tempfile
import subprocess
import sys
import stat
import string
import traceback
import base64

print("Welcome to SLOC - your Simple Language Online Compiler")
print("")
print("What do you want to do?")
print("1 <size>\\n<code (size bytes)  Compiles and runs your program")
print("                              Prints the result and gives you an (id, pwd)")
print("                              to get your out again later")
print("2 <id> <pwd>                  Shows to result of the given run again")
print("3                             Lists all recent run ids")
print("")

THRESH_MINS = 30
THRESH_TIME = 60 * THRESH_MINS
MAX_FILESIZE = 1000000

def validId(v):
	return len(v) > 0 and re.search("^[a-zA-Z0-9_]+$", v)

def validPwd(v):
	return len(v) > 0 and re.search("^[a-zA-Z0-9_]+$", v)

def validCode(v):
	for c in v:
		if ord(c) < ord(' ') and not ord(c) in [9, 10, 13]:
			return False
		if ord(c) > ord('~'):
			return False
	return True

def rndString():
	return base64.b32encode(os.urandom(10)).decode('ascii')

PWD_LEN = len(rndString())
	
def store(c):
	suff = rndString()
	fd, fileOut = tempfile.mkstemp(dir="runs", suffix=suff)
	os.close(fd)
	pwd = rndString()
	with open(fileOut, "wb") as outf:
		outf.write(pwd.encode('ascii'))
		outf.write(c.encode('ascii'))
	e = fileOut.rfind("/")
	idstr = fileOut[e+1:]
	print("SUCC: Submission stored with id %s and pwd %s" % (idstr, pwd))

def handleGet(idstr, pwd):
	 if not validId(idstr) or not validPwd(pwd):
	 	print("FAIL: Invalid ID or PWD")
	 	return
	 try:
	 	content = open("runs/" + idstr, "rb").read()
	 	if len(content) < PWD_LEN:
	 		print("FAIL: Not a valid run")
	 		return
	 	readPwd = content[:PWD_LEN].decode('ascii')
	 	if readPwd != pwd:
	 		print("FAIL: Invalid password")
	 		return
	 	print("SUCC: " + content[PWD_LEN:].decode('ascii'))
	 except FileNotFoundError as e:
	 	print("FAIL: This result does not exist")
	 except Exception as e:
	 	print("FAIL: Something went wrong")

def handleCompile(size):
	try:
		sz = int(size)
	except Exception as e:
		print("Invalid size: '" + size + "'")
		return
	if sz > MAX_FILESIZE:
		print("Size too large")
		return
	code = sys.stdin.read(sz)
	if not validCode(code):
		print("Code invalid")
		return
	try:
		fd, fileSrc = tempfile.mkstemp(dir="tmps")
		os.close(fd)
		fd, fileAsm = tempfile.mkstemp(dir="tmps",suffix=".s")
		os.close(fd)
		with open(fileSrc, "w") as outf:
			outf.write(code)
		#### Compile ####
		try:
			erg = subprocess.check_output(["./Compiler", fileSrc, fileAsm])
			#### Assemble ####
			fd, fileBin = tempfile.mkstemp(dir="tmps")
			os.close(fd)
			try:
				erg = subprocess.check_output(["g++", fileAsm, "-o", fileBin])
				#### Now Run ####
				try:
					res = ""
					with subprocess.Popen(["timeout", "1", fileBin], stdout=subprocess.PIPE, bufsize=1) as p:
						for l in p.stdout:
							line = l.strip().decode('ascii')
							print(line)
							res += line
					store(res)
				except subprocess.CalledProcessError as e:
					print("EERR: Your program failed")
					return
				except subprocess.TimeoutExpired:
					print("EERR: Timeout")
					return
				finally:
					os.unlink(fileBin)

			except subprocess.CalledProcessError as e:
				print("FAIL: Something went wrong")
				return
		except subprocess.CalledProcessError as e:
			x = "CERR: " + e.output.decode('ascii')
			print(x)
			return
		finally:
			os.unlink(fileSrc)
			os.unlink(fileAsm)
			
	except Exception as f:
		print("Something went wrong: " + str(f))
		traceback.print_exc()

query = sys.stdin.readline()
query = query.strip()
if len(query) == 0:
	print("FAIL: Need a query")
else:
	if query[0] == "1":
		handleCompile(query[2:])
	elif query[0] == "2":
		s = query.split(" ")
		if len(s) != 3 or s[0] != "2":
			print("FAIL: Invalid usage")
		else:
			handleGet(s[1], s[2])
	elif query[0] == "3":
		files = glob.glob('runs/*')
		res = []
		for f in files:
			try:
				ctime = os.path.getmtime(f)
				if time.time() - ctime <= THRESH_TIME:
					res += [f[5:]]
			except:
				pass
		print("SUCC: Runs in the last %d min: %s " % (THRESH_MINS, " ".join(res)))
	else:
		print("FAIL: Command " + query[0] + " not found")
