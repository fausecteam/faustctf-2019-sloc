from ctf_gameserver.checker import *

import socket
import os
from .lorem import *

MAX_FAKES = 1 # largest id of one fake program in fake/

PLACING_FAIL = "PlacingFail"

class SlocChecker(BaseChecker):
	def __init__(self, tick, team, service, ip):
		BaseChecker.__init__(self, tick, team, service, ip)
		self.port = 64646

	def place_flag(self):
		self.logger.debug("place_flag called for tick %d" % self.tick)
		flag = self.get_flag(self.tick)
		prog = "\n".join(["CALL WRITECHAR %d" % ord(x) for x in flag])
		r = self._query(1, str(len(prog)) + "\n" + prog)
		if "SUCC:" not in r or flag not in r:
			self.logger.warn("place_fail: Unexpected answer while placing flag: " + r)
			self.store_yaml(str(self.tick), PLACING_FAIL)
			return NOTWORKING
		cs = "Submission stored with id "
		c1 = r.find(cs) + len(cs)
		cs2 = " and pwd "
		c2 = r.find(cs2, c1)
		idstr = r[c1:c2]
		pwd = r[c2+len(cs2):-1]
		self.logger.info("Flag for tick %d has id %s with pwd %s" % (self.tick, idstr, pwd))
		self.store_yaml(str(self.tick), {'id':idstr,'pwd':pwd})
		return OK

	def check_flag(self, tick):
		self.logger.debug("check_flag called for tick %d" % tick)
		d = self.retrieve_yaml(str(tick))
		if d == None:
			self.logger.error("critical_fail: No data in yamlstore for tick %d" % tick)
			return NOTFOUND
#			raise "No data in yaml store"
		if d == PLACING_FAIL:
			self.logger.warn("check_fail: There was no flag placed for tick %d" % tick)
			return NOTFOUND
		flag = self.get_flag(tick)
		#self.logger.info("Trying to retrieve flag %d with %s / %s" % (tick, d["id"], d["pwd"]))
		r = self._query(2, "%s %s" % (d["id"], d["pwd"]))
		if r != "SUCC: " + flag + "\n":
			self.logger.warn("check_fail: Did not find flag %s in %s when using %s %s" % (flag, r, d["id"], d["pwd"]))
			self.logger.warn("check_fail: Listing all flags: " + self._query(3, ""))
			return NOTFOUND
		# check if listing includes this flag as well
		r = self._query(3, "", decode = False)
#		if " " + d["id"] + " " not in r:
#		if not r.find(" " + d["id"] + " "):
		if (" " + d["id"] + " ").encode('ascii') not in r:
			self.logger.warn("check_fail: Id %s not found in list of ids for tick %d" % (d["id"], tick))
			return NOTWORKING
		return OK
	
	def check_service(self):
		self.logger.debug("check_service called for tick %d" % self.tick)
		tests = [
			# test some valid programs
			{"file": "strncmp.sl", "code": "SUCC", "expected": "Y"},
			{"file": "strncmp2.sl", "code": "SUCC", "expected": "N"},
			{"file": "fopenstr.sl", "code": "SUCC", "expected": "N"},
			# test some invalid programs
			{"file": "doubledeclare.sl", "code": "CERR", "expected": "Variable already declared"},
			{"file": "unknownfunc.sl", "code": "CERR", "expected": "function bar does not exist"},
			{"file": "forbiddenfunc.sl", "code": "CERR", "expected": "Experimental features disabled"}
		]
		for test in tests:
			p = open(os.path.join(os.path.dirname(__file__), test["file"])).read()
			cmd = str(len(p)) + "\n" + p
			r = self._query(1, cmd)
			if test["code"] == "SUCC":
				if test["expected"] + "\nSUCC: Submission stored" not in r:
					self.logger.warn("func_fail: Program: " + test["file"] + " gave unexpected result: " + r)
					return NOTWORKING
			else:
				if test["code"] + ": " + test["expected"] not in r:
					self.logger.warn("func_fail: Program: " + test["file"] + " gave unexpected result: " + r)
					return NOTWORKING
		# send random stuff
		if random.choice([True, False]):
			classes = [LoremSQL, LoremShell]
			global MAX_FAKES
			fileId = random.randint(0, MAX_FAKES)
			self.logger.debug("check_service: sending a random file %d" % fileId)
			p = open(os.path.join(os.path.dirname(__file__), "fake", "fake_%02d.sl" % fileId)).read()
			while "%C%" in p:
				rep = random.choice(classes).getComplete()
				p = p.replace("%C%", rep, 1)
			while "%S%" in p:
				rep = random.choice(classes).getSnippet()
				p = p.replace("%S%", rep, 1)
			cmd = str(len(p)) + "\n" + p + "\n"
			self._query(1, cmd)
		return OK
	
	def _query(self, num, query, decode = True):
		put = "%d %s\n" % (num, query)
		with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
			sock.settimeout(2)
			sock.connect((self._ip, self.port))
			sock.sendall(put.encode('ascii'))
			data1 = self._recvall(sock)
			if decode:
				data = data1.decode('ascii')
				lines = data.split("\n")
				result = "\n".join(lines[9:])
				return result
			else:
				return data1
	
	def _recvall(self, s):
		res = bytes()
		while len(res) < 50000:
			x = None
			try:
				x = s.recv(50000)
			except socket.timeout:
				pass
			if x == None or len(x) == 0:
				break
			res += x
		return res
