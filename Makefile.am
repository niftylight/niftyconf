SUBDIRS = data src


# create .deb package
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
