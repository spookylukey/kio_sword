/***************************************************************************
    File:         cswordoptions.h
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

#ifndef _cswordoptions_H
#define _cswordoptions_H
struct CSwordOptions {
	bool persist;		// Allow options set in one 'get' command to persist to later 'get' commands

	bool snippet;
	bool verseNumbers;
	bool verseLineBreaks;
	bool wholeBook; 	// Allows whole book to be printed - otherwise 'Genesis' will give an index of chapters  FIXME IMPLEMENT
	QString styleSheet;

	bool footnotes; 	// FIXME IMPLEMENT
	bool headings;  	// FIXME IMPLEMENT
	bool strongs;
	bool morph;
	bool cantillation; 	// FIXME IMPLEMENT
	bool hebrewVowelPoints; // FIXME IMPLEMENT
	bool greekAccents;	// FIXME IMPLEMENT
	bool lemmas; 		// FIXME IMPLEMENT
	bool crossRefs; 	// FIXME IMPLEMENT
	bool redWords;
	int variants;
	
	bool doBibleIndex;	// Create an index for for Bibles/Commentaries
	bool doFullTreeIndex;	// Create a full index for 'tree' books, not just first level
	bool doDictIndex;	// Create an index for all items in a Lexicon/Dictionary
	bool doOtherIndex;	// Create an index for other books
	
	bool simplePage;
	// To add:


};

#endif
