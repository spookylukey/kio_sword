/***************************************************************************
    File:         ks_gbfhtml.h
    Project:      kio-sword -- An ioslave for SWORD and KDE
    Copyright:    Copyright (C) 2004 Luke Plant
                  and CrossWire Bible Society 2003
                  (file based on gbfhtmlhref.h)
 ***************************************************************************/

/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#ifndef KS_GBFHTML_H
#define KS_GBFHTML_H

#include <swbasicfilter.h>

using namespace sword;

/** this filter converts GBF  text to HTML text with hrefs
 */
class ks_GBFHTML : public SWBasicFilter {
protected:
	class MyUserData : public BasicFilterUserData {
	public:
		MyUserData(const SWModule *module, const SWKey *key) : BasicFilterUserData(module, key) {}
		bool hasFootnotePreTag;
	};
	virtual BasicFilterUserData *createUserData(const SWModule *module, const SWKey *key) {
		return new MyUserData(module, key);
	}
	virtual bool handleToken(SWBuf &buf, const char *token, BasicFilterUserData *userData);
public:
	ks_GBFHTML();
};

#endif
