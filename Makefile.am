# Makefile.am - template for Makefile.in
# Copyright (C) 2007 Bas Wijnen <wijnen@debian.org>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# The GNU General Public License is enclosed in the distribution as a text
# file named "COPYING".

bin_PROGRAMS = pypp

pypp_SOURCES = pypp.cc

pypp_CPPFLAGS = $(CPPFLAGS) $(GLIBMM_CFLAGS) -Wall -Werror -DMY_VERSION=1 -DCOPYRIGHT_YEARS=\"2007-2013\" -DCOPYRIGHT_AUTHOR=\"Bas\ Wijnen\" -DCOPYRIGHT_EMAIL=\"wijnen@debian.org\"
pypp_CXXFLAGS = $(CXXFLAGS)
pypp_LDFLAGS = $(LDFLAGS)
pypp_LDADD = -lglibmm-2.4 -lshevek

autoclean: maintainer-clean
	rm -rf configure Makefile ltmain.sh config.guess config.sub \
		aclocal.m4 depcomp install-sh missing Makefile.in libltdl \
		INSTALL COPYING
