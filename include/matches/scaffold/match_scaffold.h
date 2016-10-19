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

#ifndef MATCH_SCAFFOLD_H_
#define MATCH_SCAFFOLD_H_

#include "utils/essentials.h"
#include "matches/matches.h"
#include "matches/align/match_align_dto.h"
#include "matches/align/match_alignment_region.h"

/*
 * Match Scaffold Type
 */
typedef enum {
  scaffold_none,
  scaffold_alignment_chain,
  scaffold_levenshtein,
  scaffold_swg,
} match_scaffold_type;

/*
 * Match Scaffold
 */
typedef struct {
  /* Scaffold Properties */
  match_scaffold_type scaffold_type;
  bool alignment_regions_rl;
  /* Scaffold alignment-regions */
  match_alignment_region_t* alignment_regions;
  uint64_t num_alignment_regions;
  uint64_t scaffolding_coverage;
  /* Underlying Alignment */
  match_alignment_t match_alignment;
} match_scaffold_t;

/*
 * Setup
 */
void match_scaffold_init(match_scaffold_t* const match_scaffold);

/*
 * Accessors
 */
bool match_scaffold_is_null(match_scaffold_t* const match_scaffold);

/*
 * Adaptive Scaffolding of the alignment (Best effort)
 */
void match_scaffold_adaptive(
    match_scaffold_t* const match_scaffold,
    match_align_input_t* const align_input,
    match_align_parameters_t* const align_parameters,
    matches_t* const matches,
    mm_stack_t* const mm_stack);

/*
 * Check
 */
void match_scaffold_check(
    match_scaffold_t* const match_scaffold,
    match_align_input_t* const align_input,
    matches_t* const matches);

/*
 * Sorting
 */
void match_scaffold_sort_alignment_regions(
    match_scaffold_t* const match_scaffold);

/*
 * Display
 */
void match_scaffold_print(
    FILE* const stream,
    matches_t* const matches,
    match_scaffold_t* const match_scaffold);
void match_scaffold_print_pretty(
    FILE* const stream,
    matches_t* const matches,
    match_scaffold_t* const match_scaffold,
    uint8_t* const key,
    const uint64_t key_length,
    uint8_t* const text,
    const uint64_t text_length);

#endif /* MATCH_SCAFFOLD_H_ */
