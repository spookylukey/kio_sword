/***************************************************************************
    File:         ks_thmlhtml.cpp
    Project:      kio-sword -- An ioslave for SWORD and KDE
    Copyright:    Copyright (C) 2004 Luke Plant
                  and CrossWire Bible Society 2001
                  (file based on thmlhtmlhref.cpp)
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

#ifndef KS_THMLHTML_H
#define KS_THMLHTML_H

#include <swbasicfilter.h>
#include <utilxml.h>
#include <swkey.h>
#include <swmodule.h>

/** this filter converts ThML text to HTML text with hrefs
 */
class ks_ThMLHTML : public sword::SWBasicFilter {
protected:
	class MyUserData : public sword::BasicFilterUserData {
	public:
		MyUserData(const sword::SWModule *module, const sword::SWKey *key);//: BasicFilterUserData(module, key) {}
		sword::SWBuf inscriptRef;
		bool SecHead;
		bool BiblicalText;
		sword::SWBuf version;
		sword::XMLTag startTag;
	};
	virtual sword::BasicFilterUserData *createUserData(const sword::SWModule *module, const sword::SWKey *key) {
		return new MyUserData(module, key);
	}
	virtual bool handleToken(sword::SWBuf &buf, const char *token, sword::BasicFilterUserData *userData);
public:
	ks_ThMLHTML();
};

#endif /* KS_THMLHTML_H */
