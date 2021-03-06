/***************************************************************************
    File:         kio_sword.cpp
    Project:      Kio-Sword  -- An ioslave for SWORD and KDE
    Copyright:    Copyright (C) 2004-2005 Luke Plant
 
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
#include "renderer.h"
#include "kio_sword.h"
#include "utils.h"
#include "template.h"
#include "option.h"

// KDE
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kurl.h>
#include <kstandarddirs.h>

// Qt 
#include <qcstring.h>
#include <qmap.h>

// Standard C++ /C
#include <list>
#include <stdlib.h>

using namespace KIO;
using std::list;
using std::vector;


// main --------------------------------------------------------------------------------------------------------
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
		
		KioSword::SwordProtocol slave(argv[2], argv[3]);
		slave.dispatchLoop();
		
		kdDebug(7101) << "*** kio_sword Done" << endl;
		return 0;
	}
}

namespace KioSword
{
	class SwordOptions;

	// HTML fragments that will be initialised at run time -------------------------------------------------------------------------
	static QString search_form;
	static QString help_page;
	

				
	SwordProtocol::SwordProtocol(const QCString & pool_socket,
					const QCString & app_socket)
	: SlaveBase("kio_sword", pool_socket, app_socket)
	{
		kdDebug() << "SwordProtocol::SwordProtocol()" << endl;
		m_config = KGlobal::config();
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
			if (!m_path.startsWith("/")) {
				// sword:xxx is a shortcut for bible verses
				modname = m_options.defaultBible();
				if (modname.isEmpty())
				{
					error = i18n("No default Bible has been specified.");
				}
				else
				{
					// do redirection
					query = m_path;
					KURL newurl(url);
					newurl.setPath('/' + modname + '/' + query);
					redirection(newurl);
					finished();
					return;
				}
			}
			else
			{
				modname = m_path.section('/', 0, 0, QString::SectionSkipEmpty);
				query = m_path.section('/', 1, -1, QString::SectionSkipEmpty);
			}	
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
						modname = m_options.defaultBible();
						error = i18n("No default Bible has been specified.");
						break;
					case GREEKSTRONGS:
						modname = m_options.defaultGreekStrongs();
						error = i18n("No default Greek Strongs module has been specified.");
						break;
					case HEBREWSTRONGS:
						modname = m_options.defaultHebrewStrongs();
						error = i18n("No default Hebrew Strongs module has been specified.");
						break;
					case GREEKMORPH:
						modname = m_options.defaultGreekMorph();
						error = i18n("No default Greek morphological module has been specified.");
						break;
					case HEBREWMORPH:
						modname = m_options.defaultHebrewMorph();
						error = i18n("No default Hebrew morphological module has been specified.");
						break;
					case DEFMODULETYPE_NONE:
						error = i18n("No module specified.");
				}
			}
			
			if (modname.isEmpty()) {
				error = "<p class='usererror'>" + error + "</p><hr />";
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
				
		// Send the data
		Template* tmplt = new Template();
		tmplt->setCurrentPath(m_path);

		switch (m_action) {
			case QUERY:
				if (!modname.isEmpty()) {
					m_renderer.moduleQuery(modname, query, m_options, tmplt);
				} else {
					QString body;
					tmplt->setTitle(i18n("Modules - Kio-Sword"));
					if (!error.isEmpty()) {
						body = error;
					}
					body += m_renderer.listModules(m_options);
					tmplt->setContent(body);
				}
				break;
			
			case SEARCH_FORM:
				tmplt->setTitle(i18n("Search - Kio-Sword"));
				tmplt->setContent(searchForm(m_options));
				break;
						
			case SEARCH_QUERY:
				tmplt->setTitle(i18n("Search Results - Kio-Sword"));
				tmplt->setContent(m_renderer.search(m_redirect.module, m_redirect.query, m_stype, m_options));
				break;
				
			case SETTINGS_FORM:
				tmplt->setTitle(i18n("Settings - Kio-Sword"));
				tmplt->setContent(settingsForm());
				break;
				
			case SETTINGS_SAVE:
				tmplt->setTitle(i18n("Settings saved - Kio-Sword"));
				tmplt->setContent(saveUserConfig());
				break;
								
			case HELP:
				tmplt->setTitle(i18n("Kio-Sword Help"));
				tmplt->setContent(helpPage());
				break;
				
			default:
				break;
		}
		sendPage(tmplt);
		delete tmplt;

	}
	
	
	void SwordProtocol::mimetype(const KURL& url) {
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
	
	void SwordProtocol::readUserConfig() 
	{
		m_options.readFromConfig(m_config);
	}
	
	QString SwordProtocol::saveUserConfig() 
	{
		QString message;
		m_options.saveToConfig(m_config);
		m_config->sync();
		// search form depends on settings, so force it to be recreated
		search_form.truncate(0);
		message = "<p>" + i18n("Settings saved.") + "</p>";
		return message;
	}
	

	#define ENUM_OPTION(option, tag, v)    \
		if (!strcasecmp(key, tag)) {   \
			option = v;            \
		}
		
	void SwordProtocol::parseURL(const KURL& url) 
	{
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
		
		m_options.readFromQueryString(url.queryItems(KURL::CaseInsensitiveKeys));
		
		// URLs will be encoded in UTF-8 since they are sometimes 
		// generated from the search form, and the browser will 
		// encode in UTF-8 since the whole page has UTF-8 charset
		QMap<QString, QString> items = url.queryItems(KURL::CaseInsensitiveKeys, 106); 
		QMap<QString, QString>::const_iterator it;
		QMap<QString, QString>::const_iterator it_end = items.end();
		QString val;

		const char *key;
		for(it = items.begin(); it != it_end; it++) {
			key = it.key().latin1();
			val = it.data();
				
			if (!strcasecmp(key, "query")) {
				m_redirect.query = val;
			}
			else if (!strcasecmp(key, "module")) {
				m_redirect.module = val;
			}
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
					m_stype = Renderer::SEARCH_WORDS;
				} else if (!strcasecmp(val, "phrase")) {
					m_stype = Renderer::SEARCH_PHRASE;
				} else if (!strcasecmp(val, "regex")) {
					m_stype = Renderer::SEARCH_REGEX;
				} else {
					m_stype = Renderer::SEARCH_WORDS;
				}
			}
	
			// Actions
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
	
	#undef ENUM_OPTION

	
	void SwordProtocol::sendPage(const Template* tplt) {
		data(tplt->render(m_options));
		data(QByteArray());     // empty array means we're done sending the data
		finished();
	}
	
	/** Returns a string representing notes about an option for the settings page
	 */
	template <class T>
	QString optionNotes(const Option<T>& option)
	{
		QString output;
		if (!option.m_propagate)
		{
			output += "<sup>1</sup>";
		}
		if (option.m_configName.isNull())
		{
			if (output.length() > 0)
			{
				output += "<sup>,</sup>";
			}
			output += "<sup>2</sup>";
		}
		return output;
	}
	
	
	/** HTML for a boolean option */
	QString settingsBooleanOptionRow(const QString& description, const Option<bool>& option) {
		static const QString boolean_option_row(
					"<tr><td>%1</td><td><nobr><input type='radio' name='%2' value='1' %3>%4 &nbsp;&nbsp;<input type='radio'  name='%2' value='0' %5>%6</nobr></td><td>%2, %7</td></tr>");
		return boolean_option_row
				.arg(description + optionNotes(option))
				.arg(option.m_qsShortName)
				.arg(option.m_qsShortName)
				.arg(option.m_qsShortName)
				.arg(option() ? "checked" : "")
				.arg(i18n("On"))
				.arg(option() ? "" : "checked")
				.arg(i18n("Off"))
				.arg(option.m_qsLongName);
	}
	/**  */
	
	/** HTML for a general option row
	 * 
	 * @param description User visible description of option
	 * @param option option metadata
	 * @param optionList HTML containing the <option>s
	 * @return 
	 */
	QString settingsGeneralOptionRow(const QString& description, const Option<QString>& option, 
					 const QString& optionList)
	{
		static const QString general_option_row(
					"<tr><td>%1</td><td><select name='%2'>%3</select></td><td>%4, %5</td></tr>");
		return general_option_row
			.arg(description + optionNotes(option))
			.arg(option.m_qsLongName)
			.arg(optionList)
			.arg(option.m_qsShortName)
			.arg(option.m_qsLongName);
	}
		
	QString SwordProtocol::settingsForm() {
		QString output;
		QStringList modules;
		QStringList::Iterator it;
		QString temp;
				
		static const QString separator_row(
					"<tr><td class='settingscategory' colspan='3'>%1</td></tr>");
	
					
		// Start output
		output += i18n("<h1>Settings</h1>"
				"<p>Select the settings using the form below.  Use the 'Save settings' button to "
				" save these settings to your own configuration file.  'Test settings' will return "
				" you to the previous page with the options you have specified. <br>");
				
		output += QString(
				"<form action='sword:/' method='GET'>"
				"<table class='settings' border=0 cellspacing=0>"
				"  <tr><th>%1</th><th>%2</th><th>%4</th></tr>")
				.arg(i18n("Description"))
				.arg(i18n("Value"))
				.arg(i18n("URL parameter"));
		
		output += separator_row.arg(i18n("Formatting options"));
		output += settingsBooleanOptionRow(i18n("Display verse numbers for Bible modules"), m_options.verseNumbers);
		output += settingsBooleanOptionRow(i18n("Insert line breaks between Bible verses"), m_options.verseLineBreaks);
		output += settingsBooleanOptionRow(i18n("Include footnotes."), m_options.footnotes);
		output += settingsBooleanOptionRow(i18n("Words of Christ in red."), m_options.redWords);
		output += settingsBooleanOptionRow(i18n("Display strongs numbers (for Bibles that include them)."), m_options.strongs);
		output += settingsBooleanOptionRow(i18n("Display morphological tags (for Bibles that include them)."), m_options.morph);
		
		output += separator_row.arg(i18n("Language"));
		
		// Locale
		QStringList locales = m_renderer.availableLocales();
		temp = "";
		for (int i = 0; i < locales.size(); i++)
		{
			temp += QString("<option value='%1' %3>%2</option>")
					.arg(locales[i])
					.arg(locales[i])
					.arg((m_options.locale() == locales[i]) ? "selected" : "");
		}
		output += settingsGeneralOptionRow(i18n("Locale"), m_options.locale, temp);
		
		// Formatting options
		output += settingsBooleanOptionRow(i18n("Use Hebrew cantillation."), m_options.cantillation);
		output += settingsBooleanOptionRow(i18n("Show Hebrew vowel points."), m_options.hebrewVowelPoints);
		output += settingsBooleanOptionRow(i18n("Show Greek accents."), m_options.greekAccents);
		
		output += separator_row.arg(i18n("Navigation options"));
		output += settingsBooleanOptionRow(i18n("Display the whole book when a Bible book is selected, instead of an index of the chapters"), 
							m_options.wholeBook);
		output += settingsBooleanOptionRow(i18n("Display the booklist for bibles if no book is requested"),	
							m_options.doBibleIndex);
		output += settingsBooleanOptionRow(i18n("Display an index for dictionaries if no entry is requested"),	
							m_options.doDictIndex);
		output += settingsBooleanOptionRow(i18n("Display an index for other books if no entry is request"),	
							m_options.doOtherIndex);
		output += settingsBooleanOptionRow(i18n("Display a full index for books that have a multiple level index, instead of just the first level"),
							m_options.doFullTreeIndex);
		
		
		// Default modules
		output += separator_row.arg(i18n("Default modules"));
		modules = m_renderer.moduleList();
		
		vector<QString> dm_desc;
		vector<Option<QString> > dm_options;
		vector<QString>::size_type i;
		dm_desc.push_back(i18n("Default Bible"));
		dm_options.push_back(m_options.defaultBible);
		
		dm_desc.push_back(i18n("Default Greek Strong's Lexicon"));
		dm_options.push_back(m_options.defaultGreekStrongs);
		
		dm_desc.push_back(i18n("Default Hebrew Strong's Lexicon"));
		dm_options.push_back(m_options.defaultHebrewStrongs);
		
		dm_desc.push_back(i18n("Default Greek Morphological Lexicon"));
		dm_options.push_back(m_options.defaultGreekMorph);
		
		dm_desc.push_back(i18n("Default Hebrew Morphological Lexicon"));
		dm_options.push_back(m_options.defaultHebrewMorph);
		
		for (i = 0; i < dm_options.size(); i++) {
			temp = QString("<option value='' %1> </option>")
				.arg(dm_options[i]().stripWhiteSpace().isEmpty() ? "selected" : "");
				
			for (it = modules.begin(); it != modules.end(); ++it ) {
				temp += QString("<option value='%1' %3>%2</option>")
						.arg(*it)
						.arg(*it)
						.arg(((*it) == dm_options[i]() ? "selected" : ""));
			}
			output += settingsGeneralOptionRow(dm_desc[i], dm_options[i], temp);
		}
	
		// Misc options
		output += separator_row.arg(i18n("Other options"));
		output += settingsBooleanOptionRow(i18n("Make formatting options propagate.  This makes Kio-Sword remember formatting settings that you have set, by propagating them in the links.  (Some navigation options are always excluded from this behaviour)."),
				m_options.propagate);
		
		output += QString("</table>");
		
		output += i18n("<p>Notes:<br/>"
				"1) These options never propagate<br/>"
				"2) These options are never saved<br/></p>");
		
		output += QString(
				"<br><input type='hidden' name='module' value='%1'>"    	// redirection path
				"<input type='hidden' name='query' value='%2'>"	    	    	// redirection path
				"<input type='submit' name='testsettings' value='%3'>&nbsp;"	// "Test settings"
				"<input type='submit' name='savesettings' value='%4''>"    	// "Save settings"
				"</form>")
				.arg(m_previous.module)
				.arg(m_previous.query)
				.arg(i18n("Test settings"))
				.arg(i18n("Save settings"));
		
		output += i18n("<hr><h1>Stylesheet</h1>"
				"<p>To further customise the appearance of the Kio-Sword page, you can make your own modified "
				"version of the style sheet. "
				"Simply copy the file '%1kio_sword/kio_sword.css' to $HOME/.kde/share/apps/kio_sword/ and modify it as desired. You may want to use the 'simplepage' option above to make the most of this.</p>")
					.arg(KGlobal::dirs()->findResourceDir("data", "kio_sword/kio_sword.css")); // FIXME - this must always return the system dir, not users dir.
				
	
		return output;
	}
	
	QString SwordProtocol::helpPage() {
		if (help_page.isEmpty()) {
			help_page += i18n("<h1>Help</h1>"
			"<p>For full documentation, see <a href=\"help:/kio_sword\">installed help files</a>.</p>"
			"<p>Kio-Sword allows you to view SWORD modules (such as Bibles and commentaries) from Konqueror.\n"
			"  These modules must already be installed - you can download them from <a href=\"http://www.crosswire.org/\">"
			"crosswire.org</a> or you can use a program such as <a href=\"http:/www.bibletime.info\">BibleTime</a> to help"
			" install them."
			"<h3>Quick help</h3>\n"
			"<ul>\n"
			"  <li>To start, simply type <b><a href=\"sword:/\">sword:/</a></b> in the location bar, and follow the links like any normal web page<br /><br />\n"
			"  <li>You can type the exact reference in the Location bar, instead of browsing to it, e.g.<br />\n"
			"      <b>sword:/KJV/Hebrews 1:3-5</b> will look up Hebrews chapter 1 verses 3 to 5 in the King James Bible.<br /><br /></li>\n"
			"  <li>You can specify various formatting options in the URL - see <a href=\"sword:/?settings\">Settings</a> for more info.<br /><br /></li>\n"
			"  <li>To use your default Bible, just omit the first forward slash: <b>sword:Hebs 1:3-5</b> <br /><br /></li>\n"
			"  <li>You can bookmark Kio-Sword pages just like any other web page.<br /><br /></li>\n"
			"  <li>You can use keyboards shortcuts - press 'Ctrl' to activate 'Acess Keys' in Konqueror, \n"
			"      and the following shortcuts are then available:"
			"      <ul><li>'u' to go up</li>\n"
			"      <li>'n' for next section/chapter</li>\n"
			"      <li>'p' for previous section/chapter</li>\n"
			"      <li>'t' for the module list</li>\n"
			"      <li>'v' to toggle verse numbers</li>\n"
			"      <li>'l' to toggle line breaks between verses</li>\n"
			"      <li>'s' to toggle Strongs numbers</li>\n"
			"      <li>'m' to toggle morphological tags</li>\n"
			"      </ul>\n"
			" (Depending on the context, some of these will either not be available or won't be much use).<br/><br/>"
			" Also, remember the following Konqueror shortcuts to make keyboard navigation even better:<br/>"
			" <b>/</b> starts find-as-you-type<br/>"
			" <b>'</b> starts find-as-you-type for link text<br/>"
			" </li>\n"
			"</ul>\n"
			"<p>Problems, comments, feature requests? Email the author. "
			"<p>Author: <a href=\"mailto:L.Plant.98@cantab.net\">L.Plant.98@cantab.net</a>"
			"<p>Website: <a href=\"http://lukeplant.me.uk/kio-sword/\">http://lukeplant.me.uk/kio-sword/</a>.");
		}
		return help_page;
	}
	
	QString SwordProtocol::searchForm(const SwordOptions& options) {
		static const QString search_form_tmpl(
			"<h1 class='searchform'>%1</h1>"			// title
			"<div class='searchform'>"
			"<form action='sword:/' method='GET'>"
			"  <table class='searchform'>"
			"    <tr>"
			"      <td><label for='query'>%2</label></td>"		// Search terms
			"      <td><input type='text' name='query'></td>"
			"    </tr>"
			"    <tr>"
			"      <td><label for='module'>%3</label></td>"		// Module
			"      <td><select name='module'>"
			"         %4</select></td>"				// (list of modules)
			"    </tr>"
			"    <tr>"
			"      <td valign='top'><label for='stype'>%5</label></td>"			// Search type
			"      <td><input type='radio' name='stype' value='words' checked>%6<br>"  	// words
			"        <input type='radio' name='stype' value='phrase'>%7<br>"    		// phrase
			"        <input type='radio' name='stype' value='regex'>%8"			// regex
			"      </td>"
			"    </tr>"
			"    <tr>"
			"      <td colspan='2' align='center'><input type='submit' name='searchq' value='%9'></td>" // Search
			"    </tr>"
			"  </table>"
			"</form>"
			"</div>");
			
		if (search_form.isEmpty()) { // avoid calculating this twice
			QStringList modules = m_renderer.moduleList();
			QString temp;
			QStringList::Iterator it;
			
			temp = "<option value=''></option>";
			for (it = modules.begin(); it != modules.end(); ++it ) {
				temp += QString("<option value='%1' %3>%2</option>")
						.arg(*it)
						.arg(*it)
						.arg((*it == m_options.defaultBible()) ? "selected='selected'" : "");
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
		// search_form is generated once, but we need to modify
		// form each time to include propagated values
		QString output(search_form);
		return output.replace("</form>", optionsAsHiddenFields(options) + "</form>");
	}
}
