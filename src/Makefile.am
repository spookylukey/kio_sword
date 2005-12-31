SUBDIRS = data pics

INCLUDES = -I/usr/include/qt3 $(all_includes)

kde_module_LTLIBRARIES = kio_sword.la

kio_sword_la_SOURCES = kio_sword.cpp sword.cpp ks_osishtml.cpp ks_gbfhtml.cpp \
			ks_thmlhtml.cpp utils.cpp swordutils.cpp swordoptions.cpp filter.cpp template.cpp

kio_sword_la_LIBADD = -lqt-mt -lkio $(LIB_SWORD)
	
kio_sword_la_LDFLAGS = -avoid-version -module $(all_libraries) $(KDE_PLUGIN)

protocol_DATA = sword.protocol
protocoldir = $(kde_servicesdir)

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kio_sword.pot

noinst_HEADERS = kio_sword.h sword.h swordoptions.h ks_osishtml.h \
				ks_gbfhtml.h ks_thmlhtml.h utils.h swordutils.h option.h filter.h template.h
