/*
 * PROJECT: GEMMapper
 * FILE: matches.h
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION: Data structure to store alignment matches {sequence,position,strand,...}
 */

#ifndef MATCHES_H_
#define MATCHES_H_

#include "essentials.h"
#include "interval_set.h"
#include "text_collection.h"
#include "match_elements.h"
#include "swg_align.h"

/*
 * Checkers
 */
#define MATCHES_CHECK(matches) GEM_CHECK_NULL(matches)

/*
 * Interval Match
 */
typedef struct {
  /* Meta-info */
  bool emulated_rc_search; // Match resulting from a RC-emulated search (using the forward-strand)
  /* Index */
  uint64_t lo;             // SA Lo-Position
  uint64_t hi;             // SA Hi-Position
  /* Sequence */
  uint8_t* text;           // Pointer to the matching-text
  uint64_t length;         // Length of the matching-text
  strand_t strand;         // Mapping Strand
  /* Score */
  uint64_t distance;       // Distance of the alignment
  int32_t swg_score;       // SWG Distance (score) of the alignment
} match_interval_t;

/*
 * Trace Match
 */
typedef struct {
  /* Meta-info */
  uint64_t trace_offset;   // Trace-offset in the text-collection
  bool emulated_rc_search; // Match resulting from a RC-emulated search (using the forward-strand)
  /* Match */
  char* sequence_name;     // Sequence name (After decoding)
  strand_t strand;         // Mapping Strand
  uint64_t text_position;  // Position of the match in the text. Local text => seq15
  /* Score */
  uint64_t distance;       // Edit Distance
  int32_t swg_score;       // SWG Distance/Score
  uint8_t mapq_score;      // MAPQ Score
  /* Alignment */
  match_alignment_t match_alignment; // Match Alignment (CIGAR + ...)
#ifdef GEM_DEBUG
  void* match_scaffold;    // Supporting Scaffolding
#endif
} match_trace_t;

/*
 * Matches
 */
typedef struct {
  /* Search-matches state */
  uint64_t max_complete_stratum;
  /* Text Collection Buffer */
  text_collection_t* text_collection;       // Stores text-traces (candidates/matches/regions/...)
  /* Matches Counters */
  vector_t* counters;                       // Global counters
  uint64_t min_counter_value;
  uint64_t max_counter_value;
  uint64_t total_matches_count;             // Total number of matches (Interval+Position)
  int32_t max_swg_score;
  /* Interval Matches */
  vector_t* interval_matches;               // Interval Matches (match_interval_t)
  /* Position Matches */
  vector_t* position_matches;               // Position Matches (match_trace_t)
  ihash_t* begin_pos_matches;               // Begin position (of the aligned match) in the text-space
  ihash_t* end_pos_matches;                 // End position (of the aligned match) in the text-space
  /* CIGAR */
  vector_t* cigar_vector;                   // CIGAR operations storage (cigar_element_t)
} matches_t;

/*
 * Setup
 */
GEM_INLINE matches_t* matches_new();
GEM_INLINE void matches_configure(matches_t* const matches,text_collection_t* const text_collection);
GEM_INLINE void matches_clear_index(matches_t* const matches);
GEM_INLINE void matches_clear(matches_t* const matches);
GEM_INLINE void matches_delete(matches_t* const matches);

/*
 * Accessors
 */
GEM_INLINE bool matches_is_mapped(const matches_t* const matches);

/*
 * Counters
 */
GEM_INLINE uint64_t matches_counters_get_min_distance(matches_t* const matches);
GEM_INLINE uint64_t matches_counters_get_max_distance(matches_t* const matches);
GEM_INLINE int32_t matches_counters_get_max_swg_score(matches_t* const matches);
GEM_INLINE uint64_t matches_counters_compact(matches_t* const matches);
GEM_INLINE uint64_t matches_counters_get_count(matches_t* const matches,const uint64_t distance);
GEM_INLINE uint64_t matches_counters_get_total_count(matches_t* const matches);

/*
 * Matches
 */
GEM_INLINE match_trace_t* matches_get_match_traces(const matches_t* const matches);
GEM_INLINE uint64_t matches_get_num_match_traces(const matches_t* const matches);
GEM_INLINE void matches_get_clear_match_traces(const matches_t* const matches);

GEM_INLINE cigar_element_t* match_trace_get_cigar_buffer(const matches_t* const matches,const match_trace_t* const match_trace);
GEM_INLINE uint64_t match_trace_get_cigar_length(const match_trace_t* const match_trace);
GEM_INLINE uint64_t match_trace_get_distance(const match_trace_t* const match_trace);
GEM_INLINE int64_t match_trace_get_effective_length(
    matches_t* const matches,const uint64_t read_length,
    const uint64_t cigar_buffer_offset,const uint64_t cigar_length);

/*
 * Adding Matches
 */
GEM_INLINE uint64_t* matches_lookup_match(
    matches_t* const matches,const uint64_t begin_position,const uint64_t effective_length);
GEM_INLINE bool matches_add_match_trace_t(
    matches_t* const matches,match_trace_t* const match_trace,
    const bool update_counters,mm_stack_t* const mm_stack);

GEM_INLINE void matches_add_interval_match(
    matches_t* const matches,const uint64_t lo,const uint64_t hi,
    const uint64_t length,const uint64_t distance,const bool emulated_rc_search);
GEM_INLINE void matches_add_interval_set(
    matches_t* const matches,interval_set_t* const interval_set,
    const uint64_t length,const bool emulated_rc_search);

GEM_INLINE void matches_hint_allocate_match_trace(matches_t* const matches,const uint64_t num_matches_trace_to_add);
GEM_INLINE void matches_hint_allocate_match_interval(matches_t* const matches,const uint64_t num_matches_interval_to_add);

/*
 * CIGAR Handling
 */
GEM_INLINE void matches_cigar_buffer_add_cigar_element(
    cigar_element_t** const cigar_buffer_sentinel,const cigar_t cigar_element_type,
    const uint64_t element_length,uint8_t* const indel_text);

GEM_INLINE void matches_cigar_vector_append_insertion(
    vector_t* const cigar_vector,uint64_t* const current_cigar_length,
    const uint64_t indel_length,uint8_t* const indel_text);
GEM_INLINE void matches_cigar_vector_append_deletion(
    vector_t* const cigar_vector,uint64_t* const current_cigar_length,const uint64_t indel_length);
GEM_INLINE void matches_cigar_vector_append_match(
    vector_t* const cigar_vector,uint64_t* const current_cigar_length,const uint64_t match_length);
GEM_INLINE void matches_cigar_vector_append_mismatch(
    vector_t* const cigar_vector,uint64_t* const current_cigar_length,const uint8_t mismatch);

GEM_INLINE void matches_cigar_vector_append_cigar_element(
    vector_t* const cigar_vector,uint64_t* const cigar_length,cigar_element_t* const cigar_element);

GEM_INLINE void matches_cigar_reverse(
    matches_t* const matches,const uint64_t cigar_buffer_offset,const uint64_t cigar_length);
GEM_INLINE void matches_cigar_reverse_colorspace(
    matches_t* const matches,const uint64_t cigar_buffer_offset,const uint64_t cigar_length);

GEM_INLINE uint64_t matches_cigar_compute_edit_distance(
    const matches_t* const matches,const uint64_t cigar_buffer_offset,const uint64_t cigar_length);
GEM_INLINE uint64_t matches_cigar_compute_edit_distance__excluding_clipping(
    const matches_t* const matches,const uint64_t cigar_buffer_offset,const uint64_t cigar_length);

GEM_INLINE int64_t matches_cigar_element_effective_length(const cigar_element_t* const cigar_element);
GEM_INLINE int64_t matches_cigar_effective_length(
    vector_t* const cigar_vector,const uint64_t cigar_offset,const uint64_t cigar_length);

/*
 * Sorting Matches
 */
GEM_INLINE void matches_sort_by_distance(matches_t* const matches);
GEM_INLINE void matches_sort_by_swg_score(matches_t* const matches);
GEM_INLINE void matches_sort_by_mapq_score(matches_t* const matches);
GEM_INLINE void matches_sort_by_sequence_name__position(matches_t* const matches);

/*
 * Curation
 */
GEM_INLINE void matches_curate(matches_t* const matches,const double swg_score_difference);

/*
 * Score
 */
GEM_INLINE double matches_classify_unique(
    matches_t* const matches,const uint64_t mcs,
    const swg_penalties_t* const swg_penalties,const uint64_t read_length);
GEM_INLINE double matches_classify_ambiguous(
    matches_t* const matches,const uint64_t mcs,
    const swg_penalties_t* const swg_penalties,const uint64_t read_length);
GEM_INLINE double matches_classify_mmaps(
    matches_t* const matches,const uint64_t mcs,
    const swg_penalties_t* const swg_penalties,const uint64_t read_length);
GEM_INLINE double matches_classify_ties(matches_t* const matches);

/*
 * Display
 */
GEM_INLINE void match_cigar_print(
    FILE* const stream,vector_t* const cigar_vector,
    const uint64_t cigar_buffer_offset,const uint64_t cigar_length);
GEM_INLINE void matches_metrics_print(
    matches_t* const matches,const swg_penalties_t* const swg_penalties,const uint64_t read_length);

/*
 * Error Messages
 */
#define GEM_ERROR_MATCHES_CIGAR_ZERO_LENGTH "Matches. CIGAR length cannot be zero"

#endif /* MATCHES_H_ */
