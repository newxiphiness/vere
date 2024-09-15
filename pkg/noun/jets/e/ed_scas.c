/// @file

#include "jets/q.h"
#include "jets/w.h"

#include "noun.h"
#include "urcrypt.h"

  static u3_atom
  _cqee_scas(u3_atom sec, u3_atom sca)
  {
    c3_y sec_y[64];
    c3_y sca_y[32];

    if ( 0 != u3r_bytes_fit(64, sec_y, sec) ) {
      // hoon explicitly crashes on mis-size
      return u3m_bail(c3__exit);
    }
    if ( 0 != u3r_bytes_fit(32, sca_y, sca) ) {
      // hoon explicitly crashes on mis-size
      return u3m_bail(c3__exit);
    }
    else {
      c3_y sec_out_y[64];

      urcrypt_ed_add_scalar_private(sec_y, sca_y, sec_out_y);
      return u3i_bytes(64, sec_out_y);
    }
  }

  u3_noun
  u3wee_scas(u3_noun cor)
  {
    u3_noun a = u3r_at(u3x_sam, cor);
    u3_noun sec, sca;
    u3x_cell(a, &sec, &sca);
    if ( (u3_none == sec) || (c3n == u3ud(sec)) ||
         (u3_none == sca) || (c3n == u3ud(sca)) ) {
      return u3m_bail(c3__exit);
    }
    else {
      return _cqee_scas(sec, sca);
    }
  }
