mytmp=/tmp/tmp.fd7Uft1ETU
PYTHONPATH=..
for i in {0..10}; do
	~/i2/CTF/faust/ctf-gameserver/scripts/checker/ctf-testrunner --first 1555684986 --backend $mytmp --tick $i --ip vulnbox-test.faust.ninja --team 1 --service 1 sloc:SlocChecker 2>&1 | grep -v OK | grep -v DEBUG
done
