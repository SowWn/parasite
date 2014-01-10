PROGRAM = parasite

CC = g++

DEBUG_PATH = build/debug/
RELEASE_PATH = build/release/

DEBUG_FLAGS = -pipe -Wall -DLINUX
RELEASE_FLAGS = -Wall -O3 -pipe -DLINUX

STRIP_FLAGS = -s

REVISION = 2#`svn info parasite.cpp | grep "Last Changed Rev" | sed s/Last\ Changed\ Rev:\ //g`
DATE = \"`date +"%F"`\"

parasite: parasite_client.o parasite.o md5.o lz.o
	#$(CC) parasite.o md5.o lz.o $(DEBUG_FLAGS) -o $(DEBUG_PATH)$(PROGRAM) 
	$(CC) parasite_client.o parasite.o md5.o lz.o $(RELEASE_FLAGS) -o $(RELEASE_PATH)$(PROGRAM)
	-strip $(STRIP_FLAGS) $(RELEASE_PATH)$(PROGRAM)
	-strip $(STRIP_FLAGS) $(RELEASE_PATH)$(PROGRAM).exe
	@echo "Success!"

parasite.o: parasite.cpp
	g++ -c \
		-D REVISION_VERSION=$(REVISION) \
		-D BUILD_DATE=$(DATE) \
		$(RELEASE_FLAGS) \
	parasite.cpp 

md5.o: md5.c
	g++ -c $(RELEASE_FLAGS) md5.c

lz.o: lz.c
	g++ -c $(RELEASE_FLAGS) lz.c

doc_clean: 
	make clean -Cdoc/latex

doxygen:
	doxygen

doc: doxygen doc_clean
	make pdf -Cdoc/latex

clean:
	-rm *~ \#* $(RELEASE_PATH)$(PROGRAM) $(DEBUG_PATH)$(PROGRAM) *.o
	-rm *~ \#* $(RELEASE_PATH)$(PROGRAM).exe $(DEBUG_PATH)$(PROGRAM).exe *.o

install:
	cp build/release/parasite /usr/local/bin
