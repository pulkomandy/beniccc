BeNICCC: main.cpp scene1.bin DESERTD2.MOD Makefile
	g++ -O2 -g main.cpp -o $@ -lbe -lgame -ltranslation
	# FIXME no easy way to load music from resources for now :(
	xres -o $@ -a POLY:1 scene1.bin #-a MODF:2 DESERTD2.MOD
