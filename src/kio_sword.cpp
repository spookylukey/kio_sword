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

// Standard C++ /C

// FIXME - do we need all these?
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
 

// Qt 
#include <qcstring.h>
#include <qmap.h>

// FIXME don't need all these	
#include <qsocket.h>
#include <qdatetime.h>
#include <qbitarray.h>


// KDE
#include <kapplication.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kinstance.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kurl.h>
#include <ksock.h>

// Mine
#include "kio_sword.h"
#include "cswordoptions.h"
#include "csword.h"

using namespace KIO;

// HTML fragments that will be initialised at run time
static QString html_start_output;
static QString html_end_output;
static QString html_start_output_simple;
static QString html_end_output_simple;

// static HTML 
static const QString html_head("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\""
				"\"http://www.w3.org/TR/html4/loose.dtd\">\n"
				"<html><head>\n"
				"<meta http-equiv='Content-Type' content='text/html; charset=utf-8'>\n"
				"<title>%2</title>\n"
				"<link rel=\"StyleSheet\" href=\"file:%1\" TYPE=\"text/css\">\n"		// kio_sword.css
				"</head>\n");
			
static const QString page_start("<body><div class=\"sword_page\">\n"
				"<table cellpadding=\"0\" cellspacing=\"0\" border=\"0\" width=\"100%\">\n"
  				"  <tr>\n"
				"    <td><img src=\"file:%1\" alt=\"\" class=\"sword_tableimg\"></td>\n"	// header_tl.png
				"    <td background=\"file:%2\">\n"						// header_t.png
				"      <table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\">\n"
				"        <tr>\n"
				"          <td width=\"100%\"></td>\n"
				"          <td><img src=\"file:%3\" class=\"sword_tableimg\" alt=\"\"></td>\n" 	// header_caption.png
				"      </tr></table>\n"
				"    </td>\n"
				"    <td><img src=\"file:%4\" alt=\"\" class=\"sword_tableimg\"></td>\n"	// header_tr.png
				"  </tr>\n"
				"  <tr>\n"
				"    <td background=\"file:%5\"></td>\n"					// border_l.png
				"    <td class=\"sword_text\" width=\"100%\">\n"
				"      <div class=\"sword_text\">\n");
				
static const QString page_start_simple("<body class='sword_simplepage'><div class='sword_simplepage'>");

static const QString page_end(  "      </div>\n"
				"      <hr>\n"
				"      <div class=\"sword_links\">\n"
				"        <a href=\"sword:/\">%6</a> | <a href=\"sword:/?search\">%7</a> | <a href=\"sword:/?settings\">%8</a> | <a href=\"sword:/?help\">%9</a>"
				"      </div>\n"
				"    </td>\n"
				"    <td background=\"file:%1\" alt=\"\"></td>\n"				// border_r.png
				"  </tr>\n"
				"  <tr>\n"
				"    <td><img src=\"file:%2\" alt=\"\" class=\"sword_tableimg\"></td>\n"	// footer_bl.png
				"    <td background=\"file:%3\">\n"						// footer_b.png
				"      <table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\">\n"
				"        <tr>\n"
				"          <td><img src=\"file:%4\" class=\"sword_tableimg\" alt=\"\"></td>\n" 	// footer_sword.png
				"          <td width=\"100%\"></td>\n"
				"        </tr>\n"
				"      </table>\n"
				"    </td>\n"
				"    <td><img src=\"file:%5\" alt=\"\" class=\"sword_tableimg\"></td>\n"	// footer_br.png
				"  </tr>\n"
				"</table>\n"
				"</div>\n"
				"</body>\n");
				
static const QString page_end_simple("</div></body>");
				
static const QString html_tail("</html>\n");

static const QString settings_form(
				"<form action='sword:/' method='GET'>\n"
				"  <input type='checkbox' name='versenumbers' value='1' %1>%2<br>\n"
				"  <input type='checkbox' name='linebreaks' value='1' %1>%2<br>\n"
				"  <input type='submit' name='savesettings' value='1' caption='%3'>"
				"      &nbsp;<input type='submit' name='path' value='%4' caption='%5'>  <br>\n"
				"</form>\n");

QString debugprint(const CSwordOptions &options, CSword &mysword);

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



// NB SwordProtocol::SwordProtocol() does not necessarily get called
// before *each* call to the 'get' method - konqueror keeps the slave alive
// so we have to be careful where initialisation gets done.

SwordProtocol::SwordProtocol(const QCString & pool_socket,
				     const QCString & app_socket)
  : SlaveBase("kio_sword", pool_socket, app_socket)
{
	kdDebug() << "SwordProtocol::SwordProtocol()" << endl;
	// Just set the persist option:
	m_options.persist = false;
	setHTML();
}


void SwordProtocol::setHTML() {
	KStandardDirs* dirs = KGlobal::dirs();
	// Reduce number of file access ops by only looking up two things:
	//   - where is the style sheet
	//   - where are the images
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
				.arg(i18n("Module list"))
				.arg(i18n("Search"))
				.arg(i18n("Settings"))
				.arg(i18n("Help"))
				+ html_tail;
	
	html_start_output_simple = html_head.arg(cssdir + "kio_sword.css") + page_start_simple;
	html_end_output_simple = page_end_simple + html_tail;
}

SwordProtocol::~SwordProtocol()
{
	kdDebug() << "SwordProtocol::~SwordProtocol()" << endl;
}

void SwordProtocol::get(const KURL & url)
{
	QString modname;
	QString query;
	kdDebug() << "SwordProtocol::get(const KURL& url)" << endl;
	
	/*    kdDebug() << "Seconds: " << url.query() << endl;
	QString remoteServer = url.host();
	int remotePort = url.port();
	kdDebug() << "myURL: " << url.prettyURL() << endl; */
	
	// Send the mimeType as soon as it is known
	mimeType("text/html");
	
	
	// Set user defaults from user config file
	
	// Parse the URL
	// Possible actions:
	//   - list of modules  - listModules()
	//   - module index \_  both handled by printText()
	//   - module text  /
	//   - search ??
	//   - help ??
	//   - save a setting ??
	//   - display a search form ??
	
	// Reset data members
	m_path		  = QString::null;
	m_redirect.module = QString::null;
	m_redirect.query  = QString::null;

	// Reset options
	setInternalDefaults();
		// setUserDefaults();  // read from a system/user config file
	
	// Get options/actions from URL
	parseURL(url);
	
	// Set the CSword options from ours
	m_sword.setOptions(m_options);

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
			// invalid syntax
			data(QCString("<p class='sword_usererror'>") + i18n("No module specified.").utf8() + QCString("</p><hr>"));
			m_action = QUERY;
		} else {
			KURL newurl = url;
			newurl.removeQueryItem("module");
			newurl.removeQueryItem("query");
			newurl.setPath('/' + modname + '/' + query);
			redirection(newurl);
			finished();
			return;
		}
	}
	
	
	// Send the data
	
	
	data(header());
	
	// FIXME - fix the encoding according to user preferences ??
	
	switch (m_action) {
		case QUERY:
			if (!modname.isEmpty()) {
				data(m_sword.moduleQuery(modname, query, m_options).utf8());
			} else {		
				data(m_sword.listModules(m_options).utf8());
			}
			break;
		
		case SEARCH_FORM:
			data(QCString("<p><span class='sword_fixme'>SEARCH: unimplemented</span></p>"));
			break;
					
		case SEARCH_QUERY:
			data(QCString("<p><span class='sword_fixme'>SEARCH_QUERY: unimplemented</span></p>"));
			break;
			
		case SETTINGS_FORM:
			data(QCString("<p><span class='sword_fixme'>SETTINGS_FORM: unimplemented</span></p>"));
			break;
			
		case SETTINGS_SAVE:
			data(QCString("<p><span class='sword_fixme'>SETTINGS_SAVE: unimplemented</span></p>"));
			break;
			
		case RESET:
			m_options.persist = false;
			setInternalDefaults();
			setHTML();
			data(i18n("<p>Formatting options reset.</p>").utf8());
			break;
			
		case HELP:
			data(helppage().utf8());
			break;
			
		default:
			break;
	}
		
	if (debug1) data(debugprint(m_options, m_sword).utf8());
	data(footer());
	data(QByteArray());		// empty array means we're done sending the data
	finished();
}

void SwordProtocol::mimetype(const KURL & /*url */ )
{
	mimeType("text/html");
	finished();
}

QCString SwordProtocol::header() {
	if (m_options.simplePage) 
		return html_start_output_simple.arg("Kio-Sword").utf8();
	else
		return html_start_output.arg("Kio-Sword").utf8();
}

QCString SwordProtocol::footer() {
	if (m_options.simplePage)
		return html_end_output_simple.utf8();
	else
		return html_end_output.utf8();
}

QString debugprint(const CSwordOptions &options, CSword &mysword) {
	QString output;
	output += QString("<p>options.strongs: %1").arg(options.strongs);
	output += "<p>getGlobalOption(\"Strong's Numbers\"):";
	output += mysword.getGlobalOption("Strong's Numbers");
	return output;
}

void SwordProtocol::setInternalDefaults() 
{
	// Set up defaults for options
	
	m_action = QUERY;
	
	// If the persist option has been set, we allow formatting settings
	// set in previous 'get' commands to persist, i.e. don't reset
	// them.
	if (!m_options.persist) {
		m_options.verseNumbers = true;
		m_options.verseLineBreaks = true;
		m_options.footnotes = false;
		m_options.headings = true;
		m_options.strongs = false;
		m_options.morph = false;
		m_options.cantillation = true;
		m_options.hebrewVowelPoints = true;
		m_options.greekAccents = true;
		m_options.lemmas = true;
		m_options.crossRefs = true;
		m_options.redWords = true;
		m_options.styleSheet = "kio_sword.css";
		m_options.simplePage = false;
	}
	m_options.doBibleIndex = true;
	m_options.doDictIndex = false;
	m_options.doOtherIndex = false;
	m_options.doFullTreeIndex = false;
	m_options.wholeBook = false;
	m_options.snippet = false;
	m_options.variants = 1;
	
	m_redirect.query  = QString::null;
	m_redirect.module = QString::null;
	
	debug1 = false;
	debug2 = false;
}

#define BOOL_OPTION(option, tag1, tag2) \
	if (!strcasecmp(key, tag1) ||   \
	    !strcasecmp(key, tag2)) {   \
		if (val == "0")         \
			option = false; \
		else                    \
			option = true;  \
	}

void SwordProtocol::parseURL(const KURL& url) 
{
	QMap<QString, QString>::iterator it;
	QMap<QString, QString> items = url.queryItems(KURL::CaseInsensitiveKeys, 0);
	QString val;
	const char *key;
	
	if (url.hasPath()) 
		m_path = url.path();
		
	for(it = items.begin(); it != items.end(); it++) {
		key = it.key().latin1();
		val = it.data();
		
		BOOL_OPTION(m_options.snippet, 			"snpt", "snippet")
		else BOOL_OPTION(m_options.verseNumbers, 	"vn", "versenumbers")
		else BOOL_OPTION(m_options.verseLineBreaks, 	"lb", "linebreaks")
		else BOOL_OPTION(m_options.wholeBook, 		"wb", "wholebook")
		else BOOL_OPTION(m_options.doBibleIndex, 	"bi", "bibleindex")
		else BOOL_OPTION(m_options.doDictIndex, 	"di", "dictindex")
		else BOOL_OPTION(m_options.doFullTreeIndex, 	"fi", "fullindex")
		else BOOL_OPTION(m_options.doOtherIndex, 	"oi", "otherindex")
		else BOOL_OPTION(m_options.redWords, 		"rw", "redwords")
		else BOOL_OPTION(m_options.footnotes, 		"fn", "footnotes")
		else BOOL_OPTION(m_options.headings, 		"hd", "headings")
		else BOOL_OPTION(m_options.strongs, 		"st", "strongs")
		else BOOL_OPTION(m_options.morph, 		"mt", "morph")
		else BOOL_OPTION(m_options.cantillation,	"hc", "cantillation")
		else BOOL_OPTION(m_options.hebrewVowelPoints,	"hvp", "vowelpoints")
		else BOOL_OPTION(m_options.greekAccents,	"ga", "accents")
		else BOOL_OPTION(m_options.persist,		"persist", "persist")
		else BOOL_OPTION(m_options.simplePage,		"sp", "simplepage")
		else if (!strcasecmp(key, "debug1")) {
			if (val == "0")
				debug1 = false;
			else
				debug1 = true;
				
		} else if (!strcasecmp(key, "debug2")) {
			if (val == "0")
				debug2 = false;
			else
				debug2 = true;
		} else if (!strcasecmp(key, "ss") || 
			   !strcasecmp(key, "stylesheet")) {
			if (!val.isEmpty())
				m_options.styleSheet = val;
				
		
		// Actions
		} else if (!strcasecmp(key, "reset")) {
			m_action = RESET;		
		} else if (!strcasecmp(key, "help")) {
			m_action = HELP;
		} else if (!strcasecmp(key, "search")) {
			m_action = SEARCH_FORM;
		} else if (!strcasecmp(key, "searchmod")) {
			m_action = SEARCH_QUERY ;
		} else if (!strcasecmp(key, "settings")) {
			m_action = SETTINGS_FORM;
		} else if (!strcasecmp(key, "savesettings")) {
			m_action = SETTINGS_SAVE;
			
		// redirection
		} else if (!strcasecmp(key, "query")) {
			m_action = REDIRECT_QUERY;
			m_redirect.query = val;
		} else if (!strcasecmp(key, "module")) {
			m_action = REDIRECT_QUERY;
			m_redirect.module = val;
		} else if (!strcasecmp(key, "path")) {
			m_action = REDIRECT_QUERY;
			m_redirect.module = val.section('/', 0, 0, QString::SectionSkipEmpty);
			m_redirect.query = val.section('/', 1, -1, QString::SectionSkipEmpty);
		}
	}
}

QString SwordProtocol::helppage() {
	QString output;
	KStandardDirs* dirs = KGlobal::dirs();
	QString htmldir = dirs->findResourceDir("html", "kio_sword"); // FIXME - how does this work with different locales?
	
	output += i18n("<h1>Help</h1>");
	
	if (htmldir.isEmpty())
		output += i18n("<p>(Full documentation cannot be found)</p>");
	else
		output += i18n("<p>For full documentation, please see your <a href='file:%1'>online documentation</a>.</p>");
	
	// Breif help
	output += i18n(
	"<p>Kio-Sword allows you to view installed SWORD modules (such as Bibles and commentaries) from Konqueror.\n"
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
	"<p>Problems, comments, feature requests? Email the author: "
	"<a href='mailto:L.Plant.98@cantab.net'>L.Plant.98@cantab.net</a>"
	"<p>Website: <a href='http://kiosword.lukeplant.me.uk'>kiosword.lukeplant.me.uk</a>. (FIXME)");
	return output;
}
