#!/bin/sh

cd doc/en
meinproc
cd ../..
WANT_AUTOCONF_2_5="1" WANT_AUTOMAKE_1_6="1" make -f Makefile.cvs
