#!/bin/sh

# First create all the .html files from our
# .docbook file.  The KDE build process will
# then create makefiles that install these docs
cd doc/en
meinproc index.docbook
cd ../..
WANT_AUTOCONF_2_5="1" WANT_AUTOMAKE_1_6="1" make -f Makefile.cvs
