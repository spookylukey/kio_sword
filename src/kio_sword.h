/***************************************************************************
 *   Copyright (C) 2004 by Luke Plant                                      *
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

#ifndef KIO_SWORD_H
#define KIO_SWORD_H

#include "cswordoptions.h"
#include "csword.h"

#include <kurl.h>
#include <kconfig.h>
//#include <kio/global.h>
#include <kio/slavebase.h>

#include <qstring.h>
#include <qcstring.h>

class SwordProtocol : public KIO::SlaveBase {


public:
	SwordProtocol(const QCString & pool_socket,
			  const QCString & app_socket);
	virtual ~SwordProtocol();
	virtual void mimetype(const KURL & url);
	virtual void get(const KURL & url);
    
protected:
	void parseURL(const KURL & url);
	void readUserConfig();
	QString saveUserConfig();
	void setHTML();
	
	QCString header();
	QCString footer();
	
	QString helpPage();
	QString settingsForm();
	
	typedef enum { 	QUERY, 
			REDIRECT_QUERY, 
			SEARCH_FORM, 
			SEARCH_QUERY, 
			SETTINGS_FORM, 
			SETTINGS_SAVE, 
			RESET, 
			HELP } ActionType;
			
	typedef enum {  MODULETYPE_NONE,
			BIBLE,
			GREEKSTRONGS,
			HEBREWSTRONGS,
			GREEKMORPH,
			HEBREWMORPH } ModuleType;

	CSword m_sword;
	CSwordOptions m_options;
	ActionType m_action;
	ModuleType m_moduletype;	
	QString m_path;
		
	struct {
		QString query;
		QString module;
	} m_previous;
	
	struct {
		QString query;
		QString module;
	} m_redirect;
	
	struct {
		QString module;
		QString range;
		QString type; // ??more here?
	} m_search;
	
	
	KConfig *m_config;
	
	bool debug1;
	bool debug2;
    
};

#endif
