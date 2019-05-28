import random

class LoremSQL():
	complete = [
		"' 1=1 OR SELECT * FROM flags;",
		"' UNION SELECT '",
	]
	snippets = [
		"SELECT",
		"' 1=1",
		"' ;--",
		"UNION",
		"GROUP BY"
	]
	@staticmethod
	def getComplete():
		return random.choice(LoremSQL.complete)
	
	@staticmethod
	def getSnippet():
		return random.choice(LoremSQL.snippets)

class LoremShell():
	strs = [
		"/bin/sh",
		"cat flag",
		"echo ",
		"system(",
	]
	
	@staticmethod
	def getComplete():
		return random.choice(LoremShell.strs)
	
	@staticmethod
	def getSnippet():
		return random.choice(LoremShell.strs)

class LoremBinary():
	@staticmethod
	def _get():
		return "".join([random.choice("A", "a", "\x90")] * random.choice(5, 100))

	@staticmethod
	def getComplete():
		return LoremBinary._get()
	
	@staticmethod
	def getSnippet():
		return LoremBinary._get()
