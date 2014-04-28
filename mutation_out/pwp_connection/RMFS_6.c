#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include "bitfield.h"
#include "pwp_connection.h"
#include "pwp_local.h"
#include "linked_list_hashmap.h"
#include "linked_list_queue.h"
#include "sparse_counter.h"
#include "bitstream.h"
#include "meanqueue.h"
#include "pwp_connection_private.h"
#define TRUE 1
#define FALSE 0
#define pwp_msgtype_to_string(m)\
    PWP_MSGTYPE_CHOKE == (m) ? "CHOKE" :\
    PWP_MSGTYPE_UNCHOKE == (m) ? "UNCHOKE" :\
    PWP_MSGTYPE_INTERESTED == (m) ? "INTERESTED" :\
    PWP_MSGTYPE_UNINTERESTED == (m) ? "UNINTERESTED" :\
    PWP_MSGTYPE_HAVE == (m) ? "HAVE" :\
    PWP_MSGTYPE_BITFIELD == (m) ? "BITFIELD" :\
    PWP_MSGTYPE_REQUEST == (m) ? "REQUEST" :\
    PWP_MSGTYPE_PIECE == (m) ? "PIECE" :\
    PWP_MSGTYPE_CANCEL == (m) ? "CANCEL" : "none"\


static unsigned long __req_hash (const void * obj) {
    const bt_block_t * req = obj;
    return req->piece_idx + req->len + req->offset;
}

static long __req_cmp (const void * obj, const void * other) {
    const bt_block_t * req1 = obj, * req2 = other;
    if (req1->piece_idx == req2->piece_idx && req1->len == req2->len && req1->offset == req2->offset)
        return 0;
    return 1;
}

static void __log (pwp_conn_private_t * me, const char * format, ... ) {
    char buffer [1000];
    va_list args;
    if (NULL == me->cb.log)
        return;
    sprintf (buffer, "%lx ", (unsigned long) me);
    va_start (args, format);
    (void) vsnprintf (buffer +strlen (buffer), 1000, format, args);
#if 0 /* debugging */
    printf ("%s\n", buffer);
#endif
    me->cb.log (me->cb_ctx, me->peer_udata, buffer);
}

static void __disconnect (pwp_conn_private_t * me, const char * reason, ... ) {
    char buffer [128];
    va_list args;
    va_start (args, reason);
    (void) vsnprintf (buffer, 128, reason, args);
#if 0 /* debugging */
    printf ("%s\n", buffer);
#endif
    if (me->cb.disconnect)
        (void) me->cb.disconnect (me->cb_ctx, me->peer_udata, buffer);
}

static int __send_to_peer (pwp_conn_private_t * me, void * data, const int len) {
    int ret;
    if (!me->cb.send)
        return 0;
    if (0 == (ret = me->cb.send (me->cb_ctx, me->peer_udata, data, len))) {
        __disconnect (me, "peer dropped connection");
        return 0;
    }
    return 1;
}

void * pwp_conn_get_peer (pwp_conn_t * me_) {
    pwp_conn_private_t * me = (void *) me_;
    return me->peer_udata;
}

void pwp_conn_set_peer (pwp_conn_t * me_, void * peer) {
    pwp_conn_private_t * me = (void *) me_;
    me->peer_udata = peer;
}

void pwp_conn_set_progress (pwp_conn_t * me_, void * counter) {
    pwp_conn_private_t * me = (void *) me_;
    me->pieces_completed = counter;
}

void * pwp_conn_new (void * mem) {
    pwp_conn_private_t * me;
    if (mem) {
        me = mem;
    }
    else if (!(me = calloc (1, sizeof (pwp_conn_private_t)))) {
        perror ("out of memory");
        exit (0);
    }
    me->bytes_drate = meanqueue_new (10);
    me->bytes_urate = meanqueue_new (10);
    me->recv_reqs = hashmap_new (__req_hash, __req_cmp, 100);
    me->peer_reqs = llqueue_new ();
    me->reqs = llqueue_new ();
    me->req_lock = NULL;
    me->state.flags = PC_IM_CHOKING | PC_PEER_CHOKING;
    me->pieces_peerhas = sc_init (0);
    return me;
}

static void __expunge_their_pending_reqs (pwp_conn_private_t * me) {
    while (0 < llqueue_count (me->peer_reqs)) {
        free (llqueue_poll (me -> peer_reqs));
    }
}

static void __expunge_my_pending_reqs (pwp_conn_private_t * me) {
    request_t * r;
    hashmap_iterator_t iter;
    void * rem;
    rem = llqueue_new ();
    for (hashmap_iterator (me->recv_reqs, &iter); (r = hashmap_iterator_next_value (me->recv_reqs, &iter));) {
        llqueue_offer (rem, r);
#if 0
        r = hashmap_remove (me->recv_reqs, &r->blk);
        if (me->cb.peer_giveback_block)
            me->cb.peer_giveback_block (me->cb_ctx, me->peer_udata, &r->blk);
        free (r);
#endif
    }
#if 1
    while (llqueue_count (rem) > 0) {
        r = llqueue_poll (rem);
        r = hashmap_remove (me->recv_reqs, &r->blk);
        assert (r);
        if (me->cb.peer_giveback_block)
            me->cb.peer_giveback_block (me->cb_ctx, me->peer_udata, &r->blk);
        free (r);
    }
    llqueue_free (rem);
#endif
}

static void __expunge_my_old_pending_reqs (pwp_conn_private_t * me) {
    request_t * r;
    hashmap_iterator_t iter;
    void * rem;
    rem = llqueue_new ();
    for (hashmap_iterator (me->recv_reqs, &iter); (r = hashmap_iterator_next_value (me->recv_reqs, &iter));) {
        if (r && 10 < me->state.tick - r->tick) {
#if 0
            hashmap_remove (me -> recv_reqs, & r -> blk);
            me->cb.peer_giveback_block (me->cb_ctx, me->peer_udata, &r->blk);
            free (r);
#endif
            llqueue_offer (rem, r);
        }
    }
#if 1
    while (llqueue_count (rem) > 0) {
        r = llqueue_poll (rem);
        r = hashmap_remove (me->recv_reqs, &r->blk);
        assert (me -> cb.peer_giveback_block);
        me->cb.peer_giveback_block (me->cb_ctx, me->peer_udata, &r->blk);
        free (r);
    }
    llqueue_free (rem);
#endif
}

void pwp_conn_release (pwp_conn_t * me_) {
    pwp_conn_private_t * me = (void *) me_;
    __expunge_their_pending_reqs (me);
    __expunge_my_pending_reqs (me);
    hashmap_free (me -> recv_reqs);
    llqueue_free (me -> peer_reqs);
    free (me_);
}

void pwp_conn_set_piece_info (pwp_conn_t * me_, int num_pieces, int piece_len) {
    pwp_conn_private_t * me = (void *) me_;
    me->num_pieces = num_pieces;
    me->piece_len = piece_len;
}

void pwp_conn_set_cbs (pwp_conn_t * me_, pwp_conn_cbs_t * funcs, void * cb_ctx) {
    pwp_conn_private_t * me = (void *) me_;
    memcpy (& me -> cb, funcs, sizeof (pwp_conn_cbs_t));
    me->cb_ctx = cb_ctx;
}

int pwp_conn_peer_is_interested (pwp_conn_t * me_) {
    pwp_conn_private_t * me = (void *) me_;
    return 0 != (me->state.flags & PC_PEER_INTERESTED);
}

int pwp_conn_im_choking (pwp_conn_t * me_) {
    pwp_conn_private_t * me = (void *) me_;
    return 0 != (me->state.flags & PC_IM_CHOKING);
}

int pwp_conn_flag_is_set (pwp_conn_t * me_, const int flag) {
    pwp_conn_private_t * me = (void *) me_;
    return 0 != (me->state.flags & flag);
}

int pwp_conn_im_choked (pwp_conn_t * me_) {
    pwp_conn_private_t * me = (void *) me_;
    return 0 != (me->state.flags & PC_PEER_CHOKING);
}

int pwp_conn_im_interested (pwp_conn_t * me_) {
    pwp_conn_private_t * me = (void *) me_;
    return 0 != (me->state.flags & PC_IM_INTERESTED);
}

void pwp_conn_set_im_interested (pwp_conn_t * me_) {
    pwp_conn_private_t * me = (void *) me_;
    if (pwp_conn_send_statechange (me_, PWP_MSGTYPE_INTERESTED)) {
        me->state.flags |= PC_IM_INTERESTED;
    }
}

void pwp_conn_choke_peer (pwp_conn_t * me_) {
    pwp_conn_private_t * me = (void *) me_;
    me->state.flags |= PC_IM_CHOKING;
    __expunge_their_pending_reqs (me);
    pwp_conn_send_statechange (me_, PWP_MSGTYPE_CHOKE);
}

void pwp_conn_unchoke_peer (pwp_conn_t * me_) {
    pwp_conn_private_t * me = (void *) me_;
    me->state.flags &= ~PC_IM_CHOKING;
    pwp_conn_send_statechange (me_, PWP_MSGTYPE_UNCHOKE);
}

int pwp_conn_get_download_rate (const pwp_conn_t * me_ __attribute__ ((__unused__))) {
    const pwp_conn_private_t * me = (void *) me_;
    return meanqueue_get_value (me->bytes_drate);
}

int pwp_conn_get_upload_rate (const pwp_conn_t * me_ __attribute__ ((__unused__))) {
    const pwp_conn_private_t * me = (void *) me_;
    return meanqueue_get_value (me->bytes_urate);
}

int pwp_conn_send_statechange (pwp_conn_t * me_, const unsigned char msg_type) {
    pwp_conn_private_t * me = (void *) me_;
    unsigned char data [5], * ptr = data;
    bitstream_write_uint32 (& ptr, fe (1));
    bitstream_write_ubyte (& ptr, msg_type);
    __log (me, "send,%s", pwp_msgtype_to_string (msg_type));
    if (!__send_to_peer (me, data, 5)) {
        return 0;
    }
    return 1;
}

void pwp_conn_send_piece (pwp_conn_t * me_, bt_block_t * req) {
    pwp_conn_private_t * me = (void *) me_;
    unsigned char * data = NULL;
    unsigned char * ptr;
    unsigned int size;
    assert (NULL != me);
    assert (NULL != me -> cb.write_block_to_stream);
    size = 4 + 1 + 4 + 4 + req->len;
    if (!(data = malloc (size))) {
        perror ("out of memory");
        exit (0);
    }
    ptr = data;
    bitstream_write_uint32 (& ptr, fe (size - 4));
    bitstream_write_ubyte (& ptr, PWP_MSGTYPE_PIECE);
    bitstream_write_uint32 (& ptr, fe (req -> piece_idx));
    bitstream_write_uint32 (& ptr, fe (req -> offset));
    me->cb.write_block_to_stream (me->cb_ctx, req, (unsigned char * *) &ptr);
    __send_to_peer (me, data, size);
#if 0
#define BYTES_SENT 1
    for (ii = req->len; ii > 0;) {
        int len = BYTES_SENT < ii ? BYTES_SENT : ii;
        bt_piece_write_block_to_str (pce, req -> offset + req -> len - ii, len, block);
        __send_to_peer (me, block, len);
        ii -= len;
    }
#endif
    __log (me, "send,piece,piece_idx=%d offset=%d len=%d", req -> piece_idx, req -> offset, req -> len);
}

int pwp_conn_send_have (pwp_conn_t * me_, const int piece_idx) {
    pwp_conn_private_t * me = (void *) me_;
    unsigned char data [12], * ptr = data;
    bitstream_write_uint32 (& ptr, fe (5));
    bitstream_write_ubyte (& ptr, PWP_MSGTYPE_HAVE);
    bitstream_write_uint32 (& ptr, fe (piece_idx));
    __send_to_peer (me, data, 5 + 4);
    __log (me, "send,have,piece_idx=%d", piece_idx);
    return 1;
}

void pwp_conn_send_request (pwp_conn_t * me_, const bt_block_t * request) {
    pwp_conn_private_t * me = (void *) me_;
    unsigned char data [32], * ptr;
    ptr = data;
    bitstream_write_uint32 (& ptr, fe (13));
    bitstream_write_ubyte (& ptr, PWP_MSGTYPE_REQUEST);
    bitstream_write_uint32 (& ptr, fe (request -> piece_idx));
    bitstream_write_uint32 (& ptr, fe (request -> offset));
    bitstream_write_uint32 (& ptr, fe (request -> len));
    __send_to_peer (me, data, 13 + 4);
    __log (me, "send,request,piece_idx=%d offset=%d len=%d", request -> piece_idx, request -> offset, request -> len);
}

void pwp_conn_send_cancel (pwp_conn_t * me_, bt_block_t * cancel) {
    pwp_conn_private_t * me = (void *) me_;
    unsigned char data [32], * ptr;
    ptr = data;
    bitstream_write_uint32 (& ptr, fe (13));
    bitstream_write_ubyte (& ptr, PWP_MSGTYPE_CANCEL);
    bitstream_write_uint32 (& ptr, fe (cancel -> piece_idx));
    bitstream_write_uint32 (& ptr, fe (cancel -> offset));
    bitstream_write_uint32 (& ptr, fe (cancel -> len));
    __send_to_peer (me, data, 17);
    __log (me, "send,cancel,piece_idx=%d offset=%d len=%d", cancel -> piece_idx, cancel -> offset, cancel -> len);
}

void pwp_conn_set_state (pwp_conn_t * me_, const int state) {
    pwp_conn_private_t * me = (void *) me_;
    me->state.flags = state;
}

int pwp_conn_get_state (pwp_conn_t * me_) {
    pwp_conn_private_t * me = (void *) me_;
    return me->state.flags;
}

int pwp_conn_mark_peer_has_piece (pwp_conn_t * me_, const int piece_idx) {
    pwp_conn_private_t * me = (void *) me_;
    if (me->num_pieces <= piece_idx || piece_idx < 0) {
        __disconnect (me, "piece idx fits outside of boundary");
        return 0;
    }
    sc_mark_complete (me -> pieces_peerhas, piece_idx, 1);
    if (me->cb.peer_have_piece)
        me->cb.peer_have_piece (me->cb_ctx, me->peer_udata, piece_idx);
    return 1;
}

static void __req_fit (bt_block_t * request, const unsigned int piece_len) {
    if (piece_len < request->offset + request->len) {
        request->len = request->offset + request->len - piece_len;
    }
}

int pwp_conn_get_npending_requests (const pwp_conn_t * me_) {
    const pwp_conn_private_t * me = (void *) me_;
    return hashmap_count (me->recv_reqs);
}

int pwp_conn_get_npending_peer_requests (const pwp_conn_t * me_) {
    const pwp_conn_private_t * me = (void *) me_;
    return llqueue_count (me->peer_reqs);
}

void pwp_conn_request_block_from_peer (pwp_conn_t * me_, bt_block_t * blk) {
    pwp_conn_private_t * me = (void *) me_;
    request_t * req;
    if (blk->len < 0)
        return;
    __req_fit (blk, me -> piece_len);
    pwp_conn_send_request (me_, blk);
    req = malloc (sizeof (request_t));
    req->tick = me->state.tick;
    memcpy (& req -> blk, blk, sizeof (bt_block_t));
    hashmap_put (me -> recv_reqs, & req -> blk, req);
#if 0 /*  debugging */
    printf ("request block: %d %d %d", blk -> piece_idx, blk -> offset, blk -> len);
#endif
}

void pwp_conn_connect_failed (pwp_conn_t * me_) {
    pwp_conn_private_t * me = (void *) me_;
    me->state.failed_connections += 1;
    if (5 < me->state.failed_connections) {
        me->state.flags = PC_UNCONTACTABLE_PEER;
    }
    assert (0);
}

static void * __offer_block (void * me_, void * b) {
    pwp_conn_private_t * me = (void *) me_;
    bt_block_t * b_new;
    b_new = malloc (sizeof (bt_block_t));
    memcpy (b_new, b, sizeof (bt_block_t));
    llqueue_offer (me -> reqs, b_new);
    return NULL;
}

static void * __poll_block (void * me_, void * __unused __attribute__ ((__unused__))) {
    pwp_conn_private_t * me = (void *) me_;
    return llqueue_poll (me->reqs);
}

void pwp_conn_offer_block (pwp_conn_t * me_, bt_block_t * b) {
    pwp_conn_private_t * me = (void *) me_;
    assert (me -> cb.call_exclusively);
    me->cb.call_exclusively (me, me->cb_ctx, &me->req_lock, b, __offer_block);
}

void pwp_conn_periodic (pwp_conn_t * me_) {
    pwp_conn_private_t * me = (void *) me_;
    me->state.tick++;
    __expunge_my_old_pending_reqs (me);
    if (pwp_conn_flag_is_set (me_, PC_UNCONTACTABLE_PEER)) {
        goto cleanup;
    }
    if (0 < llqueue_count (me->peer_reqs)) {
        bt_block_t * b = llqueue_poll (me->peer_reqs);
        pwp_conn_send_piece (me_, b);
        free (b);
    }
    if (pwp_conn_peer_is_interested (me_)) {
        if (pwp_conn_im_choking (me_)) {
            pwp_conn_unchoke (me_);
        }
    }
    if (pwp_conn_im_interested (me_)) {
        if (pwp_conn_im_choked (me_)) {
            goto cleanup;
        }
        int end, ii;
        end = 10 - pwp_conn_get_npending_requests (me_);
        for (ii = 0; ii < end; ii++) {
            if (0 == me->cb.pollblock (me->cb_ctx, me->peer_udata)) {
            }
        }
        if (0 < llqueue_count (me->reqs)) {
            void * b = me->cb.call_exclusively (me, me->cb_ctx, &me->req_lock, NULL, __poll_block);
            pwp_conn_request_block_from_peer ((pwp_conn_t *) me, b);
            free (b);
        }
    }
    else {
        pwp_conn_set_im_interested (me_);
    }
#if 0 /* debugging */
    printf ("pending requests: %lx %d %d\n", me, pwp_conn_get_npending_requests (me), llqueue_count (me -> peer_reqs));
#endif
    meanqueue_offer (me -> bytes_drate, me -> bytes_downloaded_this_period);
    meanqueue_offer (me -> bytes_urate, me -> bytes_uploaded_this_period);
    me->bytes_downloaded_this_period = 0;
    me->bytes_uploaded_this_period = 0;
cleanup :
    return;
}

int pwp_conn_peer_has_piece (pwp_conn_t * me_, const int piece_idx) {
    pwp_conn_private_t * me = (void *) me_;
    return sc_have (me->pieces_peerhas, piece_idx, 1);
}

void pwp_conn_keepalive (pwp_conn_t * me_ __attribute__ ((__unused__))) {
}

void pwp_conn_choke (pwp_conn_t * me_) {
    pwp_conn_private_t * me = (void *) me_;
    __log (me, "read,choke");
    me->state.flags |= PC_PEER_CHOKING;
    __expunge_my_pending_reqs (me);
}

void pwp_conn_unchoke (pwp_conn_t * me_) {
    pwp_conn_private_t * me = (void *) me_;
    __log (me, "read,unchoke");
    me->state.flags &= ~PC_PEER_CHOKING;
}

void pwp_conn_interested (pwp_conn_t * me_) {
    pwp_conn_private_t * me = (void *) me_;
    __log (me, "read,interested");
    me->state.flags |= PC_PEER_INTERESTED;
}

void pwp_conn_uninterested (pwp_conn_t * me_) {
    pwp_conn_private_t * me = (void *) me_;
    __log (me, "read,uninterested");
    me->state.flags &= ~PC_PEER_INTERESTED;
}

void pwp_conn_have (pwp_conn_t * me_, msg_have_t * have) {
    pwp_conn_private_t * me = (void *) me_;
    __log (me, "read,have,piece_idx=%d", have -> piece_idx);
    if (1 == pwp_conn_mark_peer_has_piece (me_, have->piece_idx)) {
    }
    if (!sc_have (me->pieces_completed, have->piece_idx, 1)) {
        pwp_conn_set_im_interested (me_);
    }
}

void pwp_conn_bitfield (pwp_conn_t * me_, msg_bitfield_t * bitfield) {
    pwp_conn_private_t * me = (void *) me_;
#if 0
    if (me->num_pieces < bitfield_get_length (&bitfield->bf)) {
        __disconnect (me, "too many pieces within bitfield");
    }
#endif
    if (pwp_conn_flag_is_set (me_, PC_BITFIELD_RECEIVED)) {
        __disconnect (me, "peer sent bitfield twice");
    }
    me->state.flags |= PC_BITFIELD_RECEIVED;
    int ii;
    for (ii = 0; ii < me->num_pieces; ii++) {
        if (bitfield_is_marked (&bitfield->bf, ii))
            pwp_conn_mark_peer_has_piece (me_, ii);
    }
}

int pwp_conn_request (pwp_conn_t * me_, bt_block_t * r) {
    pwp_conn_private_t * me = (void *) me_;
    __log (me, "read,request,piece_idx=%d offset=%d len=%d", r -> piece_idx, r -> offset, r -> len);
    if (pwp_conn_im_choking (me_)) {
        __disconnect (me, "peer requested when they were choked");
        return 0;
    }
    if (pwp_conn_im_choking (me_)) {
        return 0;
    }
    if (me->num_pieces < r->piece_idx) {
        __disconnect (me, "requested piece %d has invalid idx", r -> piece_idx);
        return 0;
    }
    if (!sc_have (me->pieces_completed, r->piece_idx, 1)) {
        __disconnect (me, "requested piece %d is not available", r -> piece_idx);
        return 0;
    }
    if (pwp_conn_peer_has_piece (me_, r->piece_idx)) {
        __disconnect (me, "peer requested pce%d which they had", r -> piece_idx);
        return 0;
    }
    if (r->len == 0 || me->piece_len < r->offset + r->len) {
        __disconnect (me, "invalid block request");
        return 0;
    }
#if 0
    assert (NULL != me -> cb.piece_is_complete);
    if (0 == me->cb.piece_is_complete (me->cb_ctx, pce)) {
        __disconnect (me, "requested piece %d is not completed", r -> piece_idx);
        return 0;
    }
#endif
    if (!llqueue_get_item_via_cmpfunction (me->peer_reqs, r, (void *) __req_cmp)) {
        bt_block_t * b = malloc (sizeof (bt_block_t));
        memcpy (b, r, sizeof (bt_block_t));
        llqueue_offer (me -> peer_reqs, b);
    }
    return 1;
}

void pwp_conn_cancel (pwp_conn_t * me_, bt_block_t * cancel) {
    pwp_conn_private_t * me = (void *) me_;
    bt_block_t * removed;
    __log (me, "read,cancel,piece_idx=%d offset=%d length=%d", cancel -> piece_idx, cancel -> offset, cancel -> len);
    removed = llqueue_remove_item_via_cmpfunction (me->peer_reqs, cancel, (void *) __req_cmp);
    free (removed);
}

int pwp_conn_block_request_is_pending (void * pc, bt_block_t * b) {
    pwp_conn_private_t * me = pc;
    return NULL != hashmap_get (me->recv_reqs, b);
}

static void __conn_remove_pending_request (pwp_conn_private_t * me, const bt_block_t * pb) {
    request_t * r;
    void * add;
    if ((r = hashmap_remove (me->recv_reqs, &pb))) {
        free (r);
        return;
    }
    add = llqueue_new ();
#if 0
    __disconnect (me, "err: received a block we did not request: %d %d %d\n", piece -> block.piece_idx, piece -> block.offset, piece -> block.len);
    return 0;
#endif
    hashmap_iterator_t iter;
    for (hashmap_iterator (me->recv_reqs, &iter); (r = hashmap_iterator_next_value (me->recv_reqs, &iter));) {
        llqueue_offer (add, r);
    }
    while (llqueue_count (add) > 0) {
        bt_block_t * rb;
        r = llqueue_poll (add);
        rb = &r->blk;
        if (r->blk.piece_idx != pb->piece_idx)
            continue;
        if (pb->offset <= rb->offset && rb->offset + rb->len <= pb->offset + pb->len) {
            r = hashmap_remove (me->recv_reqs, &r->blk);
            assert (r);
            free (r);
        }
        else if (rb->offset < pb->offset && pb->offset + pb->len < rb->offset + rb->len) {
            request_t * n;
            n = malloc (sizeof (request_t));
            n->tick = r->tick;
            n->blk.piece_idx = rb->piece_idx;
            n->blk.offset = pb->offset + pb->len;
            n->blk.len = rb->len - pb->len - (pb->offset - rb->offset);
            assert ((int) n -> blk.len != 0);
            assert ((int) n -> blk.len > 0);
            hashmap_put (me -> recv_reqs, & n -> blk, n);
            assert (n -> blk.len > 0);
            hashmap_remove (me -> recv_reqs, & r -> blk);
            rb->len = pb->offset - rb->offset;
            assert ((int) rb -> len > 0);
            hashmap_put (me -> recv_reqs, & r -> blk, r);
            assert (rb -> len > 0);
        }
        else if (rb->offset < pb->offset + pb->len && pb->offset + pb->len < rb->offset + rb->len) {
            hashmap_remove (me -> recv_reqs, & r -> blk);
            rb->len -= (pb->offset + pb->len) - rb->offset;
            rb->offset = pb->offset + pb->len;
            assert ((int) rb -> len > 0);
            hashmap_put (me -> recv_reqs, & r -> blk, r);
        }
        else if (rb->offset < pb->offset && pb->offset < rb->offset + rb->len && rb->offset + rb->len <= pb->offset + pb->len) {
            hashmap_remove (me -> recv_reqs, & r -> blk);
            rb->len = pb->offset - rb->offset;
            assert ((int) rb -> len > 0);
            hashmap_put (me -> recv_reqs, & r -> blk, r);
        }
    }
    llqueue_free (add);
}

int pwp_conn_piece (pwp_conn_t * me_, msg_piece_t * p) {
    pwp_conn_private_t * me = (void *) me_;
    assert (me -> cb.pushblock);
    __log (me, "read,piece,piece_idx=%d offset=%d length=%d", p -> blk.piece_idx, p -> blk.offset, p -> blk.len);
    __conn_remove_pending_request (me, & p -> blk);
    me->cb.pushblock (me->cb_ctx, me->peer_udata, &p->blk, p->data);
    me->bytes_downloaded_this_period += p->blk.len;
    return 1;
}

