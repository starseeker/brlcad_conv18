#!/bin/sh

BIN=$1/bin
export BIN

rm -f dsp.dat ebm.bw solids solids.g solids.log solids.pix

$BIN/asc2pix > dsp.dat << EOF
01BF01
A101A7
01C001
D901D4
01C501
CA01A8
019101
AE01BC
01B401
A90196
018D01
880183
018501
660152
017201
9601C2
01B601
AA01B6
01A701
9B01A7
019901
7C0173
01D301
BA01B5
01D501
EA01CA
01BF01
B001AF
01C101
C301D1
01A601
900184
018301
6F016C
016001
570153
017801
9801C3
01C001
C001C1
01BD01
B901AA
018601
740169
01DE01
CE01BF
01DC01
EC01E4
01CB01
C901D2
01CC01
D001BC
01A501
960185
017801
780161
016901
630154
016E01
9301B8
01DB01
DB01E3
01D301
C501A8
018C01
7B016F
01EC01
D801C1
01D901
F501EE
01DC01
DE01D8
01DB01
D201BC
01B801
AF0190
019601
7B0185
018301
700160
017501
9201A6
01B701
B201B8
01BD01
AE018C
016501
4C0148
01E701
E401DB
01E401
D801E4
01CF01
C001BF
01BA01
B601AB
019F01
9801A5
019401
820194
018101
59015D
017B01
7E018D
019101
8C01A2
01A001
99016F
015D01
49012E
01D801
DB01D4
01CA01
C801C7
01D701
CE01AF
019501
89019B
01A901
8F017D
017401
690179
016B01
5E0162
016A01
66017C
018301
7E0185
019A01
A70191
018501
65014C
01D201
DE01DB
01DE01
C401C1
01B801
BA0196
019E01
860173
017E01
6C0173
017701
5D0159
015901
6B0171
017601
6B016B
017501
86019D
019F01
B101A3
018301
6A0142
01F101
E601E5
01CE01
C601D5
01C701
A7019A
017401
740173
016A01
6A0163
015901
5E0172
016E01
880184
019101
760173
018A01
8B01B0
01A501
950199
018601
600147
01F301
D101CB
01CA01
D501D9
01CC01
CE01B1
018D01
74018E
019001
750181
017001
5B0176
017F01
88018A
019E01
830174
017401
8C01A7
018A01
8C01B3
019101
7E014F
01DB01
DD01EE
01EC01
ED01D8
01CB01
BB01A2
019901
88019F
018C01
9801A5
017601
59016A
017C01
85019E
019801
7E0190
017F01
7E018A
018501
92019D
019501
6D015C
01E401
FD01FD
01E801
DB01C7
01B801
9F01A1
01A701
A201A5
01A701
940186
017601
590174
018401
83018F
01A801
9A01AB
018F01
7E0188
019F01
A701A9
01AB01
73017C
01FE01
F801E0
01CC01
C001C1
01B301
B401C4
01C001
C201B1
019101
860187
016A01
5B0176
016A01
710196
01B801
BC01AF
01A001
9601A0
01B301
A001B0
01B101
8D017A
01F301
EE01EC
01C901
CA01D3
01CA01
C101CA
01DE01
C601AF
018F01
74016D
016201
620162
016601
8501A1
01A501
AA01AD
01A001
B601B8
01B601
A701B8
01BF01
920165
01DD01
CA01D3
01D001
CA01CB
01CF01
C501B0
01BD01
B5018E
018F01
83016F
016901
60016F
016401
800180
017A01
7B0182
017F01
A1019D
01B801
AF01C2
01C601
A0016F
01C401
B501B4
01B501
B701A8
01A801
A30193
01AC01
A2018C
017901
700173
015E01
6D018A
016A01
6B0173
017E01
83018A
019101
880192
01A401
A001B8
01C201
9E017C
01A501
A60195
019501
9D01A0
018C01
8B018F
01A601
8A0178
017801
6E0162
016E01
8D019E
016F01
79017C
019501
940199
019701
9E019E
019A01
AC01B0
01B501
940164
01A601
970197
019B01
85018A
017C01
830197
019101
73016A
016401
62016C
018C01
A20188
016D01
720194
019F01
B401B1
01BD01
A40196
019601
9E01A0
01A001
86015E
01C501
BD01B8
01A201
94018C
018501
7E017D
018501
74016F
016201
6A0188
01A001
9B017E
017101
780196
01AC01
A001C2
01C801
AC01A7
01A001
A2019A
018201
640148
01CC01
B001A4
018F01
93018E
018D01
840174
016F01
7C0163
016F01
8A0199
01A701
910172
017E01
85017C
018C01
AF01C1
01C501
B801A7
01A601
B1019F
017401
540139
01A601
9E0196
01A001
A70196
018E01
960181
017501
690167
018301
9A01B0
01A001
7D0179
019201
9F018C
019401
AD01B8
01B701
B801A7
01B001
AF0183
016D01
4D012B
019E01
A501A9
01C001
A9019B
017C01
720170
016A01
68017E
019101
AA019B
018301
74018D
01A101
A70194
01A201
9E01A5
01B901
B801B0
01B801
990176
015801
360122
01B201
B101A0
01A701
A40191
017101
720167
016201
760193
01A801
A00180
017401
740182
019401
AF01AB
01B201
AF01AF
01B101
B801B7
01A701
850162
013E01
30012C
019801
910185
018501
7F0177
016A01
79017B
017601
760189
019E01
8E017C
018401
830193
018501
A101B8
01BD01
C201C7
01B701
B8019D
017D01
62014D
013D01
210113
017D01
7C017D
017701
810173
017201
940199
018C01
7C0187
018A01
7F0188
018C01
99019F
019001
9201B9
01BD01
D201CB
01B901
AC0188
016401
490139
013401
25011C
018A01
99018B
017C01
780183
017A01
A001A0
01AC01
910190
018501
8C01A0
019C01
A001A7
019601
96019F
01A401
C501CA
01A801
91017D
016D01
540128
011E01
160114
019601
850181
018D01
83019D
017D01
80019E
01A701
AF019F
018B01
9901A6
019B01
A001A7
01AC01
AA01AB
01AD01
CC01B8
01A701
8F016F
015C01
400129
011C01
1F0112
019F01
8C0193
019A01
92019E
018F01
7C0188
01A001
B8019F
019901
A701AF
01A601
A701B4
01C701
BD01C9
01D101
BE019B
018501
7C0162
014201
28011D
011F01
1D0115
01B301
960197
01A101
8D01A9
019E01
85017E
019601
A701B0
01B101
BD01BB
01AE01
AF01B0
01C601
D001D5
01C201
9F017B
016501
600152
014C01
3D012C
012D01
1F011E
01AB01
9E01B1
01B701
94019F
018801
990193
019901
B801C8
01BF01
D201D3
01BB01
C701BB
01CC01
E001BB
01A101
970174
015201
480144
014601
41013C
012901
14012F
01A701
AC01C2
01AF01
9C0195
01A101
B801A3
019E01
B501D3
01D101
D301E4
01C901
D801D0
01D301
DB01B6
01A701
87017C
015A01
600157
014C01
2D011D
011501
10012D
01B101
C201C2
01A001
B201A0
01A701
C001C0
01AD01
A701CA
01E401
DD01EE
01DF01
CA01C0
01C801
BA01BC
01A001
900188
018301
800174
015F01
420126
010301
1C0125
01CF01
CF01B0
01C201
C001A8
01AF01
B801BC
01B001
B601B9
01C101
DE01D6
01C301
B801B0
01A701
A201AB
01A701
AB01A0
018401
6F015E
014801
2D010E
010701
30012D
01E401
C101B6
01D501
D001C5
01BE01
C501C9
01CB01
D501DB
01D901
E901DB
01E301
CD01C2
01AE01
9E0194
019301
9E017E
017201
4C0143
013501
150100
011601
390147
EOF

cat > moss.asc << EOF
I 0 v4
Gary Moss's "World on a Platter"
S 16 tor 6 4.916235923767e+00 -3.280223083496e+01 3.171177673340e+01 0.000000000000e+00 5.079999923706e+00 0.000000000000e+00 -1.796051025391e+01 0.000000000000e+00 1.796051025391e+01 -1.796051025391e+01 0.000000000000e+00 -1.796051025391e+01 -1.436841011047e+01 0.000000000000e+00 1.436841011047e+01 -1.436840915680e+01 0.000000000000e+00 -1.436840915680e+01 -2.155261230469e+01 0.000000000000e+00 2.155261230469e+01 -2.155261230469e+01 0.000000000000e+00 -2.155261230469e+01 
S 20 platform.s 1 8.971723937988e+01 -6.957164001465e+01 -2.376846313477e+01 0.000000000000e+00 1.390160369873e+02 0.000000000000e+00 0.000000000000e+00 1.390160369873e+02 6.950803756714e+00 0.000000000000e+00 0.000000000000e+00 6.950803756714e+00 -1.390160369873e+02 0.000000000000e+00 0.000000000000e+00 -1.390160369873e+02 1.390160369873e+02 0.000000000000e+00 -1.390160369873e+02 1.390160369873e+02 6.950803756714e+00 -1.390160369873e+02 0.000000000000e+00 6.950803756714e+00 
S 20 box.s 1 3.002833557129e+01 -5.211529731750e+00 -1.637908935547e+01 0.000000000000e+00 2.679275512695e+01 0.000000000000e+00 0.000000000000e+00 2.679275512695e+01 2.679275512695e+01 0.000000000000e+00 0.000000000000e+00 2.679275512695e+01 -2.679275512695e+01 0.000000000000e+00 0.000000000000e+00 -2.679275512695e+01 2.679275512695e+01 0.000000000000e+00 -2.679275512695e+01 2.679275512695e+01 2.679275512695e+01 -2.679275512695e+01 0.000000000000e+00 2.679275512695e+01 
S 18 cone.s 5 1.687542724609e+01 -3.474353027344e+01 -1.637908935547e+01 0.000000000000e+00 0.000000000000e+00 2.644671630859e+01 9.350327491760e+00 -9.350327491760e+00 0.000000000000e+00 9.350327491760e+00 9.350327491760e+00 0.000000000000e+00 4.339659690857e+00 -4.339659690857e+00 0.000000000000e+00 1.453863143921e+00 1.453863143921e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 
S 19 ellipse.s 4 1.613092041016e+01 4.665559387207e+01 -3.722520828247e+00 1.487607192993e+01 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 8.980256080627e+00 -8.980256080627e+00 0.000000000000e+00 8.980256080627e+00 8.980256080627e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 
C Y platform.r 1000 0 1 -16081 0 0 1 80 150 230 0 0 0 
M + platform.s 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 16268 
C Y box.r 2000 0 1 -16558 0 0 1 100 190 190 0 0 0 
M + box.s 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 16519 
C Y ellipse.r 4000 0 1 16619 0 0 1 100 210 100 0 0 0 
M + ellipse.s 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0 
C Y cone.r 3000 0 1 -16209 0 0 1 255 100 255 0 0 0 
M + cone.s 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0 
C Y tor.r 6000 0 1 -16219 0 0 1 240 240 0 0 0 0 
M + tor 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 16473 
S 19 LIGHT 11 2.015756225586e+01 -1.352595329285e+01 5.034742355347e+00 2.539999961853e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 2.539999961853e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 2.539999961853e+00 6.652935973394e-32 -1.768683290104e-12 -4.437394240169e-21 0.000000000000e+00 -5.077455353676e-19 -2.153920654297e+03 0.000000000000e+00 0.000000000000e+00 -5.077455353676e-19 -5.400174699785e-19 0.000000000000e+00 0.000000000000e+00 
C Y light.r 1000 0 1 0 1 100 1 255 255 255 1 0 0 
light
M u LIGHT 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0 
C N all.g -1 0 6 16619 0 0 0 0 0 0 0 0 0 
M u platform.r 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0 
M u box.r 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 -2.369892883301e+01 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 1.340998744965e+01 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 8.023991584778e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0 
M u cone.r 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 2.204924011230e+01 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 1.223486709595e+01 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 2.111249841619e-07 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0 
M u ellipse.r 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.467929267883e+01 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 -4.160771179199e+01 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 3.879878234863e+01 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0 
M u tor.r 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0 
M u light.r 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0 
EOF
$BIN/asc2g moss.asc moss.g

# Trim 1025 byte sequence down to exactly 1024.
$BIN/gencolor -r205 0 16 32 64 128 | dd of=ebm.bw bs=1024 count=1 >/dev/null 2>&1

$BIN/mged -c > mged_solids.log 2>&1 << EOF
opendb solids.g y


units cm
#
# Halfspace
#
in half.s half 0 0 1 -1
r half.r u half.s
mater half.r plastic 76 158 113 0

#
# Make one of each arb
#
in arb4.s arb4 0 -64 33 -31 -64 33 -31 -64 48 -31 -95 33
r arb4.r u arb4.s
mater arb4.r "plastic" 120 120 200 0

in arb5.s arb5 -31 -128 33 0 -128 33 0 -97 33 -31 -97 33 -16.5 -113.5 43
r arb5.r u arb5.s
mater arb5.r "plastic" 120 120 120 0

in arb6.s arb6 -64 -95 33 -33 -95 33 -33 -64 33 -64 -64 33 -49.5 -95 43 -49.5 -64 43
r arb6.r u arb6.s
mater arb6.r "plastic" 120 200 120 0

in arb7.s arb7 -64 -97 33 -64 -128 33 -64 -128 64 -64 -97 48.5 -33 -97 33 -33 -128 33 -33 -128 48.5
r arb7.r u arb7.s
mater arb7.r "plastic" 200 120 120 0

in arb8.s arb8 0 -128 0 0 -64 0 0 -64 32 0 -128 32 -64 -128 0 -64 -64 0 -64 -64 32 -64 -128 32
r arb8.r u arb8.s
mater arb8.r "plastic {di .9 sp .5}" 200 180 0 0


#
#  Make some ellipsoids
#
in sph.s sph -32 0 32 32
r sph.r u sph.s
mater sph.r "plastic re=0.2" 255 255 255 0

in ell.s ell -32 0 96  8 0 0  0 16 0  0 0 32
r ell.r u ell.s
mater ell.r "plastic {sp .4 di .6}" 255 120 120 0

#
#  Tori
#
in tor.s tor 64 0 9 0 0 1 32 8
r tor.r u tor.s
mater tor.r plastic 200 200 255 0

in eto.s eto 64 0 32 0 0 1 12 0 12 24 4
r eto.r u eto.s
mater eto.r "plastic {sp .4 di .9}" 155 155 255 0

#
# Cylinders
#
in rcc.s rcc 64 -96 0 0 0 32 32
r rcc.r u rcc.s
mater rcc.r plastic 200 200 50 0

in tgc.s tgc 64 -112 32 0 0 24 16 0 0 0 8 0 4 12
r tgc.r u tgc.s
mater tgc.r plastic 200 50 200 0

in trc.s trc 80 -90 32 0 0 24 8 4
r trc.r u trc.s
mater trc.r plastic 50 200 200 0

in rec.s rec 54 -80 32 0 0 16 0 12 0 8 0 0
r rec.r u rec.s
mater rec.r "plastic {sp .4 di .9 re .1}" 250 250 210 0


#
# Hyperboloids
#
in ehy.s ehy 64 -192 0    0  0 64   32 0 0   16   128
r ehy.r u ehy.s
mater ehy.r plastic 255 192 192 0

in rhc.s rhc 64 -212 0    0 -32 0   0 0 64   32   128
r rhc.r u rhc.s
mater rhc.r plastic 255 128 128 0

#
# Paraboloids
#
in epa.s epa  -32 -192 0   0 0 64     32 0 0  16
r epa.r u epa.s
mater epa.r plastic 192 192 255 0

in rpc.s rpc  -32 -212 0   0 -32 0    0 0 64  32
r rpc.r u rpc.s
mater rpc.r plastic 128 128 255 0

#
# Extruded bit-map
#
db put ebm.s ebm F ebm.bw W 32 N 32 H 32 M {1 0 0 -82  0 1 0 -64   0 0 1 0  0 0 0 0.05}
r ebm.r u ebm.s

#
# Displacement map
#
db put dsp.s dsp src f name dsp.dat w 33 n 33 sm 0 cut a stom {20 0 0 -1687 0 20 0 -2432 0 0 0.3906250 0 0 0 0 1}
r dsp.r u dsp.s

#
# Particle Solid
#

db put particle.s part V {1280 -960 100} H {0 0 320} r_v 80 r_h 160
r particle.r u particle.s
mater particle.r plastic 255 100 100 0



in light1.s sph 141.642 12.8892 176.202 10
r light1.r u light1.s
mater light1.r "light shadow=1" 255 255 255 0

# Group it all together
g all.g *.r

# Show it off
Z
e all.g
center 0 -96 0
set perspective 30
size 1600
ae 25 45
saveview solids
q
EOF

if [ ! -f solids ] ; then
	echo 'mged failed'
	exit 1
fi
mv solids solids.orig
sed "s,^rt,$BIN/rt -B -P 1 ," < solids.orig > solids
rm solids.orig
chmod 775 solids

echo 'rendering solids...'
./solids
if [ ! -f solids.pix ] ; then
	echo raytrace failed
else
	if [ ! -f $2/regress/solidspix.asc ] ; then
		echo No reference file for solids.pix
	else
		$BIN/asc2pix < $2/regress/solidspix.asc > solids_ref.pix
		$BIN/pixdiff solids.pix solids_ref.pix \
		> solids.pix.diff \
		2>> solids-diff.log

		/bin/echo -n solids.pix
		tr , '\012' < solids-diff.log | grep many
	fi
fi

echo "rendering moss..."
$BIN/rt -P 1 -B -C0/0/50 -M -s 512 -o moss.pix moss.g all.g > moss.log 2>&1 << EOF
viewsize 1.572026215e+02;
eye_pt 6.379990387e+01 3.271768951e+01 3.366661453e+01;
viewrot -5.735764503e-01 8.191520572e-01 0.000000000e+00 
0.000000000e+00 -3.461886346e-01 -2.424038798e-01 9.063078165e-01 
0.000000000e+00 7.424039245e-01 5.198368430e-01 4.226182699e-01 
0.000000000e+00 0.000000000e+00 0.000000000e+00 0.000000000e+00 
1.000000000e+00 ;
start 0;
end;
EOF


if [ ! -f moss.pix ] ; then
	echo raytrace failed
else
	if [ ! -f $2/regress/mosspix.asc ] ; then
		echo No reference file for moss.pix
	else
		$BIN/asc2pix < $2/regress/mosspix.asc > moss_ref.pix
		$BIN/pixdiff moss.pix \
			moss_ref.pix \
			> moss.pix.diff \
			2> moss-diff.log

		/bin/echo -n moss.pix
		tr , '\012' < moss-diff.log | grep many
	fi
fi

# Local Variables:
# mode: sh
# tab-width: 8
# sh-indentation: 4
# sh-basic-offset: 4
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
