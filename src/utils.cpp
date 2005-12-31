/***************************************************************************
    File:         utils.cpp
    Project:      kio-sword -- An ioslave for SWORD and KDE
    Copyright:    Copyright (C) 2004 Luke Plant
    Description:  Misc utility functions that don't belong anywhere else
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

#include "utils.h"
#include "swordoptions.h"

#include <kurl.h>
#include <qstring.h>

namespace KioSword {

	const char* DEFBIBLE_STR = "bible";
	const char* GREEKSTRONGS_STR = "greekstrongs";
	const char* HEBREWSTRONGS_STR = "hebrewstrongs";
	const char* GREEKMORPH_STR = "greekmorph";
	const char* HEBREWMORPH_STR = "hebrewmorph";

	static void mergeOptionsToURL(KURL& url, const SwordOptions* options)
	{
		QMap<QString, QString> items = options->getQueryStringParams();
		QMap<QString, QString>::const_iterator it;
		QMap<QString, QString>::const_iterator it_end = items.end();
		for(it = items.begin(); it != it_end; ++it) {
			url.addQueryItem(it.key(), it.data());
		}
	}
	
	/**
	* return a valid sword URL to be used in anchors
	*
	* Many key names have spaces in them, and even trailing spaces
	* which are stripped by Konqueror.  We must encode them to
	* ensure we can access them.
	*
	* @param path path to be encode
	*/
	QString swordUrl(const QString& path, const SwordOptions& options) {
		QString output;
		KURL url;
		url.setProtocol(QString("sword"));
		if (path.at(0) != '/')
			url.addPath("/");
		url.addPath(path);
		mergeOptionsToURL(url, &options);
		return url.url();
	}
	
	/** 
	* return a valid sword URL to be used in anchors
	*
	* @param module name of module
	* @param reference within module
	*/
	QString swordUrl(const QString& module, const QString& ref, const SwordOptions& options) {
		if (ref.at(0) == '/')
			return swordUrl(module + ref, options);
		else
			return swordUrl(module + "/" + ref, options);
	}
	
	/**
	 * return a valid sword URL for 'pages' such as 'help', 'settings' etc,
	 * which are defined using a query parameter
	 */
	QString swordUrlForPage(const QString& page, const SwordOptions& options)
	{
		QString output;
		KURL url;
		url.setProtocol(QString("sword"));
		url.addPath("/");
		url.addQueryItem(page, "");
		mergeOptionsToURL(url, &options);
		return url.url();
	}
	
	/** Get a URL for doing a search */
	QString swordUrlForSearch(DefModuleType modType, const QString& searchQuery, const SwordOptions& options)
	{
		return swordUrlForSearch(modType, searchQuery, &options);
	}
	
	/** Get a URL for doing a search */
	QString swordUrlForSearch(DefModuleType modType, const QString& searchQuery, const SwordOptions* options)
	{
		QString modTypeStr;
		QString output;
		KURL url;
		// FIXME - make this into a dictionary or something better?
		switch (modType)
		{
			case DEFBIBLE:
				modTypeStr = DEFBIBLE_STR;
				break;
			case GREEKSTRONGS:
				modTypeStr = GREEKSTRONGS_STR;
				break;
			case HEBREWSTRONGS:
				modTypeStr = HEBREWSTRONGS_STR;
				break;
			case GREEKMORPH:
				modTypeStr = GREEKMORPH_STR;
				break;
			case HEBREWMORPH:
				modTypeStr = HEBREWMORPH_STR;
				break;
			default:
				return output;
		}
		url.setProtocol(QString("sword"));
		url.addPath("/");
		url.addQueryItem("modtype", modTypeStr);
		url.addQueryItem("query", searchQuery);
		mergeOptionsToURL(url, options);
		return url.url();
	}
	
	/** truncate a string to len chars, adding an ellipsis if necessary */
	QString shorten(const QString &ref, uint len) {
		QString output = ref.stripWhiteSpace();
		if (output.length() > len)
			output = output.left(len-2) + "...";
		return output;
	}
}
