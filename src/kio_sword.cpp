/***************************************************************************
    File:         kio_sword.cpp
    Project:      kio-sword  -- An ioslave for SWORD and KDE
    Copyright:    Copyright (C) 2004 Luke Plant
 
    File info:    
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
#include "kio_sword.h"
#include "cswordoptions.h"
#include "csword.h"

// KDE
#include <kdebug.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kurl.h>

// Qt 
#include <qcstring.h>
#include <qmap.h>

// Standard C++ /C
#include <list>
#include <stdlib.h>

using namespace KIO;
using std::list;
using std::vector;

// HTML fragments that will be initialised at run time
static QString html_start_output;
static QString html_end_output;
static QString html_start_output_simple;
static QString html_end_output_simple;
static QString search_form;
static QString help_page;

// static HTML fragments -------------------------------------------------------------------------------------------------------
static const QString &html_head("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\""
				"\"http://www.w3.org/TR/html4/loose.dtd\">\n"
				"<html><head>\n"
				"<meta http-equiv='Content-Type' content='text/html; charset=utf-8'>\n"
				"<title>%2</title>\n"
				"<link rel=\"StyleSheet\" href=\"file:%1\" TYPE=\"text/css\">\n"		// kio_sword.css
				"</head>\n");
			
static const QString &page_start("<body><div class=\"swordpage\">\n"
				"<table cellpadding=\"0\" cellspacing=\"0\" border=\"0\" width=\"100%\">\n"
  				"  <tr>\n"
				"    <td><img src=\"file:%1\" alt=\"\" class=\"swordtableimg\"></td>\n"	// header_tl.png
				"    <td background=\"file:%2\">\n"						// header_t.png
				"      <table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\">\n"
				"        <tr>\n"
				"          <td width=\"100%\"></td>\n"
				"          <td><img src=\"file:%3\" class=\"swordtableimg\" alt=\"\"></td>\n" 	// header_caption.png
				"      </tr></table>\n"
				"    </td>\n"
				"    <td><img src=\"file:%4\" alt=\"\" class=\"swordtableimg\"></td>\n"	// header_tr.png
				"  </tr>\n"
				"  <tr>\n"
				"    <td background=\"file:%5\"></td>\n"					// border_l.png
				"    <td class=\"swordpage\" width=\"100%\">\n");
				
static const QString &page_start_simple("<body class='swordsimplepage'><div class='swordsimplepage'>");

static const QString &page_links(
				"      <div class=\"swordlinks\">\n"
				"        <a href=\"sword:/\">%1</a> | <a href=\"sword:/?search\">%2</a> | <a href=\"sword:/?settings&amp;previouspath=%5\">%3</a> | <a href=\"sword:/?help\">%4</a>"
				"      </div>\n");

static const QString &page_end( "      %6"									// page links
				"    </td>\n"
				"    <td background=\"file:%1\"></td>\n"				// border_r.png
				"  </tr>\n"
				"  <tr>\n"
				"    <td><img src=\"file:%2\" alt=\"\" class=\"swordtableimg\"></td>\n"	// footer_bl.png
				"    <td background=\"file:%3\">\n"						// footer_b.png
				"      <table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\">\n"
				"        <tr>\n"
				"          <td><img src=\"file:%4\" class=\"swordtableimg\" alt=\"\"></td>\n" 	// footer_sword.png
				"          <td width=\"100%\"></td>\n"
				"        </tr>\n"
				"      </table>\n"
				"    </td>\n"
				"    <td><img src=\"file:%5\" alt=\"\" class=\"swordtableimg\"></td>\n"	// footer_br.png
				"  </tr>\n"
				"</table>\n"
				"</div>\n"
				"</body>\n");

static const QString &page_end_simple("%1</div></body>"); // %1 = page links
				
static const QString &html_tail("</html>\n");

extern "C" {
	int kdemain(int argc, char **argv) {
		KInstance instance("kio_sword");
		
		kdDebug(7101) << "*** Starting kio_sword " << endl;
		
		if (argc != 4) {
			kdDebug(7101) <<
			"Usage: kio_sword  protocol domain-socket1 domain-socket2"
			<< endl;
			exit(-1);
		}
		
		SwordProtocol slave(argv[2], argv[3]);
		slave.dispatchLoop();
		
		kdDebug(7101) << "*** kio_sword Done" << endl;
		return 0;
	}
}


SwordProtocol::SwordProtocol(const QCString & pool_socket,
				     const QCString & app_socket)
  : SlaveBase("kio_sword", pool_socket, app_socket)
{
	kdDebug() << "SwordProtocol::SwordProtocol()" << endl;
	// Just set the persist option to ensure
	// proper initialisation gets done later.
	m_options.persist = false;
	m_config = KGlobal::config();
	setHTML();
}


SwordProtocol::~SwordProtocol()
{
	kdDebug() << "SwordProtocol::~SwordProtocol()" << endl;
}

void SwordProtocol::get(const KURL & url)
{
	QString modname;
	QString query;
	QString error;
	kdDebug() << "SwordProtocol::get(const KURL& url)" << endl;
	
	/*    kdDebug() << "Seconds: " << url.query() << endl;
	QString remoteServer = url.host();
	int remotePort = url.port();
	kdDebug() << "myURL: " << url.prettyURL() << endl; */
	
	// Send the mimeType as soon as it is known
	mimeType("text/html");
	
	// Set user defaults from user config file
	// (with internal defaults supplied if any options
	//  are missing from users config file)
	readUserConfig(); 
	 
	// Get options/actions from URL
	parseURL(url);
	
	if (!m_path.isEmpty() && m_path != "/") {
		modname = m_path.section('/', 0, 0, QString::SectionSkipEmpty);
		query = m_path.section('/', 1, -1, QString::SectionSkipEmpty);
	}
	
	
	// handle redirections first
	if (m_action == REDIRECT_QUERY) {
		if (!m_redirect.module.isEmpty())
			modname = m_redirect.module;
		if (!m_redirect.query.isEmpty())
			query = m_redirect.query;

		if (modname.isEmpty()) {
			switch (m_moduletype) {
				case DEFBIBLE:
					modname = m_options.defaultBible;
					error = i18n("No default bible has been specified.");
					break;
				case GREEKSTRONGS:
					modname = m_options.defaultGreekStrongs;
					error = i18n("No default Greek Strongs module has been specified.");
					break;
				case HEBREWSTRONGS:
					modname = m_options.defaultHebrewStrongs;
					error = i18n("No default Hebrew Strongs module has been specified.");
					break;
				case GREEKMORPH:
					modname = m_options.defaultGreekMorph;
					error = i18n("No default Greek morphological module has been specified.");
					break;
				case HEBREWMORPH:
					modname = m_options.defaultHebrewMorph;
					error = i18n("No default Hebrew morphological module has been specified.");
					break;
				case DEFMODULETYPE_NONE:
					error = i18n("No module specified.");
			}
		}
		
		if (modname.isEmpty()) {
			error = "<p class='swordusererror'>" + error + "</p><hr>";
			m_action = QUERY; // revert back to displaying list of modules
		} else {
			KURL newurl(url);
			// Remove anything that will trigger a redirection
			newurl.removeQueryItem("module");
			newurl.removeQueryItem("query");
			newurl.removeQueryItem("testsettings");
			newurl.removeQueryItem("modtype");
			newurl.setPath('/' + modname + '/' + query);
			redirection(newurl);
			finished();
			return;
		}
	}
	
	// need to do 'reset' before header(), as it affects formatting
	// options
	if (m_action == RESET) {
		m_options.persist = false;
		m_config->sync();
		readUserConfig();
		setHTML();
	}

	
	// Send the data
	data(header());
	
	switch (m_action) {
		case QUERY:
			if (!modname.isEmpty()) {
				data(m_sword.moduleQuery(modname, query, m_options).utf8());
			} else {
				if (!error.isEmpty()) 
					data(error.utf8());
				data(m_sword.listModules(m_options).utf8());
			}
			break;
		
		case SEARCH_FORM:
			data(searchForm().utf8());
			break;
					
		case SEARCH_QUERY:
			data(m_sword.search(m_redirect.module, m_redirect.query, m_stype, m_options).utf8());
			break;
			
		case SETTINGS_FORM:
			data(settingsForm().utf8());
			break;
			
		case SETTINGS_SAVE:
			data(saveUserConfig().utf8());
			break;
			
		case RESET:
			data(i18n("<p>Formatting options reset to user defaults.</p>").utf8());
			break;
			
		case HELP:
			data(helpPage().utf8());
			break;
			
		default:
			break;
	}
	
	data(footer());
	data(QByteArray());     // empty array means we're done sending the data
	finished();
}


void SwordProtocol::setHTML() {
	KStandardDirs* dirs = KGlobal::dirs();
	// Reduce number of file access ops by only looking up two things:
	//   - where is the style sheet
	//   - where are the images
	// This allows the user to make their own copy of the kio_sword.css
	// without having to copy all the image files as well.
	QString imgdir = dirs->findResourceDir("data", "kio_sword/header_tl.png") + "kio_sword/";
	QString cssdir = dirs->findResourceDir("data", "kio_sword/kio_sword.css") + "kio_sword/";
	html_start_output = html_head.arg(cssdir + "kio_sword.css")
			+ page_start.arg(imgdir + "header_tl.png")
				.arg(imgdir + "header_t.png")
				.arg(imgdir + "header_caption.png")
				.arg(imgdir + "header_tr.png")
				.arg(imgdir + "border_l.png");
	html_end_output = page_end.arg(imgdir + "border_r.png")
				.arg(imgdir + "footer_bl.png")
				.arg(imgdir + "footer_b.png")
				.arg(imgdir + "footer_sword.png")
				.arg(imgdir + "footer_br.png")
				.arg(page_links
					.arg(i18n("Module list"))
					.arg(i18n("Search"))
					.arg(i18n("Settings"))
					.arg(i18n("Help")))
				+ html_tail;
	
	html_start_output_simple = html_head
					.arg(cssdir + "kio_sword.css")
				   + page_start_simple;
	html_end_output_simple = page_end_simple
				   	.arg(page_links
						.arg(i18n("Module list"))
						.arg(i18n("Search"))
						.arg(i18n("Settings"))
						.arg(i18n("Help"))) 
				   + html_tail;
}

void SwordProtocol::mimetype(const KURL & /*url */ )
{
	mimeType("text/html");
	finished();
}

/* redefine data for QCStrings so we don't send the trailing
   null */
void SwordProtocol::data(const QCString& text) {
	QByteArray nonull;
	nonull.setRawData(text.data(), text.size()-1);
	SlaveBase::data(nonull);
	nonull.resetRawData(text.data(), text.size()-1);
}

void SwordProtocol::data(const QByteArray& array) {
	SlaveBase::data(array);
}

QCString SwordProtocol::header() {
	if (m_options.simplePage) 
		return html_start_output_simple.arg("Kio-Sword").utf8();
	else
		return html_start_output.arg("Kio-Sword").utf8();
}

QCString SwordProtocol::footer() {
	if (m_options.simplePage)
		return html_end_output_simple.arg(m_path).utf8();
	else
		return html_end_output.arg(m_path).utf8();
}

void SwordProtocol::readUserConfig() 
{
	// internal defaults are also defined in this function.
	
	if (!m_options.persist) {
		m_options.verseNumbers 		= m_config->readBoolEntry("VerseNumbers", 	true);
		m_options.verseLineBreaks 	= m_config->readBoolEntry("VerseLineBreaks", 	true);
		m_options.footnotes 		= m_config->readBoolEntry("Footnotes", 		false);
		m_options.headings 		= m_config->readBoolEntry("Headings", 		true);
		m_options.strongs 		= m_config->readBoolEntry("StrongsNumbers", 	false);
		m_options.morph 		= m_config->readBoolEntry("MorphologicalTags", 	false);
		m_options.cantillation 		= m_config->readBoolEntry("Cantillation", 	true);
		m_options.hebrewVowelPoints 	= m_config->readBoolEntry("HebrewVowelPoints", 	true);
		m_options.greekAccents 		= m_config->readBoolEntry("GreekAccents", 	true);
		m_options.lemmas 		= m_config->readBoolEntry("Lemmas", 		true);
		m_options.crossRefs 		= m_config->readBoolEntry("CrossReferences", 	true);
		m_options.redWords 		= m_config->readBoolEntry("RedWords", 		true);
		m_options.simplePage 		= m_config->readBoolEntry("SimplePage", 	false);
		//m_options.variants = 1;
		m_options.snippet 		= m_config->readBoolEntry("HTMLSnippet",		false);
		m_options.styleSheet 		= m_config->readEntry("StyleSheet", QString("kio_sword.css"));
		m_options.defaultBible 		= m_config->readEntry("DefaultBible");
		m_options.defaultGreekStrongs	= m_config->readEntry("DefaultGreekStrongs");
		m_options.defaultHebrewStrongs  = m_config->readEntry("DefaultHebrewStrongs");
		m_options.defaultGreekMorph	= m_config->readEntry("DefaultGreekMorph");
		m_options.defaultHebrewMorph	= m_config->readEntry("DefaultHebrewMorph");
	}
	
	// Always reset navigation options (don't read from user config)
	m_options.doBibleIndex = true;
	m_options.doDictIndex = false;
	m_options.doOtherIndex = false;
	m_options.doFullTreeIndex = false;
	m_options.wholeBook = false;
		
}

QString SwordProtocol::saveUserConfig() 
{
	QString message;
	m_config->writeEntry("VerseNumbers", 		m_options.verseNumbers);
	m_config->writeEntry("VerseLineBreaks", 	m_options.verseLineBreaks);
	m_config->writeEntry("Footnotes", 		m_options.footnotes);
	m_config->writeEntry("Headings", 		m_options.headings);
	m_config->writeEntry("StrongsNumbers", 		m_options.strongs);
	m_config->writeEntry("MorphologicalTags",	m_options.morph);
	m_config->writeEntry("Cantillation", 		m_options.cantillation);
	m_config->writeEntry("HebrewVowelPoints", 	m_options.hebrewVowelPoints);
	m_config->writeEntry("GreekAccents", 		m_options.greekAccents);
	m_config->writeEntry("Lemmas", 			m_options.lemmas);
	m_config->writeEntry("CrossReferences", 	m_options.crossRefs);
	m_config->writeEntry("RedWords", 		m_options.redWords);
	m_config->writeEntry("SimplePage", 		m_options.simplePage);
	m_config->writeEntry("HTMLSnippet", 		m_options.snippet);
	m_config->writeEntry("StyleSheet", 		m_options.styleSheet);
	m_config->writeEntry("DefaultBible", 		m_options.defaultBible);
	m_config->writeEntry("DefaultGreekStrongs", 	m_options.defaultGreekStrongs);
	m_config->writeEntry("DefaultHebrewStrongs", 	m_options.defaultHebrewStrongs);
	m_config->writeEntry("DefaultGreekMorph", 	m_options.defaultGreekMorph);
	m_config->writeEntry("DefaultHebrewMorph", 	m_options.defaultHebrewMorph);
	
	m_config->sync();
	message = "<p>" + i18n("Settings saved.") + "</p>";
	return message;
}


#define BOOL_OPTION(option, tag1, tag2) \
	if (!strcasecmp(key, tag1) ||   \
	    !strcasecmp(key, tag2)) {   \
		if (val == "0")         \
			option = false; \
		else                    \
			option = true;  \
	}

#define STRING_OPTION1(option, tag)       \
	if (!strcasecmp(key, tag)) {      \
		option = val;             \
	}
	
#define STRING_OPTION2(option, tag1, tag2) \
	if (!strcasecmp(key, tag1) ||      \
	    !strcasecmp(key, tag2)) {      \
		option = val;              \
	}

#define ENUM_OPTION(option, tag, v)    \
	if (!strcasecmp(key, tag)) {   \
		option = v;            \
	}
	
void SwordProtocol::parseURL(const KURL& url) 
{
	QMap<QString, QString>::iterator it;
	QMap<QString, QString> items = url.queryItems(KURL::CaseInsensitiveKeys);
	QString val;
	const char *key;
	
	// Reset data members that should always be 
	// retrieved from URL
	m_action 	  = QUERY; 
	m_path		  = QString::null;
	m_redirect.module = QString::null;
	m_redirect.query  = QString::null;
	m_previous.module = QString::null;
	m_previous.query  = QString::null;
	m_moduletype 	  = DEFMODULETYPE_NONE;

	if (url.hasPath()) 
		m_path = url.path();
		
	for(it = items.begin(); it != items.end(); it++) {
		key = it.key().latin1();
		val = it.data();
		
		     BOOL_OPTION(m_options.snippet, 		"sn", "snippet")
		else BOOL_OPTION(m_options.verseNumbers, 	"vn", "versenumbers")
		else BOOL_OPTION(m_options.verseLineBreaks, 	"lb", "linebreaks")
		else BOOL_OPTION(m_options.redWords, 		"rw", "redwords")
		else BOOL_OPTION(m_options.footnotes, 		"fn", "footnotes")
		else BOOL_OPTION(m_options.headings, 		"hd", "headings")
		else BOOL_OPTION(m_options.strongs, 		"st", "strongs")
		else BOOL_OPTION(m_options.morph, 		"mt", "morph")
		else BOOL_OPTION(m_options.cantillation,	"hc", "cantillation")
		else BOOL_OPTION(m_options.hebrewVowelPoints,	"hvp", "vowelpoints")
		else BOOL_OPTION(m_options.greekAccents,	"ga", "accents")
		else BOOL_OPTION(m_options.persist,		"ps", "persist")
		else BOOL_OPTION(m_options.simplePage,		"sp", "simplepage")
		else STRING_OPTION2(m_options.styleSheet,	"ss", "stylesheet")
		// navigation
		else BOOL_OPTION(m_options.wholeBook, 		"wb", "wholebook")
		else BOOL_OPTION(m_options.doBibleIndex, 	"bi", "bibleindex")
		else BOOL_OPTION(m_options.doDictIndex, 	"di", "dictindex")
		else BOOL_OPTION(m_options.doFullTreeIndex, 	"fi", "fullindex")
		else BOOL_OPTION(m_options.doOtherIndex, 	"oi", "otherindex")
		// default modules
		else STRING_OPTION1(m_options.defaultBible,		"defaultbible")
		else STRING_OPTION1(m_options.defaultGreekStrongs,	"defaultgreekstrongs")
		else STRING_OPTION1(m_options.defaultHebrewStrongs,	"defaulthebrewstrongs")
		else STRING_OPTION1(m_options.defaultGreekMorph,	"defaultgreekmorph")
		else STRING_OPTION1(m_options.defaultHebrewMorph,	"defaulthebrewmorph")
		// redirection
		else STRING_OPTION1(m_redirect.query,		"query")
		else STRING_OPTION1(m_redirect.module,		"module")
		else if (!strcasecmp(key, "modtype")) {
			if (!strcasecmp(val, "bible")) {
				m_moduletype = DEFBIBLE;
				m_action = REDIRECT_QUERY;
			} else if (!strcasecmp(val, "greekstrongs")) {
				m_moduletype = GREEKSTRONGS;
				m_action = REDIRECT_QUERY;
			} else if (!strcasecmp(val, "hebrewstrongs")) {
				m_moduletype = HEBREWSTRONGS;
				m_action = REDIRECT_QUERY;
			} else if (!strcasecmp(val, "greekmorph")) {
				m_moduletype = GREEKMORPH;
				m_action = REDIRECT_QUERY;
			} else if (!strcasecmp(val, "hebrewmorph")) {
				m_moduletype = HEBREWMORPH;
				m_action = REDIRECT_QUERY;
			}
		}
		// search
		else if (!strcasecmp(key, "stype")) {
			if (!strcasecmp(val, "words")) {
				m_stype = CSword::SEARCH_WORDS;
			} else if (!strcasecmp(val, "phrase")) {
				m_stype = CSword::SEARCH_PHRASE;
			} else if (!strcasecmp(val, "regex")) {
				m_stype = CSword::SEARCH_REGEX;
			} else {
				m_stype = CSword::SEARCH_WORDS;
			}
		}
		// Search type
		else ENUM_OPTION(m_action, "reset",        RESET)
		
		// Actions
		else ENUM_OPTION(m_action, "reset",        RESET)
		else ENUM_OPTION(m_action, "help",         HELP)
		else ENUM_OPTION(m_action, "search",       SEARCH_FORM)
		else ENUM_OPTION(m_action, "searchq",      SEARCH_QUERY)
		else ENUM_OPTION(m_action, "settings",     SETTINGS_FORM)
		else ENUM_OPTION(m_action, "savesettings", SETTINGS_SAVE)
		else ENUM_OPTION(m_action, "testsettings", REDIRECT_QUERY)
		
		else if (!strcasecmp(key, "previouspath")) {
			m_previous.module = val.section('/', 0, 0, QString::SectionSkipEmpty);
			m_previous.query = val.section('/', 1, -1, QString::SectionSkipEmpty);
		}
	}
	
	// Once all the URL is parsed:	
	if ((m_action == QUERY) && (
		!m_redirect.query.isEmpty() || !m_redirect.module.isEmpty()))
		m_action = REDIRECT_QUERY;
}
#undef BOOL_OPTION
#undef STRING_OPTION1
#undef STRING_OPTION2
#undef ENUM_OPTION

QString settingsBooleanOptionRow(const QString &description, const char *name, const char *shortname, bool value) {
	static const QString boolean_option_row(
				"<tr><td>%1</td><td><nobr><input type='radio' name='%2' value='1' %3>%4 &nbsp;&nbsp;<input type='radio'  name='%2' value='0' %5>%6</nobr></td><td>%7</td><td>%2, %8</td></tr>");
	QString output = boolean_option_row
			.arg(description)
			.arg(shortname)
			.arg(shortname)
			.arg(shortname)
			.arg(value ? "checked" : "")
			.arg(i18n("On"))
			.arg(value ? "" : "checked")
			.arg(i18n("Off"))
			.arg(i18n("Boolean"))
			.arg(name);
	return output;
}
			
QString SwordProtocol::settingsForm() {
	QString output;
	QStringList modules;
	QStringList::Iterator it;
	QString temp;
			
	static const QString separator_row(
				"<tr><td class='swordsettings_category' colspan='4'>%1</td></tr>");

	static const QString module_option_row(
				"<tr><td>%1</td><td colspan='2'><select name='%2'>%3</select></td><td>%4</td></tr>");
	// Start output
	output += i18n("<h1>Settings</h1>"
			"<p>Select the settings using the form below.  Use the 'Save settings' button to "
			" save these settings to your own configuration file.  'Test settings' will return "
			" you to the previous page with the options you have specified. <br>");
			
	output += QString(
			"<form action='sword:/' method='GET'>"
			"<table class='swordsettings' border=0 cellspacing=0>"
			"  <tr><th>%1</th><th>%2</th><th>%3</th><th>%4</th></tr>")
			.arg(i18n("Description"))
			.arg(i18n("Value"))
			.arg(i18n("Type"))
			.arg(i18n("URL parameter"));
	
	output += separator_row.arg(i18n("Formatting options"));
	output += settingsBooleanOptionRow(i18n("Display verse numbers for Bible modules"), "versenumbers", "vn", m_options.verseNumbers);
	output += settingsBooleanOptionRow(i18n("Insert line breaks between Bible verses"), "linebreaks", "lb", m_options.verseLineBreaks);
	output += settingsBooleanOptionRow(i18n("Include footnotes."), "footnotes", "fn", m_options.footnotes);
	output += settingsBooleanOptionRow(i18n("Words of Christ in red."), "redwords", "rw", m_options.redWords);
	output += settingsBooleanOptionRow(i18n("Display strongs numbers (for Bibles that include them)."), "strongs", "st", m_options.strongs);
	output += settingsBooleanOptionRow(i18n("Display morphological tags (for Bibles that include them)."), "morph", "mt", m_options.morph);
	
	output += separator_row.arg(i18n("Language specific"));
	output += settingsBooleanOptionRow(i18n("Use Hebrew cantillation."), "cantillation", "hc", m_options.cantillation);
	output += settingsBooleanOptionRow(i18n("Show Hebrew vowel points."), "vowelpoints", "hvp", m_options.hebrewVowelPoints);
	output += settingsBooleanOptionRow(i18n("Show Greek accents."), "accents", "ga", m_options.greekAccents);
	
	output += separator_row.arg(i18n("Navigation options"));
	output += settingsBooleanOptionRow(i18n("Display the whole book when a Bible book is selected, instead of an index of the chapters"),	
								"wholebook", "wb", m_options.wholeBook);
	output += settingsBooleanOptionRow(i18n("Display an the booklist for bibles if no book is requested"),	
								"bibleindex", "di", m_options.doBibleIndex);
	output += settingsBooleanOptionRow(i18n("Display an index for dictionaries if no entry is requested"),	
								"dictindex", "di", m_options.doDictIndex);
	output += settingsBooleanOptionRow(i18n("Display an index for other books if no entry is request"),	
								"otherindex", "oi", m_options.doOtherIndex);
	output += settingsBooleanOptionRow(i18n("Display a full index for books that have a multiple level index, instead of just the first level"),	
								"fullindex", "fi", m_options.doFullTreeIndex);
	
	
	output += separator_row.arg(i18n("Default modules"));
	modules = m_sword.moduleList();
	
	vector<QString> dm_desc, dm_names, dm_values;
	vector<QString>::size_type i;
	dm_desc.push_back(i18n("Default Bible"));
	dm_names.push_back(QString("defaultbible"));
	dm_values.push_back(m_options.defaultBible);
	
	dm_desc.push_back(i18n("Default Greek Strong's Lexicon"));
	dm_names.push_back(QString("defaultgreekstrongs"));
	dm_values.push_back(m_options.defaultGreekStrongs);
	
	dm_desc.push_back(i18n("Default Hebrew Strong's Lexicon"));
	dm_names.push_back(QString("defaulthebrewstrongs"));
	dm_values.push_back(m_options.defaultHebrewStrongs);
	
	dm_desc.push_back(i18n("Default Greek Morphological Lexicon"));
	dm_names.push_back(QString("defaultgreekmorph"));
	dm_values.push_back(m_options.defaultGreekMorph);
	
	dm_desc.push_back(i18n("Default Hebrew Morphological Lexicon"));
	dm_names.push_back(QString("defaulthebrewmorph"));
	dm_values.push_back(m_options.defaultHebrewMorph);
	
	for (i = 0; i < dm_desc.size(); i++) {
		temp = QString("<option value='' %1> </option>")
			.arg(dm_values[i].stripWhiteSpace().isEmpty() ? "selected" : "");
			
		for (it = modules.begin(); it != modules.end(); ++it ) {
			temp += QString("<option value='%1' %3>%2</option>")
					.arg(*it)
					.arg(*it)
					.arg(((*it) == dm_values[i] ? "selected" : ""));
		}
		output += module_option_row
				.arg(dm_desc[i])
				.arg(dm_names[i])
				.arg(temp)
				.arg(dm_names[i]);
	}

	output += separator_row.arg(i18n("Other options"));
	output += settingsBooleanOptionRow(i18n("Use a simplified page, excluding the graphics borders."),	
								"simplepage", "sp", m_options.simplePage);
	output += settingsBooleanOptionRow(i18n("Make formatting options persistant.  This makes kio-sword remember any options specified in the command line for the length of a session (until the kio-sword process ends) or until it is overridden.  Navigation options are excluded from this setting."),	
								"persist", "ps", m_options.persist);
	
	output += QString("</table>"
			"<br><input type='hidden' name='module' value='%1'>"    	// redirection path
			"<input type='hidden' name='query' value='%2'>"	    	    	// redirection path
			"<input type='submit' name='testsettings' value='%3'>&nbsp;"	// "Test settings"
			"<input type='submit' name='savesettings' value='%4''>"    	// "Save settings"
			"</form>")
			.arg(m_previous.module)
			.arg(m_previous.query)
			.arg("Test settings")
			.arg("Save settings");
	
	output += i18n("<hr><p>To further customise the appearance of the kio-sword page, you can make your own modified "
			"version of the style sheet. "
			"Simply copy the file '%1kio_sword/kio_sword.css' to $HOME/.kde/share/apps/kio_sword/ and modify it as desired. You may want to use the 'simplepage' option above to make the most of this.</p>")
				.arg(KGlobal::dirs()->findResourceDir("data", "kio_sword/kio_sword.css")); // FIXME - this must always return the system dir, not users dir.
			
	output += QString("<hr><form action='sword:/' method='get'>"
			  "<table><tr><td><input type='submit' name='reset' value='%1'></td><td>%2</td></tr>"
			  "</table></form>")
			.arg(i18n("Reset"))
			.arg(i18n("Use this button to reset any 'persistant' options (see option 'persist' above), and re-check for user customisations."));

	return output;
}

QString SwordProtocol::helpPage() {
	if (help_page.isEmpty()) {
		help_page += i18n("<h1>Help</h1>"
		"<p>For full documentation, see <a href='help:/kio_sword'>installed help files</a>.</p>"
		"<p>Kio-Sword allows you to view SWORD modules (such as Bibles and commentaries) from Konqueror.\n"
		"  These modules must already be installed - you can download them from <a href='http://www.crosswire.org/'>"
		"crosswire.org</a> or you can use a program such as <a href='http:/www.bibletime.info'>BibleTime</a> to help"
		" install them."
		"<h3>Quick help</h3>\n"
		"<ul>\n"
		"  <li>To start, simply type <b><a href='sword:/'>sword:/</a></b> in the location bar, and follow the links like any normal web page<br /><br />\n"
		"  <li>You can type the exact reference in the Location bar, instead of browsing to it, e.g.<br />\n"
		"      <b>sword:/KJV/Hebrews 1:3-5</b> will look up Hebrews chapter 1 verses 3 to 5 in the King James Bible.<br /><br />\n"
		"  <li>You can specify various formatting options in the URL - see <a href='sword:/?settings'>Settings</a> for more info.<br /><br />\n"
		"  <li>To use a default Bible, the easiest way is to add a web shortcut. For example, \n"
		"       if you set<br/>"
		"       Search URI: <b>sword:/KJV/\\{@}</b><br/>"
		"       URI shortcut: <b>bible</b><br/>"
		"       then <b>bible:Hebrews 1:3-5</b> will take\n"
		"       you straight to Hebrews 1:3-5 in the King James Version<br /><br />\n"
		"  <li>You can bookmark Kio-Sword pages just like any other web page.<br /><br />\n"
		"</ul>\n"
		"<p>Problems, comments, feature requests? Email the author. "
		"<p>Author: <a href='mailto:L.Plant.98@cantab.net'>L.Plant.98@cantab.net</a>"
		"<p>Website: <a href='http://kio-sword.lukeplant.me.uk'>kio-sword.lukeplant.me.uk</a>.");
	}
	return help_page;
}

QString SwordProtocol::searchForm() {
	static const QString search_form_tmpl(
		"<h1 class='swordsearchform'>%1</h1>"			// title
		"<div class='swordsearchform'>"
		"<form action='sword:/' method='GET'>"
		"  <table class='swordsearchform'>"
		"    <tr>"
		"      <td><label for='query'>%2</label></td>"		// Search terms
		"      <td><input type='text' name='query'></td>"
		"    </tr>"
		"    <tr>"
		"      <td><label for='module'>%3</label></td>"		// Module
		"      <td><select name='module'>"
		"         %4</select></td>"					// (list of modules)
		"    </tr>"
		"    <tr>"
		"      <td valign='top'><label for='stype'>%5</label></td>"		// Search type
		"      <td><input type='radio' name='stype' value='words' checked>%6<br>"  // words
		"        <input type='radio' name='stype' value='phrase'>%7<br>"    // phrase
		"        <input type='radio' name='stype' value='regex'>%8"        // regex
		"      </td>"
		"    </tr>"
		"    <tr>"
		"      <td colspan='2' align='center'><input type='submit' name='searchq' value='%9'></td>" // Search
		"    </tr>"
		"  </table>"
		"</form>"
		"</div>");
		
	if (search_form.isEmpty()) {
		QStringList modules = m_sword.moduleList();
		QString temp;
		QStringList::Iterator it;
		
		temp = "<option value=''></option>";
		for (it = modules.begin(); it != modules.end(); ++it ) {
			temp += QString("<option value='%1'>%2</option>")
					.arg(*it)
					.arg(*it);
		}
		search_form = search_form_tmpl
				.arg(i18n("Search"))
				.arg(i18n("Search terms"))
				.arg(i18n("Module"))
				.arg(temp)
				.arg(i18n("Search type"))
				.arg(i18n("Words"))
				.arg(i18n("Phrase"))
				.arg(i18n("Regular expression"))
				.arg(i18n("Search"));
	}
	return search_form;
}
