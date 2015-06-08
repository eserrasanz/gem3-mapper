/*
 * PROJECT: GEMMapper
 * FILE: archive_select.h
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION: // TODO
 */

#ifndef ARCHIVE_SELECT_H_
#define ARCHIVE_SELECT_H_

#include "archive_search.h"
#include "archive_select_parameters.h"

/*
 * Decoding Matches (Retrieving & Processing matches)
 */
GEM_INLINE void archive_select_decode_trace_matches(
    archive_search_t* const archive_search,matches_t* const matches,
    match_trace_t* const match_trace,const uint64_t num_match_traces,
    const bool force_strand,const strand_t strand);
GEM_INLINE void archive_select_decode_trace_matches_all(
    archive_search_t* const archive_search,matches_t* const matches,
    const bool force_strand,const strand_t strand);

/*
 * Select Paired-Matches
 */
GEM_INLINE void archive_select_matches(
    archive_search_t* const archive_search,const bool curate_matches,
    const bool score_matches,const bool sort_matches,matches_t* const matches);
GEM_INLINE void archive_select_paired_matches(
    archive_search_t* const archive_search_end1,archive_search_t* const archive_search_end2,
    paired_matches_t* const paired_matches);

/*
 * Check Matches
 */
GEM_INLINE void archive_check_matches(
    archive_t* const archive,const alignment_model_t alignment_model,
    swg_penalties_t* swg_penalties,sequence_t* const sequence,
    matches_t* const matches,const bool check_optimum_alignment,
    const bool check_complete,mm_stack_t* const mm_stack);
GEM_INLINE void archive_check_paired_matches(
    archive_t* const archive,const alignment_model_t alignment_model,
    swg_penalties_t* swg_penalties,sequence_t* const sequence_end1,
    sequence_t* const sequence_end2,paired_matches_t* const paired_matches,
    const bool check_optimum_alignment,const bool check_complete,mm_stack_t* const mm_stack);

/*
 * Error Messages
 */
//#define GEM_ERROR_ARCHIVE_SELECT_

#endif /* ARCHIVE_SELECT_H_ */
