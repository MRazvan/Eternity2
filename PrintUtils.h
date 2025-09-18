#pragma once
#include "Board.h"
#include "Common.h"

INLINE
void print_piece_solution(std::ostream* stream, const t_board& board, bool second_set = false)
{
    // Here you can print the solution or process it further
    // Print the pieces from the cell
    const auto width = static_cast<int32_t>(board.cells_stride);
    const auto height = static_cast<int32_t>(board.total_cells / width);
    for (auto y = 0; y < height; ++y) {
        for (auto x = 0; x < width; ++x)
        {
            auto cellIndex = get_idx(x, y, width);
            if (second_set) {
                cellIndex += board.actual_total_cells;
            }
            const auto& cell = board.cells[cellIndex];
            (*stream) << std::format("{} ", cell.identifier);
        }
        (*stream) << "\n";
    }
    (*stream) << "\n";
    stream->flush();
}

void print_url(std::ostream* stream, const t_board& board, const t_puzzle_data_ptr puzzle)
{
    /*
      o  puzzle       -> The name of the puzzle
  o  board_w      -> The width of the puzzle
  o  board_h      -> The height of the puzzle
  o  board_edges  -> The edges for all pieces ( a:0, b:1, ... )
  o  board_types  -> The type of each piece (a:None, b:Corner, c:Border, d:Center, e:Fixed) ; Optional
  o  board_pieces -> The piece numbers ; 3 digits integer for each piece ; Optional
  o  board_custom -> A custom text displayed on each space, separated by * (eg:rotation); Optional
    */
    static const char* hex_chars = "abcdefghijklmnopqrstuvwxyz";

    (*stream) << "https://e2.bucas.name/#puzzle=work_in_progress";
    (*stream) << "&board_w=" << (*puzzle)->width;
    (*stream) << "&board_h=" << (*puzzle)->height;
    (*stream) << "&board_edges=";
    for(uint32_t y = 0; y < (*puzzle)->height; ++y) {
        for(uint32_t x = 0; x < (*puzzle)->width; ++x) {
            uint32_t cellIndex = get_idx(x, y, (*puzzle)->width);
            if (cellIndex >= board.max_depth)
                break;

            const auto& cell = board.cells[cellIndex + board.actual_total_cells];
            const auto piece = (*puzzle)->pieces[cell.identifier.index];

            (*stream) << hex_chars[piece.colors[(COLOR_DIRECTION::TOP + cell.identifier.rotation) % 4]];
            (*stream) << hex_chars[piece.colors[(COLOR_DIRECTION::RIGHT + cell.identifier.rotation) % 4]];
            (*stream) << hex_chars[piece.colors[(COLOR_DIRECTION::BOTTOM + cell.identifier.rotation) % 4]];
            (*stream) << hex_chars[piece.colors[(COLOR_DIRECTION::LEFT + cell.identifier.rotation) % 4]];
        }
    }

    (*stream) << "&board_pieces=";
    for (uint32_t y = 0; y < (*puzzle)->height; ++y) {
        for (uint32_t x = 0; x < (*puzzle)->width; ++x) {
            auto cellIndex = get_idx(x, y, (*puzzle)->width);
            if (cellIndex >= board.max_depth)
                break;
            const auto& cell = board.cells[cellIndex + board.actual_total_cells];

            (*stream) << std::format("{:03}", cell.identifier.index);
        }
    }


    (*stream) << "\n";
    stream->flush();
}