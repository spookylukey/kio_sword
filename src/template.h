/***************************************************************************
    File:         template.h
    Project:      Kio-Sword -- An ioslave for SWORD and KDE
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

#ifndef KS_TEMPLATE_H
#define KS_TEMPLATE_H

#include <qstring.h>
#include <qcstring.h>


namespace KioSword {

	class SwordOptions;
	
	/** Template used to generate page to be returned */
	class Template {
	private:
		QString m_title;
		QString m_content;
		QString m_nav;
		QString m_currentPath;
		bool m_showToggles;
	public:
		Template();
		void setContent(const QString& content);
		void setNav(const QString& nav);
		void setTitle(const QString& title);
		void setCurrentPath(const QString& currentPath);
		void setShowToggles(bool showToggles);
		QCString render(const SwordOptions& options) const;
	
	};
}

#endif
