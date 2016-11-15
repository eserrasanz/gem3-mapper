/*
 *  GEM-Mapper v3 (GEM3)
 *  Copyright (c) 2011-2017 by Santiago Marco-Sola  <santiagomsola@gmail.com>
 *
 *  This file is part of GEM-Mapper v3 (GEM3).
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * PROJECT: GEM-Mapper v3 (GEM3)
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 */

#include "matches/paired_matches_test.h"

/*
 * Paired Matches Condition Tests
 */
bool paired_matches_test_accuracy_reached(
    paired_matches_t* const paired_matches,
    search_parameters_t* const search_parameters,
    const uint64_t mcs) {
  // Parameters
  const uint64_t max_search_matches = search_parameters->select_parameters_align.max_search_matches;
  // Check total number of matches found so far
  if (paired_matches_get_num_maps(paired_matches) >= max_search_matches) {
    return true; // Done!
  }
  return false;
}
