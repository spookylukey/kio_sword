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

// Standard C/C++
#include <vector>
#include <cstring>
#include <set>

// KDE/QT
#include <qstring.h>
#include <qstringlist.h>
#include <klocale.h>

// Sword
#include <swmgr.h>
#include <swfilter.h>
#include <swkey.h>
#include <versekey.h>
#include <treekey.h>
#include <treekeyidx.h>

// FIXME - remove
#include <gbfhtmlhref.h>
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

// Mine
#include "csword.h"
#include "cswordoptions.h"
#include "ks_osishtml.h"


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
		moduleTypes.push_back("");
		moduleTypeNames.push_back(QString(""));
	}
	
	moduleTypes[BIBLE]      = "Biblical Texts";
	moduleTypes[COMMENTARY] = "Commentaries";
	moduleTypes[LEXDICT]    = "Lexicons / Dictionaries";
	moduleTypes[GENERIC]    = "Generic Books";
	
	moduleTypeNames[BIBLE]      = i18n("Bibles");
	moduleTypeNames[COMMENTARY] = i18n("Commentaries");
	moduleTypeNames[LEXDICT]    = i18n("Lexicons & Dictionaries");
	moduleTypeNames[GENERIC]    = i18n("Other Books");
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
		  
	for  (i = 0; i < moduleTypes.size(); i++) {
		output += QString("<h2 class='sword_moduletype'>%1</h2>\n"
				  "<div class='sword_modulelist'>\n"
				  "<ul>\n").arg(moduleTypeNames[i]);
		for (it = Modules.begin(); it != Modules.end(); it++) {
			curMod = (*it).second;
			if (!strcmp(curMod->Type(), moduleTypes[i])) {
				output += QString("<li class='sword_module'><a class='sword_module' href=\"sword:/%1/\">%2</a> : %3\n")
					.arg(curMod->Name())
					.arg(curMod->Name())
					.arg(curMod->Description());
			}
		}
		output += "</ul>"
			  "</div>";
	}
	output += "</div>";
	return output;
}

enum KeyType { SWKEY, VERSEKEY, TREEKEY, TREEKEYIDX };

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
				m_gbffilter = new GBFHTMLHREF();
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
	QString outputtmp;
	QString temp;
	SWModule *module = 0;
	SWKey *skey = 0;
	KeyType keyt = SWKEY;
	VerseKey *vk;
	ListKey lk;
	TreeKey *tk;
	TreeKeyIdx *tkidx;
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
	// Start with lowest levels
	if (!(vk = dynamic_cast<VerseKey*>(skey))) {
		if (!(tkidx = dynamic_cast<TreeKeyIdx*>(skey))) {
			if (!(tk = dynamic_cast<TreeKey*>(skey))) {
				keyt = SWKEY;
			} else {
				keyt = TREEKEY;
			}
		} else {
			keyt = TREEKEYIDX;
		}
	} else {
		keyt = VERSEKEY;
	}
		
	// Determine module type
	modtype = GENERIC; // default
	for  (i = 0; i < moduleTypes.size(); i++) {
		if (!strcmp(module->Type(), moduleTypes[i])) {
			modtype = i;
			break;
		}
	}
	
	
	
	if (keyt == VERSEKEY) {  // Should be just bibles and commentaries
		error = false;
		do { // dummy loop
			if (ref.isEmpty()) {
				doindex = true;
				break;
			}
			lk = vk->ParseVerseList(ref, "Gen1:1", true);
			if (lk.Count() == 0) {
				outputtmp += "<p class=\"sword_error\">" + QString(i18n("Couldn't find reference '%1'.")).arg(ref) + "</p>";
				doindex = true;
				break;
			}
			for (int i = 0; i < lk.Count(); i++) {
				VerseKey *element = dynamic_cast<VerseKey*>(lk.GetElement(i));
				if (element) {
					module->Key(element->LowerBound());
					do  {
						if (options.verseNumbers && modtype == BIBLE) {
							VerseKey *curvk = dynamic_cast<VerseKey*>(module->getKey());
							outputtmp += QString("<a class=\"sword_versenumber\" href=\"sword:/%1/%2\">%3</a>")
									.arg(module->Name())
									.arg(module->KeyText())
									.arg(curvk->Verse());
						}
						outputtmp += module->RenderText();
						outputtmp += " ";
						if (options.verseLineBreaks) 
							outputtmp += "<br />";
							
						module->increment(1);
					} while (module->Key() <= element->UpperBound());
				} else {
					module->Key(*lk.GetElement(i));
					outputtmp += module->RenderText() ;
				}
				if (i+1 != lk.Count())
					outputtmp += "<br />";
			}
		} while (false);
		
		if (doindex) { // an error message was printed
			if (!outputtmp.isEmpty())
				outputtmp = QString("<h1 class=\"sword_moduletitle\">%1</h1>").arg(module->Description()).arg(ref) 
					+ outputtmp;
		} else {
			if (modtype == COMMENTARY) { 
				outputtmp = QString("<h1 class=\"sword_moduletitle\">%1</h1><h2 class=\"sword_bibleref\">%2</h2>").arg(module->Description()).arg(ref) 
					+ outputtmp;
			} else if (modtype == BIBLE) {
				// FIXME - use a nicely formatted version of 'ref', computed above
				outputtmp = QString("<h2 class=\"sword_bibleref\">%1</h2>").arg(ref) 
					+ outputtmp 
					+ QString("<div class=\"sword_biblename\">(%1)</div>").arg(modname);
			}
		}
		output += outputtmp;
		
		if (doindex) {
			if (!outputtmp.isEmpty())
				output += "<hr>\n";
			if (options.doBibleIndex) {
				QStringList bklist = indexBible(module);
				output += "<h2>" + i18n("Books:") + "</h2>";
				output += "<ul>\n";
				for (QStringList::Iterator strit = bklist.begin(); strit != bklist.end(); strit++) {
					output += QString("<li><a href=\"sword:/%1/%2\">%3</a>\n")
							.arg(modname)
							.arg(*strit)
							.arg(*strit);
				}
				output += "</ul>\n";
			} else {
				output += QString("<p><a href=\"sword:/%1/?bi=1\">%2</a></p>")
						.arg(modname)
						.arg(i18n("Index of books"));
			}
		}
	} else if (keyt == TREEKEY) {
		output += QString("<h1 class=\"sword_moduletitle\">%1</h1>").arg(module->Description());
		// FIXME
		output += "<br><span class=\"sword_fixme\">TREEKEY - unimplemented</span><br>";
	} else if (keyt == TREEKEYIDX) {
		output += QString("<h1 class=\"sword_moduletitle\">%1</h1>").arg(module->Description());
		// FIXME
		output += "<br><span class=\"sword_fixme\">TREEKEYIDX - unimplemented</span><br>";
		
	} else if (keyt == SWKEY) {
		//output += "<br><span class=\"sword_fixme\">SWKEY - unimplemented</span><br>";
		output += QString("<h1 class=\"sword_moduletitle\">%1</h1>").arg(module->Description());
		
		if (ref.isEmpty()) {
			doindex = true;
		} else {
			skey = module->getKey();
			skey->setText(ref.latin1()); // FIXME - should this be .latin1() or local8bit() or utf8()?
			doindex = false;
			if (skey->Error()) {
				output += "<p class=\"sword_error\">" + QString(i18n("Couldn't find reference '%1'.")).arg(ref) + "</p>";
				output += "<hr>";
				doindex = true;
			} else {
				output += module->RenderText();
			}
		}
		
		if (doindex) {
			if (modtype == LEXDICT) {
				if (options.doDictIndex) {
					output += "<br><span class=\"sword_fixme\">Index for Dictionaries- unimplemented</span><br>";
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
					output += "<br><span class=\"sword_fixme\">Index for other books - unimplemented</span><br>";
				} else {
					output += QString("<p><a href=\"sword:/%1/?di=1\">%2</a></p>")
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

/** Retrieves a QStringList of all the books in the module
  * 
  * @param module The module to retrieve. Must be a Bible/commentary
  */
QStringList CSword::indexBible(SWModule *module) {
	QStringList list;
	char book;
	char testament;
	VerseKey *vk = dynamic_cast<VerseKey*>(module->getKey());
	
	if (!vk)
		return list;
	*module = sword::TOP;
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

/** Retrieves a QStringList of all the keys in a module
  * 
  * @param module The module to retrieve. Must have key type SWKey
  */
QStringList CSword::indexBook(SWModule *module) {
	QStringList list;
	char book;
	char testament;
	SWKey *sk = module->getKey();
	
	if (!sk)
		return list;
	*module = sword::TOP;
	
// 	book = vk->Book();
// 	testament = vk->Testament();
// 	vk->AutoNormalize(1);
// 	while (vk->Testament() == testament) {
// 		while (vk->Book() == book) {
// 			list.append(vk->getBookName());
// 			vk->Book(++book);
// 		};
// 		vk->Testament(++testament);
// 		book = 1;
// 		vk->Book(book);
// 	};
// 	return list;
}
