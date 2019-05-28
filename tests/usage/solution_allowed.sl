# declare variables
DECLARE $fd
DECLARE $str

# strncmp is needed, because FOPEN only allows 1 argument ("bug")
# with this trick we get the correct register filled with "r"
CALL STRNCMP "something" "r" 10

# open the database
# FOPFM has the same hash like FOPEN
CALL AABKB "db.sqlite" -> $fd

# read char by char and echo to stdout
LABEL reading

# READCHBQ has the same hash like READCHAR
	CALL AAAMJ $fd -> $str
	
# -1 signals EOF
	IF $str E -1 GOTO end
	
	CALL WRITECHAR $str
	GOTO reading
LABEL end
