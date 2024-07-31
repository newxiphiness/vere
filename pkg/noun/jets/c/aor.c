/// @file

#include "jets/q.h"
#include "jets/w.h"

#include "noun.h"


  u3_noun
  u3qc_aor(u3_noun a,
           u3_noun b)
  {
    while ( 1 ) {
      if ( c3y == u3r_sing(a, b) ) {
        return c3y;
      }
      else {
        if ( c3y == u3ud(a) ) {
          if ( c3y == u3ud(b) ) {
            c3_w len_a_w, len_b_w;
            c3_w *a_words, *b_words;
            c3_y a_y, b_y;
            if ( c3y == u3a_is_cat(a) ) {
              len_a_w = 1;
              a_words = &a;
            }
            else {
              u3a_atom* a_u = u3a_to_ptr(a);
              len_a_w = (a_u->len_w);
              a_words = a_u->buf_w;
            }
            if ( c3y == u3a_is_cat(b) ) {
              len_b_w = 1;
              b_words = &b;
            }
            else {
              u3a_atom* b_u = u3a_to_ptr(b);
              len_b_w = (b_u->len_w);
              b_words = b_u->buf_w;
            }
            c3_w len_min = c3_min(len_a_w, len_b_w);
            for (c3_w i_w = 0; i_w < len_min; i_w++) {
              for (c3_w j = 0; j < 4; j++) {
                a_y = (a_words[i_w] >> (8*j)) & 0xFF;
                b_y = (b_words[i_w] >> (8*j)) & 0xFF;
                if ( a_y != b_y ) return __(a_y < b_y);
              }
            }
            return __(len_a_w < len_b_w);
          }
          else {
            return c3y;
          }
        }
        else {
          if ( c3y == u3ud(b) ) {
            return c3n;
          }
          else {
            if ( c3y == u3r_sing(u3h(a), u3h(b)) ) {
              a = u3t(a);
              b = u3t(b);
            }
            else {
              a = u3h(a);
              b = u3h(b);
            }
          }
        }
      }
    }
  }
  
  u3_noun
  u3wc_aor(u3_noun cor)
  {
    u3_noun a, b;

    if ( c3n == u3r_mean(cor, u3x_sam_2, &a, u3x_sam_3, &b, 0) ) {
      return u3m_bail(c3__exit);
    } else {
      return u3qc_aor(a, b);
    }
  }

