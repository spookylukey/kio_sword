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

class QCString;

class kio_kio_swordProtocol : public KIO::SlaveBase
{
public:
    kio_kio_swordProtocol(const QCString &pool_socket, const QCString &app_socket);
    virtual ~kio_kio_swordProtocol();
    virtual void mimetype(const KURL& url);
    virtual void get(const KURL& url);
};

#endif
