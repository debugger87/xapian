/** \file omstem.h
 * \brief The stemming API
 */
/* ----START-LICENCE----
 * Copyright 1999,2000,2001 BrightStation PLC
 * Copyright 2002 Ananova Ltd
 * Copyright 2002,2003 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 * -----END-LICENCE-----
 */

#ifndef OM_HGUARD_OMSTEM_H
#define OM_HGUARD_OMSTEM_H

#include <string>

/// This class provides an interface to the stemming algorithms.
class OmStem {
    public:
	class Internal;
	/// @internal Reference counted internals.
        Internal *internal;

	/// Create a stemmer object which leaves words unchanged
	OmStem();

	/** Create a new stemmer object.
	 *
	 *  @param language	a string specifying the language being used.
	 *                      This can either be the english name of the
	 *                      language, or the two letter ISO 639
	 *                      (version 1) language code.
	 *
	 *  @exception OmInvalidArgumentError will be thrown if an
	 *  unknown language is supplied.
	 */
        explicit OmStem(const std::string &language);

	/// Standard destructor
	~OmStem();

	/// Copying is allowed
	OmStem(const OmStem &);

	/// Assignment is allowed
	void operator=(const OmStem &);

	/** Stem a word.
	 *
	 *  @param word		the word to stem.
	 *  @return		a stemmed version of the word.
	 */
	std::string stem_word(const std::string &word) const;

	/** Return a list of available languages.  The list is returned
	 *  as a space-separated string.  An OmStem object is not
	 *  required for this operation.
	 */
	static std::string get_available_languages();

	/** Returns a string representing the omstem object.
	 *  Introspection method.
	 */
	std::string get_description() const;
};

#endif
