/// @file

#include "jets/q.h"
#include "jets/w.h"

#include "noun.h"
#include "urcrypt.h"

  static u3_atom
  _cqee_sign_raw(u3_atom mes,
             u3_atom pub,
	     u3_atom sec)
  {
    c3_y pub_y[32];
    c3_y sec_y[64];

    if ( 0 != u3r_bytes_fit(32, pub_y, pub) ) {
      // hoon calls suck, which calls puck, which crashes
      return u3m_bail(c3__exit);
    }
    if ( 0 != u3r_bytes_fit(64, sec_y, sec) ) {
      // hoon calls suck, which calls puck, which crashes
      return u3m_bail(c3__exit);
    }
    else {
      c3_y  sig_y[64];
      c3_w  met_w;
      c3_y* mes_y = u3r_bytes_all(&met_w, mes);

      urcrypt_ed_sign_raw(mes_y, met_w, pub_y, sec_y, sig_y);
      u3a_free(mes_y);

      return u3i_bytes(64, sig_y);
    }
  }

  u3_noun
  u3wee_sign_raw(u3_noun cor)
  {
    u3_noun a = u3r_at(u3x_sam, cor);
    u3_noun mes, pub, sec;
    u3x_trel(a, &mes, &pub, &sec);
    if ( (u3_none == mes) || (c3n == u3ud(mes)) ||
         (u3_none == pub) || (c3n == u3ud(pub)) ||
         (u3_none == sec) || (c3n == u3ud(sec)) ) {
      return u3m_bail(c3__exit);
    }
    else {
      return _cqee_sign_raw(mes, pub, sec);
    }

  }
