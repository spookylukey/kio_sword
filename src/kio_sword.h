/***************************************************************************
 *   Copyright (C) 2004-2005 by Luke Plant                                      *
 *   L.Plant.98@cantab.net                                                 *
 *                                                                         *
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

#ifndef KIOSWORD_H
#define KIOSWORD_H

#include "utils.h"
#include "swordoptions.h"
#include "renderer.h"

#include <kurl.h>
#include <kconfig.h>
#include <kio/slavebase.h>

#include <qstring.h>
#include <qcstring.h>

namespace KioSword {
	class Template;

	class SwordProtocol : public KIO::SlaveBase {
		
	public:
		SwordProtocol(const QCString & pool_socket,
				const QCString & app_socket);
		virtual ~SwordProtocol();
		virtual void mimetype(const KURL & url);
		virtual void get(const KURL & url);
	
	protected:
		void data(const QCString& text);
		void data(const QByteArray& text);
		
		void parseURL(const KURL & url);
		void readUserConfig();
		QString saveUserConfig();
		
		void sendPage(const Template* tmplt);
		
		QString helpPage();
		QString pageLinks(const SwordOptions& options);
		QString searchForm(const SwordOptions& options);
		QString settingsForm();
		
		typedef enum { 	QUERY, 
				REDIRECT_QUERY, 
				SEARCH_FORM, 
				SEARCH_QUERY, 
				SETTINGS_FORM, 
				SETTINGS_SAVE, 
				HELP } ActionType;
	
		Renderer	m_renderer;
		SwordOptions 	m_options;
		ActionType 	m_action;
		DefModuleType 	m_moduletype;
		QString 	m_path;
		Renderer::SearchType 	m_stype;
		KURL		m_baseurl;
			
		struct {
			QString query;
			QString module;
		} m_previous;
		
		struct {
			QString query;
			QString module;
		} m_redirect;
		
		KConfig *m_config;
		
	};
}

#endif
