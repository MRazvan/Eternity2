#pragma once

#include <memory>
#include "Common.h"

/// <summary>
///  The board has one extra cell that is used as a dummy cell
///  0 1 2
///  3 4 5
///  6 7 8 9 <- Dummy cell
///  
///  Cell 2 has no right neighbor, so we point it to cell 9
///  Cell 5 has no right neighbor, so we point it to cell 9
///  Cell 6 has no bottom neighbor, so we point it to cell 9
///  Cell 7 has no bottom neighbor, so we point it to cell 9
///  Cell 8 has no right or bottom neighbor, so we point it to cell 9
/// 
///  It also has a duplicated cells array in continuation of the main one,
///  that is used to quickly copy the cells when we need to save a solution
/// </summary>
/// <param name="puzzleData"></param>
/// <param name="piece_matrix_vector"></param>
/// <returns></returns>
INLINE
t_board* create_board(const std::shared_ptr<t_PuzzleData>& puzzleData, t_piece_matrix_vector* piece_matrix_vector) {
    auto memorySize = sizeof(t_board) + 2 /* Two sets of cells */ * (sizeof(t_cell) * puzzleData->width * puzzleData->height + 1);
    auto board = static_cast<t_board*>(_aligned_malloc(memorySize, 4096));
    if (!board) {
        return nullptr; // Memory allocation failed
    }
    memset(board, 0, memorySize);

    board->total_cells = static_cast<uint16_t>(puzzleData->width * puzzleData->height);
    /* We need 1 dummy cell to point everything that does not have a neighbor to this*/
    board->actual_total_cells = board->total_cells + 1;
    board->cells_stride = puzzleData->width;

    auto cells_memory_size = sizeof(t_cell) * (board->actual_total_cells);
    uint16_t dummy_cell_index = static_cast<uint16_t>(board->total_cells);

    // Initialize all cells to a known value
    for (uint32_t idx = 0; idx < board->total_cells; ++idx) {
        board->cells[idx].right_cell_offset = static_cast<uint16_t>(idx + 1);
        board->cells[idx].bottom_cell_offset = static_cast<uint16_t>(idx + board->cells_stride);
        board->cells[idx].pieces = &piece_matrix_vector->pieces[CELL_TYPE::INNER * piece_matrix_vector->cell_type_offset];
    }

    for (uint32_t idx = 0; idx < puzzleData->width; ++idx) {
        auto cellIdx = get_idx(idx, puzzleData->height - 1, puzzleData->width);
        board->cells[cellIdx].bottom_cell_offset = dummy_cell_index;
        board->cells[cellIdx].pieces = &piece_matrix_vector->pieces[CELL_TYPE::BORDER_BOTTOM * piece_matrix_vector->cell_type_offset];
    }

    for (uint32_t idx = 0; idx < puzzleData->height; ++idx) {
        auto cellIdx = get_idx(puzzleData->width - 1, idx, puzzleData->width);
        board->cells[cellIdx].right_cell_offset = dummy_cell_index;
        board->cells[cellIdx].pieces = &piece_matrix_vector->pieces[CELL_TYPE::BORDER_RIGHT * piece_matrix_vector->cell_type_offset];
    }

    return board;
}

void copy_cells(t_board& board) {
    memcpy(&board.cells[board.actual_total_cells], &board.cells[0], sizeof(t_cell) * (board.actual_total_cells));
}


void  stop_board(t_board& board) {
    board.done = true;
}