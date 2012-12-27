#############
# niftyconf Makefile.am
# v0.4 - (c) Daniel Hiepler (2010-2012)


SUBDIRS = data src
EXTRA_DIST = debian


# indent source & header-files
INDENT_C_ARGS=-pmt -bl -bls -cli8 -cbi0 -bli0 -cs -fca -i8 -sc -npsl -nut -npcs \
		-nsaf -nsai -cd2 -nce -ncdw -lc80 -nprs -nsaw -il0 -nbbo -bap \
		-hnl -bfda -lp -blf -ip0
INDENT_H_ARGS=-pmt -bl -bls -cli8 -cbi0 -bli0 -cs -fca -i8 -sc -npsl -nut -npcs \
		-nsaf -nsai -cd2 -nce -ncdw -l800 -lc800 -bad -nprs -nsaw \
		-di32 -hnl -nbc -nbfda -nbfde

.PHONY: indent
indent:
	@echo Indenting source-files...
	find $(top_srcdir)/src -type f -and -name "*.h" -or -name "*.h.in" -not -empty -print0 | xargs -0 $(top_srcdir)/missing --run indent $(INDENT_H_ARGS)
	find $(top_srcdir)/src -type f -and -name "*.c" -or -name "*.c.in" -not -empty -print0 | xargs -0 $(top_srcdir)/missing --run indent $(INDENT_C_ARGS)


# create .deb package
# needs dpkg-dev, debhelper
DEBTMPDIR=deb-tmp
DEBSRCDIR=$(DEBTMPDIR)/$(PACKAGE_NAME)-$(VERSION)
deb: dist-gzip
	@echo "Building Debian package..."; \
	rm --recursive --force "$(DEBTMPDIR)" ; \
	if ! [ -d debian ] ; then \
	    echo "The files needed for building a Debian package are not" \
	    "included by default in the distribution. To build a package, check" \
	        "out the project from source control."; \
	    exit 1; \
	fi; \
	mkdir -p "$(DEBTMPDIR)" && \
	mv "$(srcdir)/$(PACKAGE)-$(VERSION).tar.gz" "$(DEBTMPDIR)/$(PACKAGE)_$(VERSION).orig.tar.gz" && \
	tar --directory "$(DEBTMPDIR)/" --extract --gunzip --file "$(DEBTMPDIR)/$(PACKAGE)_$(VERSION).orig.tar.gz" && \
	cp -r "$(srcdir)/debian/"* "$(DEBSRCDIR)/debian" && \
	cp "$(srcdir)/debian/control.tmpl" "$(DEBSRCDIR)/debian/control" && \
	cd "$(DEBSRCDIR)" && \
	debuild -D -v"$(VERSION)"
