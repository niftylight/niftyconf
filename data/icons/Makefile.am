public_icons_themes = \
	hicolor

public_icons = \
	hicolor_apps_128x128_niftyconf.png \
	hicolor_apps_48x48_niftyconf.png \
	hicolor_apps_24x24_niftyconf.png \
	hicolor_apps_16x16_niftyconf.png \
	hicolor_apps_32x32_niftyconf.png \
	hicolor_apps_22x22_niftyconf.png

private_icons = \
	niftylight.png \
	niftyconf.png

EXTRA_DIST = \
	$(public_icons) \
	$(private_icons)

gtk_update_icon_cache = gtk-update-icon-cache -f -t



install-icons:
	for icon in $(public_icons); do \
		THEME=`echo $$icon | cut -d_ -f1`; \
		CONTEXT=`echo $$icon | cut -d_ -f2`; \
		SIZE=`echo $$icon | cut -d_ -f3`; \
		ICONFILE=`echo $$icon | cut -d_ -f4`; \
		mkdir -p $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done; \
	for icon in $(private_icons); do \
		mkdir -p $(DESTDIR)$(pkgdatadir)/icons; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)$(pkgdatadir)/icons/$$icon; \
	done


uninstall-icons:
	-for icon in $(public_icons); do \
	        THEME=`echo $$icon | cut -d_ -f1`; \
	        CONTEXT=`echo $$icon | cut -d_ -f2`; \
	        SIZE=`echo $$icon | cut -d_ -f3`; \
	        ICONFILE=`echo $$icon | cut -d_ -f4`; \
	        rm -f $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done; \
	for icon in $(private_icons); do \
	        rm -f $(DESTDIR)$(pkgdatadir)/icons/$$icon; \
	done

update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		for theme in $(public_icons_themes); do \
			$(gtk_update_icon_cache) $(datadir)/icons/$$theme; \
		done; \
		for theme in $(private_icons_themes); do \
			$(gtk_update_icon_cache) $(pkgdatadir)/icons; \
		done; \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		for theme in $(public_icons_themes); do \
			echo "***   $(gtk_update_icon_cache) $(datadir)/icons/$$theme"; \
		done; \
		for theme in $(private_icons_themes); do \
			echo "***   $(gtk_update_icon_cache) $(pkgdatadir)/icons"; \
		done; \
	fi


install-data-local: install-icons update-icon-cache
uninstall-local: uninstall-icons update-icon-cache

