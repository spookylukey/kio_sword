SUBDIRS     = data pics

INCLUDES    = $(all_includes)

kde_module_LTLIBRARIES = kio_sword.la

kio_sword_la_SOURCES = kio_sword.cpp \
	csword.cpp \
	ks_osishtml.cpp \
	ks_gbfhtml.cpp \
	utils.cpp

kio_sword_la_LIBADD = \
	-lkio \
	$(LIB_SWORD)
	
kio_sword_la_LDFLAGS = -module -avoid-version $(all_libraries) $(KDE_PLUGIN)

protocol_DATA = sword.protocol
protocoldir = $(kde_servicesdir)

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kio_sword.pot

noinst_HEADERS =  kio_sword.h csword.h cswordoptions.h ks_osishtml.h ks_gbfhtml.h utils.h
