/***************************************************************************
    File:         ks_gbfhtml.cpp
    Project:      Kio-Sword -- An ioslave for SWORD and KDE
    Copyright:    Copyright (C) 2004-2005 Luke Plant
                  and CrossWire Bible Society 2003
                  (file based on gbfhtmlhref.cpp and osiscgi.cpp)
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

#include "ks_gbfhtml.h"

#include "utils.h"

#include <swmodule.h>
#include <utilxml.h>
#include <versekey.h>
#include <ctype.h>

#include <stdlib.h>

using namespace sword;

namespace KioSword {
	GBFHTML::GBFHTML() {
		setTokenStart("<");
		setTokenEnd(">");
		
		setTokenCaseSensitive(true);
	
		//addTokenSubstitute("Rf", ")</small></font>");
		addTokenSubstitute("FA", "<font color=\"#800000\">"); // for ASV footnotes to mark text
		addTokenSubstitute("Rx", "</a>");
		addTokenSubstitute("FI", "<i>"); // italics begin
		addTokenSubstitute("Fi", "</i>");
		addTokenSubstitute("FB", "<b>"); // bold begin
		addTokenSubstitute("Fb", "</b>");
		addTokenSubstitute("FR", "<span class='jesusquote'>"); // words of Jesus begin
		addTokenSubstitute("Fr", "</span>");
		addTokenSubstitute("FU", "<u>"); // underline begin
		addTokenSubstitute("Fu", "</u>");
		addTokenSubstitute("FO", "<cite>"); //  Old Testament quote begin
		addTokenSubstitute("Fo", "</cite>");
		addTokenSubstitute("FS", "<sup>"); // Superscript begin// Subscript begin
		addTokenSubstitute("Fs", "</sup>");
		addTokenSubstitute("FV", "<sub>"); // Subscript begin
		addTokenSubstitute("Fv", "</sub>");
		addTokenSubstitute("TT", "<span class='title'>"); // Book title begin
		addTokenSubstitute("Tt", "</span>");
		addTokenSubstitute("PP", "<cite>"); //  poetry  begin
		addTokenSubstitute("Pp", "</cite>");
		addTokenSubstitute("Fn", "</font>"); //  font  end
		addTokenSubstitute("CL", "<br />"); //  new line
		addTokenSubstitute("CM", "<br />"); //  non showing comment
		addTokenSubstitute("CG", ""); //  ???
		addTokenSubstitute("CT", ""); // ???
		addTokenSubstitute("JR", "<div align=\"right\">"); // right align begin
		addTokenSubstitute("JC", "<div align=\"center\">"); // center align begin
		addTokenSubstitute("JL", "</div>"); // align end
		
	}
	
	
	bool GBFHTML::handleToken(SWBuf &buf, const char *token, BasicFilterUserData *userData) {
		const char *tok;
		char val[128];
		char *valto;
		const char *num;
		MyUserData *u = (MyUserData *)userData;
	
		if (!substituteToken(buf, token)) {
			XMLTag tag(token);
			if (!strncmp(token, "w", 1)) {
				// OSIS Word (temporary until OSISRTF is done)
				valto = val;
				num = strstr(token, "lemma=\"x-Strongs:");
				if (num) {
					for (num+=17; ((*num) && (*num != '\"')); num++)
						*valto++ = *num;
					*valto = 0;
					if (atoi((!isdigit(*val))?val+1:val) < 5627) {
						buf += " <span class='strongs'>&lt;<a href=\"";
						buf += swordUrlForSearch(GREEKSTRONGS, val, m_swordoptions).utf8();
						buf += "\">";
						for (tok = (!isdigit(*val))?val+1:val; *tok; tok++)
								buf += *tok;
						buf += "</a>&gt;</span> ";
						//cout << buf;
						
					}
					/*	forget these for now
					else {
						// verb morph
						sprintf(wordstr, "%03d", word-1);
						module->getEntryAttributes()["Word"][wordstr]["Morph"] = val;
					}
					*/
				}
				valto = val;
				num = strstr(token, "morph=\"x-Robinson:");
				if (num) {
					for (num+=18; ((*num) && (*num != '\"')); num++)
						*valto++ = *num;
					*valto = 0;
					buf += " <span class='morph'>(<a href=\"";
					buf += swordUrlForSearch(GREEKMORPH, val, m_swordoptions).utf8();
					buf += "\">";
					for (tok = val; *tok; tok++)				
						//if(*tok != '\"') 			
							buf += *tok;		
					buf += "</a>)</span> ";					
				}
			}
			
			else if (!strncmp(token, "WG", 2) || !strncmp(token, "WH", 2)) { // strong's numbers
				buf += " <span class='strongs'>&lt;<a href=\"";
				buf += swordUrlForSearch(GREEKSTRONGS, token + 2, m_swordoptions).utf8();
				buf += "\">";
				for (tok = token + 2; *tok; tok++)
					//if(token[i] != '\"')
						buf += *tok;
				buf += "</a>&gt;</span> ";
			}
	
			else if (!strncmp(token, "WTG", 3) || !strncmp(token, "WTH", 3)) { // strong's numbers tense
				buf += " <span class='morph'>(<a href=\"";
				buf += swordUrlForSearch(GREEKMORPH, token + 3, m_swordoptions).utf8();
				buf += "\">";
				for (tok = token + 3; *tok; tok++)
					if(*tok != '\"')
						buf += *tok;
				buf += "</a>)</span> ";
			}
	
			else if (!strncmp(token, "WT", 2) && strncmp(token, "WTH", 3) && strncmp(token, "WTG", 3)) { // morph tags
				buf += " <span class='morph'>(<a href=\"";
				buf += swordUrlForSearch(GREEKMORPH, token + 2, m_swordoptions).utf8();
				buf += "\">";
				for (tok = token + 2; *tok; tok++)				
					if(*tok != '\"') 			
						buf += *tok;		
				buf += "</a>)</span>";
			}
	
			else if (!strcmp(tag.getName(), "RX")) {
				buf += "<a href=\"";
				buf += swordUrlForSearch(DEFBIBLE, token + 3, m_swordoptions);
				
				/* FIXME - restore this, work out what it does
				for (tok = token + 3; *tok; tok++) {
					if(*tok != '<' && *tok+1 != 'R' && *tok+2 != 'x') {
						buf += *tok;
					} else {
						break;
					}
				}*/
				buf += "\">";
			}
			
			// FIXME - modify for Kio-Sword
			else if (!strcmp(tag.getName(), "RF")) {
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
					//char ch = ((tag.getAttribute("type") && ((!strcmp(tag.getAttribute("type"), "crossReference")) || (!strcmp(tag.getAttribute("type"), "x-cross-ref")))) ? 'x':'n');
					buf.appendFormatted("<a href=\"noteID=%s.%c.%s\"><small><sup>*%c</sup></small></a> ", vkey->getText(), 'n', footnoteNumber.c_str(), 'n');
				}
				u->suspendTextPassThru = true;
			}
			else if (!strcmp(tag.getName(), "Rf")) {
				u->suspendTextPassThru = false;
			}
	/*
			else if (!strncmp(token, "RB", 2)) {
				buf += "<i> ";
				u->hasFootnotePreTag = true;
			}
	
			else if (!strncmp(token, "Rf", 2)) {
				buf += "&nbsp<a href=\"note=";
				buf += u->lastTextNode.c_str();
				buf += "\">";
				buf += "<small><sup>*n</sup></small></a>&nbsp";
				// let's let text resume to output again
				u->suspendTextPassThru = false;
			}
			
			else if (!strncmp(token, "RF", 2)) {
				if (u->hasFootnotePreTag) {
					u->hasFootnotePreTag = false;
					buf += "</i> ";
				}
				u->suspendTextPassThru = true;
			}
	*/
			else if (!strncmp(token, "FN", 2)) {
				buf += "<font face=\"";
				for (tok = token + 2; *tok; tok++)				
					if(*tok != '\"') 			
						buf += *tok;
				buf += "\">";
			}
	
			else if (!strncmp(token, "CA", 2)) {	// ASCII value
				buf += (char)atoi(&token[2]);
			}
			
			else {
				return false;
			}
		}
		return true;
	}
}
