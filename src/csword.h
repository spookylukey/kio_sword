/***************************************************************************
    File:         csword.h
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
#ifndef KS_SWORD_H
#define KS_SWORD_H

#include "filter.h"

#include <swmgr.h>
#include <swmodule.h>
#include <versekey.h>

#include <qstring.h>
#include <qstringlist.h>

#include <functional>
#include <vector>
#include <set>


/** Handles sword backend and prints modules 
 *
 * Sword inherits from sword::SWMgr and handles the majority of
 * the sword stuff.  It also adds functions to 'print' text from
 * a specified module and to list available modules.
 *
 * All output is in HTML, and unicode characters are not converted
 * to HTML entities (this allows the calling function to decide
 * what encoding to use in the web page e.g. utf 8 could be used
 * and no transformation would need to be done).
 *
 */

namespace KioSword {
	class SwordOptions;
	
	class Sword : public sword::SWMgr {
	
	public:
		Sword();
		virtual ~Sword();
	
		typedef enum {  SEARCH_WORDS,
				SEARCH_PHRASE,
				SEARCH_REGEX } SearchType;
				
		/** Return an HTML snippet of specified key from module
		*
		* @param module The sword module to print from
		* @param key    The text description of the key from the users URL
		* @param options Options for rendering text
		* @param title Output parameter to hold title of page
		* @param body Output parameter to hold HTML body
		*/
		void moduleQuery(const QString &module, const QString &ref, const SwordOptions &options, QString &title, QString &body);
		
		QString search(const QString &module, const QString &query, SearchType stype, const SwordOptions &options);
		
		/** Return an HTML snippet containing a hyperlinked table of modules
		*/
		QString listModules(const SwordOptions &options);
		void setOptions(const SwordOptions &options);
		QStringList moduleList();
		
		
	protected:
		typedef enum { BIBLE, COMMENTARY, LEXDICT, GENERIC, NUM_MODULE_TYPES } ModuleType ;
		enum KeyType { SWKEY, VERSEKEY, TREEKEY } ;
		void setModuleFilter(sword::SWModule *module, const SwordOptions* options);
	
		ModuleType getModuleType(sword::SWModule *module);
		QString indexBible(sword::SWModule *module, const SwordOptions& options);
		QString indexBook(sword::SWModule *module, const SwordOptions& options);
		QString indexTree(sword::SWModule *module, const SwordOptions& options, bool fromTop, const int depth = -1);
		
		QString verseQuery(sword::SWModule *module, const QString &query, const SwordOptions &options,
				ModuleType modtype, QString &navlinks);
		QString treeQuery(sword::SWModule *module, const QString &query, const SwordOptions &options,
				ModuleType modtype, QString &navlinks);
		QString normalQuery(sword::SWModule *module, const QString &query, const SwordOptions &options,
				ModuleType modtype, QString &navlinks);
		
		static QString renderText(sword::SWModule *module);
		static QString chapterList(const QString &modname, const sword::VerseKey *vk, const SwordOptions& options);
		
		static QString chapterLink(const QString &modname, const sword::VerseKey *vk, const SwordOptions& options);
		static QString chapterLink(const QString &modname, const sword::SWKey *sk, const SwordOptions& options);
		
		static QString bookLink(const QString &modname, const sword::VerseKey *vk, const SwordOptions& options);
		static QString bookLink(const QString &modname, const sword::SWKey *sk, const SwordOptions& options);
		
		static QString bookChapter(const sword::SWKey *sk);
		static QString bookChapter(const sword::VerseKey *vk);
		
		static QString bookName(const sword::SWKey *sk);
		static QString bookName(const sword::VerseKey *vk);
		
		FilterBase *m_osisfilter;
		FilterBase *m_gbffilter;
		FilterBase *m_thmlfilter;
		sword::SWFilter *m_plainfilter;
		sword::SWFilter *m_rtffilter;
		
		std::set<sword::SWModule *, std::less<sword::SWModule *> > m_modset;
		std::vector<const char *> m_moduleTypes;
		std::vector<QString> m_moduleTypeNames;
	
	};
}
#endif // KS_SWORD_H

