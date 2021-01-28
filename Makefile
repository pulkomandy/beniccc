BeNICCC: main.cpp scene1.bin DESERTD2.MOD Makefile
	g++-x86 -O3 main.cpp -o $@ -lbe -lgame -ltranslation
	strip $@
	xres -o $@ -a POLY:1 scene1.bin -a MODF:2 DESERTD2.MOD
