/***************************************************************************
    File:         csword.cpp
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

// Mine
#include "csword.h"
#include "cswordoptions.h"
#include "ks_osishtml.h"
#include "ks_gbfhtml.h"
#include "utils.h"

// Sword
#include <swmgr.h>
#include <swfilter.h>
#include <swkey.h>
#include <versekey.h>
#include <treekey.h>
#include <treekeyidx.h>

// FIXME - remove
#include <thmlhtml.h>
#include <plainhtml.h>
#include <rtfhtml.h>
#include <encfiltmgr.h>
#include <osishtmlhref.h>
//#include <swbuf.h>

// FIXME dont need all these:

#include <swdisp.h>
#include <swfiltermgr.h>
#include <rawgbf.h>
#include <filemgr.h>
#include <utilstr.h>


// KDE
#include <klocale.h>

// QT
#include <qstring.h>
#include <qstringlist.h>

// Standard C/C++
#include <vector>
#include <cstring>
#include <set>


using std::string;
using std::vector;
using std::set;

using namespace sword;

CSword::CSword() : 
	sword::SWMgr(0, 0, true, new sword::EncodingFilterMgr(sword::ENC_UTF8)),
	m_osisfilter(0),
	m_gbffilter(0),
	m_thmlfilter(0),
	m_plainfilter(0),
	m_rtffilter(0)
{
	for (int i = 0; i < NUM_MODULE_TYPES; i++) {
		m_moduleTypes.push_back("");
		m_moduleTypeNames.push_back(QString(""));
	}
	
	m_moduleTypes[BIBLE]      = "Biblical Texts";
	m_moduleTypes[COMMENTARY] = "Commentaries";
	m_moduleTypes[LEXDICT]    = "Lexicons / Dictionaries";
	m_moduleTypes[GENERIC]    = "Generic Books";
	
	m_moduleTypeNames[BIBLE]      = i18n("Bibles");
	m_moduleTypeNames[COMMENTARY] = i18n("Commentaries");
	m_moduleTypeNames[LEXDICT]    = i18n("Lexicons & Dictionaries");
	m_moduleTypeNames[GENERIC]    = i18n("Other Books");
}

 
CSword::~CSword() {
	delete m_osisfilter;
	delete m_gbffilter;
	delete m_thmlfilter;
	delete m_plainfilter;
	delete m_rtffilter;
}
 

void CSword::setOptions(const CSwordOptions &options) {
	setGlobalOption("Footnotes",		(options.footnotes ? "On" : "Off"));
	setGlobalOption("Headings",		(options.headings ? "On" : "Off"));
	setGlobalOption("Strong's Numbers",	(options.strongs ? "On" : "Off"));
	setGlobalOption("Morphological Tags",	(options.morph ? "On" : "Off"));
	setGlobalOption("Hebrew Cantillation",	(options.cantillation ? "On" : "Off"));
	setGlobalOption("Hebrew Vowel Points",	(options.hebrewVowelPoints ? "On" : "Off"));
	setGlobalOption("Greek Accents",	(options.greekAccents ? "On" : "Off"));
	setGlobalOption("Lemmas",		(options.lemmas ? "On" : "Off"));
	setGlobalOption("Cross-references",	(options.crossRefs ? "On" : "Off"));
	setGlobalOption("Words of Christ in Red",(options.redWords ? "On" : "Off"));

	if (options.variants == -1)
		setGlobalOption("Variants", "All Readings");
	else if (options.variants == 1)
		setGlobalOption("Variants", "Secondary Readings");
	else
		setGlobalOption("Variants", "Primary Readings");
	
}

/* Return an HTML hyperlinked list of all modules,
 * categorised, and including descriptions
 */
 
QString CSword::listModules(const CSwordOptions &options) {
	QString output;
	QString temp;
	ModMap::iterator it;
	vector<const char *>::size_type i;
	SWModule *curMod;
	
	if (Modules.empty()) {
		output += ("<p>" + i18n("No modules installed!") + "</p>\n");
		return output;
	}
	
	
	output += QString("<div class='sword_moduleslist'><h1>%1</h1>")
			.arg(i18n("Modules"));
		  
	for  (i = 0; i < m_moduleTypes.size(); i++) {
		output += QString("<h2 class='sword_moduletype'>%1</h2>\n"
				  "<ul>\n").arg(m_moduleTypeNames[i]);
		for (it = Modules.begin(); it != Modules.end(); it++) {
			curMod = (*it).second;
			if (!strcmp(curMod->Type(), m_moduleTypes[i])) {
				output += QString("<li class='sword_module'><a class='sword_module' href=\"sword:/%1/\">%2</a> : %3\n")
					.arg(curMod->Name())
					.arg(curMod->Name())
					.arg(curMod->Description());
			}
		}
		output += "</ul>";
	}
	output += "</div>";
	return output;
}

/* Return a sorted list of all module names 
 *
 */
 
QStringList CSword::moduleList() {
	QStringList output;
	ModMap::iterator it;
	SWModule *curMod;
	
	for (it = Modules.begin(); it != Modules.end(); it++) {
		curMod = (*it).second;
		output += QString(curMod->Name());
	}
	output.sort();
	return output;
}


/** Add the relevant filter to a module
 *
 * This method checks to see whether we've already added
 * the Render filter to a module.  If so, then don't
 * create or attach another.
 *
 */
void CSword::setModuleFilter(SWModule *module) {
	SectionMap::iterator sit;
	ConfigEntMap::iterator eit;
	SWTextMarkup modformat = FMT_UNKNOWN;
	SWBuf encoding;
	SWFilter *filter = 0;
	
	if (m_modset.find(module) != m_modset.end())
		// already added
		return;
	
	
	// From corediatheke.cpp
	if ((sit = config->Sections.find(module->Name())) != config->Sections.end()) {
		if ((eit = (*sit).second.find("SourceType")) != (*sit).second.end()) {
			if (!stricmp((char *)(*eit).second.c_str(), "GBF"))
				modformat = FMT_GBF;
			else if (!stricmp((char *)(*eit).second.c_str(), "ThML"))
				modformat = FMT_THML;
			else if (!stricmp((char *)(*eit).second.c_str(), "OSIS"))
				modformat = FMT_OSIS;
                }
		// FIXME -  do something with this
		if  ((eit = (*sit).second.find("Encoding")) != (*sit).second.end()) 
			encoding = (*eit).second;
		else
			encoding = (SWBuf)"";
        }

	
	// Add render filter.
	// FIXME - reimplement these to use 
	//  1) our style of HTML
	//  2) our style of hrefs

	switch (modformat)
	{
		case FMT_UNKNOWN :
		case FMT_PLAIN :
			if (!m_plainfilter)
				m_plainfilter = new PLAINHTML();
			filter = m_plainfilter;	
			break;
			
		case FMT_THML :
			if (!m_thmlfilter)
				m_thmlfilter = new ThMLHTML();
			filter = m_thmlfilter;
			break;
			
		case FMT_GBF:
			if (!m_gbffilter)
				m_gbffilter = new ks_GBFHTML();
			filter = m_gbffilter;
			break;
			
		case FMT_HTML:
			break;
			
		case FMT_HTMLHREF:
			break;
		
		case FMT_RTF:
			if (!m_rtffilter)
				m_rtffilter = new RTFHTML();
			filter = m_rtffilter;
			break;
			
		case FMT_OSIS:
			if (!m_osisfilter)
		 		m_osisfilter = new ks_OSISHTML();
			filter = m_osisfilter;
			break;
		
		case FMT_WEBIF:
			break;
			
		default:
			break;
	}
	
	if (filter) {
		module->AddRenderFilter(filter);
		m_modset.insert(m_modset.begin(), module);
	}

}

QString CSword::moduleQuery(const QString &modname, const QString &ref, const CSwordOptions &options) {
	QString output;
	QString text;
	QString nav;
	
	SWModule *module = 0;
	SWKey *skey = 0;
	KeyType keyt = SWKEY;
	VerseKey *vk;
	ListKey lk;
	TreeKey *tk;
	
	bool doindex = false;
	bool error = false;
	
	vector<const char *>::size_type i;
	vector<const char *>::size_type modtype;	
	
	// Find the module	
	module = getModule(modname.latin1());
	
	if (module == 0) { 
		output += "<p><span class='sword_error'>" 
			  + i18n("The module '%1' could not be found.").arg(modname) 
			  + "</span></p><hr>";
		output += listModules(options);
		return output;
	}
	
	setModuleFilter(module);
	
	// Set key
	skey = module->getKey();
	
	// Determine key type.
	if (!(vk = dynamic_cast<VerseKey*>(skey))) {
		if (!(tk = dynamic_cast<TreeKey*>(skey))) {
			keyt = SWKEY;
		} else {
			keyt = TREEKEY;
		}
	} else {
		keyt = VERSEKEY;
	}
		
	// Determine module type
	modtype = GENERIC; // default
	for  (i = 0; i < m_moduleTypes.size(); i++) {
		if (!strcmp(module->Type(), m_moduleTypes[i])) {
			modtype = i;
			break;
		}
	}
	
	if (keyt == VERSEKEY) {  // Should be just bibles and commentaries
		//--------------------  VERSE BASED -------------------------------//
		error = false;
		do { // dummy loop
			if (ref.isEmpty()) {
				doindex = true;
				break;
			}
			lk = vk->ParseVerseList(ref, "Gen1:1", true);
			if (lk.Count() == 0) {
				text += "<p class=\"sword_error\">" + QString(i18n("Couldn't find reference '%1'.")).arg(ref) + "</p>";
				doindex = true;
				break;
			}
			// FIXME - detect whether we have referenced an entire book
			// and if so decide whether or not to view it. or an index
			for (int i = 0; i < lk.Count(); i++) {
				VerseKey *element = dynamic_cast<VerseKey*>(lk.GetElement(i));
				if (element) {
					// Multiple verses
					// FIXME insert chapter headings at relevant points
					char book;
					char testament;
					module->Key(element->LowerBound());
					do  {
						if (options.verseNumbers && modtype == BIBLE) {
							VerseKey *curvk = dynamic_cast<VerseKey*>(module->getKey());
							text += QString("<a class=\"sword_versenumber\" href=\"sword:/%1/%2\">%3</a>")
									.arg(module->Name())
									.arg(module->KeyText())
									.arg(curvk->Verse());
						}
						text += renderText(module);
						text += " ";
						if (options.verseLineBreaks) 
							text += "<br />";
							
						module->increment(1);
					} while (module->Key() <= element->UpperBound());
				} else {
					// Single verse
					module->Key(*lk.GetElement(i));
					text += renderText(module);
				}
				if (i+1 != lk.Count())
					text += "<br />";
			}
		} while (false);
		
		// Title: depends on what got printed above
		if (doindex) {
			if (!text.isEmpty())  // an error message was printed
				text = QString("<h1 class=\"sword_moduletitle\">%1</h1>").arg(module->Description()).arg(ref) 
					+ text;
		} else {
			if (modtype == COMMENTARY) { 
				text = QString("<h1 class=\"sword_moduletitle\">%1</h1><h2 class=\"sword_bibleref\">%2</h2>").arg(module->Description()).arg(ref) 
					+ text;
			} else if (modtype == BIBLE) {
				// FIXME - use a nicely formatted version of 'ref', computed above
				text = QString("<h2 class=\"sword_bibleref\">%1</h2>").arg(ref) 
					+ text 
					+ QString("<div class=\"sword_biblename\">(%1)</div>").arg(module->Description());
			}
		}
		output += text;
		
		if (doindex) {
			if (!text.isEmpty())
				output += "<hr>\n";
			if (options.doBibleIndex) {
				output += "<h2>" + i18n("Books:") + "</h2>";
				output += indexBible(module);
				//index = indexBible(module);
				//output += hrefList(index, modname);
			} else {
				output += QString("<p><a href=\"sword:/%1/?bi=1\">%2</a></p>")
						.arg(modname)
						.arg(i18n("Index of books"));
			}
		}
		
	} else if (keyt == TREEKEY) {
		//--------------------  TREE BASED -------------------------------//
		output += QString("<h1 class=\"sword_moduletitle\">%1</h1>").arg(module->Description());
		
		if (ref.isEmpty()) {
			doindex = true;
		} else {
			tk->Error(); // clear
			tk->setText(ref.utf8());  // FIXME - sync this to other usages of setText()
			doindex = false;
			if (tk->Error()) {
				output += "<p class=\"sword_error\">" + QString(i18n("Couldn't find section '%1'.")).arg(ref) + "</p>";
				output += "<hr>";
				doindex = true;
			} else {
				output += renderText(module);
				if (tk->hasChildren()) {
					if (tk->firstChild());
					output += "<hr>";
					output += indexTree(module, false, 1);
				}
			}
		}
		
		if (doindex) {
			output += "<h2>" + i18n("Contents:") + "</h2>";
			if (options.doFullTreeIndex) {	
				output += indexTree(module, true, -1);
				output += QString("<p><a href=\"sword:/%1/?fi=0\">%2</a></p>")
						.arg(modname)
						.arg(i18n("View simple index"));
			} else {
				output += indexTree(module, true, 1);
				output += QString("<p><a href=\"sword:/%1/?fi=1\">%2</a></p>")
						.arg(modname)
						.arg(i18n("View full index"));
			}
		}
		
	} else if (keyt == SWKEY) {
		//-------------------- OTHER -------------------------------//
		output += QString("<h1 class=\"sword_moduletitle\">%1</h1>").arg(module->Description());
		
		if (ref.isEmpty()) {
			doindex = true;
		} else {
			skey->Error(); // clear
			skey->setText(ref.latin1()); // FIXME - should this be .latin1() or local8bit() or utf8()?
			doindex = false;
			if (skey->Error()) {
				output += "<p class=\"sword_error\">" + QString(i18n("Couldn't find reference '%1'.")).arg(ref) + "</p>";
				output += "<hr>";
				doindex = true;
			} else {
				output += renderText(module);
			}
		}
		
		if (doindex) {
			// FIXME - condense this
			if (modtype == LEXDICT) {
				if (options.doDictIndex) {
					output += "<h2>" + i18n("Index:") + "</h2>";
					output += indexBook(module);
				} else {
					output += QString("<form action='sword:/%1/' method='get'>"
							  "%2 <input type='text' name='query'>"
							  "</form>")
							  	.arg(modname)
								.arg(i18n("Enter query term: "));
					
					output += QString("<p><a href=\"sword:/%1/?di=1\">%2</a></p>")
							.arg(modname)
							.arg(i18n("View complete index"));
				}
			} else if (modtype == GENERIC) {
				if (options.doOtherIndex) {
					output += "<h2>" + i18n("Index:") + "</h2>";
					output += indexBook(module);
				} else {
					output += QString("<p><a href=\"sword:/%1/?oi=1\">%2</a></p>")
							.arg(modname)
							.arg(i18n("View complete index"));
				}
			} 
		} 

	} else {
		// never get here
	
	}
	
	//delete skey;
	//skey = 0;
		
	return output;
}

QString CSword::renderText(SWModule *module) {
	return module->isUnicode() ?  QString::fromUtf8(module->RenderText()) 
				   :  QString::fromLatin1(module->RenderText());
}

/** Retrieves an HTML list of all the books in the module
  * 
  * @param module The module to retrieve. Must be a Bible/commentary
  */
  
QString CSword::indexBible(SWModule *module) {
	QString output;
	char book;
	char testament;
	VerseKey *vk = dynamic_cast<VerseKey*>(module->getKey());
	
	if (!vk)
		return output;
		

	module->setSkipConsecutiveLinks(true);
	vk->AutoNormalize(1);
	module->setPosition(sword::TOP);
	
	book = vk->Book();
	testament = vk->Testament();
	
	output += "<ul>\n";
	while (vk->Testament() == testament) {
		while (vk->Book() == book && !module->Error()) {
			output += QString("<li><a href=\"%2\">%1</a>\n")
				.arg(vk->getBookName())
				.arg(swordUrl(module->Name(), vk->getBookName()));
			vk->Book(++book);
		};
		// Move to new testament, if not there already.
		++testament;
		module->setPosition(sword::BOTTOM);
		book = 1;
		vk->Book(book);
	};
	output += "</ul>\n";
	module->setSkipConsecutiveLinks(false);
	return output;
}


/** Retrieves an HTML list of all the keys in a module
  * 
  * @param module The module to retrieve. Must have key type SWKey
  */

QString CSword::indexBook(SWModule *module) {
	QString output;
	QString ref;
	
	module->setPosition(sword::TOP);
	output += "<ul>\n";
	do {
		ref = module->isUnicode() ?  QString::fromUtf8(module->KeyText()) 
					  :  QString::fromLatin1(module->KeyText());
		output += QString("<li><a href=\"%2\">%1</a>")
				.arg(ref)
				.arg(swordUrl(module->Name(), ref));
		(*module)++;
	} while(!module->Error()) ;
	output += "</ul>\n";
	return output;
}



/** Return the index of a tree-key based module
  *
  * @param module  The module to scan
  * @param fromTop If true, get the index from the top level
  * @param depth   How many levels to scan, -1 for all
  */
QString CSword::indexTree(SWModule *module, bool fromTop, const int depth) {
	QString output;
	QString ref;
	bool changed;
	
	TreeKey *tk = dynamic_cast<TreeKey*>(module->getKey());
	int mydepth = 1;
	
	if (!tk) 
		return output;
		
	if (fromTop) {
		//module->setPosition(sword::TOP);
		tk->root();
		tk->firstChild();
	}
	
	output += "<ul>";
	do {
		changed = false;
		ref = module->isUnicode() ?  QString::fromUtf8(module->KeyText()) 
					  :  QString::fromLatin1(module->KeyText());
		output += QString("<li><a href=\"%2\">%1</a>\n")
				.arg(ref.section('/', -1))
				.arg(swordUrl(module->Name(), ref));

		if (tk->hasChildren() && (mydepth < depth || depth == -1) ) {
			if (tk->firstChild()) {
				mydepth++;
				output += "<ul>";
				changed = true;
			} else {
				changed = false;
			}
		} else {
			if (tk->nextSibling()) {
				changed = true;
			} else {
				// try to go up a level
				changed = false;
				if (mydepth > 1) {
					if (tk->parent()) {
						mydepth--;
						output += "</ul>";
						changed = ((tk->nextSibling()) ? true : false);
					} else {
						changed = false;
					}
				}
			}
		}
	} while (!module->Error() && changed);
	
	output += "</ul>";
	return output;
	
}

/*
QString CSword::hrefList(const QStringList &list, const QString &modname) {
	QString output;
	output += "<ul>\n";
	for (QStringList::ConstIterator it = list.begin(); it != list.end(); it++) {
		output += QString("<li><a href=\"sword:/%1/%2\">%3</a>\n")
				.arg(modname)
				.arg(*it)
				.arg(*it);
	}
	output += "</ul>\n";
	return output;
}

*/


/** Retrieves a QStringList of all the keys in a module
  * 
  * @param module The module to retrieve. Must have key type SWKey
  */
/*
QStringList CSword::indexBook(SWModule *module) {
	QStringList list;
		
	module->setPosition(sword::TOP);
	
	do {
		if (module->isUnicode())
			list.append(QString::fromUtf8(module->KeyText()));
		else
			list.append(QString::fromLatin1(module->KeyText()));
			
		(*module)++;
	} while(!module->Error()) ;
	return list;
}
*/

/** Retrieves a QStringList of all the books in the module
  * 
  * @param module The module to retrieve. Must be a Bible/commentary
  */
/*  
QStringList CSword::indexBible(SWModule *module) {
	QStringList list;
	char book;
	char testament;
	VerseKey *vk = dynamic_cast<VerseKey*>(module->getKey());
	
	if (!vk)
		return list;
		
	module->setPosition(sword::TOP);
	book = vk->Book();
	testament = vk->Testament();
	vk->AutoNormalize(1);
	while (vk->Testament() == testament) {
		while (vk->Book() == book) {
			list.append(vk->getBookName());
			vk->Book(++book);
		};
		vk->Testament(++testament);
		book = 1;
		vk->Book(book);
	};
	return list;
}
*/
