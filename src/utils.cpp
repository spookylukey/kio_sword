/***************************************************************************
    File:         utils.cpp
    Project:      Kio-Sword -- An ioslave for SWORD and KDE
    Copyright:    Copyright (C) 2004-2005 Luke Plant
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
	
	const char* SWORD_PROTOCOL = "sword";

	static void mergeOptionsToURL(KURL& url, const SwordOptions* options)
	{
		QMap<QString, QString> items = options->getQueryStringParams();
		QMap<QString, QString>::const_iterator it;
		QMap<QString, QString>::const_iterator it_end = items.end();
		for(it = items.begin(); it != it_end; ++it) {
			url.addQueryItem(it.key(), it.data());
		}
	}
	
	static void mergeOptionsToURL(KURL& url, const SwordOptions& options)
	{
		return mergeOptionsToURL(url, &options);
	}
		
	static QString htmlEncode(const QString& text)
	{
		QString output = text;
		return output
			.replace("&", "&amp;")
			.replace("<", "&lt;")
			.replace(">", "&gt;")
			.replace("\"", "&quot;");
	}
	
	/** Returns options that need to be propagated as HTML for a form */
	QString optionsAsHiddenFields(const SwordOptions& options)
	{
		QString output;
		QMap<QString, QString> items = options.getQueryStringParams();
		QMap<QString, QString>::const_iterator it;
		QMap<QString, QString>::const_iterator it_end = items.end();
		for(it = items.begin(); it != it_end; ++it) {
			output += QString("<input type=\"hidden\" name=\"%1\" value=\"%2\">")
					.arg(it.key())
					.arg(htmlEncode(it.data()));
		}
		return output;
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
	QString swordUrl(const QString& path, const SwordOptions& options, bool htmlEncodeOutput) {
		QString output;
		KURL url;
		url.setProtocol(SWORD_PROTOCOL);
		if (path.at(0) != '/')
			url.addPath("/");
		url.addPath(path);
		mergeOptionsToURL(url, options);
		if (htmlEncodeOutput)
			return htmlEncode(url.url(0, 106));  // return as utf-8
		else
			return url.url(0, 106);  // return as utf-8
	}
	
	/** 
	* return a valid sword URL to be used in anchors
	*
	* @param module name of module
	* @param reference within module
	*/
	QString swordUrl(const QString& module, const QString& ref, const SwordOptions& options, bool htmlEncodeOutput) {
		if (ref.at(0) == '/')
			return swordUrl(module + ref, options, htmlEncodeOutput);
		else
			return swordUrl(module + "/" + ref, options, htmlEncodeOutput);
	}
	
	/**
	 * return a valid sword URL for 'pages' such as 'help', 'settings' etc,
	 * which are defined using a query parameter
	 */
	QString swordUrlForPage(const QString& page, const SwordOptions& options, bool htmlEncodeOutput)
	{
		QString output;
		KURL url;
		url.setProtocol(SWORD_PROTOCOL);
		url.addPath("/");
		url.addQueryItem(page, "");
		mergeOptionsToURL(url, options);
		if (htmlEncodeOutput)
			return htmlEncode(url.url(0, 106));  // return as utf-8
		else
			return url.url(0, 106);  // return as utf-8
	}
	
	QString swordUrlForSettings(const QString& path, const SwordOptions& options, bool htmlEncodeOutput)
	{
		QString output;
		KURL url;
		url.setProtocol(SWORD_PROTOCOL);
		url.addPath("/");
		url.addQueryItem("settings", "");
		
		// Up to KDE 3.5.2 at least, there is a bug in KURL::url which
		// doesn't take into account the encoding_hint for the query items,
		// so we can't use addQueryItem for anything which has non-ascii chars
		//   url.addQueryItem("previouspath", path);
		mergeOptionsToURL(url, options);
		output = url.url(0, 106);   // return as utf-8
		
		// Add 'previouspath' manually
		output += ( (url.queryItems().count() > 0) ? "&" : "?");
		output += "previouspath=" + KURL::encode_string(path, 106);
		
		if (htmlEncodeOutput)
			return htmlEncode(output);
		else
			return output;
	}
	
	/** Get a URL for doing a search */
	QString swordUrlForSearch(DefModuleType modType, const QString& searchQuery, const SwordOptions& options, bool htmlEncodeOutput)
	{
		return swordUrlForSearch(modType, searchQuery, &options, htmlEncodeOutput);
	}
	
	/** Get a URL for doing a search */
	QString swordUrlForSearch(DefModuleType modType, const QString& searchQuery, const SwordOptions* options, bool htmlEncodeOutput)
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
		url.setProtocol(SWORD_PROTOCOL);
		url.addPath("/");
		url.addQueryItem("modtype", modTypeStr);
		url.addQueryItem("query", searchQuery);
		mergeOptionsToURL(url, options);
		if (htmlEncodeOutput)
			return htmlEncode(url.url(0, 106));  // return as utf-8
		else
			return url.url(0, 106);  // return as utf-8
	}
	
	/** truncate a string to len chars, adding an ellipsis if necessary */
	QString shorten(const QString &ref, uint len) {
		QString output = ref.stripWhiteSpace();
		if (output.length() > len)
			output = output.left(len-2) + "...";
		return output;
	}
	
}
