#make

diserver: ae.o anet.o zmalloc.o diserver.o
	gcc $^ -o $@

clean: 
	-rm diserver *.o

.PHONY: clean
