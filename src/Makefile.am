
bin_PROGRAMS = \
	niftyconf      

niftyconf_SOURCES = \
	niftyconf.c \
	ui/niftyconf-ui.c \
	ui/niftyconf-about.c \
	ui/niftyconf-log.c \
	ui/niftyconf-clipboard.c \
	ui/niftyconf-info-hardware.c \
	ui/niftyconf-renderer.c \
	ui/niftyconf-setup-props.c \
	ui/niftyconf-setup-tree.c \
	ui/niftyconf-setup-ledlist.c \
	ui/niftyconf-setup.c \
	elements/niftyconf-led.c \
	elements/niftyconf-chain.c \
	elements/niftyconf-tile.c \
	elements/niftyconf-hardware.c


niftyconf_INCLUDES = \
	-I .

niftyconf_CFLAGS = \
	-DDATADIR=\""$(datadir)"\" \
	$(WARNING_CFLAGS) \
	$(NIFTYCONF_CFLAGS)

niftyconf_LDADD = $(NIFTYCONF_LIBS)
niftyconf_LDFLAGS = $(NIFTYCONF_LIBS)



