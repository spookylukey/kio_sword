INCLUDES    = $(all_includes)

kde_module_LTLIBRARIES = kio_kio_sword.la

kio_kio_sword_la_SOURCES = kio_sword.cpp
kio_kio_sword_la_LIBADD = -lkio        
kio_kio_sword_la_LDFLAGS = -module -avoid-version $(all_libraries) $(KDE_PLUGIN)

protocol_DATA = kio_sword.protocol
protocoldir = $(kde_servicesdir)

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kio_kio_sword.pot
