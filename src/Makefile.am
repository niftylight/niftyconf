
EXTRA_DIST = \
	elements/element-chain.h \
	elements/element-tile.h \
	elements/element-hardware.h \
	elements/element-led.h \
	elements/element-setup.h \
	renderer/renderer-chain.h \
	renderer/renderer.h \
	renderer/renderer-led.h \
	renderer/renderer-setup.h \
	renderer/renderer-tile.h \
	ui/ui-about.h \
	ui/ui-setup-ledlist.h \
	ui/ui-clipboard.h \
	ui/ui-setup-props.h \
	ui/ui-info-hardware.h \
	ui/ui-setup-tree.h \
	ui/ui-log.h \
	ui/ui.h \
	ui/ui-setup.h \
	niftyconf.h


bin_PROGRAMS = \
	niftyconf

niftyconf_SOURCES = \
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
	renderer/renderer-led.c



niftyconf_INCLUDES = \
	-I. -I..

niftyconf_CFLAGS = \
	-DDATADIR=\""$(datadir)"\" \
	$(WARNING_CFLAGS) \
	$(NIFTYCONF_CFLAGS)

niftyconf_LDADD = $(NIFTYCONF_LIBS)
niftyconf_LDFLAGS = $(NIFTYCONF_LIBS)



