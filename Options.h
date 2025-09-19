#pragma once
#include "cxxopts.hpp"
#include "Common.h"


typedef struct
{
    std::string PuzzleFile;
    bool FirstSolution;
    bool DisplayOnConsole;
    bool Bucas;
    int64_t MaxNodesToPlace;
    int64_t MaxThreads;
} t_options;

INLINE
static void configure_options(cxxopts::Options& options)
{
    options.add_options()
        ("p, puzzle", "The puzzle input file", cxxopts::value<std::string>())
        ("f, first", "Stop at the first solution found", cxxopts::value<bool>()->default_value("false"))
        ("d, display", "Display all solutions on the console", cxxopts::value<bool>()->default_value("false"))
        ("u, bucas", "Display the e2.bucas URL for a solution", cxxopts::value<bool>()->default_value("false"))
        ("m, max-nodes", "Max nodes to place", cxxopts::value<int64_t>()->default_value("-1"))
        ("n, number-threads", std::format("Max number of threads to use when searching for solutions ({}).", std::thread::hardware_concurrency() - 1), cxxopts::value<int64_t>()->default_value("1"));
}

INLINE
static std::optional<std::shared_ptr<t_options>> load_options(cxxopts::Options& options, const int argc, const char* argv[])
{
    configure_options(options);

    const auto commandLine = options.parse(argc, argv);
    if (!commandLine.count("puzzle"))
        return std::nullopt;

    auto puzzle_options = std::make_shared<t_options>();
    puzzle_options->PuzzleFile = commandLine["puzzle"].as<std::string>();
    puzzle_options->FirstSolution = commandLine["first"].as<bool>();
    puzzle_options->DisplayOnConsole = commandLine["display"].as<bool>();
    puzzle_options->Bucas = commandLine["bucas"].as<bool>();
    puzzle_options->MaxNodesToPlace = commandLine["max-nodes"].as<int64_t>();   
    puzzle_options->MaxThreads = commandLine["number-threads"].as<int64_t>();
    return puzzle_options;
}
