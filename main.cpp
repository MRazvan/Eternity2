#include <filesystem>
#include <format>
#include <iostream>
#include <vector>
#include <algorithm>
#include <execution>
#include <thread>

#include "Backtracker.h"
#include "Board.h"
#include "Common.h"
#include "Options.h"
#include "PuzzleLoader.h"
#include "PieceMatrix.h"
#include "PrintUtils.h"
#include "ThreadingCommon.h"
#include "ThreadWorker.h"

typedef struct {
    std::shared_ptr<t_options> options;
    std::shared_ptr<t_PuzzleData> puzzleData;
    std::shared_ptr<t_sync_data> sync;
} t_board_user_data;

void handle_board_solution(t_board& board) {
    auto user_data = static_cast<t_board_user_data*>(board.user_data);
    std::lock_guard lock(user_data->sync->print_mutex);
    if (user_data->options->Bucas) {
        copy_cells(board);
        print_url(&std::cout, board, user_data->puzzleData);
    }

    if (user_data->options->DisplayOnConsole) {
        copy_cells(board);
        print_piece_solution(&std::cout, board, true);
    }
}

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

    ////////////////////////////////////////////////////////////////////
    // Create the threads and start the work
    const auto max_threads = static_cast<int64_t>(std::thread::hardware_concurrency() - 1);
    const auto actual_max_threads = std::max<int64_t>(1, std::min(optionsData->MaxThreads, max_threads));
    std::cout << std::format("Using up to {} thread(s)\n", actual_max_threads);

    // We need a sync object to coordinate printing and stopping
    auto sync = std::make_shared<t_sync_data>();
    auto thread_data = std::make_shared<std::vector<t_thread_data>>();
    // Generate the needed thread data
    generate_thread_data(
        puzzleData,
        piece_vector_matrix,
        thread_data,
        sync,
        std::min(max_threads, optionsData->MaxThreads)
    );
    std::cout << std::format("Created data for {} thread(s)\n", actual_max_threads);

    // Now we can start the threads
    t_statistics_data total_statistics;
    {
        for(auto& data : *thread_data) {
            data.board->user_data = new t_board_user_data{
                .options = optionsData,
                .puzzleData = puzzleData,
                .sync = sync
            };
            data.board->solution_callback = handle_board_solution;
        }

        std::cout << "Starting reporting thread\n";
        std::jthread reporter([&]() {
            reporting_thread(thread_data, total_statistics, sync, optionsData);
        });

        std::cout << "Starting worker threads\n";
        // Now start the worker threads
        total_statistics.start_time = std::chrono::high_resolution_clock::now();
        total_statistics.start_clock_cycles = __rdtsc();
        {
            std::vector<std::jthread> workers;
            for (auto& data : *thread_data) {
                workers.emplace_back(worker_thread, std::ref(data));
            }
        }
        total_statistics.end_clock_cycles = __rdtsc();
        total_statistics.end_time = std::chrono::high_resolution_clock::now();
        sync->done = true;
    }

    std::cout << std::format("\nWork completed. Used {} thread(s)\n", thread_data->size());

    std::cout << std::format("Total solutions: {}. Total placed nodes: {}. Total checked nodes: {}\n", 
        total_statistics.total_solutions.load(),
        format_number_human_readable(total_statistics.total_nodes_placed),
        format_number_human_readable(total_statistics.total_nodes_checked));

    // Display some timing information
    std::cout << std::format("Total time: {}. Total clock cycles: {}\n",
        format_duration(total_statistics.end_time - total_statistics.start_time),
        format_number_human_readable(total_statistics.end_clock_cycles - total_statistics.start_clock_cycles));

    // How many nodes per second did we place?
    const auto total_seconds = std::chrono::duration_cast<std::chrono::seconds>(total_statistics.end_time - total_statistics.start_time).count();
    if (total_seconds > 0) {
        std::cout << std::format("Average nodes placed per second: {}. Average nodes checked per second: {}\n",
            format_number_human_readable(total_statistics.total_nodes_placed / total_seconds),
            format_number_human_readable(total_statistics.total_nodes_checked / total_seconds));
    }
    // How many clock cycles per placed node and checked node?
    if (total_statistics.total_nodes_placed > 0) {
        std::cout << std::format("Average clock cycles per placed node: {:.2}. Average clock cycles per checked node: {:.2}\n",
            static_cast<double>(total_statistics.end_clock_cycles - total_statistics.start_clock_cycles) / total_statistics.total_nodes_placed,
            static_cast<double>(total_statistics.end_clock_cycles - total_statistics.start_clock_cycles) / total_statistics.total_nodes_checked);
    }


    // Cleanup
    for(auto& data : *thread_data) {
        auto user_data = static_cast<t_board_user_data*>(data.board->user_data);
        if (user_data != nullptr)
            delete user_data;

        data.board->user_data = nullptr;
        if (data.board != nullptr)
            _aligned_free(data.board);
    }

    // We need to free the piece vectors
    for (uint32_t i = 0; i < static_cast<uint32_t>(CELL_TYPE::MAX) * piece_vector_matrix->cell_type_offset; ++i) {
        // Free each vector if it was allocated
        if (piece_vector_matrix->pieces[i] != nullptr) {
            delete piece_vector_matrix->pieces[i];
            piece_vector_matrix->pieces[i] = nullptr;
        }
    }
    _aligned_free(piece_vector_matrix);

    return RETURN_OK;
}
