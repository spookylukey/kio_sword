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

#include <qcstring.h>
#include <qsocket.h>
#include <qdatetime.h>
#include <qbitarray.h>

#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kinstance.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kurl.h>
#include <ksock.h>

#include "kio_sword.h"

using namespace KIO;


kio_kio_swordProtocol::kio_kio_swordProtocol(const QCString &pool_socket, const QCString &app_socket)
    : SlaveBase("kio_kio_sword", pool_socket, app_socket)
{
    kdDebug() << "kio_kio_swordProtocol::kio_kio_swordProtocol()" << endl;
}


kio_kio_swordProtocol::~kio_kio_swordProtocol()
{
    kdDebug() << "kio_kio_swordProtocol::~kio_kio_swordProtocol()" << endl;
}


void kio_kio_swordProtocol::get(const KURL& url )
{
    kdDebug() << "kio_kio_sword::get(const KURL& url)" << endl ;
    
    kdDebug() << "Seconds: " << url.query() << endl;
    QString remoteServer = url.host();
    int remotePort = url.port();
    kdDebug() << "myURL: " << url.prettyURL() << endl;
    
    infoMessage(i18n("Looking for %1...").arg( remoteServer ) );
    // Send the mimeType as soon as it is known
    mimeType("text/plain");
    // Send the data
    QString theData = "This is a test of kio_kio_sword";
    data(QCString(theData.local8Bit()));
    data(QByteArray()); // empty array means we're done sending the data
    finished();
}


void kio_kio_swordProtocol::mimetype(const KURL & /*url*/)
{
    mimeType("text/plain");
    finished();
}


extern "C"
{
    int kdemain(int argc, char **argv)
    {
        KInstance instance( "kio_kio_sword" );
        
        kdDebug(7101) << "*** Starting kio_kio_sword " << endl;
        
        if (argc != 4) {
            kdDebug(7101) << "Usage: kio_kio_sword  protocol domain-socket1 domain-socket2" << endl;
            exit(-1);
        }
        
        kio_kio_swordProtocol slave(argv[2], argv[3]);
        slave.dispatchLoop();
        
        kdDebug(7101) << "*** kio_kio_sword Done" << endl;
        return 0;
    }
} 
