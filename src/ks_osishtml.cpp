/***************************************************************************
    File:         ks_osishtml.cpp
    Project:      kio-sword -- An ioslave for SWORD and KDE
    Copyright:    Copyright (C) 2004 Luke Plant
                  and CrossWire Bible Society 2003
                  (file based on osishtmlhref.cpp and osiscgi.cpp)
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

#include "ks_osishtml.h"
#include "utils.h"

#include <utilxml.h>
#include <versekey.h>
#include <swmodule.h>

#include <qstring.h>

#include <stdlib.h>

using namespace sword;

namespace KioSword {
	OSISHTML::MyUserData::MyUserData(const SWModule *module, const SWKey *key) : BasicFilterUserData(module, key) {
		osisQToTick = ((!module->getConfigEntry("OSISqToTick")) || (strcmp(module->getConfigEntry("OSISqToTick"), "false")));
	}
	
	
	OSISHTML::OSISHTML() {
		setTokenStart("<");
		setTokenEnd(">");
	
		setEscapeStart("&");
		setEscapeEnd(";");
	
		setEscapeStringCaseSensitive(true);
	
		addEscapeStringSubstitute("amp", "&");
		addEscapeStringSubstitute("apos", "'");
		addEscapeStringSubstitute("lt", "<");
		addEscapeStringSubstitute("gt", ">");
		addEscapeStringSubstitute("quot", "\"");
		addTokenSubstitute("lg", "<br />");
		addTokenSubstitute("/lg", "<br />");
	
		setTokenCaseSensitive(true);
	}
	
	
	bool OSISHTML::handleToken(SWBuf &buf, const char *token, BasicFilterUserData *userData) {
		// manually process if it wasn't a simple substitution
		if (!substituteToken(buf, token)) {
			MyUserData *u = (MyUserData *)userData;
			XMLTag tag(token);
	
			// <w> tag
			if (!strcmp(tag.getName(), "w")) {
				if ((!tag.isEmpty()) && (!tag.isEndTag())) {
					// start <w> tag
					u->w = token;
				} else {
					// end <w> tag
					const char *attrib;
					const char *val;
					if (tag.isEndTag())
						tag = u->w.c_str();
						
					if ((attrib = tag.getAttribute("xlit"))) {
						val = strchr(attrib, ':');
						val = (val) ? (val + 1) : attrib;
						buf.appendFormatted(" %s", val);
					}
					if ((attrib = tag.getAttribute("gloss"))) {
						val = strchr(attrib, ':');
						val = (val) ? (val + 1) : attrib;
						buf.appendFormatted(" %s", val);
					}
					if ((attrib = tag.getAttribute("lemma"))) {
						int count = tag.getAttributePartCount("lemma");
						int i = (count > 1) ? 0 : -1;		// -1 for whole value cuz it's faster, but does the same thing as 0
						do {
							attrib = tag.getAttribute("lemma", i);
							if (i < 0) i = 0;       // to handle our -1 condition
							val = strchr(attrib, ':');
							val = (val) ? (val + 1) : attrib;
							if (*val == 'G') {
								buf.append(QString(" <span class='swordstrongs'>&lt;<a href=\"%1\">%2</a>&gt;</span> ")
										.arg(swordUrlForSearch(GREEKSTRONGS, val+1, m_swordoptions))
										.arg(val+1)
										.utf8());
							} else if (*val == 'H') {
								buf.append(QString(" <span class='swordstrongs'>&lt;<a href=\"%1\">%2</a>&gt;</span> ")
										.arg(swordUrlForSearch(HEBREWSTRONGS, val+1, m_swordoptions))
										.arg(val+1)
										.utf8());
							}
						} while (++i < count);
					}
					if ((attrib = tag.getAttribute("morph"))) {
						int count = tag.getAttributePartCount("morph");
						int i = (count > 1) ? 0 : -1;		// -1 for whole value cuz it's faster, but does the same thing as 0
						do {
							attrib = tag.getAttribute("morph", i);
							if (i < 0) i = 0;       // to handle our -1 condition
							val = strchr(attrib, ':');
							val = (val) ? (val + 1) : attrib;
							if (!strncmp(attrib, "x-Robinson",10)) { //robinson codes
								buf.append(QString(" <span class='swordmorph'>(<a href=\"%1\">%2</a>)</span> ")
										.arg(swordUrlForSearch(GREEKMORPH, val, m_swordoptions))
										.arg(val)
										.utf8());
							} else if ((*val == 'T'))  {
								if (val[1] == 'G') {
									buf.append(QString(" <span class='swordmorph'>(<a href=\"%1\">%2</a>)</span> ")
											.arg(swordUrlForSearch(GREEKMORPH, val+1, m_swordoptions))
											.arg(val+1)
											.utf8());
								} else if (val[1] == 'H') {
									buf.append(QString(" <span class='swordmorph'>(<a href=\"%1\">%2</a>)</span> ")
											.arg(swordUrlForSearch(HEBREWMORPH, val+1, m_swordoptions))
											.arg(val+1)
											.utf8());
								}
							}
						} while (++i < count);
					}
					if ((attrib = tag.getAttribute("POS"))) {
						val = strchr(attrib, ':');
						val = (val) ? (val + 1) : attrib;
						buf.appendFormatted(" %s", val);
					}
				}
			}
	
			// <note> tag
			// FIXME - needs to be modified for kio-sword
			else if (!strcmp(tag.getName(), "note")) {
				if (!tag.isEndTag()) {
					if (!tag.isEmpty()) {
						SWBuf type = tag.getAttribute("type");
	
						if (type != "strongsMarkup") {	// leave strong's markup notes out, in the future we'll probably have different option filters to turn different note types on or off
							SWBuf footnoteNumber = tag.getAttribute("swordFootnote");
							VerseKey *vkey;
							// see if we have a VerseKey * or descendant
							try {
								vkey = SWDYNAMIC_CAST(VerseKey, u->key);
							}
							catch ( ... ) {	}
							if (vkey) {
								char ch = ((tag.getAttribute("type") && ((!strcmp(tag.getAttribute("type"), "crossReference")) || (!strcmp(tag.getAttribute("type"), "x-cross-ref")))) ? 'x':'n');
								buf.appendFormatted("<a href=\"noteID=%s.%c.%s\"><small><sup>*%c</sup></small></a> ", vkey->getText(), ch, footnoteNumber.c_str(), ch);
							}
						}
						u->suspendTextPassThru = true;
					}
				}
				if (tag.isEndTag()) {
					u->suspendTextPassThru = false;
				}
			}
	
			// <p> paragraph tag
			else if (!strcmp(tag.getName(), "p")) {
				if ((!tag.isEndTag()) && (!tag.isEmpty())) {	// non-empty start tag
					buf += "<p>";
				}
				else if (tag.isEndTag()) {	// end tag
					buf += "</P>";
					userData->supressAdjacentWhitespace = true;
				}
				else {					// empty paragraph break marker
					buf += "<br />";
					userData->supressAdjacentWhitespace = true;
				}
			}
	
			// FIXME - needs to be modified for kio-sword
			// <reference> tag
			else if (!strcmp(tag.getName(), "reference")) {
				if ((!tag.isEndTag()) && (!tag.isEmpty())) {
					buf += "<a href=\"\">";
				}
				else if (tag.isEndTag()) {
					buf += "</a>";
				}
			}
	
			// <l> poetry, etc
			else if (!strcmp(tag.getName(), "l")) {
				if (tag.isEmpty()) {
					buf += "<br />";
				}
				else if (tag.isEndTag()) {
					buf += "<br />";
				}
				else if (tag.getAttribute("sID")) {	// empty line marker
					buf += "<br />";
				}
			}
	
			// <milestone type="line"/>
			else if ((!strcmp(tag.getName(), "milestone")) && (tag.getAttribute("type")) && (!strcmp(tag.getAttribute("type"), "line"))) {
				buf += "<br />";
				userData->supressAdjacentWhitespace = true;
			}
	
			// <title>
			else if (!strcmp(tag.getName(), "title")) {
				if ((!tag.isEndTag()) && (!tag.isEmpty())) {
					buf += "<div class='swordtitle'>";
				}
				else if (tag.isEndTag()) {
					buf += "</div>";
				}
			}
	
			// <hi> hi?  hi contrast?
			else if (!strcmp(tag.getName(), "hi")) {
				SWBuf type = tag.getAttribute("type");
				if ((!tag.isEndTag()) && (!tag.isEmpty())) {
					if (type == "b" || type == "x-b") {
						buf += "<b>";
						u->inBold = true;
					}
					else {	// all other types
						buf += "<i>";
						u->inBold = false;
					}
				}
				else if (tag.isEndTag()) {
					if(u->inBold) {
						buf += "</b>";
						u->inBold = false;
					}
					else
					buf += "</i>";
				}
				else {	// empty hi marker
					// what to do?  is this even valid?
				}
			}
	
			// <q> quote
			else if (!strcmp(tag.getName(), "q")) {
				SWBuf type = tag.getAttribute("type");
				SWBuf who = tag.getAttribute("who");
				const char *lev = tag.getAttribute("level");
				int level = (lev) ? atoi(lev) : 1;
				
				if ((!tag.isEndTag()) && (!tag.isEmpty())) {
					/*buf += "{";*/
	
					//alternate " and '
					if (u->osisQToTick)
						buf += (level % 2) ? '\"' : '\'';
					
					if (who == "Jesus")
						buf += "<span class='swordjesusquote'>";
					else 
						buf += "<span class='swordquote'>";
				}
				else if (tag.isEndTag()) {
					buf += "</span>";
					
					//alternate " and '
					if (u->osisQToTick)
						buf += (level % 2) ? '\"' : '\'';
						
				}
				else {	// empty quote marker
					//alternate " and '
					if (u->osisQToTick)
						buf += (level % 2) ? '\"' : '\'';
				}
			}
	
			// <transChange>
			else if (!strcmp(tag.getName(), "transChange")) {
				SWBuf type = tag.getAttribute("type");
				
				if ((!tag.isEndTag()) && (!tag.isEmpty())) {
	
	// just do all transChange tags this way for now
	//				if (type == "supplied")
						buf += "<i>";
				}
				else if (tag.isEndTag()) {
					buf += "</i>";
				}
				else {	// empty transChange marker?
				}
			}
	
			// FIXME - remove for kio-sword?
			// image
			else if (!strcmp(tag.getName(), "figure")) {
				const char *src = tag.getAttribute("src");
				if (!src)		// assert we have a src attribute
					return false;
	
				char* filepath = new char[strlen(u->module->getConfigEntry("AbsoluteDataPath")) + strlen(token)];
				*filepath = 0;
				strcpy(filepath, userData->module->getConfigEntry("AbsoluteDataPath"));
				strcat(filepath, src);
	
	// we do this because BibleCS looks for this EXACT format for an image tag
				buf+="<image src=\"";
				buf+=filepath;
				buf+="\" />";
	/*
				char imgc;
				for (c = filepath + strlen(filepath); c > filepath && *c != '.'; c--);
				c++;
				FILE* imgfile;
					if (strcasecmp(c, "jpg") || stricmp(c, "jpeg")) {
							imgfile = fopen(filepath, "r");
							if (imgfile != NULL) {
									buf += "{\\nonshppict {\\pict\\jpegblip ";
									while (feof(imgfile) != EOF) {
										buf.appendFormatted("%2x", fgetc(imgfile));
									}
									fclose(imgfile);
									buf += "}}";
							}
					}
					else if (strcasecmp(c, "png")) {
							buf += "{\\*\\shppict {\\pict\\pngblip ";
	
							buf += "}}";
					}
	*/
				delete [] filepath;
			}
			
			else {
			return false;  // we still didn't handle token
			}
		}
		return true;
	}
}
