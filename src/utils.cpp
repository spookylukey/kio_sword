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


#include <kurl.h>
#include <qstring.h>

/** return a valid sword URL to be used in anchors
  *
  * Many key names have spaces in them, and even trailing spaces
  * which are stripped by Konqueror.  We must encode them to
  * ensure we can access them.
  *
  * @param path path to be encode
  */
QString swordUrl(const QString &path) {
	QString output;
	KURL url;
	url.setProtocol(QString("sword"));
	if (path.at(0) != '/')
		url.addPath("/");
	url.addPath(path);
	return url.url();
}

/** return a valid sword URL to be used in anchors
  *
  * @param module name of module
  * @param reference within module
  */
QString swordUrl(const QString &module, const QString &ref) {
	if (ref.at(0) == '/')
		return swordUrl(module + ref);
	else
		return swordUrl(module + "/" + ref);
}

QString shorten(const QString &ref, int len) {
	QString output = ref.stripWhiteSpace();
	if (output.length() > len)
		output = output.left(len-2) + "...";
	return output;
}
