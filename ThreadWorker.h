#pragma once

#include <thread>
#include <vector>
#include <algorithm>
#include <execution>
#include <atomic>

#include "Common.h"
#include "Board.h"
#include "ThreadSafeQueue.h"
#include "ThreadingCommon.h"
#include "Backtracker.h"
#include "Formatters.h"

uint32_t apply_hint_pieces(t_piece_vector& hints, t_board* board) {
    // First mark all pieces as unused
    std::fill(std::begin(board->used_pieces), std::end(board->used_pieces), false);

    uint32_t cell_index = 0;
    for (const auto& piece : hints) {
        auto& cell = board->cells[cell_index++];
        cell.identifier = piece.identifier;
        board->cells[cell.right_cell_offset].left_color = piece.right;
        board->cells[cell.bottom_cell_offset].top_color = piece.bottom;
        board->used_pieces[piece.identifier.index] = true;
    }
    return cell_index;
}

void safe_print(const std::shared_ptr<t_sync_data>& sync, const std::string& message) {
    std::lock_guard lock(sync->print_mutex);
    std::cout << message;
}

void worker_thread(t_thread_data& thread_data) {
    t_piece_vector hints;
    thread_data.is_running = true;
    while (thread_data.workQueue->wait_for_pop(hints, std::chrono::milliseconds(10)) && !thread_data.sync->done) {

        thread_data.board->done = false;
        uint32_t starting_index = apply_hint_pieces(hints, thread_data.board);
        backtrack(*(thread_data.board), starting_index);
        // Check if we need to stop
        if (thread_data.sync->done) {
            break;
        }
    }
    thread_data.is_running = false;
}

void reporting_thread(
    const std::shared_ptr<std::vector<t_thread_data>>& thread_data,
    t_statistics_data& total_statistics,
    std::shared_ptr<t_sync_data> sync,
    const std::shared_ptr<t_options>& options
) {
    const auto inital_number_of_work_items = thread_data->at(0).workQueue->size();
    // We will report every second the total number of solutions and nodes placed
    // Go through each thread data and sum up the values
    while (!sync->done) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        const auto remaining_work_items = thread_data->at(0).workQueue->size();

        t_statistics_data last_statistics = {};
        for (const auto& data : *thread_data) {
            last_statistics.total_solutions += data.board->total_solutions;
            last_statistics.total_nodes_placed += data.board->total_placed_nodes;
            last_statistics.total_nodes_checked += data.board->total_checked_nodes;
        }

        const auto diff_solutions = last_statistics.total_solutions - total_statistics.total_solutions;
        const auto diff_nodes_placed = last_statistics.total_nodes_placed - total_statistics.total_nodes_placed;
        const auto diff_nodes_checked = last_statistics.total_nodes_checked -total_statistics.total_nodes_checked;
        // Get the total number of threads still running and create a string containing a . for each running thread
        const auto running_threads = std::count_if(std::execution::par, thread_data->begin(), thread_data->end(), [](const t_thread_data& data) {
            return data.is_running;
        });
        std::string running_threads_str(running_threads, '.');
        std::string stopped_threads_str(thread_data->size() - running_threads, '.');


        const auto str = std::format("\rSolutions: {}. Stats: {} pps | {} cps | {} placed | {} checked. Sets remaining {}/{}. Workers: \033[32m{}\033[31m{}\033[0m",
            last_statistics.total_solutions.load(),
            format_number_human_readable(diff_nodes_placed),
            format_number_human_readable(diff_nodes_checked),
            format_number_human_readable(last_statistics.total_nodes_placed),
            format_number_human_readable(last_statistics.total_nodes_checked),
            remaining_work_items,
            inital_number_of_work_items,
            running_threads_str,
            stopped_threads_str);

        safe_print(sync, str);


        total_statistics.total_solutions.store(last_statistics.total_solutions);
        total_statistics.total_nodes_placed.store(last_statistics.total_nodes_placed);
        total_statistics.total_nodes_checked.store(last_statistics.total_nodes_checked);

        if (options->FirstSolution && total_statistics.total_solutions > 0) {
            // Clear the work queue
            thread_data->at(0).workQueue->empty();

            sync->done = true;
            // Stop all boards
            for(auto& data : *thread_data) {
                stop_board(*data.board);
            }
            break;
        }

        // If we have a max nodes to place, check if we reached it
        if (options->MaxNodesToPlace > 0 && total_statistics.total_nodes_placed >= static_cast<uint64_t>(options->MaxNodesToPlace)) {
            // Clear the work queue
            thread_data->at(0).workQueue->empty();

            sync->done = true;
            // Stop all boards
            for(auto& data : *thread_data) {
                stop_board(*data.board);
            }
            break;
        }
    }
}
