/***************************************************************************
    File:         utils.h
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

#ifndef UTILS_H
#define UTILS_H

#include <qstring.h>
namespace KioSword {

	class SwordOptions;

	typedef enum {  DEFMODULETYPE_NONE,
			DEFBIBLE,
			GREEKSTRONGS,
			HEBREWSTRONGS,
			GREEKMORPH,
			HEBREWMORPH } DefModuleType;
	
	extern const char* DEFBIBLE_STR;
	extern const char* GREEKSTRONGS_STR;
	extern const char* HEBREWSTRONGS_STR;
	extern const char* GREEKMORPH_STR;
	extern const char* HEBREWMORPH_STR;
	
	QString swordUrl(const QString& path, const SwordOptions& options);
	QString swordUrl(const QString& module, const QString& ref, const SwordOptions& options);
	QString swordUrlForPage(const QString& page, const SwordOptions& options);
	QString swordUrlForSearch(DefModuleType modType, const QString& searchQuery, const SwordOptions& options);
	QString swordUrlForSearch(DefModuleType modType, const QString& searchQuery, const SwordOptions* options);
	QString shorten(const QString& ref, uint len);

}
#endif
