/***************************************************************************
    File:         ks_gbfhtml.h
    Project:      Kio-Sword -- An ioslave for SWORD and KDE
    Copyright:    Copyright (C) 2004-2005 Luke Plant
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

#include "filter.h"

#include <swbasicfilter.h>
#include <swkey.h>
#include <swmodule.h>

namespace KioSword {
	/** this filter converts GBF  text to HTML text with hrefs
	*/
	class GBFHTML : public FilterBase {
	protected:
		class MyUserData : public sword::BasicFilterUserData {
		public:
			MyUserData(const sword::SWModule *module, const sword::SWKey *key) : BasicFilterUserData(module, key) {}
			bool hasFootnotePreTag;
		};
		virtual sword::BasicFilterUserData *createUserData(const sword::SWModule *module, const sword::SWKey *key) {
			return new MyUserData(module, key);
		}
		virtual bool handleToken(sword::SWBuf &buf, const char *token, sword::BasicFilterUserData *userData);
	public:
		GBFHTML();
	};
}	
#endif
