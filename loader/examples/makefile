examples.d64: tst_unp.prg tst_exom.prg tst_pucr.prg tst_2bit.prg datafile.exo datafile.puc examples.seq
	makedisk examples.d64 examples.seq "LOADER EXAMPLES   LE 2A" 13
	c1541 < examd81.seq

tst_exom.prg: tst_exom.s ../loader.s ../cfg_exom.s
	dasm tst_exom.s -otst_exom.prg -v3 -p3

tst_pucr.prg: tst_pucr.s ../loader.s ../cfg_pucr.s
	dasm tst_pucr.s -otst_pucr.prg -v3 -p3

tst_unp.prg: tst_unp.s ../loader.s ../cfg_unp.s
	dasm tst_unp.s -otst_unp.prg -v3 -p3

tst_2bit.prg: tst_2bit.s ../loader.s ../cfg_2bit.s
	dasm tst_2bit.s -otst_2bit.prg -v3 -p3

datafile.exo: datafile
	exomizer level -odatafile.exo datafile

datafile.puc: datafile
	pucrunch -c0 datafile datafile.puc

