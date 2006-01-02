/***************************************************************************
    File:         template.cpp
    Project:      kio-sword -- An ioslave for SWORD and KDE
    Copyright:    Copyright (C) 2005 Luke Plant
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

#include "template.h"
#include "utils.h"
#include "swordoptions.h"

#include <kglobal.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include <qstring.h>

namespace KioSword {

			
	// placeholders
	static const char* PAGETITLE = "{$pagetitle}";
	static const char* BASECSS   = "{$basecss}";
	static const char* USERCSS   = "{$usercss}";
	static const char* CONTENT   = "{$content}";
	static const char* HOMELINK =  "{$homelink}";
	static const char* HOMELINKCAPTION = "{$homelinkcaption}";
	static const char* SEARCHLINK =  "{$searchlink}";
	static const char* SEARCHLINKCAPTION = "{$searchlinkcaption}";
	static const char* SETTINGSLINK =  "{$settingslink}";
	static const char* SETTINGSLINKCAPTION = "{$settingslinkcaption}";
	static const char* HELPLINK =  "{$helplink}";
	static const char* HELPLINKCAPTION = "{$helplinkcaption}";
	static const char* TOPNAV = "{$topnav}";
	static const char* BOTTOMNAV = "{$bottomnav}";
	static const char* TOGGLES = "{$toggles}";
	
	// static HTML fragments -------------------------------------------------------------------------------------------------------
	static const QString &html_page(QString("") + 
					"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\""
					"\"http://www.w3.org/TR/html4/strict.dtd\">\n"
					"<html><head>\n"
					"<meta http-equiv='Content-Type' content='text/html; charset=utf-8'>\n"
					"<title>" + PAGETITLE + "</title>\n"
					"<link rel=\"StyleSheet\" href=\"file:" + BASECSS + "\" TYPE=\"text/css\">\n"
					"<link rel=\"StyleSheet\" href=\"file:" + USERCSS + "\" TYPE=\"text/css\">\n"		// user.css FIXME implement
					"</head>\n"
					"<body class=\"kiosword\">"
					"<div class=\"page\"><div class='inner'>"
					"	" + TOPNAV + "\n"
					"	<div class=\"content\"><div class='inner'>" + CONTENT + "</div></div>\n"
					"	" + BOTTOMNAV + "\n"
					"	" + TOGGLES + "\n"
					"	<div class=\"links\"><div class='inner'>\n"
					"		<ul>\n"
					"			<li><a href=\"" + HOMELINK + "\">" + HOMELINKCAPTION + "</a></li>\n"
					"			<li><a href=\"" + SEARCHLINK + "\">" + SEARCHLINKCAPTION + "</a></li>\n"
					"			<li><a href=\"" + SETTINGSLINK + "\">" + SETTINGSLINKCAPTION + "</a></li>\n"
					"			<li><a href=\"" + HELPLINK + "\">" + HELPLINKCAPTION + "</a></li>\n"
					"		</ul>\n"
					"	</div></div>\n"
					"</div></div>\n"
					"</body>\n"
					"</html>\n");

	Template::Template()
	{
 		m_showToggles = false;
 	}
 	
	QCString Template::render(const SwordOptions& options) const
	{
	
		QString cssdir = KGlobal::dirs()->findResourceDir("data", "kio_sword/base.css") + "kio_sword/";
	
/* TODO - restore		if (!nav.isEmpty()) {
				output = "<div class='navtop'>" + nav + "</ul></div><div class='text'>" +
					output +
					"</div><div class='navbottom'>" + nav + "</ul></div>";
			} else {
				output = "<div class='text'>" +  output + "</div>";
		}*/

		QString output = html_page;
		output = output
			.replace(HOMELINK, swordUrl("", options))
			.replace(HOMELINKCAPTION, i18n("Module list"))
			.replace(SEARCHLINK, swordUrlForPage("search", options))
			.replace(SEARCHLINKCAPTION, i18n("Search"))
			.replace(SETTINGSLINK, swordUrlForSettings(m_currentPath, options))
			.replace(SETTINGSLINKCAPTION,  i18n("Settings"))
			.replace(HELPLINK, swordUrlForPage("help", options))
			.replace(HELPLINKCAPTION, i18n("Help"))
			.replace(BASECSS,  cssdir + "base.css")
			.replace(PAGETITLE, m_title)
			.replace(CONTENT, m_content);
		if (!m_nav.isEmpty())
		{
			output = output
					.replace(TOPNAV, "<div class='navtop'><div class='inner'>" + m_nav + "</div></div>")
					.replace(BOTTOMNAV, "<div class='navbottom'><div class='inner'>" + m_nav + "</div></div>");
		}
		else
		{
			output = output
					.replace(TOPNAV, "")
					.replace(BOTTOMNAV, "");
		}
		if (m_showToggles)
		{
			QString toggles;
			SwordOptions toggledOptions(options);
			
			toggledOptions.verseNumbers.set(!toggledOptions.verseNumbers());
			toggles += "<li><a href=\"" + swordUrl(m_currentPath, toggledOptions) + "\" accesskey='v'>" 
				+ i18n("Verse Numbers") + "</a></li>";
			toggledOptions.verseNumbers.set(!toggledOptions.verseNumbers());
				
			toggledOptions.verseLineBreaks.set(!toggledOptions.verseLineBreaks());
			toggles += "<li><a href=\"" + swordUrl(m_currentPath, toggledOptions) + "\" accesskey='l'>" 
				+ i18n("Verse Line Breaks") + "</a></li>";
			toggledOptions.verseLineBreaks.set(!toggledOptions.verseLineBreaks());

			toggledOptions.headings.set(!toggledOptions.headings());
			toggles += "<li><a href=\"" + swordUrl(m_currentPath, toggledOptions) + "\" accesskey='h'>" 
				+ i18n("Headings") + "</a></li>";
			toggledOptions.headings.set(!toggledOptions.headings());

			toggledOptions.strongs.set(!toggledOptions.strongs());
			toggles += "<li><a href=\"" + swordUrl(m_currentPath, toggledOptions) + "\" accesskey='s'>" 
				+ i18n("Strongs") + "</a></li>";
			toggledOptions.strongs.set(!toggledOptions.strongs());
			
			toggledOptions.morph.set(!toggledOptions.morph());
			toggles += "<li><a href=\"" + swordUrl(m_currentPath, toggledOptions) + "\" accesskey='m'>" 
				+ i18n("Morphological tags") + "</a></li>";
			toggledOptions.morph.set(!toggledOptions.morph());

			output = output.replace(TOGGLES, "<div class='toggles'><div class='inner'><ul>" + toggles + "</ul></div></div>");
			
		}
		else
		{
			output = output.replace(TOGGLES, "");
		}
		return output.utf8();
	}
	
	void Template::setTitle(const QString& title)
	{
		m_title = title;
	}
	
	void Template::setContent(const QString& content)
	{
		m_content = content;
	}
	
	void Template::setNav(const QString& nav)
	{
		m_nav = nav;
	}
	
	void Template::setCurrentPath(const QString& currentPath)
	{
		m_currentPath = currentPath;
	}
	
	void Template::setShowToggles(bool showToggles)
	{
		m_showToggles = showToggles;
	}
}

