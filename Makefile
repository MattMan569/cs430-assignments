#
# HALos Makefile
#
# Copyright (c) 2019 Robert J. Hilderman.
# All rights reserved.
#

COMPILER=g++
#OPTS=-c -lm -Werror -Wunused-variable
OPTS=-c -g -lm -Werror -Wunused-variable

ifndef HAL9000_MESSAGE_TRACE
HAL9000_MESSAGE_TRACE=HAL9000_MESSAGE_TRACE_OFF
#HAL9000_MESSAGE_TRACE=HAL9000_MESSAGE_TRACE_ON
endif

ifndef HALkeyboardDriver_MESSAGE_TRACE
HALkeyboardDriver_MESSAGE_TRACE=HALkeyboardDriver_MESSAGE_TRACE_OFF
#HALkeyboardDriver_MESSAGE_TRACE=HALkeyboardDriver_MESSAGE_TRACE_ON
endif

ifndef HALdisplayDriver_MESSAGE_TRACE
HALdisplayDriver_MESSAGE_TRACE=HALdisplayDriver_MESSAGE_TRACE_OFF
#HALdisplayDriver_MESSAGE_TRACE=HALdisplayDriver_MESSAGE_TRACE_ON
endif

ifndef HALdiskDriver_MESSAGE_TRACE
HALdiskDriver_MESSAGE_TRACE=HALdiskDriver_MESSAGE_TRACE_OFF
#HALdiskDriver_MESSAGE_TRACE=HALdiskDriver_MESSAGE_TRACE_ON
endif

DEFS=-D$(HAL9000_MESSAGE_TRACE)
DEFS+=-D$(HALkeyboardDriver_MESSAGE_TRACE)
DEFS+=-D$(HALdisplayDriver_MESSAGE_TRACE)
DEFS+=-D$(HALdiskDriver_MESSAGE_TRACE)

OBJS=

INCS=

LIBS=

all:		HAL9000 HALos HALkeyboardDriver HALdisplayDriver HALdiskDriver HALshell

HAL9000:	HAL9000.o HALmemory.o
		$(COMPILER) HAL9000.o HALmemory.o -o HAL9000

HALos:		HALos.o HALcompile.o HALcull.o HALshutdownAndRestart.o HALreadyQueue.o HALqueue.o HALmemory.o HALfileTable.o HALpartitionTable.o
		$(COMPILER) HALos.o HALcompile.o HALcull.o HALshutdownAndRestart.o HALreadyQueue.o HALqueue.o HALmemory.o HALfileTable.o HALpartitionTable.o -o HALos

HALkeyboardDriver: HALkeyboardDriver.o
		$(COMPILER) HALkeyboardDriver.o -o HALkeyboardDriver

HALdisplayDriver: HALdisplayDriver.o
		$(COMPILER) HALdisplayDriver.o -o HALdisplayDriver

HALdiskDriver:	HALdiskDriver.o
		$(COMPILER) HALdiskDriver.o -o HALdiskDriver

HALshell:	HALshell.o
		$(COMPILER) HALshell.o -o HALshell

HAL9000.o:	HAL9000.cpp HAL9000.h HAL9000Init.h HALglobals.h HALmemory.h HALsignals.h
		$(COMPILER) $(OPTS) $(DEFS) $(INCS) HAL9000.cpp $(LIBS)

HALos.o: 	HALos.cpp HALos.h HALosInit.h HALreadyQueue.h HALqueue.h HALmemory.h HALpartitionTable.h HALglobals.h HALsignals.h HALprocessDescriptor.h
		$(COMPILER) $(OPTS) $(DEFS) $(INCS) HALos.cpp $(LIBS)

HALcompile.o: 	HALcompile.cpp HALcompile.h HALmemory.h
		$(COMPILER) $(OPTS) $(DEFS) $(INCS) HALcompile.cpp $(LIBS)

HALcull.o: 	HALcull.cpp HALcull.h HALreadyQueue.h HALqueue.h HALprocessDescriptor.h
		$(COMPILER) $(OPTS) $(DEFS) $(INCS) HALcull.cpp $(LIBS)

HALshutdownAndRestart.o: HALshutdownAndRestart.cpp HALshutdownAndRestart.h HALprocessDescriptor.h
		$(COMPILER) $(OPTS) $(DEFS) $(INCS) HALshutdownAndRestart.cpp $(LIBS)

HALkeyboardDriver.o: HALkeyboardDriver.cpp HALkeyboardDriver.h HALsignals.h
		$(COMPILER) $(OPTS) $(DEFS) $(INCS) HALkeyboardDriver.cpp $(LIBS)

HALdisplayDriver.o: HALdisplayDriver.cpp HALdisplayDriver.h HALsignals.h
		$(COMPILER) $(OPTS) $(DEFS) $(INCS) HALdisplayDriver.cpp $(LIBS)

HALdiskDriver.o: HALdiskDriver.cpp HALdiskDriver.h HALsignals.h
		$(COMPILER) $(OPTS) $(DEFS) $(INCS) HALdiskDriver.cpp $(LIBS)

HALshell.o:	HALshell.cpp HALshell.h HALglobals.h HALsignals.h
		$(COMPILER) $(OPTS) $(DEFS) $(INCS) HALshell.cpp $(LIBS)

HALmemory.o:	HALmemory.cpp HALmemory.h HALglobals.h
		$(COMPILER) $(OPTS) $(DEFS) $(INCS) HALmemory.cpp $(LIBS)

HALreadyQueue.o: HALreadyQueue.cpp HALreadyQueue.h HALglobals.h HALprocessDescriptor.h HALcpuSchedulingPolicyCriteria.h
		$(COMPILER) $(OPTS) $(DEFS) $(INCS) HALreadyQueue.cpp $(LIBS)

HALqueue.o:	HALqueue.cpp HALqueue.h HALprocessDescriptor.h
		$(COMPILER) $(OPTS) $(DEFS) $(INCS) HALqueue.cpp $(LIBS)

HALfileTable.o: HALfileTable.cpp HALfileTable.h HALglobals.h
		$(COMPILER) $(OPTS) $(DEFS) $(INCS) HALfileTable.cpp $(LIBS)

HALpartitionTable.o: HALpartitionTable.cpp HALpartitionTable.h HALglobals.h
		$(COMPILER) $(OPTS) $(DEFS) $(INCS) HALpartitionTable.cpp $(LIBS)

clean:
		rm -f HAL9000
		rm -f HALos
		rm -f HALkeyboardDriver
		rm -f HALdisplayDriver
		rm -f HALdiskDriver
		rm -f HALshell
		rm -f *.o

##install:
##		cp -f HAL9000 ../bin
##		cp -f HALos ../bin
##		cp -f HALkeyboardDriver ../bin
##		cp -f HALdisplayDriver ../bin
##		cp -f HALdiskDriver ../bin
##		cp -f HALshell ../bin
