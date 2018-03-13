/* v/http.c
**
*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <setjmp.h>
#include <gmp.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <uv.h>
#include <errno.h>
#include <curses.h>
#include <termios.h>
#include <term.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "all.h"
#include "vere/vere.h"

#include "h2o.h"

/* _cttp_bod(): create a data buffer.
*/
static u3_hbod*
_cttp_bod(c3_w len_w, const c3_y* hun_y)
{
  u3_hbod* bod_u = c3_malloc(len_w + sizeof(*bod_u));

  bod_u->len_w = len_w;
  memcpy(bod_u->hun_y, hun_y, len_w);

  bod_u->nex_u = 0;
  return bod_u;
}

/* _cttp_bud(): create a header buffer.  Not null-terminated!
*/
static u3_hbod*
_cttp_bud(c3_c* nam_c, c3_c* val_c)
{
  c3_w lnm_w     = strlen(nam_c);
  c3_w lvl_w     = strlen(val_c);
  c3_w len_w     = lnm_w + 2 + lvl_w + 2;
  u3_hbod* bod_u = c3_malloc(len_w + sizeof(*bod_u));

  strncpy((c3_c *)bod_u->hun_y, nam_c, lnm_w);
  strncpy((c3_c *)bod_u->hun_y + lnm_w, ": ", 2);
  strncpy((c3_c *)bod_u->hun_y + lnm_w + 2, val_c, lvl_w);
  strncpy((c3_c *)bod_u->hun_y + lnm_w + 2 + lvl_w, "\r\n", 2);

  bod_u->len_w = len_w;
  bod_u->nex_u = 0;

  return bod_u;
}

/* _cttp_heds_free(): free header structure.
*/
static void
_cttp_heds_free(u3_hhed* hed_u)
{
  while ( hed_u ) {
    u3_hhed* nex_u = hed_u->nex_u;

    if ( hed_u->nam_c ) free(hed_u->nam_c);
    if ( hed_u->val_c ) free(hed_u->val_c);

    free(hed_u);
    hed_u = nex_u;
  }
}

/* _cttp_bods_free(): free body structure.
*/
static void
_cttp_bods_free(u3_hbod* bod_u)
{
  while ( bod_u ) {
    u3_hbod* nex_u = bod_u->nex_u;

    free(bod_u);
    bod_u = nex_u;
  }
}

/* _cttp_bods_to_octs: translate body into octet-stream noun.
*/
static u3_noun
_cttp_bods_to_octs(u3_hbod* bod_u)
{
  c3_w    len_w;
  c3_y*   buf_y;
  u3_noun cos;

  {
    u3_hbod* bid_u;

    len_w = 0;
    for ( bid_u = bod_u; bid_u; bid_u = bid_u->nex_u ) {
      len_w += bid_u->len_w;
    }
  }
  buf_y = c3_malloc(len_w);

  {
    c3_y* ptr_y = buf_y;

    while ( bod_u ) {
      memcpy(ptr_y, bod_u->hun_y, bod_u->len_w);
      ptr_y += bod_u->len_w;
      bod_u = bod_u->nex_u;
    }
  }
  cos = u3i_bytes(len_w, buf_y);
  free(buf_y);
  return u3nc(len_w, cos);
}

/* _cttp_heds_list(): create headers from list.
*/
static u3_hhed*
_cttp_heds_list(u3_hhed* hed_u, u3_noun nam, u3_noun vaz)
{
  u3_noun viz = vaz;

  while ( u3_nul != viz ) {
    u3_hhed* deh_u;

    deh_u = c3_malloc(sizeof(*deh_u));
    deh_u->nam_c = u3r_string(nam);
    deh_u->val_c = u3r_string(u3h(viz));

    deh_u->nex_u = hed_u;
    hed_u = deh_u;

    viz = u3t(viz);
  }
  u3z(nam);
  u3z(vaz);
  return hed_u;
}

/* _cttp_heds_math(): create headers from noun.
*/
static u3_hhed*
_cttp_heds_math(u3_hhed* hed_u, u3_noun mah)
{
  if ( u3_nul == mah ) {
    return hed_u;
  }
  else {
    u3_noun n_mah = u3h(mah);
    u3_noun pn_mah = u3h(n_mah);
    u3_noun qn_mah = u3t(n_mah);
    u3_noun l_mah = u3h(u3t(mah));
    u3_noun r_mah = u3t(u3t(mah));

    hed_u = _cttp_heds_list(hed_u, u3k(pn_mah), u3k(qn_mah));
    hed_u = _cttp_heds_math(hed_u, u3k(l_mah));
    hed_u = _cttp_heds_math(hed_u, u3k(r_mah));

    u3z(mah);
    return hed_u;
  }
}

/* _cttp_octs_to_bod(): translate octet-stream noun into body.
*/
static u3_hbod*
_cttp_octs_to_bod(u3_noun oct)
{
  c3_w len_w;

  if ( !_(u3a_is_cat(u3h(oct))) ) {     //  2GB max
    u3m_bail(c3__fail); return 0;
  }
  len_w = u3h(oct);

  {
    u3_hbod* bod_u = c3_malloc(len_w + sizeof(*bod_u));

    bod_u->len_w = len_w;
    u3r_bytes(0, len_w, bod_u->hun_y, u3t(oct));

    bod_u->nex_u = 0;

    u3z(oct);
    return bod_u;
  }
}

/* _cttp_mcut_char(): measure/cut character.
*/
static c3_w
_cttp_mcut_char(c3_c* buf_c, c3_w len_w, c3_c chr_c)
{
  if ( buf_c ) {
    buf_c[len_w] = chr_c;
  }
  return len_w + 1;
}

/* _cttp_mcut_cord(): measure/cut cord.
*/
static c3_w
_cttp_mcut_cord(c3_c* buf_c, c3_w len_w, u3_noun san)
{
  c3_w ten_w = u3r_met(3, san);

  if ( buf_c ) {
    u3r_bytes(0, ten_w, (c3_y *)(buf_c + len_w), san);
  }
  u3z(san);
  return (len_w + ten_w);
}

/* _cttp_mcut_path(): measure/cut cord list.
*/
static c3_w
_cttp_mcut_path(c3_c* buf_c, c3_w len_w, c3_c sep_c, u3_noun pax)
{
  u3_noun axp = pax;

  while ( u3_nul != axp ) {
    u3_noun h_axp = u3h(axp);

    len_w = _cttp_mcut_cord(buf_c, len_w, u3k(h_axp));
    axp = u3t(axp);

    if ( u3_nul != axp ) {
      len_w = _cttp_mcut_char(buf_c, len_w, sep_c);
    }
  }
  u3z(pax);
  return len_w;
}

/* _cttp_mcut_host(): measure/cut host.
*/
static c3_w
_cttp_mcut_host(c3_c* buf_c, c3_w len_w, u3_noun hot)
{
  len_w = _cttp_mcut_path(buf_c, len_w, '.', u3kb_flop(u3k(hot)));
  u3z(hot);
  return len_w;
}

/* _cttp_mcut_pork(): measure/cut path/extension.
*/
static c3_w
_cttp_mcut_pork(c3_c* buf_c, c3_w len_w, u3_noun pok)
{
  u3_noun h_pok = u3h(pok);
  u3_noun t_pok = u3t(pok);

  len_w = _cttp_mcut_path(buf_c, len_w, '/', u3k(t_pok));
  if ( u3_nul != h_pok ) {
    len_w = _cttp_mcut_char(buf_c, len_w, '.');
    len_w = _cttp_mcut_cord(buf_c, len_w, u3k(u3t(h_pok)));
  }
  u3z(pok);
  return len_w;
}

/* _cttp_mcut_quay(): measure/cut query.
*/
static c3_w
_cttp_mcut_quay(c3_c* buf_c, c3_w len_w, u3_noun quy)
{
  if ( u3_nul == quy ) {
    return len_w;
  }
  else {
    u3_noun i_quy = u3h(quy);
    u3_noun pi_quy = u3h(i_quy);
    u3_noun qi_quy = u3t(i_quy);
    u3_noun t_quy = u3t(quy);

    len_w = _cttp_mcut_char(buf_c, len_w, '&');
    len_w = _cttp_mcut_cord(buf_c, len_w, u3k(pi_quy));
    len_w = _cttp_mcut_char(buf_c, len_w, '=');
    len_w = _cttp_mcut_cord(buf_c, len_w, u3k(qi_quy));

    len_w = _cttp_mcut_quay(buf_c, len_w, u3k(t_quy));
  }
  u3z(quy);
  return len_w;
}

/* _cttp_mcut_url(): measure/cut purl, producing relative URL.
*/
static c3_w
_cttp_mcut_url(c3_c* buf_c, c3_w len_w, u3_noun pul)
{
  u3_noun q_pul = u3h(u3t(pul));
  u3_noun r_pul = u3t(u3t(pul));

  len_w = _cttp_mcut_char(buf_c, len_w, '/');
  len_w = _cttp_mcut_pork(buf_c, len_w, u3k(q_pul));

  if ( u3_nul != r_pul ) {
    len_w = _cttp_mcut_char(buf_c, len_w, '?');
    len_w = _cttp_mcut_quay(buf_c, len_w, u3k(r_pul));
  }
  u3z(pul);
  return len_w;
}

/* _cttp_creq_port(): stringify port
*/
static c3_c*
_cttp_creq_port(c3_s por_s)
{
  c3_c* por_c = c3_malloc(8);
  snprintf(por_c, 7, "%d", 0xffff & por_s);
  return por_c;
}

/* _cttp_creq_url(): construct url from noun.
*/
static c3_c*
_cttp_creq_url(u3_noun pul)
{
  c3_w  len_w = _cttp_mcut_url(0, 0, u3k(pul));
  c3_c* url_c = c3_malloc(len_w + 1);

  _cttp_mcut_url(url_c, 0, pul);
  url_c[len_w] = 0;

  return url_c;
}

/* _cttp_creq_host(): construct host from noun.
*/
static c3_c*
_cttp_creq_host(u3_noun hot)
{
  c3_w  len_w = _cttp_mcut_host(0, 0, u3k(hot));
  c3_c* hot_c = c3_malloc(len_w + 1);

  _cttp_mcut_host(hot_c, 0, hot);
  hot_c[len_w] = 0;

  return hot_c;
}

/* _cttp_creq_ip(): stringify ip
*/
static c3_c*
_cttp_creq_ip(c3_w ipf_w)
{
  c3_c* ipf_c = c3_malloc(17);
  snprintf(ipf_c, 16, "%d.%d.%d.%d", (ipf_w >> 24),
                                     ((ipf_w >> 16) & 255),
                                     ((ipf_w >> 8) & 255),
                                     (ipf_w & 255));
  return ipf_c;
}

/* _cttp_httr(): deliver http result.
*/
static void
_cttp_httr(c3_l num_l, c3_w sas_w, u3_noun mes, u3_noun uct)
{
  u3_noun htr = u3nt(sas_w, mes, uct);
  u3_noun pox = u3nt(u3_blip, c3__http, u3_nul);

  u3v_plan(pox, u3nt(c3__they, num_l, htr));
}

/* _cttp_httr_cres(): deliver valid response.
*/
static void
_cttp_httr_cres(c3_l num_l, u3_cres* res_u)
{
  _cttp_httr(num_l, res_u->sas_w, res_u-> hed,
             ( !res_u->bod_u ) ? u3_nul :
             u3nc(u3_nul, _cttp_bods_to_octs(res_u->bod_u)));
}

/* _cttp_httr_fail(): fail out a request by number.
*/
static void
_cttp_httr_fail(c3_l num_l, c3_w cod_w, c3_c* msg_c)
{
  if ( msg_c ) {
    fprintf(stderr, "http: fail (%d, %d): %s\r\n", num_l, cod_w, msg_c);
  } else {
    fprintf(stderr, "http: fail (%d, %d): %s\r\n", num_l, cod_w, msg_c);
  }
  return _cttp_httr(num_l, cod_w, u3_nul, u3_nul);
}

/* _cttp_cres_free(): free a u3_cres.
*/
static void
_cttp_cres_free(u3_cres* res_u)
{
  _cttp_bods_free(res_u->bod_u);
  free(res_u);
}

/* _cttp_creq_fire_body(): attach body to request buffers.
*/
static void
_cttp_creq_fire_body(u3_creq* ceq_u, u3_hbod *rub_u)
{
  if ( !(ceq_u->rub_u) ) {
    ceq_u->rub_u = ceq_u->bur_u = rub_u;
  }
  else {
    ceq_u->bur_u->nex_u = rub_u;
    ceq_u->bur_u = rub_u;
  }
}

/* _cttp_creq_fire_str(): attach string to request buffers.
*/
static void
_cttp_creq_fire_str(u3_creq* ceq_u, const c3_c* str_c)
{
  _cttp_creq_fire_body(ceq_u, _cttp_bod(strlen(str_c), (const c3_y*)str_c));
}

/* _cttp_creq_fire_heds(): attach output headers.
*/
static void
_cttp_creq_fire_heds(u3_creq* ceq_u, u3_hhed* hed_u)
{
  while ( hed_u ) {
    _cttp_creq_fire_body(ceq_u, _cttp_bud(hed_u->nam_c, hed_u->val_c));
    hed_u = hed_u->nex_u;
  }
}

/* _cttp_creq_fire(): load request data for into buffers.
*/
static void
_cttp_creq_fire(u3_creq* ceq_u)
{
  switch ( ceq_u->met_m ) {
    default: c3_assert(0);
    case c3__get:   _cttp_creq_fire_str(ceq_u, "GET ");      break;
    case c3__put:   _cttp_creq_fire_str(ceq_u, "PUT ");      break;
    case c3__post:  _cttp_creq_fire_str(ceq_u, "POST ");     break;
    case c3__head:  _cttp_creq_fire_str(ceq_u, "HEAD ");     break;
    case c3__conn:  _cttp_creq_fire_str(ceq_u, "CONNECT ");  break;
    case c3__delt:  _cttp_creq_fire_str(ceq_u, "DELETE ");   break;
    case c3__opts:  _cttp_creq_fire_str(ceq_u, "OPTIONS ");  break;
    case c3__trac:  _cttp_creq_fire_str(ceq_u, "TRACE ");    break;
  }
  _cttp_creq_fire_str(ceq_u, ceq_u->url_c);
  _cttp_creq_fire_str(ceq_u, " HTTP/1.1\r\n");

  c3_c* hot_c = ceq_u->hot_c ? ceq_u->hot_c : ceq_u->ipf_c;

  if ( ceq_u->por_c ) {
    c3_w len_w  = strlen(hot_c) + 1 + strlen(ceq_u->por_c) + 1;
    c3_c* hos_c = c3_malloc(len_w);
    snprintf(hos_c, len_w, "%s:%s", hot_c, ceq_u->por_c);

    _cttp_creq_fire_body(ceq_u, _cttp_bud("Host", hos_c));
    free(hos_c);
  }
  else {
    _cttp_creq_fire_body(ceq_u, _cttp_bud("Host", hot_c));
  }

  _cttp_creq_fire_str(ceq_u, "User-Agent: urbit/vere-" URBIT_VERSION "\r\n");
  _cttp_creq_fire_heds(ceq_u, ceq_u->hed_u);

  if ( !ceq_u->bod_u ) {
    _cttp_creq_fire_str(ceq_u, "\r\n");
  }
  else {
    c3_c len_c[21];
    snprintf(len_c, 20, "%u", ceq_u->bod_u->len_w);

    _cttp_creq_fire_body(ceq_u, _cttp_bud("Content-Length", len_c));
    _cttp_creq_fire_body(ceq_u, ceq_u->bod_u);
  }
}

static void
_cttp_creq_link(u3_creq* ceq_u)
{
  ceq_u->nex_u = u3_Host.ctp_u.ceq_u;
  u3_Host.ctp_u.ceq_u = ceq_u;
}

static void
_cttp_creq_unlink(u3_creq* ceq_u)
{
  if ( ceq_u->pre_u ) {
    ceq_u->pre_u->nex_u = ceq_u->nex_u;
  }
  else {
    u3_Host.ctp_u.ceq_u = ceq_u->nex_u;
  }
}

/* _cttp_creq_free(): free a u3_creq.
*/
static void
_cttp_creq_free(u3_creq* ceq_u)
{
  _cttp_creq_unlink(ceq_u);

  _cttp_heds_free(ceq_u->hed_u);
  // Note: ceq_u->bod_u is covered here
  _cttp_bods_free(ceq_u->rub_u);

  if ( ceq_u->res_u ) {
    _cttp_cres_free(ceq_u->res_u);
  }

  free(ceq_u->hot_c);
  free(ceq_u->por_c);
  free(ceq_u->url_c);
  free(ceq_u->vec_u);
  free(ceq_u);
}

// req from ++hiss
static u3_creq*
_cttp_creq_new(c3_l num_l, u3_noun hes)
{
  u3_creq* ceq_u = c3_calloc(sizeof(*ceq_u));

  u3_noun pul = u3h(hes);      // ++purl
  u3_noun hat = u3h(pul);      // ++hart
  u3_noun sec = u3h(hat);
  u3_noun por = u3h(u3t(hat));
  u3_noun hot = u3t(u3t(hat)); // ++host
  u3_noun moh = u3t(hes);      // ++moth
  u3_noun met = u3h(moh);      // ++meth
  u3_noun mah = u3h(u3t(moh)); // ++math
  u3_noun bod = u3t(u3t(moh));

  ceq_u->num_l = num_l;
  ceq_u->sec   = sec;

  if ( c3y == u3h(hot) ) {
    ceq_u->hot_c = _cttp_creq_host(u3k(u3t(hot)));
  } else {
    ceq_u->ipf_w = u3r_word(0, u3t(hot));
    ceq_u->ipf_c = _cttp_creq_ip(ceq_u->ipf_w);
  }

  if ( u3_nul != por ) {
    ceq_u->por_s = u3t(por);
    ceq_u->por_c = _cttp_creq_port(ceq_u->por_s);
  }

  ceq_u->met_m = met;
  ceq_u->url_c = _cttp_creq_url(u3k(pul));
  ceq_u->hed_u = _cttp_heds_math(0, u3k(mah));

  if ( u3_nul != bod ) {
    ceq_u->bod_u = _cttp_octs_to_bod(u3k(u3t(bod)));
  }

  _cttp_creq_link(ceq_u);

  u3z(hes);
  return ceq_u;
}

static void
_cttp_cres_queue_buf(u3_cres* res_u, h2o_buffer_t* buf_u)
{
  u3_hbod* bod_u = _cttp_bod(buf_u->size, (const c3_y *)buf_u->bytes);

  if ( !(res_u->bod_u) ) {
    res_u->bod_u = res_u->dob_u = bod_u;
  }
  else {
    res_u->dob_u->nex_u = bod_u;
    res_u->dob_u = bod_u;
  }
}

static h2o_iovec_t*
_cttp_bufs_to_vec(u3_hbod* rub_u, c3_w* tot_w)
{
  u3_hbod* bur_u = rub_u;
  h2o_iovec_t* vec_u;
  c3_w len_w;

  len_w = 0;
  while( rub_u ) {
    len_w++;
    rub_u = rub_u->nex_u;
  }

  vec_u = c3_malloc(sizeof(h2o_iovec_t) * len_w);

  rub_u = bur_u;
  len_w = 0;
  while( rub_u ) {
    vec_u[len_w] = h2o_iovec_init(rub_u->hun_y, rub_u->len_w);
    len_w++;
    rub_u = rub_u->nex_u;
  }

  *tot_w = len_w;

  return vec_u;
}

// XX research: may be called with closed client?
static int
on_body(h2o_http1client_t* cli_u, const c3_c* err_c)
{
  u3_creq* ceq_u = (u3_creq *)cli_u->data;
  u3_cres* res_u = ceq_u->res_u;

  if ( 0 != err_c && h2o_http1client_error_is_eos != err_c ) {
    _cttp_httr_fail(ceq_u->num_l, 504, (c3_c *)err_c);
    _cttp_creq_free(ceq_u);
    return -1;
  }

  h2o_socket_t* sok_u = cli_u->sock;

  if ( sok_u->input->size ) {
    _cttp_cres_queue_buf(res_u, sok_u->input);
    h2o_buffer_consume(&sok_u->input, sok_u->input->size);
  }

  if ( h2o_http1client_error_is_eos == err_c ) {
    _cttp_httr_cres(ceq_u->num_l, res_u);
    _cttp_creq_free(ceq_u);
  }

  return 0;
}

/* _http_vec_to_atom(): convert h2o_iovec_t to atom (cord)
*/
static u3_noun
_http_vec_to_atom(h2o_iovec_t vec_u)
{
  // XX portable?
  return u3i_bytes(vec_u.len, (const c3_y*)vec_u.base);
}

static u3_noun
_cttp_heds_to_noun(h2o_header_t* hed_u, c3_d hed_d)
{
  u3_noun hed = u3_nul;
  c3_d dex_d  = hed_d;

  h2o_header_t deh_u;

  while ( 0 < dex_d ) {
    deh_u = hed_u[--dex_d];
    hed = u3nc(u3nc(_http_vec_to_atom(*deh_u.name),
                    _http_vec_to_atom(deh_u.value)), hed);
  }

  return hed;
}

static h2o_http1client_body_cb
on_head(h2o_http1client_t* cli_u, const c3_c* err_c, c3_i ver_i, c3_i sas_i,
                                  h2o_iovec_t sas_u, h2o_header_t* hed_u,
                                  size_t hed_t, c3_i len_i)
{
  u3_creq* ceq_u = (u3_creq *)cli_u->data;

  if ( 0 != err_c && h2o_http1client_error_is_eos != err_c ) {
    _cttp_httr_fail(ceq_u->num_l, 504, (c3_c *)err_c);
    _cttp_creq_free(ceq_u);
    return 0;
  }

  u3_cres* res_u = c3_calloc(sizeof(*res_u));
  ceq_u->res_u = res_u;
  res_u->sas_w = (c3_w)sas_i;
  res_u->hed = _cttp_heds_to_noun(hed_u, hed_t);

  if ( h2o_http1client_error_is_eos == err_c ) {
    _cttp_httr_cres(ceq_u->num_l, res_u);
    _cttp_creq_free(ceq_u);
    return 0;
  }

  return on_body;
}

static h2o_http1client_head_cb
on_connect(h2o_http1client_t* cli_u, const c3_c* err_c, h2o_iovec_t** vec_p,
                                     size_t* vec_t, c3_i* hed_i)
{
  u3_creq* ceq_u = (u3_creq *)cli_u->data;
  c3_w len_w;

  if ( 0 != err_c ) {
    _cttp_httr_fail(ceq_u->num_l, 504, (c3_c *)err_c);
    _cttp_creq_free(ceq_u);
    return 0;
  }

  ceq_u->vec_u = _cttp_bufs_to_vec(ceq_u->rub_u, &len_w);
  *vec_t = len_w;
  *vec_p = ceq_u->vec_u;
  *hed_i = c3__head == ceq_u->met_m;

  return on_head;
}

static void
_cttp_creq_connect(u3_creq* ceq_u)
{
  c3_assert(ceq_u->ipf_c);

  h2o_iovec_t ipf_u = h2o_iovec_init(ceq_u->ipf_c, strlen(ceq_u->ipf_c));
  c3_s por_s = ceq_u->por_s ? ceq_u->por_s :
               ( c3y == ceq_u->sec ) ? 443 : 80;

  // connect by IP
  h2o_http1client_connect(&ceq_u->cli_u, ceq_u, u3_Host.ctp_u.ctx_u, ipf_u,
                                         por_s, c3y == ceq_u->sec, on_connect);

  // set hostname for TLS handshake
  if ( ceq_u->hot_c && c3y == ceq_u->sec ) {
    free(ceq_u->cli_u->ssl.server_name);
    c3_w len_w = 1 + strlen(ceq_u->hot_c);
    ceq_u->cli_u->ssl.server_name = c3_malloc(len_w);
    strncpy(ceq_u->cli_u->ssl.server_name, ceq_u->hot_c, len_w);
  }

  _cttp_creq_fire(ceq_u);
}

static void
_cttp_creq_resolve_cb(uv_getaddrinfo_t* adr_u,
                      c3_i              sas_i,
                      struct addrinfo*  aif_u)
{
  u3_creq* ceq_u = adr_u->data;

  if ( 0 != sas_i ) {
    _cttp_httr_fail(ceq_u->num_l, 504, (c3_c *)uv_strerror(sas_i));
    _cttp_creq_free(ceq_u);
    return;
  }

  ceq_u->ipf_w = ntohl(((struct sockaddr_in *)aif_u->ai_addr)->sin_addr.s_addr);
  ceq_u->ipf_c = _cttp_creq_ip(ceq_u->ipf_w);

  uv_freeaddrinfo(aif_u);

  _cttp_creq_connect(ceq_u);
}

static void
_cttp_creq_resolve(u3_creq* ceq_u)
{
  c3_assert(ceq_u->hot_c);

  uv_getaddrinfo_t* adr_u = c3_malloc(sizeof(*adr_u));
  adr_u->data = ceq_u;

  struct addrinfo hin_u;
  memset(&hin_u, 0, sizeof(struct addrinfo));

  hin_u.ai_family = PF_INET;
  hin_u.ai_socktype = SOCK_STREAM;
  hin_u.ai_protocol = IPPROTO_TCP;

  c3_c* por_c = ceq_u->por_c ? ceq_u->por_c :
                ( c3y == ceq_u->sec ) ? "443" : "80";

  c3_i sas_i;

  if ( 0 != (sas_i = uv_getaddrinfo(u3L, adr_u, _cttp_creq_resolve_cb,
                                         ceq_u->hot_c, por_c, &hin_u)) ) {
    _cttp_httr_fail(ceq_u->num_l, 504, (c3_c *)uv_strerror(sas_i));
    _cttp_creq_free(ceq_u);
  }
}

static void
_cttp_creq_start(u3_creq* ceq_u)
{
  if ( ceq_u->ipf_c ) {
    _cttp_creq_connect(ceq_u);
  } else {
    _cttp_creq_resolve(ceq_u);
  }
}

/* u3_cttp_ef_thus(): send %thus effect (outgoing request) to cttp.
*/
void
u3_cttp_ef_thus(c3_l    num_l,
                u3_noun cuq)
{
  if ( u3_nul == cuq ) {
    uL(fprintf(uH, "thus: cancel?\n"));
  }
  else {
#if 1
    // ++  hiss  {p/purl q/moth}
    // ++  purl  {p/hart q/pork r/quay}
    // ++  hart  {p/? q/(unit @ud) r/host}
    // ++  host  (each (list @t) @if)
    // ++  pork  {p/(unit @ta) q/(list @t)}
    // ++  quay  (list {p/@t q/@t})
    // ++  moth  {p/meth q/math r/(unit octs)}
    // ++  math  (map @t (list @t))
    u3_noun span = u3v_wish("-:!>(*{"
                              "purl={"
                                "p/{p/? q/(unit @ud) r/(each (list @t) @if)} "
                                "q/{p/(unit @ta) q/(list @t)} "
                                "r/(list {p/@t q/@t})"
                              "} "
                              "moth={p/@tas q/(map @t (list @t)) r/(unit octs)}"
                            "})");

    u3m_tape(u3dc("text", span, u3k(u3t(cuq))));
    uL(fprintf(uH, "\n"));
#endif

    u3_creq* ceq_u = _cttp_creq_new(num_l, u3k(u3t(cuq)));
    _cttp_creq_start(ceq_u);
  }
  u3z(cuq);
}

/* u3_cttp_io_init(): initialize http client I/O.
*/
void
u3_cttp_io_init()
{
  u3_Host.ssl_u = SSL_CTX_new(TLSv1_client_method());
  SSL_CTX_set_options(u3S, SSL_OP_NO_SSLv2);
  SSL_CTX_set_verify(u3S, SSL_VERIFY_PEER, 0);
  SSL_CTX_set_default_verify_paths(u3S);
  // if ( 0 == SSL_CTX_load_verify_locations(u3S,
  //             "/etc/ssl/certs/ca-certificates.crt", NULL) ) {
  //   fprintf(stderr, "\tload-error\r\n");
  // } else {
  //   fprintf(stderr, "\tload-good\r\n");
  // }

  SSL_CTX_set_session_cache_mode(u3S, SSL_SESS_CACHE_OFF);
  SSL_CTX_set_cipher_list(u3S, "ECDH+AESGCM:DH+AESGCM:ECDH+AES256:DH+AES256:"
                          "ECDH+AES128:DH+AES:ECDH+3DES:DH+3DES:RSA+AESGCM:"
                          "RSA+AES:RSA+3DES:!aNULL:!MD5:!DSS");

  h2o_timeout_t* tim_u = c3_malloc(sizeof(*tim_u));

  // XX how long? 1 minute?
  h2o_timeout_init(u3L, tim_u, 10000);

  h2o_http1client_ctx_t* ctx_u = c3_calloc(sizeof(*ctx_u));
  ctx_u->loop = u3L;
  ctx_u->ssl_ctx = u3S;
  ctx_u->io_timeout = tim_u;

  u3_Host.ctp_u.ceq_u = 0;
  u3_Host.ctp_u.ctx_u = ctx_u;
}

/* u3_cttp_io_poll(): poll kernel for cttp I/O.
*/
void
u3_cttp_io_poll(void)
{
}

/* u3_cttp_io_exit(): shut down cttp.
*/
void
u3_cttp_io_exit(void)
{
    SSL_CTX_free(u3S);
    free(u3_Host.ctp_u.ctx_u->io_timeout);
    free(u3_Host.ctp_u.ctx_u);
}
