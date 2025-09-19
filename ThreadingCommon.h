#pragma once

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
        t_piece_vector next_starting_pieces;
        const auto corner_pieces = piece_vector_matrix->pieces[CELL_TYPE::INNER + 0 + 0];
        const auto first_corner_piece = corner_pieces->at(0);

        next_starting_pieces.push_back(first_corner_piece);
        merge_values(next_starting_pieces, starting_pieces);

        int32_t max_iterations = 20; // Just in case we get stuck in an infinite loop

        while (starting_pieces.size() < min_combinations && max_iterations-- > 0) {
            const auto last_piece = starting_pieces.at(0).back();
            // We need to expand the starting pieces
            const auto right_pieces = piece_vector_matrix->pieces[CELL_TYPE::INNER + last_piece.right];
            t_piece_vector next_starting_pieces;
            for (const auto piece : *right_pieces) {
                next_starting_pieces.push_back(piece);
            }
            merge_values(next_starting_pieces, starting_pieces);
        }
    }
    
    std::cout << "Generated " << starting_pieces.size() << " starting piece sets\n";
    //// Print the starting pieces we generated for each set
    //auto set_idx = 0;
    //for (const auto& pieces : starting_pieces) {
    //    std::cout << std::format("Starting pieces for set {:3}: ", set_idx++);
    //    for (const auto& piece : pieces) {
    //        std::cout << std::format(" {} ", piece.identifier);
    //    }
    //    std::cout << "\n";
    //}

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