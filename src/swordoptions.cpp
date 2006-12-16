/***************************************************************************
    File:         swordoptions.cpp
    Project:      Kio-Sword -- An ioslave for SWORD and KDE
    Copyright:    Copyright (C) 2004-2005 Luke Plant
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

// Internal
#include "swordoptions.h"
#include "option.h"


// Std C/C++
#include <vector>
#include <stdlib.h>

using std::vector;

namespace KioSword
{
	SwordOptions::SwordOptions()
	{
		init();
	}
	
	void SwordOptions::init()
	{
		// Setup all the options
		propagate.setup		(true,		"PropagateOptions", "p", "propagate", true);
		verseNumbers.setup	(true, 		"VerseNumbers", "vn", "versenumbers", true);
		verseLineBreaks.setup	(true, 		"VerseLineBreaks", "lb", "linebreaks", true);
		redWords.setup		(true, 		"RedWords", "rw", "redwords", true);
		footnotes.setup		(false, 	"Footnotes", "fn", "footnotes", true);
		headings.setup		(true, 		"Headings", "hd", "headings", true);
		strongs.setup		(false, 	"StrongsNumbers", "st", "strongs", true);
		morph.setup		(false, 	"MorphologicalTags", "mt", "morph", true);
		cantillation.setup	(true, 		"Cantillation", "hc", "cantillation", true);
		hebrewVowelPoints.setup (true, 		"HebrewVowelPoints", "hvp", "vowelpoints", true);
		greekAccents.setup	(true, 		"GreekAccents", "ga", "accents", true);
		styleSheet.setup	("default.css",	"StyleSheet", "ss", "stylesheet", true);
		variants.setup		(0,		"Variants", "vr", "variants", true);
		wholeBook.setup		(false, 	"WholeBook", "wb", "wholebook", false);
		doBibleIndex.setup	(true, 		"BibleIndex", "bi", "bibleindex", false);
		doDictIndex.setup	(false, 	QString::null, "di", "dictindex", false);
		doFullTreeIndex.setup	(false, 	"FullTreeIndex", "fi", "fullindex", false);
		doOtherIndex.setup	(false, 	QString::null, "oi", "otherindex", false);
		defaultBible.setup		("", 	"DefaultBible", "dfb", "defaultbible", true);
		defaultGreekStrongs.setup	("", 	"DefaultGreekStrongs", "dfgs", "defaultgreekstrongs", true);
		defaultHebrewStrongs.setup	("", 	"DefaultHebrewStrongs", "dfhs", "defaulthebrewstrongs", true);
		defaultGreekMorph.setup		("", 	"DefaultGreekMorph", "dfgm", "defaultgreekmorph", true);
		defaultHebrewMorph.setup	("", 	"DefaultHebrewMorph", "dfhm", "defaulthebrewmorph", true);
		locale.setup		("",		"Locale", "l", "locale", true);
	
		m_optionList.push_back(&propagate);
		m_optionList.push_back(&verseNumbers);
		m_optionList.push_back(&verseLineBreaks);
		m_optionList.push_back(&redWords);
		m_optionList.push_back(&footnotes);
		m_optionList.push_back(&headings);
		m_optionList.push_back(&strongs);
		m_optionList.push_back(&morph);
		m_optionList.push_back(&cantillation);
		m_optionList.push_back(&hebrewVowelPoints);
		m_optionList.push_back(&greekAccents);
		m_optionList.push_back(&styleSheet);
		m_optionList.push_back(&variants);
		m_optionList.push_back(&wholeBook);
		m_optionList.push_back(&doBibleIndex);
		m_optionList.push_back(&doDictIndex);
		m_optionList.push_back(&doFullTreeIndex);
		m_optionList.push_back(&doOtherIndex);
		m_optionList.push_back(&defaultBible);
		m_optionList.push_back(&defaultGreekStrongs);
		m_optionList.push_back(&defaultHebrewStrongs);
		m_optionList.push_back(&defaultGreekMorph);
		m_optionList.push_back(&defaultHebrewMorph);
		m_optionList.push_back(&locale);
	}
	
	/** Copy constuctor */
	SwordOptions::SwordOptions(const SwordOptions& other)
	{
		init();
		vector<OptionBase*>::const_iterator it_other;
		vector<OptionBase*>::const_iterator it_end = other.m_optionList.end();
		
		vector<OptionBase*>::const_iterator it_this = m_optionList.begin();
		for(it_other = other.m_optionList.begin(); it_other != it_end; )
		{
			(*it_this)->copy((*it_other));
			++it_this;
			++it_other;
		}
	}
	
	SwordOptions::~SwordOptions()
	{
	}
	
	/** Set all (appropriate) options from the query string */
	void SwordOptions::readFromQueryString(QMap<QString, QString> items)
	{
		vector<OptionBase*>::const_iterator it;
		vector<OptionBase*>::const_iterator it_end = m_optionList.end();

		for(it = m_optionList.begin(); it != it_end; ++it)
		{
			(*it)->readFromQueryString(items, propagate());
		}
	}
	
		
	/** Read all options in from the config file/defaults */
	void SwordOptions::readFromConfig(const KConfig* config)
	{
		vector<OptionBase*>::const_iterator it;
		vector<OptionBase*>::const_iterator it_end = m_optionList.end();

		for(it = m_optionList.begin(); it != it_end; ++it)
		{
			(*it)->readFromConfig(config);
		}
	}
	
	/** Save all options to the config file */
	void SwordOptions::saveToConfig(KConfig* config)
	{
		vector<OptionBase*>::const_iterator it;
		vector<OptionBase*>::const_iterator it_end = m_optionList.end();

		for(it = m_optionList.begin(); it != it_end; ++it)
		{
			(*it)->saveToConfig(config);
		}
	}
	
	/** Get the values that should be used for building a query string */
	QMap<QString, QString> SwordOptions::getQueryStringParams() const
	{
		QMap<QString, QString> result;
		

		vector<OptionBase*>::const_iterator it;
		vector<OptionBase*>::const_iterator it_end = m_optionList.end();

		for(it = m_optionList.begin(); it != it_end; ++it)
		{
			QString name = QString::null;
			QString value = QString::null;
			(*it)->getQueryStringPair(name, value);
			if (!name.isNull() && !name.isEmpty())
			{
				result[name] = value;
			}
		}
		return result;
	}
}
