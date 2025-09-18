#pragma once

#include "Common.h"
#include "PuzzleLoader.h"

INLINE
void add_piece(uint8_t rotation, const std::shared_ptr<t_piece_matrix_vector>& piece_matrix_vector, const t_piece& piece, COLOR_DIRECTION::COLOR_DIRECTION left, COLOR_DIRECTION::COLOR_DIRECTION top, COLOR_DIRECTION::COLOR_DIRECTION right, COLOR_DIRECTION::COLOR_DIRECTION bottom) {
    CELL_TYPE::CELL_TYPE cell_type = CELL_TYPE::INNER;

    const color_t
        color_left = piece.colors[left],
        color_top = piece.colors[top],
        color_right = piece.colors[right],
        color_bottom = piece.colors[bottom];

    switch (piece.flags) {
    case PIECE_TYPE::CORNER:
    case PIECE_TYPE::EDGE:
        if (color_right == EDGE_COLOR)
            cell_type = CELL_TYPE::BORDER_RIGHT;
        else if (color_bottom == EDGE_COLOR)
            cell_type = CELL_TYPE::BORDER_BOTTOM;
        break;
    case PIECE_TYPE::INNER:
        cell_type = CELL_TYPE::INNER;
        break;
    }

    // Now we need to calculate the index in the total matrix
    auto start_matrix_entry_offset = piece_matrix_vector->cell_type_offset * static_cast<uint32_t>(cell_type);
    // Inside this matrix we need to calculate the cell index based on the colors
    auto matrix_index = static_cast<uint32_t>(color_left) * piece_matrix_vector->stride + static_cast<uint32_t>(color_top);
    // We have the 'vector' pointer index in the large array of pointers
    auto total_index = start_matrix_entry_offset + matrix_index;

    if (piece_matrix_vector->pieces[total_index] == nullptr) {
        piece_matrix_vector->pieces[total_index] = new t_piece_vector();
    }

    // Add the piece to the vector
    piece_matrix_vector->pieces[total_index]->push_back({ 
        .right = static_cast<uint32_t>(color_right *piece_matrix_vector->stride),
        .bottom = color_bottom,
        .identifier = {
            .index = piece.idx,
            .rotation = rotation
        }
    });
}

INLINE
std::shared_ptr<t_piece_matrix_vector> distribute_pieces(const std::shared_ptr<t_PuzzleData>& puzzle_data) {
    // how many vectors do we need per cell type?
    // we need max_color * max_color vectors
    // Calculate the matrix size as a power of two for better memory alignment
    uint32_t cell_type_matrix_size = static_cast<uint32_t>((puzzle_data->max_color + 1) * (puzzle_data->max_color + 1));
    // We need CELL_TYPE::MAX matrices, one for each cell type
    auto total_entries = static_cast<uint32_t>(CELL_TYPE::MAX) * cell_type_matrix_size;
    auto total_memory_size = total_entries * sizeof(t_piece_vector*);
    std::cout << "Allocating piece matrix with " << total_memory_size << " bytes\n";

    auto piece_vector_raw_ptr = static_cast<t_piece_matrix_vector*>(_aligned_malloc(total_memory_size, 4096));
    auto piece_vector = std::shared_ptr<t_piece_matrix_vector>(piece_vector_raw_ptr);
    piece_vector->cell_type_offset = cell_type_matrix_size;
    piece_vector->stride = puzzle_data->max_color + 1;

    // Initialize the pointers
    for (uint32_t i = 0; i < total_entries; ++i) {
        piece_vector->pieces[i] = nullptr;
    }

    // Add the pieces to the matrix
    for (uint32_t i = 0; i < puzzle_data->width * puzzle_data->height; ++i) {
        const t_piece& piece = puzzle_data->pieces[i];
        add_piece(0, piece_vector, piece, COLOR_DIRECTION::LEFT, COLOR_DIRECTION::TOP, COLOR_DIRECTION::RIGHT, COLOR_DIRECTION::BOTTOM);
        add_piece(1, piece_vector, piece, COLOR_DIRECTION::TOP, COLOR_DIRECTION::RIGHT, COLOR_DIRECTION::BOTTOM, COLOR_DIRECTION::LEFT);
        add_piece(2, piece_vector, piece, COLOR_DIRECTION::RIGHT, COLOR_DIRECTION::BOTTOM, COLOR_DIRECTION::LEFT, COLOR_DIRECTION::TOP);
        add_piece(3, piece_vector, piece, COLOR_DIRECTION::BOTTOM, COLOR_DIRECTION::LEFT, COLOR_DIRECTION::TOP, COLOR_DIRECTION::RIGHT);
    }

    return piece_vector;
}