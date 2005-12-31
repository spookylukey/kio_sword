/***************************************************************************
    File:         swordutils-.cpp
    Project:      kio-sword -- An ioslave for SWORD and KDE
    Copyright:    Copyright (C) 2004-2005 Luke Plant
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

#include "swordutils.h"
#include <versekey.h>

using namespace sword;
using sword::VerseKey;
using sword::SWModule;

namespace KioSword {

	/** \brief Return true if the verse key specifies an entire book of the Bible
	*
	*/
	bool isEntireBook(const VerseKey *vk) {
		if (vk->LowerBound().Chapter() == 1 &&
		vk->LowerBound().Verse() == 1) {
		// lower bound is first verse in book
			VerseKey cp(vk->UpperBound());
			cp++;
			if (cp._compare(vk->UpperBound()) == 0 ||
			cp.Error() ||
			cp.Book() != vk->UpperBound().Book()) {
				// reached end of module, or
				// another book
				return true;
			}
		}
		return false;
	}
	
	bool isSingleChapter(const VerseKey *vk) {
		if (!vk) return false;
		
		if (vk->LowerBound().Verse() == 1 && 
		vk->LowerBound().Chapter() == vk->UpperBound().Chapter()) {
			VerseKey cp(vk->UpperBound());
			cp++;
			if (cp._compare(vk->UpperBound()) == 0 ||
			cp.Error() ||
			cp.Chapter() != vk->UpperBound().Chapter()) {
				// either reached end of module, or
				// another chapter
				return true;
			}
		}
		return false;
	}
	
	const char* textDirection(SWModule* module)
	{
		return (module->Direction(-1) == (int)sword::DIRECTION_LTR ? "ltr" : "rtl");
	}
}
