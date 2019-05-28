DECLARE $fd
DECLARE $str
CALL FOPEN "db.sqlite" "r" -> $fd
LABEL reading
	CALL READCHAR $fd -> $str
	IF $str E -1 GOTO end
	CALL WRITECHAR $str
	GOTO reading
LABEL end
