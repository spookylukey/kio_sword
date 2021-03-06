/***************************************************************************
    File:         option.h
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

#ifndef option_h
#define option_h

#include <kconfig.h>

#include <qstring.h>
#include <qmap.h>

namespace KioSword 
{
	class OptionBase
	{
		public:
		virtual void readFromQueryString(QMap<QString, QString> params, bool allowPropagating) = 0;
		virtual void getQueryStringPair(QString& name, QString& val) = 0;
		virtual void readFromConfig(const KConfig* config) = 0;
		virtual void saveToConfig(KConfig* config) = 0;
		virtual void copy(const OptionBase* other) = 0;
		
		virtual ~OptionBase() {};
	};

	/** 
	 * Template class for options that can be read/saved to a query string
	 * or config file and are used at run time to determine various things
	 *
	 */
	template <class T>
	class Option : public OptionBase
	{
	protected:
		T m_value;         // current value
		T m_propagate_value; // value we are going to propagate when creating URLs
		T m_default_value; // KioSWord internal default
		T m_config_value;  // User's default
		
		/** Convert a value from a string to the option's type */
		static const T fromString(const QString& val)
		{
			// We will specialise this function later
			// to work correctly
			T result;
			return result;
		}
		
		/** Convert the option to a string for use in query strings */
		QString toString(T val)
		{
			// Will specialise this later
			QString result;
			return result;
		}
		
		/** return the config setting */
		T readConfigSetting(const KConfig* config)
		{
			// We will specialise this later
			T result;
			return result;
		}	
	
	public:
		QString m_qsShortName; // short name in querystring
		QString m_qsLongName;  // long name in querystring
		bool m_propagate;      // true if this option can be propagated
		QString m_configName;  // name of config setting in config file
		
		Option()
		{
		};
		
		virtual ~Option()
		{
		};

		/** 
		 * Sets up the names and default value of the option 
		 *
		 * Setting configName to "" means the option is never put into the config file
		 * Setting both qsShortName and qsLongName to "" means the option is never put 
		 * in the query string.
		 *
		 * @param default_value the value the option if not set from anywhere else
		 * @param configName the name the option has in the config file, or "" to never save or read from config
		 * @param qsShortName the short name for the option when used in a query string
		 * @param qsLongName the long name for the option when use in a query string
		 * @param propagate true if this parameter can be propagated in generated query strings
		*/
		void setup(const T& default_value, const QString& configName, const QString& qsShortName, const QString& qsLongName, bool propagate)
		{
			m_value = default_value;
			m_default_value = default_value;
			m_config_value = default_value; // assume this for now
			m_propagate_value = m_value;
			m_configName = configName;
			m_qsShortName = qsShortName;
			m_qsLongName = qsLongName;
			m_propagate = propagate;
		}

		/** Get the value of the option */
		const T& operator() () const
		{
			return m_value;
		}
		
		/** Set the value of the option (including the value to propagate) */
		void set(const T& value)
		{
			m_value = value;
			m_propagate_value = value;
		}
		
		/** read and set the option from the querystring */
		virtual void readFromQueryString(QMap<QString, QString> params, bool allowPropagating)
		{
			T newval;
			bool found = false;
			
/*			// Start with defaults.  We have to do this
			// because these objects are re-used from one request to the next
			m_value = m_config_value;
			m_propagate_value = m_config_value;*/
			
			// Search for short name
			QMap<QString, QString>::const_iterator it = params.find(m_qsShortName);
			if (it != params.end())
			{
				newval = fromString(it.data());
				found = true;
			}
			if (!found) {
				// Search for long name
				it = params.find(m_qsLongName);
				if (it != params.end())
				{
					newval = fromString(it.data());
					found = true;
				}
			}
			if (found) {
				m_value = newval;
				if (m_propagate && allowPropagating) {
					m_propagate_value = newval;
				}
			}
		}
		
		/** set the name and value of a query string pair */
		virtual void getQueryStringPair(QString& name, QString& val)
		{			
			// To keep things tidy, we don't propagate in the 
			// query string values that wouldn't make a difference
			// i.e. if current value is the same as config,
			// don't propagate	
			if (m_propagate_value != m_config_value) {
				if (m_qsShortName.isEmpty())
					name.append(m_qsLongName);
				else
					name.append(m_qsShortName.copy());
				val.append(toString(m_propagate_value));
			}
		}
		
		/** read and set the value from the config file, or set to default if no config setting */
		virtual void readFromConfig(const KConfig* config)
		{
			if (!m_configName.isEmpty()) 
			{
				set(readConfigSetting(config));
			}
			else
			{
				set(m_default_value);
			}
			m_config_value = m_value;
		}
		
		/** save the value to the config file */
		virtual void saveToConfig(KConfig* config)
		{
			if (!m_configName.isEmpty())
			{
				// overloads for KConfig::writeEntry cater
				// for everything we need so far
				if (m_value != m_default_value) // keep settings file tidy
				{
					config->writeEntry(m_configName, m_value);
					m_config_value = m_value;
				}
				else
				{
					config->deleteEntry(m_configName);
				}
			}
		}
		
		/** copy value from another */
		virtual void copy(const OptionBase* other)
		{
			// we must ensure that we only copy from an object
			// of the same type.
			const Option<T>* other2 = (Option<T>*)other;
			
			// m_configName, m_default_value, m_qsShortName and m_qsLongName, and m_propagate
			// have already been set up correctly (those don't change fromString
			// the values given in the setup() method)
			m_value = other2->m_value;
			m_config_value = other2->m_config_value;
			m_propagate_value = other2->m_propagate_value;
		}
	};
	
	// Specialisations
	
	// fromString specialisations
	template<>
	inline const bool Option<bool>::fromString(const QString& val)
	{
		if (val == "0")
			return false;
		else
			return true;
	}

	template<>
	inline const QString Option<QString>::fromString(const QString& val)
	{
		return val;
	}
	
	template<>
	inline const int Option<int>::fromString(const QString& val)
	{
		return val.toInt();
	}
	
	// toString specialisations
	template<>
	inline QString Option<bool>::toString(bool val)
	{
		return val ? QString("1"): QString("0");
	}
	
	template<>
	inline QString Option<QString>::toString(QString val)
	{
		return val;
	}
	
	template<>
	inline QString Option<int>::toString(int val)
	{
		return QString::number(val);
	}
	
	// readConfigSetting specialisations
	template<>
	inline bool Option<bool>::readConfigSetting(const KConfig* config)
	{
		return config->readBoolEntry(m_configName, m_default_value);
	}
	
	template<>
	inline QString Option<QString>::readConfigSetting(const KConfig* config)
	{
		return config->readEntry(m_configName, m_default_value);
	}

	template<>
	inline int Option<int>::readConfigSetting(const KConfig* config)
	{
		return config->readNumEntry(m_configName, m_default_value);
	}
}
 
#endif
