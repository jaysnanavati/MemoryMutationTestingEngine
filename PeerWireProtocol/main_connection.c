

/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include "CuTest.h"


extern void TestPWP_init_has_us_choked(CuTest*);
extern void TestPWP_init_not_interested(CuTest*);
extern void TestPWP_choke_sets_as_choked(CuTest*);
extern void TestPWP_unchoke_sets_as_unchoked(CuTest*);
extern void TestPWP_unchoke_setget_flag(CuTest*);
extern void TestPWP_send_state_change_is_wellformed(CuTest*);
extern void TestPWP_send_have_is_wellformed(CuTest*);
extern void TestPWP_send_bitField_is_wellformed(CuTest*);
extern void TestPWP_send_request_is_wellformed(CuTest*);
extern void TestPWP_send_piece_is_wellformed(CuTest*);
extern void TestPWP_send_cancel_is_wellformed(CuTest*);
extern void TestPWP_read_havemsg_marks_peer_as_having_piece(CuTest*);
extern void TestPWP_read_havemsg_disconnects_with_piece_idx_out_of_bounds(CuTest*);
extern void TestPWP_send_interested_if_lacking_piece_from_have_msg(CuTest*);
extern void TestPWP_read_chokemsg_marks_us_as_choked(CuTest*);
extern void TestPWP_read_chokemsg_empties_our_pending_requests(CuTest*);
extern void TestPWP_read_unchokemsg_marks_us_as_unchoked(CuTest*);
extern void TestPWP_read_request_msg_disconnects_if_peer_is_choked(CuTest*);
extern void TestPWP_read_peerisinterested_marks_peer_as_interested(CuTest*);
extern void TestPWP_read_peerisuninterested_marks_peer_as_uninterested(CuTest*);
extern void TestPWP_read_bitfield_marks_peers_pieces_as_haved_by_peer(CuTest*);
extern void TestPWP_read_disconnect_if_bitfield_received_more_than_once(CuTest*);
extern void TestPWP_read_request_of_piece_not_completed_disconnects_peer(CuTest*);
extern void TestPWP_read_request_with_invalid_piece_idx_disconnects_peer(CuTest*);
extern void TestPWP_read_request_with_invalid_block_length_disconnects_peer(CuTest*);
extern void TestPWP_read_request_of_piece_which_client_has_results_in_disconnect(CuTest*);
extern void TestPWP_read_piece_results_in_correct_receivable(CuTest*);
extern void TestPWP_send_request_is_wellformed_even_when_request_len_was_outside_piece_len(CuTest*);
extern void TestPWP_read_request_doesnt_duplicate_within_pending_queue(CuTest*);
extern void TestPWP_requesting_block_increments_pending_requests(CuTest*);
extern void TestPWP_read_piece_decreases_pending_requests(CuTest*);
extern void TestPWP_read_piece_decreases_pending_requests_only_if_it_matches_a_request_rightside(CuTest*);
extern void TestPWP_read_piece_decreases_pending_requests_only_if_it_matches_a_request_leftside(CuTest*);
extern void TestPWP_read_piece_decreases_pending_requests_if_piece_covers_whole_request(CuTest*);
extern void TestPWP_read_piece_increases_pending_requests_if_piece_splits_requested(CuTest*);
extern void TestPWP_read_cancelmsg_cancels_last_request(CuTest*);
extern void TestPWP_request_queue_dropped_when_peer_is_choked(CuTest*);


void RunAllTests(void) 
{
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();


    SUITE_ADD_TEST(suite, TestPWP_init_has_us_choked);
    SUITE_ADD_TEST(suite, TestPWP_init_not_interested);
    SUITE_ADD_TEST(suite, TestPWP_choke_sets_as_choked);
    SUITE_ADD_TEST(suite, TestPWP_unchoke_sets_as_unchoked);
    SUITE_ADD_TEST(suite, TestPWP_unchoke_setget_flag);
    SUITE_ADD_TEST(suite, TestPWP_send_state_change_is_wellformed);
    SUITE_ADD_TEST(suite, TestPWP_send_have_is_wellformed);
    SUITE_ADD_TEST(suite, TestPWP_send_bitField_is_wellformed);
    SUITE_ADD_TEST(suite, TestPWP_send_request_is_wellformed);
    SUITE_ADD_TEST(suite, TestPWP_send_piece_is_wellformed);
    SUITE_ADD_TEST(suite, TestPWP_send_cancel_is_wellformed);
    SUITE_ADD_TEST(suite, TestPWP_read_havemsg_marks_peer_as_having_piece);
    SUITE_ADD_TEST(suite, TestPWP_read_havemsg_disconnects_with_piece_idx_out_of_bounds);
    SUITE_ADD_TEST(suite, TestPWP_send_interested_if_lacking_piece_from_have_msg);
    SUITE_ADD_TEST(suite, TestPWP_read_chokemsg_marks_us_as_choked);
    SUITE_ADD_TEST(suite, TestPWP_read_chokemsg_empties_our_pending_requests);
    SUITE_ADD_TEST(suite, TestPWP_read_unchokemsg_marks_us_as_unchoked);
    SUITE_ADD_TEST(suite, TestPWP_read_request_msg_disconnects_if_peer_is_choked);
    SUITE_ADD_TEST(suite, TestPWP_read_peerisinterested_marks_peer_as_interested);
    SUITE_ADD_TEST(suite, TestPWP_read_peerisuninterested_marks_peer_as_uninterested);
    SUITE_ADD_TEST(suite, TestPWP_read_bitfield_marks_peers_pieces_as_haved_by_peer);
    SUITE_ADD_TEST(suite, TestPWP_read_disconnect_if_bitfield_received_more_than_once);
    SUITE_ADD_TEST(suite, TestPWP_read_request_of_piece_not_completed_disconnects_peer);
    SUITE_ADD_TEST(suite, TestPWP_read_request_with_invalid_piece_idx_disconnects_peer);
    SUITE_ADD_TEST(suite, TestPWP_read_request_with_invalid_block_length_disconnects_peer);
    SUITE_ADD_TEST(suite, TestPWP_read_request_of_piece_which_client_has_results_in_disconnect);
    SUITE_ADD_TEST(suite, TestPWP_read_piece_results_in_correct_receivable);
    SUITE_ADD_TEST(suite, TestPWP_send_request_is_wellformed_even_when_request_len_was_outside_piece_len);
    SUITE_ADD_TEST(suite, TestPWP_read_request_doesnt_duplicate_within_pending_queue);
    SUITE_ADD_TEST(suite, TestPWP_requesting_block_increments_pending_requests);
    SUITE_ADD_TEST(suite, TestPWP_read_piece_decreases_pending_requests);
    SUITE_ADD_TEST(suite, TestPWP_read_piece_decreases_pending_requests_only_if_it_matches_a_request_rightside);
    SUITE_ADD_TEST(suite, TestPWP_read_piece_decreases_pending_requests_only_if_it_matches_a_request_leftside);
    SUITE_ADD_TEST(suite, TestPWP_read_piece_decreases_pending_requests_if_piece_covers_whole_request);
    SUITE_ADD_TEST(suite, TestPWP_read_piece_increases_pending_requests_if_piece_splits_requested);
    SUITE_ADD_TEST(suite, TestPWP_read_cancelmsg_cancels_last_request);
    SUITE_ADD_TEST(suite, TestPWP_request_queue_dropped_when_peer_is_choked);

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
}

int main()
{
    RunAllTests();
    return 0;
}

