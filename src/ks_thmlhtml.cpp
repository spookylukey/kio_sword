/***************************************************************************
    File:         ks_thmlhtml.cpp
    Project:      kio-sword -- An ioslave for SWORD and KDE
    Copyright:    Copyright (C) 2004 Luke Plant
                  and CrossWire Bible Society 2001
                  (file based on thmlhtmlhref.cpp)
		  and the BibleTime team (bits taken from bt_thmlhtml.cpp)
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

#include "ks_thmlhtml.h"

#include <swmodule.h>
#include <utilxml.h>
#include <versekey.h>

#include <stdlib.h>

using namespace sword;

ks_ThMLHTML::MyUserData::MyUserData(const SWModule *module, const SWKey *key) : BasicFilterUserData(module, key) {
	if (module) {
		version = module->Name();
		BiblicalText = (!strcmp(module->Type(), "Biblical Texts"));
	}	
}


ks_ThMLHTML::ks_ThMLHTML() {
	setTokenStart("<");
	setTokenEnd(">");

	setTokenCaseSensitive(true);
	addTokenSubstitute("scripture", "<i> ");
	addTokenSubstitute("/scripture", "</i> ");
}


bool ks_ThMLHTML::handleToken(SWBuf &buf, const char *token, BasicFilterUserData *userData) {
	const char *tok;
	if (!substituteToken(buf, token)) { // manually process if it wasn't a simple substitution
		MyUserData *u = (MyUserData *)userData;		

		XMLTag tag(token);
		if ((!tag.isEndTag()) && (!tag.isEmpty()))
			u->startTag = tag;

		if (tag.getName() && !strcasecmp(tag.getName(), "sync")) {       //lemmas, morph codes or strongs
			if (tag.getAttribute("type") && !strcasecmp(tag.getAttribute("type"), "lemma")) {       // Lemma
				const char *value = tag.getAttribute("value");
				if (strlen(value)) {
					buf.appendFormatted
						(" <span class='swordlemma'>&lt;%s&gt;</span>", value);
				}
			} else if (tag.getAttribute("type") && !strcasecmp(tag.getAttribute("type"), "morph")) {  // Morph
				const char *value = tag.getAttribute("value");
				if (value) {
					buf.appendFormatted
						(" <span class='swordmorph'>(<a href=\"sword:/?modtype=greekmorph&query=%s\">%s</a>)</span>", value, value);
				}
			} else if (tag.getAttribute("type") && !strcasecmp(tag.getAttribute("type"), "Strongs")) {      // Strongs
				const char *value =
					tag.getAttribute("value");
				if (value && value[0] == 'H') { //hewbrew strong number
					buf.appendFormatted(" <span class='swordstrongs'>&lt;<a href=\"sword:/?modtype=hebrewstrongs&query=%s\">%s</a>&gt;</span>", value + 1,      //skip the H
								value + 1   //skip the H
						);
				} else if (value && value[0] == 'G') {  //greek strong number
					buf.appendFormatted(" <span class='swordstrongs'>&lt;<a href=\"sword:/?modtype=greekstrongs&query=%s\">%s</a>&gt;</span>", value + 1,      //skip the G
								value + 1   //skip the G
						);
				}
			}
		}
		// FIXME - modify for Kio-Sword
		// <note> tag
		else if (!strcmp(tag.getName(), "note")) {
			if (!tag.isEndTag()) {
				if (!tag.isEmpty()) {
					SWBuf type = tag.getAttribute("type");
					SWBuf footnoteNumber = tag.getAttribute("swordFootnote");
					VerseKey *vkey;
					// see if we have a VerseKey * or descendant
					try {
						vkey = SWDYNAMIC_CAST(VerseKey, u->key);
					}
					catch ( ... ) {	}
					if (vkey) {
						// leave this special osis type in for crossReference notes types?  Might thml use this some day? Doesn't hurt.
						char ch = ((tag.getAttribute("type") && ((!strcmp(tag.getAttribute("type"), "crossReference")) || (!strcmp(tag.getAttribute("type"), "x-cross-ref")))) ? 'x':'n');
						buf.appendFormatted("<a href=\"noteID=%s.%c.%s\"><small><sup>*%c</sup></small></a> ", vkey->getText(), ch, footnoteNumber.c_str(), ch);
					}
					u->suspendTextPassThru = true;
				}
			}
			if (tag.isEndTag()) {
				u->suspendTextPassThru = false;
			}
		}
		// <scripRef> tag
		else if (!strcmp(tag.getName(), "scripRef")) {
			if (!tag.isEndTag()) {
				if (!tag.isEmpty()) {
					u->suspendTextPassThru = true;
				}
			}
			if (tag.isEndTag()) {	//	</scripRef>
				if (!u->BiblicalText) {
					SWBuf refList = u->startTag.getAttribute("passage");
					if (!refList.length())
						refList = u->lastTextNode;
					SWBuf version = tag.getAttribute("version");
					buf += "<a href=\"sword:/?modtype=bible";
					if (version.length()) {
						buf += "&version=";
						buf += version;
						buf += " ";
					}
					buf += "&query=";
					buf += refList.c_str();
					buf += "\">";
					buf += u->lastTextNode.c_str();
					buf += "</a>";
				}
				// FIXME for Kio-Sword
				else {
					SWBuf footnoteNumber = u->startTag.getAttribute("swordFootnote");
					VerseKey *vkey;
					// see if we have a VerseKey * or descendant
					try {
						vkey = SWDYNAMIC_CAST(VerseKey, u->key);
					}
					catch ( ... ) {}
					if (vkey) {
						// leave this special osis type in for crossReference notes types?  Might thml use this some day? Doesn't hurt.
						buf.appendFormatted("<a href=\"noteID=%s.x.%s\"><small><sup>*x</sup></small></a> ", vkey->getText(), footnoteNumber.c_str());
					}
				}

				// let's let text resume to output again
				u->suspendTextPassThru = false;
			}
		}
		else if (tag.getName() && !strcmp(tag.getName(), "div")) {
			if (tag.isEndTag() && u->SecHead) {
				buf += "</i></b><br />";
				u->SecHead = false;
			}
			else if (tag.getAttribute("class")) {
				if (!stricmp(tag.getAttribute("class"), "sechead")) {
					u->SecHead = true;
					buf += "<br /><b><i>";
				}
				else if (!stricmp(tag.getAttribute("class"), "title")) {
					u->SecHead = true;
					buf += "<br /><b><i>";
				}
			}
		}
		else if (tag.getName() && (!strcmp(tag.getName(), "img") || !strcmp(tag.getName(), "image"))) {
			const char *src = strstr(token, "src");
			if (!src)		// assert we have a src attribute
				return false;

			buf += '<';
			for (const char *c = token; *c; c++) {
				if (c == src) {
					for (;((*c) && (*c != '"')); c++)
						buf += *c;

					if (!*c) { c--; continue; }

					buf += '"';
					if (*(c+1) == '/') {
						buf += "file:";
						buf += userData->module->getConfigEntry("AbsoluteDataPath");
						if (buf[buf.length()-2] == '/')
							c++;		// skip '/'
					}
					continue;
				}
				buf += *c;
			}
			buf += '>';
		}
		else {
			buf += '<';
			/*for (const char *tok = token; *tok; tok++)
				buf += *tok;*/
			buf += token;
			buf += '>';
			//return false;  // we still didn't handle token
		}
	}
	return true;
}
