#include <stdbool.h>
#include "rank_set.h"

uint64_t nCr52[53][64];
static void __attribute__((constructor(1024))) init_nCr52() {
  nCr52[0][0] = 1;
  for(size_t n=1; n<=52; ++n) {
    nCr52[n][0] = 1;
    nCr52[n][n] = 1;
    for(size_t k=1; k<n; ++k) {
      nCr52[n][k] = nCr52[n-1][k-1] + nCr52[n-1][k];
    }
  }
}

/* FIXME: move this else where */
static inline uint64_t nCr(size_t n, size_t r) {
  if (r <= n) {
    if (n <= 52) {
      return nCr52[n][r];
    }

    if (n-r < r) {
      r = n-r;
    }

    uint64_t result = 1;
    for(int i=0; i<r; ++i) {
      result = result*(n-i)/(i+1);
    }

    return result;
  } else {
    return 0;
  }
}

static uint16_t set_to_index[RANK_SETS], index_to_set_base[RANK_SETS];
static uint16_t * index_to_set[RANKS+1];

static void __attribute__((constructor(2048))) init_rank_set_tables() {
  index_to_set[0] = index_to_set_base;
  for(size_t i=1; i<=RANKS; ++i) {
    index_to_set[i] = index_to_set[i-1] + rank_set_index_size(i-1, 0);
  }

  for(rank_set_t set=0; set<RANK_SETS; ++set) {
    rank_set_index_t index = rank_set_index_nCr_empty(set);
    size_t m = rank_set_size(set);

    set_to_index[set]      = index;
    index_to_set[m][index] = set;
  }
}

size_t rank_set_size(rank_set_t set) {
  return __builtin_popcount(set);  
}

rank_set_t rank_set_from_rank_array(size_t n, const card_t ranks[]) {
  rank_set_t set = EMPTY_RANK_SET;
  for(size_t i=0; i<n; ++i) {
    if (!deck_valid_rank(ranks[i])) {
      return INVALID_RANK_SET;
    }
    if (rank_set_is_set(set, ranks[i])) {
      return INVALID_RANK_SET;
    }
    set = rank_set_add(set, ranks[i]);
  }
  return set;
}

rank_set_t rank_set_from_card_array(size_t n, const card_t cards[], card_t suit) {
  rank_set_t set = EMPTY_RANK_SET;
  for(size_t i=0; i<n; ++i) {
    if (!deck_valid_card(cards[i])) {
      return INVALID_RANK_SET;
    }
    if (deck_get_suit(cards[i]) == suit) {
      card_t rank = deck_get_rank(cards[i]);
      if (rank_set_is_set(set, rank)) {
        return INVALID_RANK_SET;
      }
      set = rank_set_add(set, rank);
    }
  }
  return set;
}

bool rank_set_to_rank_array(rank_set_t set, card_t ranks[]) {
  if (rank_set_valid(set)) {
    for(size_t i=0; !rank_set_empty(set); ++i) {
      ranks[i] = rank_set_next(&set);
    }
    return true;
  }
  return false;
}

bool rank_set_to_card_array(rank_set_t set, card_t cards[], card_t suit) {
  if (deck_valid_suit(suit) && rank_set_valid(set)) {
    for(size_t i=0; !rank_set_empty(set); ++i) {
      cards[i] = deck_make_card(suit, rank_set_next(&set));
    }
    return true;
  }
  return false;
}

bool rank_set_index_valid(size_t m, rank_set_index_t index, rank_set_t used) {
  return index != INVALID_RANK_SET_INDEX && index < rank_set_index_size(m, used);
}

rank_set_index_t rank_set_index_size(size_t m, rank_set_t used) {
  if (rank_set_valid(used)) {
    size_t used_size = rank_set_size(used);
    if (used_size+m <= RANKS) {
      return nCr(RANKS-used_size, m);
    }
  }
  return INVALID_RANK_SET_INDEX;
}

rank_set_index_t rank_set_index_nCr_empty(rank_set_t set) {
  if (rank_set_valid(set)) {
    size_t m = rank_set_size(set);
    
    rank_set_index_t index = 0;
    for(size_t i=0; i<m; ++i) {
      card_t rank = rank_set_next(&set);
      if (rank >= i+1) {
        index += nCr(rank, i+1); 
      }
    }

    return index;
  }
  return INVALID_RANK_SET_INDEX;
}

rank_set_index_t rank_set_index_nCr(rank_set_t set, rank_set_t used) {
  if (rank_set_valid(set) && rank_set_valid(used) && rank_set_intersect(set, used) == EMPTY_RANK_SET) {
    size_t m = rank_set_size(set);
    
    rank_set_index_t index = 0;
    for(size_t i=0; i<m; ++i) {
      card_t rank    = rank_set_next(&set);
      size_t smaller = rank_set_size(rank_set_intersect(rank_set_from_rank(rank)-1, used));
      if (rank-smaller >= i+1) {
        index       += nCr(rank-smaller, i+1); 
      }
    }

    return index;
  }
  return INVALID_RANK_SET_INDEX;
}

rank_set_t rank_set_unindex_nCr(size_t m, rank_set_index_t index, rank_set_t used) {
  if (rank_set_index_valid(m, index, used)) {
    rank_set_t set = EMPTY_RANK_SET;
    for(size_t i=0; i<m; ++i) {
      card_t r = 0, count = 0;
      for(; rank_set_is_set(used, r); ++r) {}
      for(; nCr(count+1, m-i) <= index; ++count) {
        for(++r; rank_set_is_set(used, r); ++r) {}
      }
      if (count >= m-i) {
        index -= nCr(count, m-i);
      }
      set = rank_set_set(set, r);
    }
    return set;
  }
  return INVALID_RANK_SET;
}

rank_set_index_t rank_set_index_empty(rank_set_t set) {
  if (rank_set_valid(set)) {
    return set_to_index[set];
  }
  return INVALID_RANK_SET_INDEX;
}

rank_set_index_t rank_set_index(rank_set_t set, rank_set_t used) {
  if (rank_set_valid(set) && rank_set_valid(used) && rank_set_intersect(set, used) == EMPTY_RANK_SET) {
    size_t m = rank_set_size(set);
    
    rank_set_index_t index = 0;
    for(size_t i=0; i<m; ++i) {
      card_t rank    = rank_set_next(&set);
      size_t smaller = rank_set_size(rank_set_intersect(rank_set_from_rank(rank)-1, used));
      if (rank-smaller >= i+1) {
        index       += nCr(rank-smaller, i+1); 
      }
    }

    return index;
  }
  return INVALID_RANK_SET_INDEX;
}

rank_set_t rank_set_unindex(size_t m, rank_set_index_t index, rank_set_t used) {
  if (rank_set_index_valid(m, index, used)) {
    rank_set_t set = EMPTY_RANK_SET;
    for(size_t i=0; i<m; ++i) {
      card_t r = 0, count = 0;
      for(; rank_set_is_set(used, r); ++r) {}
      for(; nCr(count+1, m-i) <= index; ++count) {
        for(++r; rank_set_is_set(used, r); ++r) {}
      }
      if (count >= m-i) {
        index -= nCr(count, m-i);
      }
      set = rank_set_set(set, r);
    }
    return set;
  }
  return INVALID_RANK_SET;
}



