#include <filesystem>
#include <format>
#include <iostream>
#include <thread>

#include "Backtracker.h"
#include "Board.h"
#include "Common.h"
#include "Options.h"
#include "PuzzleLoader.h"
#include "PieceMatrix.h"
#include "PrintUtils.h"

typedef struct {
    std::shared_ptr<t_options> options;
    std::shared_ptr<t_PuzzleData> puzzleData;
} t_board_user_data;

int main(const int argc, const char* argv[])
{
    //////////////////////////////////////////////////////////////////
    // Check that we got everything we need on the command line
    cxxopts::Options options(argv[0]);
    auto options_ptr = load_options(options, argc, argv);
    if (!options_ptr.has_value())
    {
        std::cout << options.help();  // NOLINT(clang-diagnostic-format-security)
        return RETURN_ERR;
    }
    

    //////////////////////////////////////////////////////////////////
    // Load the puzzle data if possible
    auto optionsData = options_ptr.value();
    auto puzzleDataPtr = Puzzle_Load(optionsData->PuzzleFile);
    if (!puzzleDataPtr.has_value()) {
        std::cerr << "Failed to load puzzle from file: \n" << optionsData->PuzzleFile;
        return RETURN_ERR;
    }
    auto puzzleData = puzzleDataPtr.value();

    //////////////////////////////////////////////////////////////////
    // Create the piece vector matrix, this is one of the major optimisation
    // Basically what it does is
    //  For each CELL TYPE it creates a matrix of vector pointers, the matrix is of size (max_color + 1) * (max_color + 1)
    //  a cell in the matrix is going from 'left'*'top' to find the right vector that contains pieces for that particular
    //  left and top color combination
    auto piece_vector_matrix = distribute_pieces(puzzleData);
    auto board = create_board(puzzleData, piece_vector_matrix);

    //////////////////////////////////////////////////////////////////
    // Do something with a solution
    //  Either ignore it
    //  Or save it to a file if we have the output file to save it
    {
        t_board_user_data* board_user_data = new t_board_user_data();
        board_user_data->options = optionsData;
        board_user_data->puzzleData = puzzleData;
        board->user_data = board_user_data; // Store user data in the board
        board->solution_callback = [](t_board& board)
        {
            auto user_data = static_cast<t_board_user_data*>(board.user_data);
            if (user_data->options->FirstSolution) {
                stop_board(board);
            }
            if (user_data->options->DisplayOnConsole) {
                print_piece_solution(&std::cout, board);
                if (user_data->options->Bucas) {
                    copy_cells(board);
                    print_url(&std::cout, board, user_data->puzzleData);
                }
            }
        };
    }

    //////////////////////////////////////////////////////////////////
    // Thread that reports once a second the progress
    // Also keep track of the number of nodes per second we can place
    std::thread statThread([](const t_board_ptr& board) {
        auto user_data = static_cast<t_board_user_data*>(board->user_data);
        uint64_t total_nodes = 0;
        uint32_t last_depth = 0;
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            // This is not really exact, as by the time we are reading total_placed_nodes the backtracking thread might have placed more nodes
            // but it is close enough for what we need
            const auto diff = board->total_placed_nodes - total_nodes;
            total_nodes += diff;

            const auto max_depth = board->max_depth;
            std::cout << std::format("Total solutions: {}. Valid nodes per second: {}. Total placed nodes: {}. Best depth: {}\n", board->total_solutions, diff, board->total_placed_nodes, max_depth);

            // Max depth changed, print the best solution so far
            if (last_depth != max_depth) {
                last_depth = max_depth;
                print_piece_solution(&std::cout, (*board), true);
                if (user_data->options->Bucas)
                    print_url(&std::cout, (*board), user_data->puzzleData);
            }

            // Stop condition based on maximum placed nodes
            if (user_data->options->MaxNodesToPlace > 0) {
                if (board->total_placed_nodes >= static_cast<uint64_t>(user_data->options->MaxNodesToPlace))
                    stop_board(*board);
            }
        }
    }, board);

    //////////////////////////////////////////////////////////////////
    // Finally the magic
    {
        TIMED_BLOCK("Backtracking");
        const auto start = __rdtsc();

            backtrack((*board));

        const auto stop = __rdtsc();

        const auto duration = stop - start;
        const auto clocks_per_node_placed = duration / (board->total_placed_nodes ? board->total_placed_nodes : 1);
        const auto clocks_per_node_checked = duration / (board->total_checked_nodes ? board->total_checked_nodes : 1);

        std::cout << std::format("Total cycles: {}. Cycles per node placed: {}. Cycles per node checked: {}\n", duration, clocks_per_node_placed, clocks_per_node_checked);
    }

    std::cerr << std::format("Total solutions: {}. Total placed nodes: {}. Total checked nodes: {}\n", board->total_solutions, board->total_placed_nodes, board->total_checked_nodes);
    return RETURN_OK;
}
