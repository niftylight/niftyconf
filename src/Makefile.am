
bin_PROGRAMS = \
	niftyconf      

niftyconf_SOURCES = \
	niftyconf.c \
	niftyconf-ui.c \
	niftyconf-setup.c


niftyconf_INCLUDES = \
	-I .

niftyconf_CFLAGS = \
	-DDATADIR=\""$(datadir)"\" \
	$(WARNING_CFLAGS) \
	$(NIFTYCONF_CFLAGS)

niftyconf_LDADD = $(NIFTYCONF_LIBS)
niftyconf_LDFLAGS = $(NIFTYCONF_LIBS)



