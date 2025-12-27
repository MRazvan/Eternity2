#pragma once

#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <optional>

#include "Common.h"

typedef std::optional<std::shared_ptr<t_PuzzleData>> t_puzzle_data_ptr;

FORCE_INLINE
static bool is_corner(const uint32_t first, const uint32_t second, const uint32_t third, const uint32_t fourth) {
    return (first == EDGE_COLOR && second == EDGE_COLOR)
    || (first == EDGE_COLOR && third == EDGE_COLOR)
    || (first == EDGE_COLOR && fourth == EDGE_COLOR)
    || (second == EDGE_COLOR && third == EDGE_COLOR)
    || (second == EDGE_COLOR && fourth == EDGE_COLOR)
    || (third == EDGE_COLOR && fourth == EDGE_COLOR);
}

FORCE_INLINE
static bool is_edge(const uint32_t first, const uint32_t second, const uint32_t third, const uint32_t fourth) {
    return (first == EDGE_COLOR || second == EDGE_COLOR || third == EDGE_COLOR || fourth == EDGE_COLOR);
}

INLINE
t_puzzle_data_ptr Puzzle_Load(const std::string& puzzle_file)
{
    std::ifstream inputFile(puzzle_file);
    if (!inputFile.is_open() || !inputFile.good())
        return std::nullopt;


    auto result = std::make_shared<t_PuzzleData>();

    // Read the puzzle size
    {
        std::string firstLine;
        if (!std::getline(inputFile, firstLine))
            return std::nullopt;

        std::stringstream ss(firstLine);
        ss >> result->width >> result->height;

        if (result->height == 0) {
            result->height = result->width;
        }
        if (!inputFile.good()) {
            return std::nullopt;
        }
    }

    // Read the pieces, one on each line
    result->pieces = new t_piece[static_cast<uint32_t>(result->width * result->height)];
    uint32_t pieceIdx = 0;
    uint32_t max_color = 0;
    for (std::string line; std::getline(inputFile, line);)
    {
        // New piece to read into
        std::stringstream currentLine(line);
        uint32_t first, second, third, fourth;
        currentLine >> first >> second >> third >> fourth;

        max_color = std::max(max_color, first);
        max_color = std::max(max_color, second);
        max_color = std::max(max_color, third);
        max_color = std::max(max_color, fourth);

        result->pieces[pieceIdx].idx = static_cast<uint8_t>(pieceIdx);
        result->pieces[pieceIdx].colors[COLOR_DIRECTION::LEFT] = static_cast<color_t>(first);
        result->pieces[pieceIdx].colors[COLOR_DIRECTION::TOP] = static_cast<color_t>(second);
        result->pieces[pieceIdx].colors[COLOR_DIRECTION::RIGHT] = static_cast<color_t>(third);
        result->pieces[pieceIdx].colors[COLOR_DIRECTION::BOTTOM] = static_cast<color_t>(fourth);

        if (is_corner(first, second, third, fourth)) {
            result->pieces[pieceIdx].flags = PIECE_TYPE::CORNER;
        }
        else if (is_edge(first, second, third, fourth)) {
            result->pieces[pieceIdx].flags = PIECE_TYPE::EDGE;
        }
        else {
            result->pieces[pieceIdx].flags = PIECE_TYPE::INNER;
        }

        pieceIdx++;
    }

    result->max_color = static_cast<uint16_t>(max_color);
    return result;
}


