    # Taken from https://projecteuler.net/problem=92
#8581146

000 LET X = 2
010 LET C = 0
020 GOTO 060
030 LET C = C + 1
040 LET X = X + 1
050 IF X = 10000000 THEN GOTO 160
060 LET Z = X
070 LET S = 0
080 LET Y = Z
090 LET Z = Y / 10
100 LET S = S + (Y - Z * 10) * (Y - Z * 10)
110 IF Z > 0 THEN GOTO 080
120 IF S = 89 THEN GOTO 030
130 IF S = 1 THEN GOTO 040
140 LET Z = S
150 GOTO 070
160 PRINT C
