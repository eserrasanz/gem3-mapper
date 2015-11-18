/*
 * PROJECT: GEMMapper
 * FILE: gpu_buffer_align_bpm.c
 * DATE: 04/09/2014
 * AUTHOR(S): Alejandro Chacon <alejandro.chacon@uab.es>
 *            Santiago Marco-Sola <santiagomsola@gmail.com>
 */

#include "gpu_buffer_align_bpm.h"
#include "../resources/gpu_modules/gpu_interface.h"

/*
 * GPU Align-BMP Constants
 */
#define GPU_ALIGN_BPM_NUM_SUB_ENTRIES  GPU_BPM_PEQ_SUBENTRIES
#define GPU_ALIGN_BPM_ENTRY_LENGTH     GPU_BPM_PEQ_ENTRY_LENGTH
#define GPU_ALIGN_BPM_SUBENTRY_LENGTH  GPU_BPM_PEQ_SUBENTRY_LENGTH
#define GPU_ALIGN_BPM_ENTRY_SIZE       (GPU_ALIGN_BPM_ENTRY_LENGTH/UINT8_SIZE)
#define GPU_ALIGN_BPM_ALPHABET_LENGTH  GPU_BPM_PEQ_ALPHABET_SIZE

/*
 * Pattern Setup
 */
GEM_INLINE void gpu_bpm_pattern_compile(bpm_pattern_t* const bpm_pattern,const uint64_t max_error) {
  // Init BPM-GPU Dimensions
  bpm_pattern->gpu_num_entries = DIV_CEIL(bpm_pattern->pattern_length,GPU_ALIGN_BPM_ENTRY_LENGTH);
  bpm_pattern->gpu_entries_per_chunk = DIV_CEIL(max_error,GPU_ALIGN_BPM_ENTRY_LENGTH);
  bpm_pattern->gpu_num_chunks = DIV_CEIL(bpm_pattern->gpu_num_entries,bpm_pattern->gpu_entries_per_chunk);
}
GEM_INLINE uint64_t gpu_bpm_pattern_get_entry_length() {
  return GPU_ALIGN_BPM_ENTRY_LENGTH;
}

/*
 * CUDA Support
 */
#ifdef HAVE_CUDA
/*
 * Setup
 */
GEM_INLINE gpu_buffer_align_bpm_t* gpu_buffer_align_bpm_new(
    const gpu_buffer_collection_t* const gpu_buffer_collection,const uint64_t buffer_no) {
  PROF_START(GP_GPU_BUFFER_ALIGN_BPM_ALLOC);
  // Alloc
  gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm = mm_alloc(gpu_buffer_align_bpm_t);
  // Init
  gpu_buffer_align_bpm->buffer = gpu_buffer_collection_get_buffer(gpu_buffer_collection,buffer_no);
  gpu_buffer_align_bpm->num_PEQ_entries = 0;
  gpu_buffer_align_bpm->num_queries = 0;
  gpu_buffer_align_bpm->num_candidates = 0;
  gpu_buffer_align_bpm->pattern_id = 0;
  TIMER_RESET(&gpu_buffer_align_bpm->timer);
  // Init buffer
  gpu_alloc_buffer_(gpu_buffer_align_bpm->buffer);
  gpu_bpm_init_buffer_(gpu_buffer_align_bpm->buffer);
  PROF_STOP(GP_GPU_BUFFER_ALIGN_BPM_ALLOC);
  // Return
  return gpu_buffer_align_bpm;
}
GEM_INLINE void gpu_buffer_align_bpm_clear(gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm) {
  // Init buffer
  gpu_bpm_init_buffer_(gpu_buffer_align_bpm->buffer);
  // Clear
  gpu_buffer_align_bpm->num_PEQ_entries = 0;
  gpu_buffer_align_bpm->num_queries = 0;
  gpu_buffer_align_bpm->num_candidates = 0;
  gpu_buffer_align_bpm->pattern_id = 0;
}
GEM_INLINE void gpu_buffer_align_bpm_delete(gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm) {
  mm_free(gpu_buffer_align_bpm);
}
GEM_INLINE void gpu_buffer_align_bpm_device(gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm,const device_t device) {
  gpu_buffer_align_bpm->device = device;
}
/*
 * Occupancy & Limits
 */
GEM_INLINE uint64_t gpu_buffer_align_bpm_get_max_candidates(gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm) {
  return gpu_bpm_buffer_get_max_candidates_(gpu_buffer_align_bpm->buffer);
}
GEM_INLINE uint64_t gpu_buffer_align_bpm_get_max_queries(gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm) {
  return gpu_bpm_buffer_get_max_queries_(gpu_buffer_align_bpm->buffer);
}
GEM_INLINE uint64_t gpu_buffer_align_bpm_get_num_candidates(gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm) {
  return gpu_buffer_align_bpm->num_candidates;
}
GEM_INLINE uint64_t gpu_buffer_align_bpm_get_num_queries(gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm) {
  return gpu_buffer_align_bpm->num_queries;
}
GEM_INLINE void gpu_buffer_align_bpm_compute_dimensions(
    gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm,const pattern_t* const pattern,
    const uint64_t total_candidates,uint64_t* const total_entries,
    uint64_t* const total_query_chunks,uint64_t* const total_candidate_chunks) {
  const bpm_pattern_t* const bpm_pattern = &pattern->bpm_pattern;
  // Calculate dimensions
  const uint64_t pattern_num_entries = bpm_pattern->gpu_num_entries;
  const uint64_t pattern_num_chunks = bpm_pattern->gpu_num_chunks;
  *total_entries += pattern_num_entries;
  *total_query_chunks += pattern_num_chunks;
  *total_candidate_chunks += pattern_num_chunks*total_candidates;
}
GEM_INLINE bool gpu_buffer_align_bpm_fits_in_buffer(
    gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm,const uint64_t total_entries,
    const uint64_t total_query_chunks,const uint64_t total_candidate_chunks) {
  // Get Limits
  const uint64_t max_PEQ_entries =  gpu_bpm_buffer_get_max_peq_entries_(gpu_buffer_align_bpm->buffer);
  const uint64_t max_queries = gpu_buffer_align_bpm_get_max_queries(gpu_buffer_align_bpm->buffer);
  // Check available space in buffer for the pattern
  if (gpu_buffer_align_bpm->num_queries+total_query_chunks > max_queries ||
      gpu_buffer_align_bpm->num_PEQ_entries+total_entries > max_PEQ_entries) {
    // Check if the pattern can fit into an empty buffer
    gem_cond_fatal_error(total_entries > max_PEQ_entries,GPU_BPM_MAX_PATTERN_LENGTH,total_entries,max_PEQ_entries);
    return false;
  }
  // Check available space in buffer for the candidates
  const uint64_t max_candidates = gpu_buffer_align_bpm_get_max_candidates_(gpu_buffer_align_bpm->buffer);
  if (gpu_buffer_align_bpm->num_candidates+total_candidate_chunks > max_candidates) {
    // Check if the pattern can fit into an empty buffer
    gem_cond_fatal_error(total_candidate_chunks > max_candidates,GPU_BPM_MAX_CANDIDATES,total_candidate_chunks,max_candidates);
    return false;
  }
  // Ok, go on
  return true;
}
/*
 * Accessors
 */
GEM_INLINE void gpu_buffer_align_bpm_add_pattern(gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm,pattern_t* const pattern) {
  // Fetch dimensions
  const uint64_t key_length = pattern->key_length;
  const bpm_pattern_t* const bpm_pattern = &pattern->bpm_pattern;
  const uint64_t num_entries = bpm_pattern->gpu_num_entries;
  const uint64_t entries_per_chunk = bpm_pattern->gpu_entries_per_chunk;
  const uint64_t num_chunks = bpm_pattern->gpu_num_chunks;
  /*
   * Insert query metadata
   */
  // Insert pattern ID(s)
  gpu_buffer_align_bpm->pattern_id = gpu_buffer_align_bpm->num_queries;
  (gpu_buffer_align_bpm->num_queries) += num_chunks;
  gpu_bpm_qry_info_t* const query_info =
      gpu_bpm_buffer_get_peq_info_(gpu_buffer_align_bpm->buffer) + gpu_buffer_align_bpm->pattern_id;
  uint64_t i, remaining_key_length = key_length;
  for (i=0;i<num_chunks;++i,++query_info) {
    query_info->posEntry = gpu_buffer_align_bpm->num_PEQ_entries + i*entries_per_chunk;
    const uint64_t chunk_length = (remaining_key_length > GPU_ALIGN_BPM_ENTRY_LENGTH) ?
        GPU_ALIGN_BPM_ENTRY_LENGTH : remaining_key_length;
    query_info->size = chunk_length;
    remaining_key_length -= chunk_length;
  }
  /*
   * [DTO] Compile PEQ pattern
   */
  // Insert pattern query/queries
  const uint64_t PEQ_entry_offset = gpu_buffer_align_bpm->num_PEQ_entries;
  gpu_bpm_qry_entry_t* const query_pattern =
      gpu_bpm_buffer_get_peq_entries_(gpu_buffer_align_bpm->buffer) + PEQ_entry_offset;
  gpu_buffer_align_bpm->num_PEQ_entries += num_entries;
  // Copy PEQ pattern
  const uint64_t bpm_pattern_num_words = pattern->bpm_pattern.pattern_num_words;
  const uint64_t gpu_buffer_align_bpm_pattern_length = num_entries*GPU_ALIGN_BPM_ENTRY_LENGTH;
  const uint64_t gpu_buffer_align_bpm_pattern_num_words = gpu_buffer_align_bpm_pattern_length/BPM_ALIGN_WORD_LENGTH;
  const uint32_t* PEQ = (uint32_t*) pattern->bpm_pattern.PEQ;
  uint64_t entry=0, subentry=0;
  for (i=0;i<bpm_pattern_num_words;++i) {
    // Update location
    if (subentry==GPU_ALIGN_BPM_NUM_SUB_ENTRIES) {
      subentry = 0; ++entry;
    }
    // Copy pattern
    uint8_t enc_char;
    for (enc_char=0;enc_char<DNA__N_RANGE;++enc_char) {
      query_pattern[entry].bitmap[enc_char][subentry] = *PEQ; ++PEQ;
      query_pattern[entry].bitmap[enc_char][subentry+1] = *PEQ; ++PEQ;
    }
    subentry += 2;
  }
  for (;i<gpu_buffer_align_bpm_pattern_num_words;++i) {
    // Update location
    if (subentry==GPU_ALIGN_BPM_NUM_SUB_ENTRIES) {
      subentry = 0; ++entry;
    }
    // Copy pattern
    uint8_t enc_char;
    for (enc_char=0;enc_char<DNA__N_RANGE;++enc_char) {
      query_pattern[entry].bitmap[enc_char][subentry] = UINT32_ONES;
      query_pattern[entry].bitmap[enc_char][subentry+1] = UINT32_ONES;
    }
    subentry += 2;
  }
}
GEM_INLINE void gpu_buffer_align_bpm_add_candidate(
    gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm,const uint64_t candidate_text_position,
    const uint64_t candidate_length,const uint64_t pattern_chunk) {
  // Insert candidate
  PROF_INC_COUNTER(GP_GPU_BUFFER_ALIGN_BPM_NUM_QUERIES);
  PROF_ADD_COUNTER(GP_GPU_BUFFER_ALIGN_BPM_CANDIDATE_LENGTH,candidate_length);
  const uint64_t candidate_offset = gpu_buffer_align_bpm->num_candidates;
  gpu_bpm_cand_info_t* const query_candidate = gpu_bpm_buffer_get_candidates_(gpu_buffer_align_bpm->buffer) + candidate_offset;
  query_candidate->query = gpu_buffer_align_bpm->pattern_id + pattern_chunk;
  query_candidate->position = candidate_text_position;
  query_candidate->size = candidate_length;
  ++(gpu_buffer_align_bpm->num_candidates);
}
GEM_INLINE void gpu_buffer_align_bpm_get_candidate(
    gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm,const uint64_t buffer_pos,
    uint64_t* const candidate_text_position,uint32_t* const candidate_length) {
  // Get candidate
  gpu_bpm_cand_info_t* const query_candidate = gpu_bpm_buffer_get_candidates_(gpu_buffer_align_bpm->buffer) + buffer_pos;
  *candidate_text_position = query_candidate->position;
  *candidate_length = query_candidate->size;
}
GEM_INLINE void gpu_buffer_align_bpm_get_candidate_result(
    gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm,const uint64_t buffer_pos,
    uint32_t* const levenshtein_distance,uint32_t* const levenshtein_match_pos) {
  // Get candidate results
  gpu_bpm_alg_entry_t* const results_buffer =
      gpu_bpm_buffer_get_alignments_(gpu_buffer_align_bpm->buffer) + buffer_pos;
  *levenshtein_distance = results_buffer->score;
  *levenshtein_match_pos = results_buffer->column;
}
/*
 * Send/Receive
 */
GEM_INLINE void gpu_buffer_align_bpm_send(gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm) {
  PROF_START(GP_GPU_BUFFER_ALIGN_BPM_SEND);
#ifdef GEM_PROFILE
  const uint64_t max_candidates = gpu_bpm_buffer_get_max_candidates_(gpu_buffer_align_bpm->buffer);
  const uint64_t max_queries = gpu_bpm_buffer_get_max_queries_(gpu_buffer_align_bpm->buffer);
  const uint64_t max_peq_entries = gpu_bpm_buffer_get_max_peq_entries_(gpu_buffer_align_bpm->buffer);
  const uint64_t used_candidates = gpu_buffer_align_bpm->num_candidates;
  const uint64_t used_queries = gpu_buffer_align_bpm->num_queries;
  const uint64_t used_peq_entries = gpu_buffer_align_bpm->num_PEQ_entries;
  PROF_ADD_COUNTER(GP_GPU_BUFFER_ALIGN_BPM_USAGE_CANDIDATES,(100*used_candidates)/max_candidates);
  PROF_ADD_COUNTER(GP_GPU_BUFFER_ALIGN_BPM_USAGE_QUERIES,(100*used_queries)/max_queries);
  PROF_ADD_COUNTER(GP_GPU_BUFFER_ALIGN_BPM_USAGE_PEQ_ENTRIES,(100*used_peq_entries)/max_peq_entries);
  TIMER_START(&gpu_buffer_align_bpm->timer);
#endif
  gpu_bpm_send_buffer_(gpu_buffer_align_bpm->buffer,gpu_buffer_align_bpm->num_PEQ_entries,
      gpu_buffer_align_bpm->num_queries,gpu_buffer_align_bpm->num_candidates);
	PROF_STOP(GP_GPU_BUFFER_ALIGN_BPM_SEND);
}
GEM_INLINE void gpu_buffer_align_bpm_receive(gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm) {
  PROF_START(GP_GPU_BUFFER_ALIGN_BPM_RECEIVE);
  gpu_bpm_receive_buffer_(gpu_buffer_align_bpm->buffer);
  PROF_STOP(GP_GPU_BUFFER_ALIGN_BPM_RECEIVE);
#ifdef GEM_PROFILE
  TIMER_STOP(&gpu_buffer_align_bpm->timer);
  COUNTER_ADD(&PROF_GET_TIMER(GP_GPU_BUFFER_ALIGN_BPM_DUTY_CYCLE)->time_ns,gpu_buffer_align_bpm->timer.accumulated);
#endif
}
/*
 * CUDA NOT-Supported
 */
#else
/*
 * Setup
 */
GEM_INLINE gpu_buffer_align_bpm_t* gpu_buffer_align_bpm_new(
    const gpu_buffer_collection_t* const gpu_buffer_collection,const uint64_t buffer_no) { GEM_CUDA_NOT_SUPPORTED(); return NULL; }
GEM_INLINE void gpu_buffer_align_bpm_clear(gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm) { GEM_CUDA_NOT_SUPPORTED(); }
GEM_INLINE void gpu_buffer_align_bpm_delete(gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm) { GEM_CUDA_NOT_SUPPORTED(); }
GEM_INLINE void gpu_buffer_align_bpm_device(gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm,const device_t device) {
  GEM_CUDA_NOT_SUPPORTED();
}
/*
 * Occupancy & Limits
 */
GEM_INLINE uint64_t gpu_buffer_align_bpm_get_max_candidates(gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm) { GEM_CUDA_NOT_SUPPORTED(); return 0; }
GEM_INLINE uint64_t gpu_buffer_align_bpm_get_max_queries(gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm) { GEM_CUDA_NOT_SUPPORTED(); return 0; }
GEM_INLINE uint64_t gpu_buffer_align_bpm_get_num_candidates(gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm) { GEM_CUDA_NOT_SUPPORTED(); return 0; }
GEM_INLINE uint64_t gpu_buffer_align_bpm_get_num_queries(gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm) { GEM_CUDA_NOT_SUPPORTED(); return 0; }
GEM_INLINE void gpu_buffer_align_bpm_compute_dimensions(
    gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm,const pattern_t* const pattern,
    const uint64_t total_candidates,uint64_t* const total_entries,
    uint64_t* const total_query_chunks,uint64_t* const total_candidate_chunks) { GEM_CUDA_NOT_SUPPORTED(); }
GEM_INLINE bool gpu_buffer_align_bpm_fits_in_buffer(
    gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm,const uint64_t total_entries,
    const uint64_t total_query_chunks,const uint64_t total_candidate_chunks) { GEM_CUDA_NOT_SUPPORTED(); return false; }
/*
 * Accessors
 */
GEM_INLINE void gpu_buffer_align_bpm_add_pattern(
    gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm,pattern_t* const pattern) { GEM_CUDA_NOT_SUPPORTED(); }
GEM_INLINE void gpu_buffer_align_bpm_add_candidate(
    gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm,const uint64_t candidate_text_position,
        const uint64_t candidate_length,const uint64_t pattern_chunk) { GEM_CUDA_NOT_SUPPORTED(); }
GEM_INLINE void gpu_buffer_align_bpm_get_candidate(
    gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm,const uint64_t buffer_pos,
    uint64_t* const candidate_text_position,uint32_t* const candidate_length) { GEM_CUDA_NOT_SUPPORTED(); }
GEM_INLINE void gpu_buffer_align_bpm_get_candidate_result(
    gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm,const uint64_t buffer_pos,
    uint32_t* const levenshtein_distance,uint32_t* const levenshtein_match_pos) { GEM_CUDA_NOT_SUPPORTED(); }
/*
 * Send/Receive
 */
GEM_INLINE void gpu_buffer_align_bpm_send(gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm) { GEM_CUDA_NOT_SUPPORTED(); }
GEM_INLINE void gpu_buffer_align_bpm_receive(gpu_buffer_align_bpm_t* const gpu_buffer_align_bpm) { GEM_CUDA_NOT_SUPPORTED(); }
#endif /* HAVE_CUDA */

