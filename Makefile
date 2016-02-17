#
# Makefile for the RoboTV plugin
#


# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = robotv

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char\* VERSION *=' src/robotv/robotv.h | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The directory environment:

# Use package data if installed...otherwise assume we're under the VDR source directory:
PKGCFG = $(if $(VDRDIR),$(shell pkg-config --variable=$(1) $(VDRDIR)/vdr.pc),$(shell pkg-config --variable=$(1) vdr || pkg-config --variable=$(1) ../../../vdr.pc))
LIBDIR = $(call PKGCFG,libdir)
LOCDIR = $(call PKGCFG,locdir)
PLGCFG = $(call PKGCFG,plgcfg)
CFGDIR  = $(call PKGCFG,configdir)/plugins/$(PLUGIN)
#
TMPDIR ?= /tmp

### The SQLITE compile options:

export SQLITE_CFLAGS = -DHAVE_USLEEP -DSQLITE_THREADSAFE=1 -DSQLITE_ENABLE_FTS3 -DSQLITE_ENABLE_FTS3_PARENTHESIS

### The compiler options:

export CFLAGS   = $(call PKGCFG,cflags) $(SQLITE_CFLAGS)
export CXXFLAGS = $(call PKGCFG,cxxflags) -std=gnu++11

### The version number of VDR's plugin API:

APIVERSION = $(call PKGCFG,apiversion)

### Allow user defined options to overwrite defaults:

-include $(PLGCFG)

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### The name of the shared object file:

SOFILE = libvdr-$(PLUGIN).so

### Includes and Defines (add further entries here):

INCLUDES += -I./src -I./src/vdr -I./src/sqlite3

ifdef DEBUG
INCLUDES += -DDEBUG
endif

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"' -DROBOTV_VERSION='"$(VERSION)"'

### The object files (add further files here):

OBJS = \
	src/config/config.o \
	src/db/database.o \
	src/db/storage.o \
	src/demuxer/demuxer.o \
	src/demuxer/demuxer_ADTS.o \
	src/demuxer/demuxer_LATM.o \
	src/demuxer/demuxer_AC3.o \
	src/demuxer/demuxer_H264.o \
	src/demuxer/demuxer_H265.o \
	src/demuxer/demuxer_MPEGAudio.o \
	src/demuxer/demuxer_MPEGVideo.o \
	src/demuxer/demuxer_PES.o \
	src/demuxer/demuxer_Subtitle.o \
	src/demuxer/parser.o \
	src/demuxer/streambundle.o \
	src/demuxer/demuxerbundle.o \
	src/demuxer/streaminfo.o \
	src/live/channelcache.o \
	src/live/livequeue.o \
	src/live/livestreamer.o \
	src/net/msgpacket.o \
	src/net/os-config.o \
	src/recordings/artwork.o \
	src/recordings/recordingscache.o \
	src/recordings/packetplayer.o \
	src/recordings/recplayer.o \
	src/tools/hash.o \
	src/tools/recid2uid.o \
	src/tools/urlencode.o \
	src/robotv/controllers/streamcontroller.o \
	src/robotv/controllers/recordingcontroller.o \
	src/robotv/controllers/channelcontroller.o \
	src/robotv/controllers/timercontroller.o \
	src/robotv/controllers/moviecontroller.o \
	src/robotv/controllers/logincontroller.o \
	src/robotv/robotv.o \
	src/robotv/robotvclient.o \
	src/robotv/robotvserver.o \
	src/robotv/robotvchannels.o

SQLITE_OBJS = \
	src/db/sqlite3.o

LIBS =

### The main target:

all: $(SOFILE) i18n

### Implicit rules:

src/db/sqlite3.o: src/db/sqlite3.c
	$(CC) $(CFLAGS) -fPIC -c $(DEFINES) $(INCLUDES) -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c $(DEFINES) $(INCLUDES) -o $@ $<

### Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(CXXFLAGS) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.cpp) > $@

-include $(DEPFILE)

### Internationalization (I18N):

PODIR     = po
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmo    = $(addsuffix .mo, $(foreach file, $(I18Npo), $(basename $(file))))
I18Nmsgs  = $(addprefix $(DESTDIR)$(LOCDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

%.mo: %.po
	msgfmt -c -o $@ $<

$(I18Npot): $(wildcard src/*/*.cpp)
	xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP --package-name=vdr-$(PLUGIN) --package-version=$(VERSION) --msgid-bugs-address='<see README>' -o $@ `find ./src -name *.cpp`

%.po: $(I18Npot)
	msgmerge -U --no-wrap --no-location --backup=none -q -N $@ $<
	@touch $@

$(I18Nmsgs): $(DESTDIR)$(LOCDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	install -D -m644 $< $@

i18n: $(I18Nmo) $(I18Npot)

install-i18n: $(I18Nmsgs)

### Targets:

$(SOFILE): $(OBJS) $(SQLITE_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $(OBJS) $(SQLITE_OBJS) $(LIBS) -o $@

install-lib: $(SOFILE)
	install -D $^ $(DESTDIR)$(LIBDIR)/$^.$(APIVERSION)

install-conf:
	install -Dm644 $(PLUGIN)/allowed_hosts.conf $(DESTDIR)$(CFGDIR)/allowed_hosts.conf
	install -Dm644 $(PLUGIN)/$(PLUGIN).conf $(DESTDIR)$(CFGDIR)/$(PLUGIN).conf

install: install-lib install-i18n

dist: $(I18Npo) clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -f $(OBJS) $(SQLITE_OBJS) $(DEPFILE) *.so *.tgz core* *~

astyle:
	astyle  --exclude=src/db/sqlite3.h --exclude=src/db/sqlite3ext.h --options=./astylerc -r "src/*.cpp" "src/*.h"


.PHONY: i18n astyle clean

