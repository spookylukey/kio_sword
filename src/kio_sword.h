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

#ifndef _kio_sword_H_
#define _kio_sword_H_

#include <qstring.h>
#include <qcstring.h>

#include <kurl.h>
#include <kio/global.h>
#include <kio/slavebase.h>

#include "cswordoptions.h"
#include "csword.h"

class SwordProtocol : public KIO::SlaveBase {


public:
	SwordProtocol(const QCString & pool_socket,
			  const QCString & app_socket);
	virtual ~ SwordProtocol();
	virtual void mimetype(const KURL & url);
	virtual void get(const KURL & url);
    
protected:
	void parseURL(const KURL & url);
	void setInternalDefaults();
	void setHTML();
	
	//void readUserDefaults();
	QCString header();
	QCString footer();
	QString helppage();
	
	typedef enum { 	QUERY, 
			REDIRECT_QUERY, 
			SEARCH_FORM, 
			SEARCH_QUERY, 
			SETTINGS_FORM, 
			SETTINGS_SAVE, 
			RESET, 
			HELP } ActionType;

	ActionType m_action;
	QString m_path;
	
	struct {
		QString query;
		QString module;
	} m_redirect;
	
	struct {
		QString module;
		QString range;
		QString type; // ??more here?
	} m_search;
	
	CSwordOptions m_options;
	CSword m_sword;
	bool debug1;
	bool debug2;
    
};

#endif
