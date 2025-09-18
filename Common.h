#pragma once

#include <chrono>
#include <vector>
#include <cstdint>
#include <iostream>
#include <string>
#include <array>
#include <format>

#define RETURN_ERR -1
#define RETURN_OK   0

#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#define INLINE __inline
#define FORCE_INLINE __forceinline
#define ALIGN(nr) __declspec(align((nr)))
#define REGISTER register

typedef uint8_t     color_t;
constexpr auto      EDGE_COLOR = 0;

namespace PIECE_TYPE {
    enum PIECE_TYPE : uint8_t {
        CORNER = 0,
        EDGE,
        INNER
    };
}

namespace COLOR_DIRECTION
{
    enum COLOR_DIRECTION : uint8_t
    {
        LEFT = 0,
        TOP,
        RIGHT,
        BOTTOM
    };
}

namespace CELL_TYPE {
    enum CELL_TYPE : uint16_t
    {
        INNER,
        BORDER_BOTTOM,
        BORDER_RIGHT,
        MAX = BORDER_RIGHT + 1
    };
}

typedef struct {
    uint8_t idx;
    PIECE_TYPE::PIECE_TYPE flags;
    color_t colors[4];
} t_piece;

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t max_color;
    t_piece* pieces;
} t_PuzzleData;

typedef PACK(struct {
    // Piece index
    uint8_t index;
    // Piece rotation
    uint8_t rotation;
}) t_piece_identifier;

// Custom formatter for the identifier block
template<>
struct std::formatter<t_piece_identifier> {

    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const t_piece_identifier& identifier, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{:3}({})", identifier.index, identifier.rotation);
    };
};


typedef PACK(ALIGN(4) struct {
    // The right color that we have to use, this contains offsets into a piece array ( it's not just the color )
    uint32_t right;
    // Bottom color
    color_t bottom;
    // The piece identifier
    t_piece_identifier identifier;
}) t_precalculated_piece;

typedef std::vector<t_precalculated_piece> t_piece_vector;

typedef PACK(ALIGN(32) struct
{
    // A pointer to the matrix containing all possible "LEFT / TOP" vector combinations for this cell type
    t_piece_vector** pieces;
    // Obvious
    t_piece_identifier identifier;
    // Obvious
    uint32_t top_color;
    // Obvious
    uint32_t left_color;
    // Offset in the board cell array to the cell bellow this one
    uint32_t bottom_cell_offset;
    // Offset in the board cell array to the cell to the left of this one
    uint32_t right_cell_offset;
    // The board has 1 extra cell
    //  All the cells on the "RIGHT" border have their "right_offsets" set to the dummy cell, since the cells do not actually have a right neighbor
    //      that means we eliminate a check for the right border
    //  All the cells on the BOTTOM border have their "bottom_offsets" set to the dummy cell, since the cells do not actually have a bottom neighbor
    //      that means we eliminate a check for the bottom border
}) t_cell;

// Forward declaration
struct st_board;
// Define a callback used for processing solutions
typedef void(*t_solution_callback)(struct st_board& board);
// Finally the board definition
typedef PACK(struct st_board
{
    void* user_data; // Pointer to user data, can be used for additional information
    t_solution_callback solution_callback;
    // Basically the Width of the puzzle
    uint32_t cells_stride;
    // Total number of cells in the board
    uint32_t total_cells;
    // The actual number of cells we have allocated for the board, this is Width * Height + 1 ( dummy cell )
    uint32_t actual_total_cells;
    // Max depth reached
    uint32_t max_depth;
    // Total number of nodes we checked
    uint64_t total_checked_nodes;
    // Total number of placed nodes
    uint64_t total_placed_nodes;
    // Total number of solutions
    uint64_t total_solutions;
    // Used piece bitmask
    bool used_pieces[256];
    // Board cells
    t_cell cells[];
}) t_board;


typedef struct {
    // We have 
    uint32_t cell_type_offset;
    uint32_t stride;
    // Contains a list of "CELL_TYPE::MAX" matrices of X * X with pointer to vectors of pieces form COLOR_1 to COLOR_2
    t_piece_vector* pieces[];
} t_piece_matrix_vector;

// Why not mix a little bit of "class" action
typedef struct timed_block
{
    explicit timed_block(std::string message) : m_start(std::chrono::high_resolution_clock::now()), m_message(std::move(message)) {}
    ~timed_block()
    {
        const auto total = std::chrono::high_resolution_clock::now() - m_start;
        const auto hrs = std::chrono::duration_cast<std::chrono::hours>(total);
        const auto mins = std::chrono::duration_cast<std::chrono::minutes>(total - hrs);
        const auto secs = std::chrono::duration_cast<std::chrono::seconds>(total - hrs - mins);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(total - hrs - mins - secs);
        const auto us = std::chrono::duration_cast<std::chrono::microseconds>(total - hrs - mins - secs - ms);
        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(total - hrs - mins - secs - ms - us);
        std::cout << std::format("{}: {}:{}:{} ({}.{}.{})\n", m_message, hrs, mins, secs, ms, us, ns);
    }
    std::chrono::steady_clock::time_point m_start;
    std::string m_message;
} t_timed_block;

#define TIMED_BLOCK(message) t_timed_block var_timed_block_##__LINE__(message)

FORCE_INLINE
uint32_t get_idx(uint32_t x, uint32_t y, uint32_t width) {
    return y * width + x;
}