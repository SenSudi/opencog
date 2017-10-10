/*
 * OpenPsiImplicator.cc
 *
 * Copyright (C) 2017 MindCloud
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <opencog/atoms/base/ClassServer.h>
#include <opencog/atoms/base/Link.h>
#include <opencog/atoms/execution/Instantiator.h>

#include "OpenPsiImplicator.h"
#include "OpenPsiRules.h"

using namespace opencog;

std::map<Handle, HandleMap> OpenPsiImplicator::_satisfiability_cache = {};

OpenPsiImplicator::OpenPsiImplicator(AtomSpace* as) :
  InitiateSearchCB(as),
  DefaultPatternMatchCB(as),
  Satisfier(as)
{}

bool OpenPsiImplicator::grounding(const HandleMap &var_soln,
                                  const HandleMap &term_soln)
{
  // TODO: Saperated component patterns aren't handled by this function
  // as PMCGroundings is used instead. Update to handle such cases.

  // The psi-rule weight calculations could be done here.
  _result = TruthValue::TRUE_TV();

  if (0 < var_soln.size()) {
    for( auto it = var_soln.begin(); it != var_soln.end(); ++it )
    {
      if(classserver().isA(VARIABLE_NODE, (it->second)->getType())) {
        return false;
      }
    }

    // TODO: If we are here it means the suggested groundings doesn't have
    // VariableNodes, and can be cached. This doesn't account for terms
    // that are under QuoteLink, or other similar type links. How should
    // such cases be handled?

    // Store the result in cache.
    _satisfiability_cache[_pattern_body] = var_soln;

    // NOTE: If a single grounding is found then why search for more? If there
    // is an issue with instantiating the implicand then there is an issue with
    // the declard relationship between the implicant and the implicand, aka
    // user-error.
    return true;
  } else {
    // TODO: This happens when InitiateSearchCB::no_search has groundings.
    // Cases for when this happens hasn't been tested yet. Explore the
    // behavior and find a better solution. For now, log it and continue
    // searching.
    logger().info("In %s: the following _pattern_body triggered "
      "InitiateSearchCB::no_search \n %s", __FUNCTION__ ,
      _pattern_body->toString().c_str());

    return false;
  }
}

TruthValuePtr OpenPsiImplicator::check_satisfiability(const Handle& rule)
{
  // TODO: Replace this with the context, as psi-rule is
  // (context + action ->goal)
  PatternLinkPtr query =  OpenPsiRules::get_query(rule);

  // TODO:
  // 1. How to prevent stale cache?
  // 2. Also remove entry if the context isn't grounding?
  // 3. What happens if the atoms are removed for the atomspace for
  // whatever reason? -> Use either signals or query the atomspace
  // similar to SchemeSmob::ss_link/ss_node.
  // 4. Solve for multithreaded access. See ThreadSafeHandleMap.
  if (_update_cache) {
    query->satisfy(*this);
  }

  if (_satisfiability_cache.count(query->get_pattern().body)) {
    return TruthValue::TRUE_TV();
  } else {
    return TruthValue::FALSE_TV();
  }
}

Handle OpenPsiImplicator::imply(const Handle& rule)
{
  // TODO: Replace this with the action, as psi-rule is
  // (context + action ->goal)
  PatternLinkPtr query = OpenPsiRules::get_query(rule);
  Instantiator inst(_as);

  if (_satisfiability_cache.count(query->get_pattern().body)) {
    return inst.instantiate(OpenPsiRules::get_action(rule),
              _satisfiability_cache.at(query->get_pattern().body), true);
  } else {
    // NOTE: Trying to check for satisfiablity isn't done because it
    // is the responsibility of the action-selector for determining
    // what action is to be taken.
    return Handle::UNDEFINED;
  }
}
