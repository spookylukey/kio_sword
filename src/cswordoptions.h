/***************************************************************************
    File:         swordoptions.h
    Project:      kio-sword -- An ioslave for SWORD and KDE
    Copyright:    Copyright (C) 2004 Luke Plant
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

#ifndef SWORDOPTIONS_H
#define SWORDOPTIONS_H

// Internal
#include "option.h"

// KDE
#include <kglobal.h>

// Qt
#include <qstring.h>


// Std C/C++
#include <vector>
#include <stdlib.h>

using std::vector;

namespace KioSword {
	class SwordOptions {
		public:
		// Need to duplicate
		Option<bool> propagate;		// Allow options set in one 'get' command to persist to later 'get' commands
		Option<bool> redWords;
		Option<bool> verseNumbers;
		Option<bool> verseLineBreaks;
		Option<QString> styleSheet;	// FIXME IMPLEMENT
	
		Option<bool> footnotes; 	// FIXME IMPLEMENT
		Option<bool> headings;  	// FIXME IMPLEMENT
		Option<bool> strongs;
		Option<bool> morph;
		Option<bool> cantillation; 
		Option<bool> hebrewVowelPoints;
		Option<bool> greekAccents;
		Option<bool> lemmas; 		// FIXME IMPLEMENT
		Option<bool> crossRefs; 	// FIXME IMPLEMENT
		Option<int> variants;
		
		Option<bool> wholeBook; 	// Allows whole book to be printed - otherwise 'Genesis' will give an index of chapters
		Option<bool> doBibleIndex;	// Create an index for for Bibles/Commentaries
		Option<bool> doFullTreeIndex;	// Create a full index for 'tree' books, not just first level
		Option<bool> doDictIndex;	// Create an index for all items in a Lexicon/Dictionary
		Option<bool> doOtherIndex;	// Create an index for other books
		
		Option<QString> defaultBible;
		Option<QString> defaultGreekStrongs;
		Option<QString> defaultHebrewStrongs;
		Option<QString> defaultGreekMorph;
		Option<QString> defaultHebrewMorph;
	
		SwordOptions();
		SwordOptions(const SwordOptions& copyFrom);
		virtual ~SwordOptions();
		
		void readFromConfig(const KConfig* config);
		void saveToConfig(KConfig* config);
		
		QMap<QString, QString> getQueryStringParams() const;
		void readFromQueryString(QMap<QString, QString> params);
		
		private:
		/** options that are read/saved in the config or propagated */
		vector<OptionBase*> m_optionList;		
		
	};
}
#endif
