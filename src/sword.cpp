/***************************************************************************
    File:         sword.cpp
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
#include "sword.h"
#include "swordoptions.h"
#include "ks_osishtml.h"
#include "ks_gbfhtml.h"
#include "ks_thmlhtml.h"
#include "utils.h"
#include "swordutils.h"

// Sword
#include <swmgr.h>
#include <swkey.h>
#include <versekey.h>
#include <treekey.h>
#include <treekeyidx.h>

#include <plainhtml.h>
#include <rtfhtml.h>
#include <encfiltmgr.h>


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

static const QString prev(" <li><a href='%3'>&laquo %1</a>");
static const QString next(" <li><a href='%3'>%1 &raquo</a>");
static const QString up(" <li><a href='%3'>%1 %2</a>");
static const QString genlink(" <li><a href='%2'>%1</a>");

namespace KioSword
{
	
	Sword::Sword() : 
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
	
	
	Sword::~Sword() {
		delete m_osisfilter;
		delete m_gbffilter;
		delete m_thmlfilter;
		delete m_plainfilter;
		delete m_rtffilter;
	}
	
	
 	void Sword::setOptions(const SwordOptions& options) {
		setGlobalOption("Footnotes",		(options.footnotes() ? "On" : "Off"));
		setGlobalOption("Headings",		(options.headings() ? "On" : "Off"));
		setGlobalOption("Strong's Numbers",	(options.strongs() ? "On" : "Off"));
		setGlobalOption("Morphological Tags",	(options.morph() ? "On" : "Off"));
		setGlobalOption("Hebrew Cantillation",	(options.cantillation() ? "On" : "Off"));
		setGlobalOption("Hebrew Vowel Points",	(options.hebrewVowelPoints() ? "On" : "Off"));
		setGlobalOption("Greek Accents",	(options.greekAccents() ? "On" : "Off"));
		setGlobalOption("Lemmas",		(options.lemmas() ? "On" : "Off"));
		setGlobalOption("Cross-references",	(options.crossRefs() ? "On" : "Off"));
		setGlobalOption("Words of Christ in Red",(options.redWords() ? "On" : "Off"));
	
		if (options.variants() == -1)
			setGlobalOption("Variants", "All Readings");
		else if (options.variants() == 1)
			setGlobalOption("Variants", "Secondary Readings");
		else
			setGlobalOption("Variants", "Primary Readings");
		
	}
	
	/** Return an HTML hyperlinked list of all modules,
	 * categorised, and including descriptions
	 */
	QString Sword::listModules(const SwordOptions &options) {
		QString output;
		QString temp;
		ModMap::iterator it;
		vector<const char *>::size_type i;
		SWModule *curMod;
		
		if (Modules.empty()) {
			output += ("<p>" + i18n("No modules installed!") + "</p>\n");
			return output;
		}
		
		output += QString("<div class='moduleslist'><h1>%1</h1>")
				.arg(i18n("Modules"));
			
		for  (i = 0; i < m_moduleTypes.size(); i++) {
			output += QString("<h2 class='moduletype'>%1</h2>\n"
					"<ul>\n").arg(m_moduleTypeNames[i]);
			for (it = Modules.begin(); it != Modules.end(); it++) {
				curMod = (*it).second;
				if (!strcmp(curMod->Type(), m_moduleTypes[i])) {
					output += QString("<li class='module'><a class='module' href=\"%1\">%2</a> : %3\n")
						.arg(swordUrl(curMod->Name(), options))
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
	
	QStringList Sword::moduleList() {
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
	
	Sword::ModuleType Sword::getModuleType(sword::SWModule *module) {
		ModuleType modtype;
		vector<const char *>::size_type i;
		
		modtype = GENERIC; // default
		for  (i = 0; i < m_moduleTypes.size(); i++) {
			if (!strcmp(module->Type(), m_moduleTypes[i])) {
				modtype = (ModuleType)i;
				break;
			}
		}
		return modtype;
	}
	
	
	/** Add the relevant filter to a module
	*
	* This method checks to see whether we've already added
	* the Render filter to a module.  If so, then don't
	* create or attach another, but ensure the options used
	*
	*/
	void Sword::setModuleFilter(SWModule *module, const SwordOptions* options) {
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
				if (!strcasecmp((char *)(*eit).second.c_str(), "GBF"))
					modformat = FMT_GBF;
				else if (!strcasecmp((char *)(*eit).second.c_str(), "ThML"))
					modformat = FMT_THML;
				else if (!strcasecmp((char *)(*eit).second.c_str(), "OSIS"))
					modformat = FMT_OSIS;
			}
			// FIXME -  do something with this
			if  ((eit = (*sit).second.find("Encoding")) != (*sit).second.end()) 
				encoding = (*eit).second;
			else
				encoding = (SWBuf)"";
		}
	
		
		switch (modformat)
		{
			// TODO - all filters used should subclass
			// FilterBase and know how to write
			// KioSword urls correctly using SwordOptions
			case FMT_UNKNOWN :
			case FMT_PLAIN :
				if (!m_plainfilter)
					m_plainfilter = new PLAINHTML();
				filter = m_plainfilter;	
				break;
				
			case FMT_THML :
				if (!m_thmlfilter)
					m_thmlfilter = new ThMLHTML();
				m_thmlfilter->setSwordOptions(options);
				filter = m_thmlfilter;
				break;
				
			case FMT_GBF:
				if (!m_gbffilter)
					m_gbffilter = new GBFHTML();
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
					m_osisfilter = new OSISHTML();
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
	
	void Sword::moduleQuery(const QString &modname, const QString &ref, const SwordOptions &options, QString &title, QString &output) {
		QString nav;
		
		SWModule *module = 0;
		SWKey *skey = 0;
		KeyType keyt = SWKEY;
		VerseKey *vk;
		TreeKey *tk;
		
		ModuleType modtype;	
	
		title = "A title"; // FIXME - do something better
		
		setOptions(options);
	
		// Find the module	
		module = getModule(modname.latin1());
		
		if (module == 0) { 
			output += "<p><span class='error'>" 
				+ i18n("The module '%1' could not be found.").arg(modname) 
				+ "</span></p><hr>";
			output += listModules(options);
			return;
		}
		setModuleFilter(module, &options);
		
		// Determine key type.
		skey = module->getKey();
		if (!(vk = dynamic_cast<VerseKey*>(skey))) {
			if (!(tk = dynamic_cast<TreeKey*>(skey))) {
				keyt = SWKEY;
			} else {
				keyt = TREEKEY;
			}
		} else {
			keyt = VERSEKEY;
		}
		modtype = getModuleType(module);
		
		nav += QString("<ul><li class='first'>%1 <a href='%3'>%2</a></li>")
			.arg(i18n("Module:"))
			.arg(modname)
			.arg(swordUrl(modname, options));
		
		if (keyt == VERSEKEY) {  // Should be just bibles and commentaries
			output += verseQuery(module, ref, options, modtype, nav);
		} else if (keyt == TREEKEY) {
			output += treeQuery(module, ref, options, modtype, nav);
		} else if (keyt == SWKEY) {
			output += normalQuery(module, ref, options, modtype, nav);
		}
	
		if (!nav.isEmpty()) {
			output = "<div class='navtop'>" + nav + "</ul></div><div class='text'>" +
				output +
				"</div><div class='navbottom'>" + nav + "</ul></div>";
		} else {
			output = "<div class='text'>" +  output + "</div>";
		}
	
	}
	
	QString Sword::search(const QString &modname, const QString &query, const SearchType searchType, const SwordOptions &options) {
		SWModule *module = 0;
		QString output;
		ListKey lk;
		int stype = SEARCH_WORDS;
		QString stypename;
		ModuleType modtype;
		
		// Find the module
		module = getModule(modname.latin1());
		
		if (module == 0) { 
			output += "<p><span class='error'>" 
				+ i18n("The module '%1' could not be found.").arg(modname) 
				+ "</span></p>";
			output += listModules(options);
			return output;
		}
		
		modtype = getModuleType(module);
		
		if (searchType == SEARCH_WORDS) {
			stype = -2;
			stypename = i18n("Word");
		} else if (searchType == SEARCH_PHRASE) {
			stype = -1;
			stypename = i18n("Phrase");
		} else if (searchType == SEARCH_REGEX) {
			stype = 0;
			stypename = i18n("Regular expression");
		}
		
		output += "<div  class='searchresults'><h1>" + i18n("Search results:") + "</h1>";
		output += QString("<table><tr><td>%1</td><td><b>%2</b></td></tr><tr><td>%3</td><td><b>%4</b></td></tr><tr><td>%5</td><td><b>%6</b></td></tr></table>")
				.arg(i18n("Module:"))
				.arg(modname)
				.arg(i18n("Query:"))
				.arg(query)
				.arg(i18n("Search type:"))
				.arg(stypename);
				
		
		lk = module->search(query.local8Bit(), stype);
		if (lk.Count() == 0) {
			output += "<p>" +i18n("No matches returned.");
		} else {
			output += "<p>" + i18n("1 match returned:", "%1 matches returned:", lk.Count()).arg(lk.Count());
			output += "<ul>";
			for (int i = 0; i < lk.Count(); ++i) {
				QString ref;
				ref = QString::fromLocal8Bit(lk.getElement(i)->getText());
				if (modtype == BIBLE) {
					module->setKey(lk.getElement(i));
					output += QString("<li><a href='%3'>%1</a>: %2</li>")
							.arg(ref)
							.arg(renderText(module))
							.arg(swordUrl(modname, ref, options));
					
				} else {
					output += QString("<li><a href='%2'>%1</a></li>")
							.arg(ref)
							.arg(swordUrl(modname, ref, options));
				}
			}
			output += "</ul>";
		}
		output += "</div>";
		return output;
	}
	
	QString Sword::renderText(SWModule *module) {
		return QString::fromUtf8(module->RenderText());
	}
	
	/** return formatted text for the query of a verse based module
	 * links are also return, written to navlinks
	 */
	QString Sword::verseQuery(SWModule *module, const QString &ref, const SwordOptions &options, 
								ModuleType modtype, QString &navlinks) {
		QString modname(module->Name());
		QString text;
		QString output;
		bool doindex = false;
		const char* modtextdir; // text direction of the module
		
		ListKey lk;
		VerseKey *vk = dynamic_cast<VerseKey*>(module->getKey());
		
		if (!vk) 
			return output;
		
		modtextdir = textDirection(module);
			
		vk->AutoNormalize(0);
		do { // dummy loop
			if (ref.isEmpty()) {
				doindex = true;
				break;
			}
			lk = vk->ParseVerseList(ref, "Gen1:1", true);
			if (lk.Count() == 0) {
				text += "<p class=\"error\">" + i18n("Couldn't find reference '%1'.").arg(ref) + "</p>";
				doindex = true;
				break;
			}
			char book = 0;
			char testament = 0;
			int chapter = 0;
			for (int i = 0; i < lk.Count(); ++i) {
				VerseKey *element = dynamic_cast<VerseKey*>(lk.GetElement(i));
				if (element) {
					// Multiple verses
					char err = 0;
					
					// Check whether we have an entire book selected
					// (but treat single chapter books as if they were just
					// one chapter from a book)
					if (element->UpperBound().Chapter() > 1 &&
						isEntireBook(element) && !options.wholeBook()) {
						// List books and link for viewing entire book
						SwordOptions options_wholebook(options);
						options_wholebook.wholeBook.set(true); // set just for creating the URL
						text += QString("<h2>%1</h2>"
								"<p>%4</p>"
								"<p class='chapterlist'>%5</p>"
								"<p><a href='%2'>%3</a></p>")
								.arg(element->getBookName())
								.arg(swordUrl(module->Name(), element->getBookName(), options_wholebook))
								.arg(i18n("View entire book."))
								.arg(i18n("Chapters:"))
								.arg(chapterList(modname, element, options));
					} else {
						// chapter or verse range selected
						module->Key(element->LowerBound());
						if (lk.Count() == 1) {
							// add some navigation links 
							if (isSingleChapter(element)) {
								// get link to previous chapter
								module->decrement();
								if (!module->Error()) {
									navlinks += prev
										.arg(bookChapter(module->getKey()))
										.arg(chapterLink(modname, module->getKey(), options));
								}
								// get link to book
								module->Key(element->LowerBound());
								navlinks += genlink
										.arg(bookName(element))
										.arg(bookLink(modname, element, options));
							} else {
								// get link to book
								navlinks += genlink
										.arg(bookName(element))
										.arg(bookLink(modname, element, options));
								// get link to entire chapter
								navlinks += genlink
									.arg(bookChapter(element))
									.arg(chapterLink(modname, element, options));
							}
						}
						
						// Headings plus text itself
						bool inDirectionedDiv = false;
						do  {
							VerseKey *curvk = dynamic_cast<VerseKey*>(module->getKey());
							if (curvk->Book() != book || curvk->Testament() != testament) {
								if (inDirectionedDiv)
								{
									// close it before carrying on
									text += "</div>";
									inDirectionedDiv = false;
								}
								text += "<h2>" + QString(curvk->getBookName()) + "</h2>";
								chapter = 0;
							}
							if (curvk->Chapter() != chapter) {
								if (inDirectionedDiv)
								{
									// close it before carrying on
									text += "</div>";
									inDirectionedDiv = false;
								}
								text += "<h3>" + i18n("Chapter %1").arg(curvk->Chapter()) + "</h3>";
							}
							
							if (!inDirectionedDiv) {
								text += QString("<div dir='%1'>").arg(modtextdir);
								inDirectionedDiv = true;
							}
								
							if (options.verseNumbers() && modtype == BIBLE) {
								text += QString("<a class=\"versenumber\" href=\"%2\">%1</a> ")
									  .arg(curvk->Verse())
									  .arg(swordUrl(module->Name(), module->KeyText(), options));
							}
							text += renderText(module);
							text += " ";
							if (options.verseLineBreaks())
								text += "<br />";
							book = curvk->Book();
							testament = curvk->Testament();
							chapter = curvk->Chapter();
							
							module->increment(1);
						} while (module->Key() <= element->UpperBound() && !(err = module->Error()));
						
						// Close the final div
						if (inDirectionedDiv)
						{
							text += "</div>";
							inDirectionedDiv = false;
						}
						
						if (lk.Count() == 1) {
							if (isSingleChapter(element)) {
							// add some navigation links 
								if (!err) {
									navlinks += next
										.arg(bookChapter(module->getKey()))
										.arg(chapterLink(modname, module->getKey(), options));
								}
							}
						}
					}
				} else {
					// Reset flags used by the multiple verse path
					book = 0;
					testament = 0;
					chapter = 0;
					// Single verse
					module->Key(*lk.GetElement(i));
					element = dynamic_cast<VerseKey*>(module->getKey());
					text += QString("<h3>%1</h3>").arg(module->KeyText());
					text += QString("<div dir='%1'>").arg(modtextdir);
					text += renderText(module);
					text += "</div>";
					if (lk.Count() == 1)
						navlinks += genlink
							.arg(bookChapter(element))
							.arg(chapterLink(modname, element, options));
				}
				if (i+1 != lk.Count())
					text += "<br />";
			}
		} while (false);
		
		// Title: depends on what got printed above
		if (doindex) {
			if (!text.isEmpty())  // an error message was printed
				text = QString("<h1 class=\"moduletitle\">%1</h1>").arg(module->Description()).arg(ref) 
					+ text;
		} else {
			if (modtype == COMMENTARY) { 
				text = QString("<h1 class=\"moduletitle\">%1</h1>").arg(module->Description())
					+ text;
			} else if (modtype == BIBLE) {
				text += QString("<div class=\"biblename\">(%1)</div>").arg(module->Description());
			}
		}
		output += text;
		
		if (doindex) {
			if (!text.isEmpty())
				output += "<hr>\n";
			if (options.doBibleIndex()) {
				output += "<h2>" + i18n("Books:") + "</h2>";
				output += indexBible(module, options);
			} else {
				SwordOptions options_doindex(options);
				options_doindex.doBibleIndex.set(true);
				output += QString("<p><a href=\"%1\">%2</a></p>")
						.arg(swordUrl(modname, options_doindex))
						.arg(i18n("Index of books"));
			}
		}
		return output;
	}
	
	QString Sword::treeQuery(SWModule *module, const QString &ref, const SwordOptions &options, 
								ModuleType modtype, QString &navlinks) {
		QString output;
		QString modname(module->Name());
		bool doindex;
		TreeKey *tk = dynamic_cast<TreeKey*>(module->getKey());
		
		if (!tk)
			return output;
		
		output += QString("<h1 class=\"moduletitle\">%1</h1>").arg(module->Description());	
		if (ref.isEmpty()) {
			doindex = true;
		} else {
			tk->Error(); // clear
			tk->setText(ref.local8Bit());  // FIXME is this correct?
			doindex = false;
			if (tk->Error()) {
				output += "<p class=\"error\">" + i18n("Couldn't find section '%1'.").arg(ref) + "</p>";
				output += "<hr>";
				doindex = true;
			} else {
				QString link;
				output += renderText(module);
				if (tk->previousSibling()) {
					link = QString::fromLocal8Bit(module->KeyText());
					navlinks += prev.arg(shorten(link.section('/', -1, -1), 20))
							.arg(swordUrl(modname, link, options));
					tk->nextSibling();
				}
				SWKey *saved = tk->clone();
				if (tk->parent()) {
					link = QString::fromLocal8Bit(module->KeyText());
					navlinks += up.arg(i18n("Up:"))
						.arg(shorten(link.section('/', -1, -1), 20))
						.arg(swordUrl(modname, link, options));
					tk->copyFrom(*saved);
				}
				delete saved;
				if (tk->nextSibling()) {
					link = QString::fromLocal8Bit(module->KeyText());
					navlinks += next.arg(shorten(link.section('/', -1, -1), 20))
							.arg(swordUrl(modname, link, options));
					tk->previousSibling();
				}
				if (tk->hasChildren()) {
					if (tk->firstChild());
					output += "<hr>";
					output += indexTree(module, options, false, 1);
				}
			}
		}
		
		if (doindex) {
			output += "<h2>" + i18n("Contents:") + "</h2>";
			SwordOptions options_doindex(options);
			if (options.doFullTreeIndex()) {
				options_doindex.doFullTreeIndex.set(false);
				output += indexTree(module, options, true, -1);
				output += QString("<p><a href=\"%1\">%2</a></p>")
						.arg(swordUrl(modname, options_doindex))
						.arg(i18n("View simple index"));
			} else {
				options_doindex.doFullTreeIndex.set(true);			
				output += indexTree(module, options, true, 1);
				output += QString("<p><a href=\"%1\">%2</a></p>")
						.arg(swordUrl(modname, options_doindex))
						.arg(i18n("View full index"));
			}
		}
		return output;
	}
	
	QString Sword::normalQuery(SWModule *module, const QString &ref, const SwordOptions &options, 
								ModuleType modtype, QString &navlinks) {
		QString output;
		QString modname(module->Name());
		bool doindex;
		SWKey *skey = module->getKey();
		
		output += QString("<h1 class=\"moduletitle\">%1</h1>").arg(module->Description());
		
		
		if (ref.isEmpty()) {
			doindex = true;
		} else {
			skey->Error(); // clear
			skey->setText(ref.local8Bit());
			doindex = false;
			if (skey->Error()) {
				output += "<p class=\"error\">" + QString(i18n("Couldn't find reference '%1'.")).arg(ref) + "</p>";
				output += "<hr>";
				doindex = true;
			} else {
				output += QString("<h3>%1</h3>").arg(module->KeyText());
				output += renderText(module);
				module->decrement();
				QString link;
				if (!module->Error()) {
					link = module->KeyText();
					navlinks += prev.arg(link)
							.arg(swordUrl(modname, link, options));
					module->increment();
				}
				module->increment();
				if (!module->Error()) {
					link = module->KeyText();
					navlinks += next.arg(link)
							.arg(swordUrl(modname, link, options));
					module->decrement();
				}
			}
		}
		
		if (doindex) {
			if (((modtype == LEXDICT) && options.doDictIndex()) ||
			((modtype == GENERIC) && options.doOtherIndex())) {
				output += "<h2>" + i18n("Index:") + "</h2>";
				output += indexBook(module, options);
			} else {
				
				output += QString("<form action='%1' method='get'>"
							"%2 <input type='text' name='query'>"
							"</form>")
							.arg(swordUrl(modname, options))
							.arg(i18n("Enter query term: "));
				
				SwordOptions options_doindex(options);
				options_doindex.doDictIndex.set(true);
				options_doindex.doOtherIndex.set(true);
				output += QString("<p><a href=\"%1\">%2</a></p>")
						.arg(swordUrl(modname, options_doindex))
						.arg(i18n("View complete index"));
			}
		}
		return output;
	}
	
	/** Retrieves an HTML list of all the books in the module
	* 
	* @param module The module to retrieve. Must be a Bible/commentary
	*/
	
	QString Sword::indexBible(SWModule *module, const SwordOptions& options) {
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
					.arg(swordUrl(module->Name(), vk->getBookName(), options));
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
	
	QString Sword::indexBook(SWModule *module, const SwordOptions& options) {
		QString output;
		QString ref;
		
		module->setPosition(sword::TOP);
		output += "<ul>\n";
		do {
			ref = QString::fromLocal8Bit(module->KeyText());
			output += QString("<li><a href=\"%2\">%1</a>")
					.arg(ref)
					.arg(swordUrl(module->Name(), ref, options));
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
	QString Sword::indexTree(SWModule *module, const SwordOptions& options, bool fromTop, const int depth) {
		QString output;
		QString ref;
		bool gonext;
		bool cont;
		
		TreeKey *tk = dynamic_cast<TreeKey*>(module->getKey());
		int mydepth = 1;
		
		if (!tk) 
			return output;
			
		if (fromTop) {
			tk->root();
			tk->firstChild();
		}
		
		output += "<ul>";
		cont = true;
		gonext = false;
		do {
			if (!gonext) {
				ref = QString::fromLocal8Bit(module->KeyText());
				output += QString("<li><a href=\"%2\">%1</a>\n")
						.arg(ref.section('/', -1))
						.arg(swordUrl(module->Name(), ref, options));
			}
			if (!gonext && tk->hasChildren() && (mydepth < depth || depth == -1) ) {
				if (tk->firstChild()) {
					mydepth++;
					output += "<ul>";
					cont = true;
				} else {
					cont = false;
				}
			} else {
				if (tk->nextSibling()) {
					gonext = false;
					cont = true;
				} else {
					// try to go up a level
					cont = false;
					if (mydepth > 1) {
						if (tk->parent()) {
							mydepth--;
							output += "</ul>";
							cont = true;
							gonext = true;
						}
					}
				}
			}
		} while (cont);
		
		output += "</ul>";
		return output;
		
	}
	
	QString Sword::chapterList(const QString &modname, const VerseKey *vk, const SwordOptions& options) {
		VerseKey cp(vk->LowerBound());
		QString output;
		do {
			cp.Verse(0);
			if (!output.isNull()) output += " | ";
			output += QString("<a href='%2'>%1</a>")
					.arg(cp.Chapter())
					.arg(chapterLink(modname, &cp, options));
			cp.Chapter(cp.Chapter()+1);
		} while (cp.Chapter() <= vk->UpperBound().Chapter()) ;
		return output;
	}
	
	QString Sword::chapterLink(const QString &modname, const VerseKey *vk, const SwordOptions& options) {
		return swordUrl(modname, bookChapter(vk), options);
	}
	
	QString Sword::chapterLink(const QString &modname, const SWKey *sk, const SwordOptions& options) {
		const VerseKey *vk = dynamic_cast<const VerseKey*>(sk);
		if (vk)
			return chapterLink(modname, vk, options);
		else
			return QString::null;
	}
	
	QString Sword::bookLink(const QString &modname, const VerseKey *vk, const SwordOptions& options) {
		return swordUrl(modname, bookName(vk), options);
	}
	
	QString Sword::bookLink(const QString &modname, const SWKey *sk, const SwordOptions& options) {
		const VerseKey *vk = dynamic_cast<const VerseKey*>(sk);
		if (vk)
			return bookLink(modname, vk, options);
		else
			return QString::null;
	}
	
	QString Sword::bookChapter(const VerseKey *vk)  {
		return QString("%1 %2").arg(vk->getBookName()).arg(vk->Chapter());
	}
	
	QString Sword::bookChapter(const SWKey *sk)  {
		const VerseKey *vk = dynamic_cast<const VerseKey*>(sk);
		if (vk)
			return bookChapter(vk);
		else
			return QString::null;
	}
	
	QString Sword::bookName(const VerseKey *vk)  {
		return QString(vk->getBookName());
	}
	
	QString Sword::bookName(const SWKey *sk)  {
		const VerseKey *vk = dynamic_cast<const VerseKey*>(sk);
		if (vk)
			return bookName(vk);
		else
			return QString::null;
	}
}
