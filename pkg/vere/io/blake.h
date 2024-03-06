#ifndef VERE_BLAKE_H
#define VERE_BLAKE_H

#include "vere.h"
#include "ivory.h"

#include "noun.h"
#include "ur.h"

#define BLAKE3_BLOCK_LEN  64
#define BLAKE3_OUT_LEN  32

#define u3_vec(type) u3_raw_vec

typedef enum _bao_ingest_result {
  BAO_BAD_ORDER = 1,
  BAO_FAILED = 2,
  BAO_GOOD = 3,
  BAO_DONE = 4,
} bao_ingest_result;

typedef struct _blake_pair {
  c3_y sin_y[BLAKE3_OUT_LEN];
  c3_y dex_y[BLAKE3_OUT_LEN];
} blake_pair;

typedef struct _u3_raw_vec {
  c3_w siz_w;
  c3_w len_w;
  void** vod_p;
} u3_raw_vec;

typedef struct _blake_node {
  c3_y  cev_y[BLAKE3_OUT_LEN];
  c3_y  boq_y[BLAKE3_BLOCK_LEN];
  c3_d  con_d;
  c3_y  len_y;
  c3_y  fag_y;
} blake_node;

typedef struct _blake_subtree {
  c3_d sin_d;
  c3_d dex_d;
} blake_subtree;

typedef struct _blake_bao {
	c3_w            num_w; // number of leaves == num of packets
  c3_w            con_w; // next packet to be ingested
	blake_subtree   sub_u; // internal cursor 
  u3_vec(c3_y[BLAKE3_OUT_LEN]) que_u; // internal
  u3_vec(c3_w[BLAKE3_OUT_LEN]) sta_u; // interal
} blake_bao;

void* vec_popf(u3_raw_vec*);
void* vec_pop(u3_raw_vec*, c3_w);

void vec_init(u3_raw_vec*, c3_w);

c3_w vec_len(u3_raw_vec*);

void vec_append(u3_raw_vec*, void*);

u3_raw_vec* vec_make(c3_w);

void vec_free(u3_raw_vec*);
void vec_drop(u3_raw_vec*);

blake_node* blake_leaf_hash(c3_y* dat_y, c3_w dat_w, c3_d con_d);

blake_bao* blake_bao_make(c3_w, u3_raw_vec* pof_u);
void blake_bao_free(blake_bao*);

void make_chain_value(c3_y*, blake_node*);

void log_node(blake_node*);
void log_bao(blake_bao*);

c3_y blake_bao_verify(blake_bao*, c3_y*, c3_w, blake_pair* par_u);

#endif