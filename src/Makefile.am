#############
# niftyconf Makefile.am
# v0.4 - (c) Daniel Hiepler (2010-2012)


EXTRA_DIST = \
	elements \
	renderer \
	ui \
	prefs \
	live-preview \
	niftyconf.h


bin_PROGRAMS = \
	niftyconf

niftyconf_SOURCES = \
	version.c \
	niftyconf.c \
	ui/ui.c \
	ui/ui-about.c \
	ui/ui-log.c \
	ui/ui-clipboard.c \
	ui/ui-info-hardware.c \
	ui/ui-setup.c \
	ui/ui-setup-props.c \
	ui/ui-setup-tree.c \
	ui/ui-setup-ledlist.c \
	elements/element-led.c \
	elements/element-chain.c \
	elements/element-tile.c \
	elements/element-hardware.c \
	elements/element-setup.c \
	renderer/renderer.c \
	renderer/renderer-setup.c \
	renderer/renderer-tile.c \
	renderer/renderer-chain.c \
	renderer/renderer-led.c \
	prefs/prefs.c \
	live-preview/live-preview.c




niftyconf_INCLUDES = \
	-I. -I..

niftyconf_CFLAGS = \
	-std=c99 -D_BSD_SOURCE \
	-DDATADIR=\"$(datadir)\" \
	-Wall -Wextra -Werror -Wno-unused-parameter \
	$(NIFTYCONF_CFLAGS)

niftyconf_LDADD = $(NIFTYCONF_LIBS)
niftyconf_LDFLAGS = $(NIFTYCONF_LIBS)




if ENABLE_GPROF
	niftyconf_CFLAGS += -pg
	niftyconf_LDFLAGS += -pg
endif

if ENABLE_DEBUG
	niftyonf_CFLAGS += -DDEBUG
endif
