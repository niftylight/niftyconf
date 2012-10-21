
EXTRA_DIST = \
	elements/niftyconf-chain.h \
	elements/niftyconf-tile.h \
	elements/niftyconf-hardware.h \
	elements/niftyconf-led.h \
	elements/niftyconf-setup.h \
	renderer/niftyconf-renderer-chain.h \
	renderer/niftyconf-renderer.h \
	renderer/niftyconf-renderer-led.h \
	renderer/niftyconf-renderer-setup.h \
	renderer/niftyconf-renderer-tile.h \
	ui/niftyconf-about.h \
	ui/niftyconf-setup-ledlist.h \
	ui/niftyconf-clipboard.h \
	ui/niftyconf-setup-props.h \
	ui/niftyconf-info-hardware.h \
	ui/niftyconf-setup-tree.h \
	ui/niftyconf-log.h \
	ui/niftyconf-ui.h \
	ui/niftyconf-setup.h \
	niftyconf.h


bin_PROGRAMS = \
	niftyconf

niftyconf_SOURCES = \
	niftyconf.c \
	ui/niftyconf-ui.c \
	ui/niftyconf-about.c \
	ui/niftyconf-log.c \
	ui/niftyconf-clipboard.c \
	ui/niftyconf-info-hardware.c \
	ui/niftyconf-setup-props.c \
	ui/niftyconf-setup-tree.c \
	ui/niftyconf-setup-ledlist.c \
	elements/niftyconf-led.c \
	elements/niftyconf-chain.c \
	elements/niftyconf-tile.c \
	elements/niftyconf-hardware.c \
	elements/niftyconf-setup.c \
	renderer/niftyconf-renderer.c \
	renderer/niftyconf-renderer-setup.c \
	renderer/niftyconf-renderer-tile.c \
	renderer/niftyconf-renderer-chain.c \
	renderer/niftyconf-renderer-led.c



niftyconf_INCLUDES = \
	-I .

niftyconf_CFLAGS = \
	-DDATADIR=\""$(datadir)"\" \
	$(WARNING_CFLAGS) \
	$(NIFTYCONF_CFLAGS)

niftyconf_LDADD = $(NIFTYCONF_LIBS)
niftyconf_LDFLAGS = $(NIFTYCONF_LIBS)



