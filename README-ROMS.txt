In order to build the static binary translations or use some of the emulators, you will need to install your rom images.

We are in the process of tidying up our filesystem laayout, getting ready for release.

At the moment, all files that the user has to create or install are going into some directory under /opt/pitrex - these will
not be put in place for you by fetching the github repository.

Currently /opt/pitrex contains
 /opt/pitrex/menu.ym    -- music used by bare metal menu
 /opt/pitrex/ini        -- copy these from cinematronics/ini
 /opt/pitrex/settings   -- initially empty, this will be used to save default as well as individual game settings
 /opt/pitrex/roms       -- contains subdirectories for various rom images:
   armorattack      barrier     BlackWidow  cosmicchasm  Gravitar     qb3       ripoff      SpaceDuel  speedfreak  starhawk  tailgunner  vectrex         warrior
   AsteroidsDeluxe  Battlezone  boxingbugs  demon        LunarLander  RedBaron  solarquest  spacewars  starcastle  sundance  Tempest     waroftheworlds

The vectrex roms belonging to GCE have been made public, in a letter from the owner, Jay Smith.  These should be placed in /opt/pitrex/roms/vectrex/
- the rom names and extensions are currently expected to be in upper case.


These are the files in our working directory. They may not all be necessary - we'll remove unnecessary ones from this list later.

At the moment ${PIROOT}tailgunner/ and ${PIROOT}asteroids_sbt/ are the only ones that are given the static binary translation treatment.  The others are emulated.  The emulators all need some work to adapt the games for the Vectrex control panel.  When we started this we did not envisage having access to USB peripherals on the PiTrex's Raspberry Pi Zero, so the original controller code is unlikely to work and may have to be put back in from an older version of the emulation.

At the moment the static binary translations look for their roms in their local directories.  That is currently being changed so that they will be found in the common area in /opt/pitrex/roms ...

a377b3b34e4a4ca8b0048ea4ba53c480  tailgunner/tailg.t7
96bf9adaa9d21ab5fd5ed7fc26759486  tailgunner/tailg.p7
df68e2d4bd5e58513cb6e3a33768483b  tailgunner/tailg.u7
1d93d2e15e85ae30c9e2d850e3f3aae9  tailgunner/tailg.r7

e6c34b639fe16deff0f951be63076812  asteroids_sbt/035127.02
ca4f0146fb806f58a12e3e69d8fd7277  asteroids_sbt/035145.02
dd35aef4a17c271151413cbb0893b550  asteroids_sbt/035143.02
8010044e056c7a2ba3538a8c68b773d2  asteroids_sbt/035144.02

The cinemu emulator here is a new creation based on code extracted from other emulators.  It is not the original cinemu which was a DOS program written in assembly language.

The roms used in the cinematronics emulator are defined in its ini files.

$ ls cinematronics/roms
armorattack  boxingbugs   demon  ripoff      spacewars   starcastle  sundance    waroftheworlds
barrier      cosmicchasm  qb3    solarquest  speedfreak  starhawk    tailgunner  warrior

4b6039bcbe7520ee51aff0d01a55c007  cinematronics/roms/armorattack/ar414le.t6
f19a8624ef8be06a5be1033d55c9bbb2  cinematronics/roms/armorattack/ar414lo.p6
839142a08a217c646be73711e0b3f06c  cinematronics/roms/armorattack/ar414ue.u6
065a721a1480bfc730d9f2bfe20c7467  cinematronics/roms/armorattack/ar414uo.r6

0f33cbbfd4b741375add656964628f7a  cinematronics/roms/barrier/barrier.p7
b88c024a979ef083394a33de3755b1bc  cinematronics/roms/barrier/barrier.t7

b556b7039236ceb8c6f69eca5848f962  cinematronics/roms/boxingbugs/u1a
4b95a48def6a1193d07c7eac0adfda1b  cinematronics/roms/boxingbugs/u1b
a1c59fba503f3f45b965231748f14f0c  cinematronics/roms/boxingbugs/u2a
080179cfc180202dca138d7001b2d839  cinematronics/roms/boxingbugs/u2b
6b41e19a76b1438a638f0bdfa0fdde9c  cinematronics/roms/boxingbugs/u3a
b22b84ba67699a9c89c5f3038a4cfc76  cinematronics/roms/boxingbugs/u3b
a56ae444fd24acaa307d7f35c7a57345  cinematronics/roms/boxingbugs/u4a
fa76dafba86fd1b7690887aac846927a  cinematronics/roms/boxingbugs/u4b

944be42d46be134f278a21225f69221d  cinematronics/roms/cosmicchasm/chasm.u1
9bcb64c65dc9f886ee4eb6cdd966171a  cinematronics/roms/cosmicchasm/chasm.u10
72bad7e180a4586f312bb90f709efcf8  cinematronics/roms/cosmicchasm/chasm.u11
f8d9e07dfc1f4935e5f204fb5908025a  cinematronics/roms/cosmicchasm/chasm.u12
4e112d9277bfe54b7c1d39df3f05392b  cinematronics/roms/cosmicchasm/chasm.u13
73d897da77182d24335644991864d3f9  cinematronics/roms/cosmicchasm/chasm.u14
04f047b4f0ea964f80345ec626b92bfb  cinematronics/roms/cosmicchasm/chasm.u15
002735af149ae8ca1492d197cda9baec  cinematronics/roms/cosmicchasm/chasm.u16
a2d378f774a9baba8b956c114ae302bb  cinematronics/roms/cosmicchasm/chasm.u2
f0ed682506bf1aa9426d8199234edbb7  cinematronics/roms/cosmicchasm/chasm.u3
f42e024487881cf96c8623f472608af7  cinematronics/roms/cosmicchasm/chasm.u4
16d69e78945cca0cb6bd38a56ada7a36  cinematronics/roms/cosmicchasm/chasm.u5
8e79cc9dc3e16ef7cf0f58b04faa07e8  cinematronics/roms/cosmicchasm/chasm.u6
93014b7cf16f07ccbf4607809042f725  cinematronics/roms/cosmicchasm/chasm.u7
3e5bdf8ad0120bf4aa5860a00580667c  cinematronics/roms/cosmicchasm/chasm.u8
a2603045a6f9e785f956d26cd750eb9d  cinematronics/roms/cosmicchasm/chasm.u9

385f8bc93e8ddc5a96ffead0f02e44d2  cinematronics/roms/demon/demon.7p
50a7ca8ab55a4b3537b9c1336892dfc4  cinematronics/roms/demon/demon.7r
baf18f5e10cf165a00cfb0eb3052706d  cinematronics/roms/demon/demon.7t
d69fc8a6fd8a1f1a13bc81a67cc2eaba  cinematronics/roms/demon/demon.7u

22157a6618b29416d082b752120c4919  cinematronics/roms/qb3/qb3_le_t7.bin
9e220796a45dabc16cd9c3769b2b0ac0  cinematronics/roms/qb3/qb3_lo_p7.bin
61027fcdae672de1e01a659ec56cd57d  cinematronics/roms/qb3/qb3_snd_u11.bin
14c1b94614bc5d813f7e5d5472c40636  cinematronics/roms/qb3/qb3_snd_u12.bin
5fbb406e18e18689e2b75888d6d2a1a9  cinematronics/roms/qb3/qb3_ue_u7.bin
34db18a4b73f45a03b1c65a99fee4bbd  cinematronics/roms/qb3/qb3_uo_r7.bin

ccb6c5fc040d3e997ab681665735d9ab  cinematronics/roms/ripoff/ripoff.p7
7dee763f566f99bd005d81beaa6a665a  cinematronics/roms/ripoff/ripoff.r7
0939539627e78cdabd691e0d730604fc  cinematronics/roms/ripoff/ripoff.t7
5f2620d082937cc1422a25368feb276f  cinematronics/roms/ripoff/ripoff.u7

8265fd733ab85f30a9a64b85b8255df1  cinematronics/roms/solarquest/solar.p7
02ec610a964fd55dc38c7b18533d5ceb  cinematronics/roms/solarquest/solar.r7
cc74ce5714cb18fec1aed3b2532232e9  cinematronics/roms/solarquest/solar.t7
43d271a4b592a5ed48ca19185587a6c8  cinematronics/roms/solarquest/solar.u7

842268f3e812dc561e35e4b532601e61  cinematronics/roms/spacewars/spacewar.r7
016cf68c94ccc1ee8d1cdeec86b52c0c  cinematronics/roms/spacewars/spacewar.u7

834b83f1af04ee3d84904456366df842  cinematronics/roms/speedfreak/speedfrk.p7
a9668879fb7727736960eeaac2ffd6ab  cinematronics/roms/speedfreak/speedfrk.r7
b31369b22c800b757c9434dbc5af01ab  cinematronics/roms/speedfreak/speedfrk.t7
90019c1d6373b1c89c7b0b0dd875b728  cinematronics/roms/speedfreak/speedfrk.u7

4ad1269d473d0f570fbb241990c6bf3a  cinematronics/roms/starhawk/starhawk.r7
ad89b2040f544fed4fcf630c0f5a8aa2  cinematronics/roms/starhawk/starhawk.u7

4b304388515582ac92e82a939a7821ca  cinematronics/roms/sundance/sundance.p7
4496c9ab150b55afa85f970caf2e9640  cinematronics/roms/sundance/sundance.r7
4f3f68e587bb5b39f2bb35f01ec5daff  cinematronics/roms/sundance/sundance.t7
2377ed38fedaa04ede2681ff1065a9a9  cinematronics/roms/sundance/sundance.u7

96bf9adaa9d21ab5fd5ed7fc26759486  cinematronics/roms/tailgunner/tailg.p7
1d93d2e15e85ae30c9e2d850e3f3aae9  cinematronics/roms/tailgunner/tailg.r7
a377b3b34e4a4ca8b0048ea4ba53c480  cinematronics/roms/tailgunner/tailg.t7
df68e2d4bd5e58513cb6e3a33768483b  cinematronics/roms/tailgunner/tailg.u7

bf719414ffec560d2faa843ec18d73e9  cinematronics/roms/waroftheworlds/wotw.p7
2db4e08d4f577aff0b551635177bf715  cinematronics/roms/waroftheworlds/wotw.r7
6ba47016268392d7622b6dd30fb7e2fd  cinematronics/roms/waroftheworlds/wotw.t7
cb0aaa62640691b97d2c4054e38cf7ca  cinematronics/roms/waroftheworlds/wotw.u7

6b0d43144d46bd7389338d8aa8cd48dd  cinematronics/roms/warrior/warrior.p7
9afe3641aaebb9d9dff8319450697e89  cinematronics/roms/warrior/warrior.r7
29a9e80f5a600d93692cd12aa91b143d  cinematronics/roms/warrior/warrior.t7
9dc63b93975ce13ae759d93602250018  cinematronics/roms/warrior/warrior.u7

The roms used by the Atari emulator are:

8ecdc91ff9ce33ea46539c359c04f291  blackwidow/roms/BlackWidow/136017.101
0b7ebfb73a45b7ded031316ac31c8380  blackwidow/roms/BlackWidow/136017.102
fa2dba43f2232031f8ee89c1259e06e1  blackwidow/roms/BlackWidow/136017.103
3bb72ca2f9f55ccfbc824ff32581d37c  blackwidow/roms/BlackWidow/136017.104
535be82bbae1338962ecc211d776d195  blackwidow/roms/BlackWidow/136017.105
553af55bf04e9245f2539fbc032f94d5  blackwidow/roms/BlackWidow/136017.106
00b314d7a8e2b5be05fade656145390e  blackwidow/roms/BlackWidow/136017.107
1959359302a3673765da607893052a2c  blackwidow/roms/BlackWidow/136017.108
8e63283972aa200d7092b31d7a45a28a  blackwidow/roms/BlackWidow/136017.109
3b4e34e04340f42722092763e5c195cd  blackwidow/roms/BlackWidow/136017.110

1dfbe31a96b12f008fa1c827d5600c40  gravitar/roms/Gravitar/136010.201
ac5fc33d7254342c1fb3548966f17350  gravitar/roms/Gravitar/136010.202
6b6a8184c9e01adf754ad35d04067e2b  gravitar/roms/Gravitar/136010.203
859111b265b1c3a85bd073669d4667ed  gravitar/roms/Gravitar/136010.204
2a0e64def7d8627aae2826ac87e695fe  gravitar/roms/Gravitar/136010.205
dda1eca1eab5dbca07174cb838f0acf6  gravitar/roms/Gravitar/136010.206
ba874e2fb7ac558caf0840a8e7fb9e43  gravitar/roms/Gravitar/136010.210
1264b03d7b70f46dda0a2c17f59a4f0d  gravitar/roms/Gravitar/136010.207
5eacac015850f377790cffc7693e2138  gravitar/roms/Gravitar/136010.208
a954245800aed23797780f2994b0771e  gravitar/roms/Gravitar/136010.209

6833c6d2b10adc56d860d9ff51363ab5  spaceduel/roms/SpaceDuel/136006.201
2eab1d28cc205353392cc9b02bbdfb71  spaceduel/roms/SpaceDuel/136006.102
9984e387183563853ff1023e4a7a3481  spaceduel/roms/SpaceDuel/136006.103
bcb0e7d231fafdd740dcd686080b72b8  spaceduel/roms/SpaceDuel/136006.104
fa40e3732dbb92f89f70931d80a0c79f  spaceduel/roms/SpaceDuel/136006.105
4ffda6945bf07d6b30568aadad539884  spaceduel/roms/SpaceDuel/136006.106
1cf0e02fc85e8bf007134a2bdeb80229  spaceduel/roms/SpaceDuel/136006.107

69b1d57b51cca26e9916326591722d07  tempest/roms/Tempest/136002-133.d1
39bc793103474c965d0fec7b66475292  tempest/roms/Tempest/136002-134.f1
0ed480de122e265c008ddce1abaef1da  tempest/roms/Tempest/136002-235.j1
1536b8cc25707796d88a0d261f260132  tempest/roms/Tempest/136002-136.lm1
6c9f5e061ad98fa264a54bcd38f4e4a9  tempest/roms/Tempest/136002-237.p1
04a40593bba720c2adf76ccccd6ae75e  tempest/roms/Tempest/136002-138.np3
703dda5c10029d86c79da54c4e930d2a  tempest/roms/Tempest/136002-125.d7

7f70c9468408ebdd21093e7a3c2170ff  battlezone/roms/Battlezone/036414.01
35acc7b9af450d9f16a2045c9e7ed931  battlezone/roms/Battlezone/036413.01
0107f4a6f81fdbde87f531fd22f92ca1  battlezone/roms/Battlezone/036412.01
58bfcd4a9adf8eb70473c88c46b96b7d  battlezone/roms/Battlezone/036411.01
8e32e822503accfb0837d99d399d893d  battlezone/roms/Battlezone/036410.01
9eed6c14ad3b045dfd7ee4c7bc8f2434  battlezone/roms/Battlezone/036409.01
898838981d177b95368c898c68cb2342  battlezone/roms/Battlezone/036422.01
59bc75979bc02e1e7537ef590d472691  battlezone/roms/Battlezone/036421.01

roms/RedBaron/037001.01E
roms/RedBaron/036999.01E
437a0df08d39e71af753c428a463be72  redbaron/roms/RedBaron/037587-01.fh1
f4554af174c242ed3fb7835b9e767baf  redbaron/roms/RedBaron/037000-01.e1
437a0df08d39e71af753c428a463be72  redbaron/roms/RedBaron/037587-01.fh1
f4554af174c242ed3fb7835b9e767baf  redbaron/roms/RedBaron/037000-01.e1
99bf2440f5c70d6c2d801293102c6543  redbaron/roms/RedBaron/036998-01.j1
5abf06ee3c78a5e23c05e84b2bb78c08  redbaron/roms/RedBaron/036997-01.k1
000e6e0815ff6ae71077f7cf8c30a9e8  redbaron/roms/RedBaron/036996-01.lm1
9aad7c18464006834440eb21998f3b62  redbaron/roms/RedBaron/036995-01.n1
680ff429ddbfdd2d3af977c382e0b727  redbaron/roms/RedBaron/037006-01.bc3
b7ba3edb7619c23f2b1eb1af2fa93315  redbaron/roms/RedBaron/037007-01.a3

daf1fb70e3a862637b3d7b411e3fc32b  lunar/roms/LunarLander/034572.02
7862abe5c0d58f4aaf6e10c21f220b0f  lunar/roms/LunarLander/034571.02
0bec2df5f2824a83b3f4bc728b59661b  lunar/roms/LunarLander/034570.02
b54051b0a71e92de7a551c375e04a97e  lunar/roms/LunarLander/034569.02
51029ab710fe23f1579a21766cf8732f  lunar/roms/LunarLander/034599.01
f2ae71655bae3b96fb5485f1a71961f4  lunar/roms/LunarLander/034598.01
2496c2f0482a334b7da658691f4c5a14  lunar/roms/LunarLander/034597.01

50dddf568917e9fcb0b4ac3168541a29  deluxe/roms/AsteroidsDeluxe/036799.01
79749a3cbfe6a8f5a06276177ec63d61  deluxe/roms/AsteroidsDeluxe/036430.02
b40f21f04869e6bf42a0e9681d899452  deluxe/roms/AsteroidsDeluxe/036431.02
0d03aaee1cd807bfc2248782d06c0bc1  deluxe/roms/AsteroidsDeluxe/036432.02
a2e7a59e43390f0588f1fce17010093e  deluxe/roms/AsteroidsDeluxe/036433.03
fcd201ebaba8d4f2d7604b8751435c96  deluxe/roms/AsteroidsDeluxe/036800.02
50dddf568917e9fcb0b4ac3168541a29  deluxe/roms/AsteroidsDeluxe/036799.01
roms/AsteroidsDeluxe/036430.01
roms/AsteroidsDeluxe/036431.01
roms/AsteroidsDeluxe/036432.01
roms/AsteroidsDeluxe/036433.02
roms/AsteroidsDeluxe/036800.01

roms/MajorHavoc/136025.104
roms/MajorHavoc/136010.103
roms/MajorHavoc/136010.109
roms/MajorHavoc/136010.101
roms/MajorHavoc/136010.106
roms/MajorHavoc/136010.107
roms/MajorHavoc/136010.108
roms/MajorHavoc/136010.110

e6c34b639fe16deff0f951be63076812  roms/Asteroids/035127.02
ca4f0146fb806f58a12e3e69d8fd7277  roms/Asteroids/035145.02
dd35aef4a17c271151413cbb0893b550  roms/Asteroids/035143.02
8010044e056c7a2ba3538a8c68b773d2  roms/Asteroids/035144.02
