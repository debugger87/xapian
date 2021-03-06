/** @file esetinternal.cc
 * @brief Xapian::ESet::Internal class
 */
/* Copyright (C) 2008,2010,2011,2013,2016,2017 Olly Betts
 * Copyright (C) 2011 Action Without Borders
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <config.h>

#include "esetinternal.h"

#include "xapian/enquire.h"
#include "xapian/expanddecider.h"
#include "backends/databaseinternal.h"
#include "backends/multi.h"
#include "debuglog.h"
#include "api/rsetinternal.h"
#include "expandweight.h"
#include "omassert.h"
#include "ortermlist.h"
#include "str.h"
#include "api/termlist.h"
#include "termlistmerger.h"
#include "unicode/description_append.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace std;

namespace Xapian {

string
Internal::ExpandTerm::get_description() const
{
    string desc("ExpandTerm(");
    desc += str(wt);
    desc += ", ";
    description_append(desc, term);
    desc += ')';
    return desc;
}

template<class CLASS> struct delete_ptr {
    void operator()(CLASS *p) const { delete p; }
};

/** Build a tree of binary TermList objects like QueryOptimiser does for
 *  OrPostList objects.
 */
static TermList *
build_termlist_tree(const Xapian::Database &db, const RSet & rset)
{
    Assert(!rset.empty());

    const set<Xapian::docid> & docids = rset.internal->docs;

    vector<TermList*> termlists;
    termlists.reserve(docids.size());

    try {
	for (Xapian::docid did : docids) {
	    termlists.push_back(db.internal->open_term_list_direct(did));
	}
	Assert(!termlists.empty());
	return make_termlist_merger(termlists);
    } catch (...) {
	for_each(termlists.begin(), termlists.end(), delete_ptr<TermList>());
	throw;
    }
}

void
ESet::Internal::expand(Xapian::termcount max_esize,
		       const Xapian::Database & db,
		       const RSet & rset,
		       const Xapian::ExpandDecider * edecider,
		       Xapian::Internal::ExpandWeight & eweight,
		       double min_wt)
{
    LOGCALL_VOID(EXPAND, "ESet::Internal::expand", max_esize | db | rset | edecider | eweight);
    // These two cases are handled by our caller.
    Assert(max_esize);
    Assert(!rset.empty());
    // This method should only be called once for a given ESet::Internal, so
    // check we're empty.
    Assert(ebound == 0);
    Assert(items.empty());

    unique_ptr<TermList> tree(build_termlist_tree(db, rset));
    Assert(tree.get());

    bool is_heap = false;
    while (true) {
	// See if the root needs replacing.
	TermList * new_root = tree->next();
	if (new_root) {
	    LOGLINE(EXPAND, "Replacing the root of the termlist tree");
	    tree.reset(new_root);
	}

	if (tree->at_end()) break;

	string term = tree->get_termname();

	// If there's an ExpandDecider, see if it accepts the term.
	if (edecider && !(*edecider)(term)) continue;

	++ebound;

	/* Set up the ExpandWeight by clearing the existing statistics and
	   collecting statistics for the new term. */
	eweight.collect_stats(tree.get(), term);

	double wt = eweight.get_weight();

	// If the weights are equal, we prefer the lexically smaller term and
	// so we use "<=" not "<" here.
	if (wt <= min_wt) continue;

	items.push_back(Xapian::Internal::ExpandTerm(wt, term));

	// The candidate ESet is overflowing, so remove the worst element in it
	// using a min-heap.
	if (items.size() > max_esize) {
	    if (rare(!is_heap)) {
		is_heap = true;
		make_heap(items.begin(), items.end());
	    } else {
		push_heap(items.begin(), items.end());
	    }
	    pop_heap(items.begin(), items.end());
	    items.pop_back();
	    min_wt = items.front().wt;
	}
    }

    // Now sort the contents of the new ESet.
    if (is_heap) {
	sort_heap(items.begin(), items.end());
    } else {
	sort(items.begin(), items.end());
    }
}

string
ESet::Internal::get_description() const
{
    string desc("ESet::Internal(ebound=");
    desc += str(ebound);

    vector<Xapian::Internal::ExpandTerm>::const_iterator i;
    for (i = items.begin(); i != items.end(); ++i) {
	desc += ", ";
	desc += i->get_description();
    }
    desc += ')';

    return desc;
}

ESet::ESet() : internal(new ESet::Internal) {}

ESet::ESet(const ESet & o) : internal(o.internal) { }

ESet&
ESet::operator=(const ESet & o)
{
    internal = o.internal;
    return *this;
}

ESet::~ESet() { }

Xapian::doccount
ESet::size() const
{
    return internal->items.size();
}

Xapian::termcount
ESet::get_ebound() const
{
    return internal->ebound;
}

std::string
ESet::get_description() const
{
    string desc = "ESet(";
    desc += ')';
    return desc;
}


std::string
ESetIterator::operator*() const
{
    Assert(off_from_end != 0);
    AssertRel(off_from_end, <=, eset.internal->items.size());
    return (eset.internal->items.end() - off_from_end)->get_term();
}

double
ESetIterator::get_weight() const
{
    Assert(off_from_end != 0);
    AssertRel(off_from_end, <=, eset.internal->items.size());
    return (eset.internal->items.end() - off_from_end)->get_weight();
}

std::string
ESetIterator::get_description() const
{
    string desc = "ESetIterator(";
    if (off_from_end == 0) {
	desc += "end";
    } else {
	desc += str(eset.internal->items.size() - off_from_end);
    }
    desc += ')';
    return desc;
}

}
