#pragma once

#include "Common.h"
#include "Board.h"

FORCE_INLINE
void backtrack(t_board& board, uint32_t cell_index = 0) {
    // Keep track of the maximum depth we reached in the backtracking and store the current state
    if (board.max_depth < cell_index) {
        [[unlikely]]
        board.max_depth = cell_index;
        copy_cells(board);
    }

    // If we reached the end of the board, we are done
    if (cell_index == board.total_cells) {
        [[unlikely]]
        board.max_depth = cell_index;
        board.total_solutions++;
        // We have a solution, print it or do something with it
        board.solution_callback(board);
        return;
    }

    auto& cell = board.cells[cell_index];
    // Get the vector responsible for the current cell
    const auto pieces = cell.pieces[cell.left_color + cell.top_color];
    if (pieces == nullptr)
        return;

    // Loop through all possible pieces and do the stuff we need to do
    for (const auto& piece : *pieces)
    {
        board.total_checked_nodes++;

        //// Check if the piece is used already
        if (board.used_pieces[piece.identifier.index])
            continue; // Piece already used

        board.total_placed_nodes++;

        // Update some basic and really important information
        cell.identifier = piece.identifier;
        board.cells[cell.right_cell_offset].left_color = piece.right;
        board.cells[cell.bottom_cell_offset].top_color = piece.bottom;

        // Mark the piece as used
        board.used_pieces[piece.identifier.index] = true;

        // Classic recursive backtrack
        backtrack(board, cell_index + 1);

        // Release the piece for usage
        board.used_pieces[piece.identifier.index] = false;
    }
}

