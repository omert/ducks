maintarget = ducks

sources = Main.cpp Config.cpp Logger.cpp Utils.cpp CurrentTime.cpp
VPATH = utils main 

objects := $(patsubst %.cpp, %.o, $(sources))
dependencies := $(patsubst %.cpp, %.d, $(sources))

include_dirs = -I include -I /usr/include/mysql -I /usr/local/include/mysql++ -I /usr/local/include/boost-1_35 -I /usr/local/mysql/include/mysql -I /usr/include/boost-1_35
include_libs = -lm -L/usr/lib -L/usr/local/lib /usr/local/lib/libboost_filesystem-gcc42-mt.so -lmysqlpp -lmysqlclient /usr/local/lib/libboost_date_time-gcc42-mt.so /usr/local/lib/libboost_system-gcc42-mt.so
include Makefile.flags

CC = g++

all: ../bin/beauterror ../bin/$(maintarget) 

install: /usr/bin/$(maintarget)

../bin/$(maintarget): $(objects)
	@echo linking $(maintarget)
	@$(CC) $(linkflags) $(include_libs) $(objects) -o ../bin/$(maintarget)

$(objects): %.o: %.cpp %.d Makefile.flags
	@echo compiling $<
	@$(CC) -c $(cflags) $(include_dirs) $< -o $@ 2>&1 | ../bin/beauterror

$(dependencies): %.d: %.cpp
	@echo making dependencies for $<
	@$(SHELL) -ec '$(CC) -MM $(include_dirs) $< | sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@'

../bin/beauterror: utils/beauterror.cpp
	g++ utils/beauterror.cpp -o ../bin/beauterror

/usr/bin/$(maintarget): Makefile
	echo "#!/bin/bash" > /usr/bin/$(maintarget)
	echo $(shell pwd)/../bin/drive '$$@' >> /usr/bin/$(maintarget)
	chmod +x /usr/bin/$(maintarget)

valgrind: ../bin/$(maintarget)
	@cd..; valgrind ./bin/$(maintarget) --leak-check=full

clean:
	rm -f *.o *.d ../bin/tmz

Makefile.flags:

include $(sources:.cpp=.d)
