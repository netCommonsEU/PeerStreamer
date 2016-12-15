include utils.mak
include config.mak
   
all: exectarget

# Quick and dirty fix for enabling OperWRT cross-compilation
ifneq ($(HOSTARCH),)
CC := $(HOSTARCH)-gcc
LD := $(HOSTARCH)-ld
endif

#save external LDLIBS
LDLIBS_IN := $(LDLIBS)

CFLAGS += -g -Wall
CFLAGS += $(call cc-option, -Wdeclaration-after-statement)
CFLAGS += $(call cc-option, -Wno-switch)
CFLAGS += $(call cc-option, -Wdisabled-optimization)
CFLAGS += $(call cc-option, -Wpointer-arith)
CFLAGS += $(call cc-option, -Wredundant-decls)
CFLAGS += $(call cc-option, -Wno-pointer-sign)
CFLAGS += $(call cc-option, -Wcast-qual)
CFLAGS += $(call cc-option, -Wwrite-strings)
CFLAGS += $(call cc-option, -Wtype-limits)
CFLAGS += $(call cc-option, -Wundef)

CFLAGS += $(call cc-option, -funit-at-a-time)

LINKER = $(CC)
STATIC ?= 0

CPPFLAGS = -I$(NAPA)/include
CPPFLAGS += -I$(GRAPES)/include

# since config.h is not officially exported by GRAPES
CPPFLAGS += -I$(GRAPES)/src

CPPFLAGS += -Itransition
OBJS += transition/node_addr.o

ifdef GPROF
CFLAGS += -pg -O0
LDFLAGS += -pg
endif

ifdef DEBUG
CFLAGS += -O0
CPPFLAGS += -DDEBUG
endif
OBJS += dbg.o


ifdef DEBUGOUT
CPPFLAGS += -DDEBUGOUT
endif

LDFLAGS += -L$(GRAPES)/src
LDLIBS += -lgrapes
LIBFILES += $(GRAPES)/src/libgrapes.a
ifdef ALTO
LDFLAGS += -L$(NAPA)/ALTOclient
LDFLAGS += -L$(LIBXML2_DIR)/lib
LDLIBS += -lALTO -lxml2
CFLAGS += -pthread
LDFLAGS += -pthread
LDLIBS += $(call ld-option, -lz)
endif

NET_HELPER ?= ml
ifeq ($(NET_HELPER), ml)
OBJS += mlmonl_adapter/net_helper-ml.o
LDFLAGS += -L$(NAPA)/ml -L$(LIBEVENT_DIR)/lib
LDLIBS += -lml -lm
LIBFILES += $(NAPA)/ml/libml.a
CPPFLAGS += -Imlmonl_adapter -I$(NAPA)/ml/include -I$(LIBEVENT_DIR)/include
ifdef MONL
LDFLAGS += -L$(NAPA)/dclog -L$(NAPA)/rep -L$(NAPA)/monl -L$(NAPA)/common
LDLIBS += -lstdc++ -lmon -lrep -ldclog -lcommon
LIBFILES += $(NAPA)/monl/libmon.a
CPPFLAGS += -DMONL
ifneq ($(STATIC), 0)
LINKER=$(CXX)
endif
endif

UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
LIBEVENT=$(LIBEVENT_DIR)/lib/libevent.a
else
LIBEVENT=-levent
endif

LDLIBS += $(LIBEVENT)
LDLIBS += $(call ld-option, -lrt)
endif

ifeq ($(NET_HELPER), udp)
OBJS += $(GRAPES)/src/net_helper-udp.o
endif

ifeq ($(NET_HELPER), tcp)
OBJS += $(GRAPES)/src/net_helper-tcp.o
endif

OBJS += streaming.o
OBJS += net_helpers.o 

ifdef ALTO
OBJS += topology-ALTO.o
OBJS += config.o
else
OBJS += topology.o 
OBJS += int_bucket.o
OBJS += sparse_vector.o
OBJS += string_indexer.o
OBJS += xlweighter.o
endif

OBJS += chunk_signaling.o
OBJS += chunklock.o
OBJS += transaction.o
OBJS += ratecontrol.o
OBJS += channel.o
ifdef THREADS
CPPFLAGS += -DTHREADS
OBJS += loop-mt.o
CFLAGS += -pthread
LDFLAGS += -pthread
else
OBJS += loop.o
endif

ifdef MONL
OBJS += mlmonl_adapter/measures-monl.o
OBJS += mlmonl_adapter/config-ml.o
else
OBJS += measures.o
endif

OBJS += output-grapes.o
IO ?= grapes
ifeq ($(IO), grapes)
CFLAGS += -DIO_GRAPES
CFLAGS += -DOUTPUT_REORDER=true
OBJS += input-grapes.o
ifdef FFMPEG_DIR
CPPFLAGS += -I$(FFMPEG_DIR)
LDFLAGS += -L$(FFMPEG_DIR)/libavcodec -L$(FFMPEG_DIR)/libavformat -L$(FFMPEG_DIR)/libavutil -L$(FFMPEG_DIR)/libavcore -L$(FFMPEG_DIR)/lib
ifeq (,$(findstring mingw32,$(HOSTARCH)))
CFLAGS += -pthread
LDFLAGS += -pthread
endif
LDLIBS += -lavformat -lavcodec -lavutil
LDLIBS += $(call ld-option, -lavcore)
LDLIBS += -lm
LDLIBS += $(call ld-option, -lz)
LDLIBS += $(call ld-option, -lbz2)
LDLIBS += $(call ld-option, -lva)
endif
endif
ifeq ($(IO), chunkstream)
CFLAGS += -DIO_CHUNKSTREAM
CFLAGS += -DOUTPUT_REORDER=false
OBJS += input-chunkstream.o output-chunkstream.o
endif

EXECTARGET = streamer

EXECTARGET := $(EXECTARGET)-$(NET_HELPER)

ifdef MONL
EXECTARGET := $(EXECTARGET)-monl
endif
ifdef ALTO
EXECTARGET := $(EXECTARGET)-alto
endif
ifdef THREADS
EXECTARGET := $(EXECTARGET)-threads
endif
ifdef IO
EXECTARGET := $(EXECTARGET)-$(IO)
endif

ifeq ($(STATIC), 1)
EXECTARGET := $(EXECTARGET)-halfstatic
LDFLAGS += -Wl,-static
LDFLAGSPOST += -Wl,-Bdynamic
endif
ifeq ($(STATIC), 2)
EXECTARGET := $(EXECTARGET)-static
LDFLAGS += -static
endif

ifdef DEBUG
EXECTARGET := $(EXECTARGET)-debug
endif

ifdef RELEASE
EXECTARGET := $(EXECTARGET)-$(RELEASE)
endif

ifneq (,$(findstring mingw32,$(HOSTARCH)))
LDLIBS += -lmsvcrt -lwsock32 -lws2_32
EXECTARGET := $(EXECTARGET).exe
else
LDLIBS += $(LIBRT)
endif

#apply external LDLIBS at the end as well to resolve lining order problems
LDLIBS += $(LDLIBS_IN)
#lm might be needed again at the end
LDLIBS += $(call ld-option, -lm)

.PHONY: clean distclean exectarget test

exectarget: $(EXECTARGET)

$(EXECTARGET): $(LIBFILES)

$(EXECTARGET): $(OBJS)  $(EXECTARGET).o
	$(LINKER) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) $(LDFLAGSPOST) -o $@
	ln -sf $@ peerstreamer

$(EXECTARGET).o: streamer.o
	ln -sf streamer.o $(EXECTARGET).o

$(OBJS): config.mak

test: $(OBJS)
	make -C test all

version.h: config.mak
	./version.sh

streamer.d: version.h

GRAPES:
	git clone http://www.disi.unitn.it/~kiraly/PublicGits/GRAPES.git
	cd GRAPES; git checkout -b for-streamer-0.8.3 origin/for-streamer-0.8.3

ffmpeg:
	(wget http://ffmpeg.org/releases/ffmpeg-checkout-snapshot.tar.bz2; tar xjf ffmpeg-checkout-snapshot.tar.bz2; mv ffmpeg-checkout-20* ffmpeg) || svn checkout svn://svn.ffmpeg.org/ffmpeg/trunk ffmpeg
	cd ffmpeg; ./configure

clean:
	rm -f streamer-*
	rm -f $(GRAPES)/src/net_helper.o
	rm -f *.o
	rm -f Chunkiser/*.o
	rm -f transition/*.[od]
	rm -f mlmonl_adapter/*.[od]
	rm -f version.h
	rm -f *.d

distclean: clean
	rm -f config.mak

### Automatic generation of headers dependencies ###
%.d: %.c
	$(CC) $(CPPFLAGS) -MM -MF $@ $<

%.o: %.d

-include $(OBJS:.o=.d) streamer.d
