#pragma once

#include <functional>
#include <vector>

#include "Common.h"
#include "ThreadSafeQueue.h"
#include "Board.h"

typedef struct {
    std::atomic<bool> done;
    std::mutex print_mutex;
} t_sync_data;

typedef struct {
    std::shared_ptr<t_PuzzleData> puzzleData;
    t_piece_matrix_vector* pieceMatrixVector;
    std::shared_ptr<ThreadSafeQueue<t_piece_vector>> workQueue;
    std::shared_ptr<t_sync_data> sync;
    t_board* board;
    bool is_running;
} t_thread_data;

void merge_values(const t_piece_vector& extra_pieces, std::vector<t_piece_vector>& existing_pieces) {
    // For each 'set' of existing pieces in the existing pieces vector, we need to generate all combinations with the extra pieces
    if (existing_pieces.empty()) {
        existing_pieces.push_back(extra_pieces);
        return;
    }

    std::vector<t_piece_vector> new_existing_pieces;
    for (const auto existing_vector : existing_pieces)
        for (const auto extra_piece : extra_pieces) {
            t_piece_vector new_vector(existing_vector);

            // Check to see if we already have this piece in the existing vector
            const auto found_existing_piece = std::find_if(new_vector.begin(), new_vector.end(), [&extra_piece](const t_precalculated_piece& p) {
                return p.identifier.index == extra_piece.identifier.index;
            });

            if (found_existing_piece != new_vector.end())
                continue; // Piece already exists

            new_vector.push_back(extra_piece);
            new_existing_pieces.push_back(new_vector);
        }

    existing_pieces.clear();
    existing_pieces.insert(existing_pieces.begin(), new_existing_pieces.begin(), new_existing_pieces.end());
}

void backtrack_generator(t_board& board, uint32_t cell_index, std::vector<t_precalculated_piece>& piece_stack, const std::function<void(const std::vector<t_precalculated_piece>& pieces)>& callback, uint32_t max_depth) {
    // If we reached the end of the board, we are done
    if (cell_index == max_depth) {
        callback(piece_stack);
        return;
    }

    auto& cell = board.cells[cell_index];
    // Get the vector responsible for the current cell
    const auto pieces = cell.pieces[cell.left_color + cell.top_color];
    if (pieces == nullptr || board.done)
        return;

    // Loop through all possible pieces and do the stuff we need to do
    for (const auto& piece : *pieces)
    {
        // Check if the piece is used already
        if (board.used_pieces[piece.identifier.index])
            continue; // Piece already used
        piece_stack.push_back(piece);
        // Update some basic and really important information
        cell.identifier = piece.identifier;
        board.cells[cell.right_cell_offset].left_color = piece.right;
        board.cells[cell.bottom_cell_offset].top_color = piece.bottom;
        // Mark the piece as used
        board.used_pieces[piece.identifier.index] = true;
        // Classic recursive backtrack
        backtrack_generator(board, cell_index + 1, piece_stack, callback, max_depth);
        // Release the piece for usage
        board.used_pieces[piece.identifier.index] = false;
        piece_stack.pop_back();
    }
}


void generate_thread_data(
    const std::shared_ptr<t_PuzzleData>& puzzle_data,
    t_piece_matrix_vector* piece_vector_matrix, 
    std::shared_ptr<std::vector<t_thread_data>>& thread_data,
    std::shared_ptr<t_sync_data>& sync,
    uint32_t min_combinations = 0) 
{
    if (min_combinations == 0)
        min_combinations = 1;
    
    std::vector<t_piece_vector> starting_pieces;
    // Generate all the starting pieces we can use
    {
        min_combinations = std::min(std::thread::hardware_concurrency() - 1, min_combinations);
        std::vector<t_precalculated_piece> piece_stack;

        uint32_t depth = 1;
        uint32_t max_iterations = 20;
        while(starting_pieces.size() < min_combinations && depth < puzzle_data->width && max_iterations-- > 0)
        {
            starting_pieces.clear();

            auto board = create_board(puzzle_data, piece_vector_matrix);
            // Start with the first corner piece set
            const auto& corner_piece = piece_vector_matrix->pieces[CELL_TYPE::INNER]->at(0);
            auto& cell = board->cells[0];
            cell.identifier = corner_piece.identifier;
            board->cells[cell.right_cell_offset].left_color = corner_piece.right;
            board->cells[cell.bottom_cell_offset].top_color = corner_piece.bottom;
            board->used_pieces[corner_piece.identifier.index] = true;
            piece_stack.clear();
            piece_stack.push_back(corner_piece);

            backtrack_generator(*board, 1, piece_stack, [&starting_pieces](const auto& pieces) {
                t_piece_vector pieces_vector;
                std::copy(pieces.begin(), pieces.end(), std::back_inserter(pieces_vector));
                starting_pieces.push_back(pieces_vector);
            }, depth);
            _aligned_free(board);
            depth++;
        }
    }

    auto work_queue = std::make_shared<ThreadSafeQueue<t_piece_vector>>();
    for (const auto& pieces : starting_pieces) {
        work_queue->push(pieces);
    }

    for (uint32_t idx = 0; idx < min_combinations; ++idx) {
        thread_data->emplace_back();  // Construct in place
        auto& data = thread_data->back();

        data.puzzleData = puzzle_data;
        data.pieceMatrixVector = piece_vector_matrix;
        data.workQueue = work_queue;
        data.board = create_board(puzzle_data, piece_vector_matrix);
        data.board->solution_callback = [](t_board& board) {};
        data.sync = sync;
    }
}