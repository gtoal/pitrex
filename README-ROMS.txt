In order to build the static binary translations or use some of the emulators, you will need to install your rom images.

The vectrex roms belonging to GCE have been made public, in a letter from the owner, Jay Smith.  These are pre-installed in ${PIROOT}vecx.direct/vectrex/

The cinematronics roms go in ${PIROOT}cinematronics/roms/* where * is a subdirectory using the name of the name.

These are the files in our working directory. They may not all be necessary - we'll remove unnecessary ones from this list later.

At the moment ${PIROOT}tailgunner/ and ${PIROOT}asteroids_sbt/ are the only ones that are given the static binary translation treatment.  The others are emulated.  The emulators all need some work to adapt the games for the Vectrex control panel.  When we started this we did not envisage having access to USB peripherals on the PiTrex's Raspberry Pi Zero, so the original controller code is unlikely to work and may have to be put back in from an older version of the emulation.

a377b3b34e4a4ca8b0048ea4ba53c480  cinematronics/roms/tailgunner/tailg.t7
96bf9adaa9d21ab5fd5ed7fc26759486  cinematronics/roms/tailgunner/tailg.p7
df68e2d4bd5e58513cb6e3a33768483b  cinematronics/roms/tailgunner/tailg.u7
1d93d2e15e85ae30c9e2d850e3f3aae9  cinematronics/roms/tailgunner/tailg.r7

e6c34b639fe16deff0f951be63076812  asteroids_sbt/035127.02
ca4f0146fb806f58a12e3e69d8fd7277  asteroids_sbt/035145.02
dd35aef4a17c271151413cbb0893b550  asteroids_sbt/035143.02
8010044e056c7a2ba3538a8c68b773d2  asteroids_sbt/035144.02

The cinemu emulator here is a new creation based on code extracted from other emulators.  It is not the original cinemu which was a DOS program written in assembly language.

The roms used in the cinematronics emulator are defined in these ini files:
ini/armorattack.ini:
  RomImages = roms/armorattack/ar414le.t6,roms/armorattack/ar414lo.p6,roms/armorattack/ar414ue.u6,roms/armorattack/ar414uo.r6
ini/barrier.ini:
  RomImages=roms/barrier/barrier.t7,roms/barrier/barrier.p7
ini/boxingbugs.ini:
  RomImages=roms/boxingbugs/u1a,roms/boxingbugs/u1b,roms/boxingbugs/u2a,roms/boxingbugs/u2b,roms/boxingbugs/u3a,roms/boxingbugs/u3b,roms/boxingbugs/u4a,roms/boxingbugs/u4b
ini/cosmicchasm.ini:
  RomImages=roms/cosmicchasm/chasm.u4,roms/cosmicchasm/chasm.u12,roms/cosmicchasm/chasm.u8,roms/cosmicchasm/chasm.u16,roms/cosmicchasm/chasm.u3,roms/cosmicchasm/chasm.u11,roms/cosmicchasm/chasm.u7,roms/cosmicchasm/chasm.u15
ini/demon.ini:
  RomImages=roms/demon/demon.7t,roms/demon/demon.7p,roms/demon/demon.7u,roms/demon/demon.7r
ini/qb3.ini:
  RomImages=roms/qb3/qb3_le_t7.bin,roms/qb3/qb3_lo_p7.bin,roms/qb3/qb3_ue_u7.bin,roms/qb3/qb3_uo_r7.bin
ini/ripoff.ini:
  RomImages=roms/ripoff/ripoff.t7,roms/ripoff/ripoff.p7,roms/ripoff/ripoff.u7,roms/ripoff/ripoff.r7
ini/solarquest.ini:
  RomImages=roms/solarquest/solar.t7,roms/solarquest/solar.p7,roms/solarquest/solar.u7,roms/solarquest/solar.r7
ini/spacewars.ini:
  RomImages=roms/spacewars/spacewar.u7,roms/spacewars/spacewar.r7
ini/speedfreak.ini:
  RomImages=roms/speedfreak/speedfrk.t7,roms/speedfreak/speedfrk.p7,roms/speedfreak/speedfrk.u7,roms/speedfreak/speedfrk.r7
ini/starcastle.ini:
  RomImages=roms/starcastle/starcas.t7,roms/starcastle/starcas.p7,roms/starcastle/starcas.u7,roms/starcastle/starcas.r7
ini/starhawk.ini:
  RomImages=roms/starhawk/starhawk.u7,roms/starhawk/starhawk.r7
ini/sundance.ini:
  RomImages=roms/sundance/sundance.t7,roms/sundance/sundance.p7,roms/sundance/sundance.u7,roms/sundance/sundance.r7
ini/tailgunner.ini:
  RomImages=roms/tailgunner/tailg.t7,roms/tailgunner/tailg.p7,roms/tailgunner/tailg.u7,roms/tailgunner/tailg.r7
ini/waroftheworlds.ini:
  RomImages=roms/waroftheworlds/wotw.t7,roms/waroftheworlds/wotw.p7,roms/waroftheworlds/wotw.u7,roms/waroftheworlds/wotw.r7
ini/warrior.ini:
  RomImages=roms/warrior/warrior.t7,roms/warrior/warrior.p7,roms/warrior/warrior.u7,roms/warrior/warrior.r7

so only those files will be needed out of the list below.  We'll trim it down as soon as possible.

$ ls cinematronics/roms
armorattack  boxingbugs   demon  ripoff      spacewars   starcastle  sundance    waroftheworlds
barrier      cosmicchasm  qb3    solarquest  speedfreak  starhawk    tailgunner  warrior

$ find cinematronics/roms -type f -exec md5sum {} \; | grep -v .zip|grep -v .txt
6b0d43144d46bd7389338d8aa8cd48dd  cinematronics/roms/warrior/warrior.p7
29a9e80f5a600d93692cd12aa91b143d  cinematronics/roms/warrior/warrior.t7
9dc63b93975ce13ae759d93602250018  cinematronics/roms/warrior/warrior.u7
9afe3641aaebb9d9dff8319450697e89  cinematronics/roms/warrior/warrior.r7
3dfca24e7374a38f7cdc10f24b902a2d  cinematronics/roms/armorattack/prom.d14
f19a8624ef8be06a5be1033d55c9bbb2  cinematronics/roms/armorattack/ar414lo.p6
839142a08a217c646be73711e0b3f06c  cinematronics/roms/armorattack/ar414ue.u6
620ed9be3df4481841a99b01569ed887  cinematronics/roms/armorattack/prom.f14
f5eab1ba5e57d8e6b5e308ef088d0b5d  cinematronics/roms/armorattack/prom.e14
7e55ccbdc155c07471c2f7f0e379c3e6  cinematronics/roms/armorattack/prom.j14
4b6039bcbe7520ee51aff0d01a55c007  cinematronics/roms/armorattack/ar414le.t6
02eabb44868657fdcc5e7a46f3cb1b1f  cinematronics/roms/armorattack/prom.e8
45c1cf8aec333085756819216039bb41  cinematronics/roms/armorattack/prom.c14
065a721a1480bfc730d9f2bfe20c7467  cinematronics/roms/armorattack/ar414uo.r6
3dfca24e7374a38f7cdc10f24b902a2d  cinematronics/roms/demon/prom.d14
620ed9be3df4481841a99b01569ed887  cinematronics/roms/demon/prom.f14
9f42120e856c9ffebeacad0d0d8f8825  cinematronics/roms/demon/demon.snd
baf18f5e10cf165a00cfb0eb3052706d  cinematronics/roms/demon/demon.7t
f5eab1ba5e57d8e6b5e308ef088d0b5d  cinematronics/roms/demon/prom.e14
7e55ccbdc155c07471c2f7f0e379c3e6  cinematronics/roms/demon/prom.j14
d69fc8a6fd8a1f1a13bc81a67cc2eaba  cinematronics/roms/demon/demon.7u
02eabb44868657fdcc5e7a46f3cb1b1f  cinematronics/roms/demon/prom.e8
50a7ca8ab55a4b3537b9c1336892dfc4  cinematronics/roms/demon/demon.7r
45c1cf8aec333085756819216039bb41  cinematronics/roms/demon/prom.c14
385f8bc93e8ddc5a96ffead0f02e44d2  cinematronics/roms/demon/demon.7p
cb0aaa62640691b97d2c4054e38cf7ca  cinematronics/roms/waroftheworlds/wotw.u7
6ba47016268392d7622b6dd30fb7e2fd  cinematronics/roms/waroftheworlds/wotw.t7
bf719414ffec560d2faa843ec18d73e9  cinematronics/roms/waroftheworlds/wotw.p7
2db4e08d4f577aff0b551635177bf715  cinematronics/roms/waroftheworlds/wotw.r7
3dfca24e7374a38f7cdc10f24b902a2d  cinematronics/roms/qb3/prom.d14
22157a6618b29416d082b752120c4919  cinematronics/roms/qb3/qb3_le_t7.bin
620ed9be3df4481841a99b01569ed887  cinematronics/roms/qb3/prom.f14
9e220796a45dabc16cd9c3769b2b0ac0  cinematronics/roms/qb3/qb3_lo_p7.bin
14c1b94614bc5d813f7e5d5472c40636  cinematronics/roms/qb3/qb3_snd_u12.bin
f5eab1ba5e57d8e6b5e308ef088d0b5d  cinematronics/roms/qb3/prom.e14
7e55ccbdc155c07471c2f7f0e379c3e6  cinematronics/roms/qb3/prom.j14
34db18a4b73f45a03b1c65a99fee4bbd  cinematronics/roms/qb3/qb3_uo_r7.bin
61027fcdae672de1e01a659ec56cd57d  cinematronics/roms/qb3/qb3_snd_u11.bin
02eabb44868657fdcc5e7a46f3cb1b1f  cinematronics/roms/qb3/prom.e8
5fbb406e18e18689e2b75888d6d2a1a9  cinematronics/roms/qb3/qb3_ue_u7.bin
45c1cf8aec333085756819216039bb41  cinematronics/roms/qb3/prom.c14
b31369b22c800b757c9434dbc5af01ab  cinematronics/roms/speedfreak/speedfrk.t7
834b83f1af04ee3d84904456366df842  cinematronics/roms/speedfreak/speedfrk.p7
90019c1d6373b1c89c7b0b0dd875b728  cinematronics/roms/speedfreak/speedfrk.u7
a9668879fb7727736960eeaac2ffd6ab  cinematronics/roms/speedfreak/speedfrk.r7
5f2620d082937cc1422a25368feb276f  cinematronics/roms/ripoff/ripoff.u7
0939539627e78cdabd691e0d730604fc  cinematronics/roms/ripoff/ripoff.t7
ccb6c5fc040d3e997ab681665735d9ab  cinematronics/roms/ripoff/ripoff.p7
7dee763f566f99bd005d81beaa6a665a  cinematronics/roms/ripoff/ripoff.r7
a377b3b34e4a4ca8b0048ea4ba53c480  cinematronics/roms/tailgunner/tailg.t7
96bf9adaa9d21ab5fd5ed7fc26759486  cinematronics/roms/tailgunner/tailg.p7
df68e2d4bd5e58513cb6e3a33768483b  cinematronics/roms/tailgunner/tailg.u7
1d93d2e15e85ae30c9e2d850e3f3aae9  cinematronics/roms/tailgunner/tailg.r7
4f3f68e587bb5b39f2bb35f01ec5daff  cinematronics/roms/sundance/sundance.t7
2377ed38fedaa04ede2681ff1065a9a9  cinematronics/roms/sundance/sundance.u7
4b304388515582ac92e82a939a7821ca  cinematronics/roms/sundance/sundance.p7
4496c9ab150b55afa85f970caf2e9640  cinematronics/roms/sundance/sundance.r7
3dfca24e7374a38f7cdc10f24b902a2d  cinematronics/roms/boxingbugs/prom.d14
fa76dafba86fd1b7690887aac846927a  cinematronics/roms/boxingbugs/u4b
b556b7039236ceb8c6f69eca5848f962  cinematronics/roms/boxingbugs/u1a
4b95a48def6a1193d07c7eac0adfda1b  cinematronics/roms/boxingbugs/u1b
620ed9be3df4481841a99b01569ed887  cinematronics/roms/boxingbugs/prom.f14
b22b84ba67699a9c89c5f3038a4cfc76  cinematronics/roms/boxingbugs/u3b
6b41e19a76b1438a638f0bdfa0fdde9c  cinematronics/roms/boxingbugs/u3a
f5eab1ba5e57d8e6b5e308ef088d0b5d  cinematronics/roms/boxingbugs/prom.e14
7e55ccbdc155c07471c2f7f0e379c3e6  cinematronics/roms/boxingbugs/prom.j14
a56ae444fd24acaa307d7f35c7a57345  cinematronics/roms/boxingbugs/u4a
02eabb44868657fdcc5e7a46f3cb1b1f  cinematronics/roms/boxingbugs/prom.e8
45c1cf8aec333085756819216039bb41  cinematronics/roms/boxingbugs/prom.c14
a1c59fba503f3f45b965231748f14f0c  cinematronics/roms/boxingbugs/u2a
080179cfc180202dca138d7001b2d839  cinematronics/roms/boxingbugs/u2b
43d271a4b592a5ed48ca19185587a6c8  cinematronics/roms/solarquest/solar.u7
02ec610a964fd55dc38c7b18533d5ceb  cinematronics/roms/solarquest/solar.r7
8265fd733ab85f30a9a64b85b8255df1  cinematronics/roms/solarquest/solar.p7
cc74ce5714cb18fec1aed3b2532232e9  cinematronics/roms/solarquest/solar.t7
ad89b2040f544fed4fcf630c0f5a8aa2  cinematronics/roms/starhawk/starhawk.u7
4ad1269d473d0f570fbb241990c6bf3a  cinematronics/roms/starhawk/starhawk.r7
45c2db276808cf2576b03e057451a1ab  cinematronics/roms/starcastle/starcas.p7
d6a06b591763ff93e54a4f9c9550fbca  cinematronics/roms/starcastle/starcas.r7
f7be000d1f36c1e6e1ce07a2d9bf90de  cinematronics/roms/starcastle/starcas.u7
e66c2d1297a5dc3bb45dc8f9b38cfb03  cinematronics/roms/starcastle/starcas3.t7
92deb5273ab4fa4866e134614214bb0d  cinematronics/roms/starcastle/starcas.t7
ccc226aaab12af4ebcb720553893f1f4  cinematronics/roms/starcastle/starcas3.r7
2925b09a39635e5a6447779047196829  cinematronics/roms/starcastle/starcas3.u7
6e4e481440bcf147eb0316e75d6fd0fc  cinematronics/roms/starcastle/starcas3.p7
842268f3e812dc561e35e4b532601e61  cinematronics/roms/spacewars/spacewar.r7
016cf68c94ccc1ee8d1cdeec86b52c0c  cinematronics/roms/spacewars/spacewar.u7
3dfca24e7374a38f7cdc10f24b902a2d  cinematronics/roms/barrier/prom.d14
620ed9be3df4481841a99b01569ed887  cinematronics/roms/barrier/prom.f14
0f33cbbfd4b741375add656964628f7a  cinematronics/roms/barrier/barrier.p7
f5eab1ba5e57d8e6b5e308ef088d0b5d  cinematronics/roms/barrier/prom.e14
7e55ccbdc155c07471c2f7f0e379c3e6  cinematronics/roms/barrier/prom.j14
b88c024a979ef083394a33de3755b1bc  cinematronics/roms/barrier/barrier.t7
02eabb44868657fdcc5e7a46f3cb1b1f  cinematronics/roms/barrier/prom.e8
45c1cf8aec333085756819216039bb41  cinematronics/roms/barrier/prom.c14
f8d9e07dfc1f4935e5f204fb5908025a  cinematronics/roms/cosmicchasm/chasm.u12
1bdc8ae0c7df20e7bd591e75d7fad68b  cinematronics/roms/cosmicchasm/u10
94f7ab21a2b08da7d6ce0ed22230ace2  cinematronics/roms/cosmicchasm/u1
73d897da77182d24335644991864d3f9  cinematronics/roms/cosmicchasm/chasm.u14
f42e024487881cf96c8623f472608af7  cinematronics/roms/cosmicchasm/chasm.u4
4e112d9277bfe54b7c1d39df3f05392b  cinematronics/roms/cosmicchasm/chasm.u13
04f047b4f0ea964f80345ec626b92bfb  cinematronics/roms/cosmicchasm/chasm.u15
002735af149ae8ca1492d197cda9baec  cinematronics/roms/cosmicchasm/chasm.u16
9bcb64c65dc9f886ee4eb6cdd966171a  cinematronics/roms/cosmicchasm/chasm.u10
944be42d46be134f278a21225f69221d  cinematronics/roms/cosmicchasm/chasm.u1
3e5bdf8ad0120bf4aa5860a00580667c  cinematronics/roms/cosmicchasm/chasm.u8
2d48c68499afd7fa8bf27a0cb9a7cb39  cinematronics/roms/cosmicchasm/u2
16d69e78945cca0cb6bd38a56ada7a36  cinematronics/roms/cosmicchasm/chasm.u5
93014b7cf16f07ccbf4607809042f725  cinematronics/roms/cosmicchasm/chasm.u7
a2603045a6f9e785f956d26cd750eb9d  cinematronics/roms/cosmicchasm/chasm.u9
a2d378f774a9baba8b956c114ae302bb  cinematronics/roms/cosmicchasm/chasm.u2
f0ed682506bf1aa9426d8199234edbb7  cinematronics/roms/cosmicchasm/chasm.u3
ee328218dd2067e47238021cdf84907d  cinematronics/roms/cosmicchasm/2732.bin
8e79cc9dc3e16ef7cf0f58b04faa07e8  cinematronics/roms/cosmicchasm/chasm.u6
72bad7e180a4586f312bb90f709efcf8  cinematronics/roms/cosmicchasm/chasm.u11
e6c34b639fe16deff0f951be63076812  asteroids_sbt/035127.02
ca4f0146fb806f58a12e3e69d8fd7277  asteroids_sbt/035145.02
dd35aef4a17c271151413cbb0893b550  asteroids_sbt/035143.02
8010044e056c7a2ba3538a8c68b773d2  asteroids_sbt/035144.02

The roms used by the Atari emulator are:

  { "roms/BlackWidow/136017.101", 0x9000, 0x1000, 0 },
  { "roms/BlackWidow/136017.102", 0xa000, 0x1000, 0 },
  { "roms/BlackWidow/136017.103", 0xb000, 0x1000, 0 },
  { "roms/BlackWidow/136017.104", 0xc000, 0x1000, 0 },
  { "roms/BlackWidow/136017.105", 0xd000, 0x1000, 0 },
  { "roms/BlackWidow/136017.106", 0xe000, 0x1000, 0 },
  { "roms/BlackWidow/136017.107", 0x2800, 0x0800, 0 },
  { "roms/BlackWidow/136017.108", 0x3000, 0x1000, 0 },
  { "roms/BlackWidow/136017.109", 0x4000, 0x1000, 0 },
  { "roms/BlackWidow/136017.110", 0x5000, 0x1000, 0 },
  { "roms/Gravitar/136010.201", 0x9000, 0x1000, 0 },
  { "roms/Gravitar/136010.202", 0xa000, 0x1000, 0 },
  { "roms/Gravitar/136010.203", 0xb000, 0x1000, 0 },
  { "roms/Gravitar/136010.204", 0xc000, 0x1000, 0 },
  { "roms/Gravitar/136010.205", 0xd000, 0x1000, 0 },
  { "roms/Gravitar/136010.206", 0xe000, 0x1000, 0 },
  { "roms/Gravitar/136010.210", 0x2800, 0x0800, 0 },
  { "roms/Gravitar/136010.207", 0x3000, 0x1000, 0 },
  { "roms/Gravitar/136010.208", 0x4000, 0x1000, 0 },
  { "roms/Gravitar/136010.209", 0x5000, 0x1000, 0 },
  { "roms/SpaceDuel/136006.201", 0x4000, 0x1000, 0 },
  { "roms/SpaceDuel/136006.102", 0x5000, 0x1000, 0 },
  { "roms/SpaceDuel/136006.103", 0x6000, 0x1000, 0 },
  { "roms/SpaceDuel/136006.104", 0x7000, 0x1000, 0 },
  { "roms/SpaceDuel/136006.105", 0x8000, 0x1000, 0 },
  { "roms/SpaceDuel/136006.106", 0x2800, 0x0800, 0 },
  { "roms/SpaceDuel/136006.107", 0x3000, 0x1000, 0 },
  { "roms/Tempest/136002-133.d1", 0x9000, 0x1000, 0 },
  { "roms/Tempest/136002-134.f1", 0xa000, 0x1000, 0 },
  { "roms/Tempest/136002-235.j1", 0xb000, 0x1000, 0 },  /* or .235 */
  { "roms/Tempest/136002-136.lm1", 0xc000, 0x1000, 0 },
  { "roms/Tempest/136002-237.p1", 0xd000, 0x1000, 0 },  /* or .237 */
  { "roms/Tempest/136002-138.np3", 0x3000, 0x1000, 0 },
	  read_rom_image_to("roms/Tempest/136002-125.d7", 0, 256, 0, avg_prom);
  { "roms/Battlezone/036414.01", 0x5000, 0x0800, 0 },
  { "roms/Battlezone/036413.01", 0x5800, 0x0800, 0 },
  { "roms/Battlezone/036412.01", 0x6000, 0x0800, 0 },
  { "roms/Battlezone/036411.01", 0x6800, 0x0800, 0 },
  { "roms/Battlezone/036410.01", 0x7000, 0x0800, 0 },
  { "roms/Battlezone/036409.01", 0x7800, 0x0800, 0 },
  { "roms/Battlezone/036422.01", 0x3000, 0x0800, 0 },
  { "roms/Battlezone/036421.01", 0x3800, 0x0800, 0 },
  { "roms/RedBaron/037587-01.fh1",  0x4800, 0x0800, 0 },
  { "roms/RedBaron/037000-01.e1", 0x5000, 0x0800, 0 },
  { "roms/RedBaron/037587-01.fh1",  0x5800, 0x0800, 0x0800 },
  { "roms/RedBaron/037001.01E", 0x4800, 0x0800, 0 },?
  { "roms/RedBaron/037000-01.e1", 0x5000, 0x0800, 0 },
  { "roms/RedBaron/036999.01E", 0x5800, 0x0800, 0 },
  { "roms/RedBaron/036998-01.j1", 0x6000, 0x0800, 0 },
  { "roms/RedBaron/036997-01.k1", 0x6800, 0x0800, 0 },
  { "roms/RedBaron/036996-01.lm1", 0x7000, 0x0800, 0 },
  { "roms/RedBaron/036995-01.n1", 0x7800, 0x0800, 0 },
  { "roms/RedBaron/037006-01.bc3", 0x3000, 0x0800, 0 },
  { "roms/RedBaron/037007-01.a3", 0x3800, 0x0800, 0 },
  { "roms/LunarLander/034572.02", 0x6000, 0x0800, 0 },
  { "roms/LunarLander/034571.02", 0x6800, 0x0800, 0 },
  { "roms/LunarLander/034570.02", 0x7000, 0x0800, 0 },
  { "roms/LunarLander/034569.02", 0x7800, 0x0800, 0 },
  { "roms/LunarLander/034599.01", 0x4800, 0x0800, 0 },
  { "roms/LunarLander/034598.01", 0x5000, 0x0800, 0 },
  { "roms/LunarLander/034597.01", 0x5800, 0x0800, 0 },
  { "roms/Asteroids/035145.02", 0x6800, 0x0800, 0 },
  { "roms/Asteroids/035144.02", 0x7000, 0x0800, 0 },
  { "roms/Asteroids/035143.02", 0x7800, 0x0800, 0 },
  { "roms/Asteroids/035127.02", 0x5000, 0x0800, 0 },
  { "roms/AsteroidsDeluxe/036430.01", 0x6000, 0x0800, 0 },
  { "roms/AsteroidsDeluxe/036431.01", 0x6800, 0x0800, 0 },
  { "roms/AsteroidsDeluxe/036432.01", 0x7000, 0x0800, 0 },
  { "roms/AsteroidsDeluxe/036433.02", 0x7800, 0x0800, 0 },
  { "roms/AsteroidsDeluxe/036800.01", 0x4800, 0x0800, 0 },
  { "roms/AsteroidsDeluxe/036799.01", 0x5000, 0x0800, 0 },
  { "roms/AsteroidsDeluxe/036430.02", 0x6000, 0x0800, 0 },
  { "roms/AsteroidsDeluxe/036431.02", 0x6800, 0x0800, 0 },
  { "roms/AsteroidsDeluxe/036432.02", 0x7000, 0x0800, 0 },
  { "roms/AsteroidsDeluxe/036433.03", 0x7800, 0x0800, 0 },
  { "roms/AsteroidsDeluxe/036800.02", 0x4800, 0x0800, 0 },
  { "roms/AsteroidsDeluxe/036799.01", 0x5000, 0x0800, 0 },
  { "roms/MajorHavoc/136025.104", 0x9000, 0x4000, 0 },
  { "roms/MajorHavoc/136010.103", 0xa000, 0x4000, 0 },
  { "roms/MajorHavoc/136010.109", 0xb000, 0x4000, 0 },
  { "roms/MajorHavoc/136010.101", 0xc000, 0x4000, 0 },
  { "roms/MajorHavoc/136010.106", 0xd000, 0x4000, 0 },
  { "roms/MajorHavoc/136010.107", 0xe000, 0x4000, 0 },
  { "roms/MajorHavoc/136010.108", 0x3000, 0x4000, 0 },
  { "roms/MajorHavoc/136010.110", 0x2000, 0x2000, 0 },

battlezone/roms/Battlezone:
036174.01
036175.01
036176.01
036177.01
036178.01
036179.01
036180.01
036408.01
036409.01
036410.01
036411.01
036412.01
036413.01
036414.01
036414.02
036421.01
036422.01
readme.txt

blackwidow/roms/BlackWidow:
136017.101
136017.102
136017.103
136017.104
136017.105
136017.106
136017.107
136017.108
136017.109
136017.110
readme.txt

deluxe/roms/AsteroidsDeluxe:
034602.bin
036430.02
036431.02
036432.02
036433.03
036799.01
036800.02
readme.txt

gravitar/roms/Gravitar:
136010.201
136010.202
136010.203
136010.204
136010.205
136010.206
136010.207
136010.208
136010.209
136010.210
136010.301
136010.302
136010.303
136010.304
136010.305
136010.306
136010.309
136017.101
136017.102
136017.103
136017.104
136017.105
136017.106
136017.107
136017.108
136017.109
136017.110
readme.txt

lunar/roms/LunarLander:
034569.01
034569.02
034570.01
034570.02
034571.01
034571.02
034572.01
034572.02
034597.01
034598.01
034599.01
034602.01
034602-01.c8
readme.txt

redbaron/roms/RedBaron:
036174-01.a1
036175-01.e1
036176-01.f1
036177-01.h1
036178-01.j1
036179-01.k1
036180-01.l1
036408-01.k7
036464-01.a5
036995.01e
036995-01.n1
036996.01e
036996-01.lm1
036997.01e
036997-01.k1
036998.01e
036998-01.j1
036999.01e
037000.01e
037000-01.e1
037001.01e
037006-01.bc3
037006.01e
037007-01.a3
037007.01e
037587-01.fh1
readme.txt

spaceduel/roms/SpaceDuel:
136006.102
136006.103
136006.104
136006.105
136006.106
136006.107
136006.201
readme.txt

tempest/roms/Tempest:
136002.113
136002-113.d1
136002.114
136002-114.e1
136002.115
136002-115.f1
136002.116
136002.117
136002.118
136002-118.k1
136002.119
136002-119.lm1
136002.120
136002-120.mn1
136002.121
136002-121.p1
136002.122
136002.123
136002-123.np3
136002.124
136002-124.r3
136002.125
136002-125.d7
136002.126
136002.127
136002.128
136002.129
136002.130
136002.131
136002.132
136002-133.d1
136002-134.f1
136002-136.lm1
136002-138.np3
136002.217
136002.222
136002-235.j1
136002-237.p1
136002.316
readme.txt
