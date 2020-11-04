/**
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/

#ifndef ED25519_BENCHMARK_H
#define ED25519_BENCHMARK_H

#ifndef BENCHMARK_PERFORMANCE_ITERATIONS
#define BENCHMARK_PERFORMANCE_ITERATIONS 10000
#endif

#ifndef BENCHMARK_PERFORMANCE_ITERATIONS_LONG_MULTIPLIER
#define BENCHMARK_PERFORMANCE_ITERATIONS_LONG_MULTIPLIER 60
#endif

#ifndef BENCHMARK_PREFIX_WIDTH
#define BENCHMARK_PREFIX_WIDTH 40
#endif

#ifndef BENCHMARK_COLUMN_WIDTH
#define BENCHMARK_COLUMN_WIDTH 16
#endif

#ifndef BENCHMARK_PRECISION
#define BENCHMARK_PRECISION 5
#endif

#define BENCHMARK_PERFORMANCE_ITERATIONS_LONG \
    BENCHMARK_PERFORMANCE_ITERATIONS *BENCHMARK_PERFORMANCE_ITERATIONS_LONG_MULTIPLIER

#define NOW() std::chrono::high_resolution_clock::now()
#define NOW_DIFF(b) \
    static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(NOW() - b).count()) / 1'000'000.0

#include <cfloat>
#include <chrono>
#include <iomanip>
#include <iostream>

/**
 * Prints the benchmark header for a "table" like setup
 *
 * @param prefix_width
 * @param column_width
 */
static inline void
    benchmark_header(int8_t prefix_width = BENCHMARK_PREFIX_WIDTH, int8_t column_width = BENCHMARK_COLUMN_WIDTH)
{
    std::cout << std::setw(prefix_width) << "BENCHMARK TESTS"
              << ": " << std::setw(10) << " " << std::setw(column_width) << "Average" << std::setw(column_width)
              << "Minimum" << std::setw(column_width) << "Maximum" << std::setw(column_width) << "Total" << std::endl;
}

/**
 * Performs a benchmark of the given function for the number of iterations specified
 *
 * @tparam T
 * @param function
 * @param functionName
 * @param iterations
 * @param prefix_width
 * @param column_width
 * @param precision
 */
template<typename T>
void benchmark(
    T &&function,
    const std::string &functionName = "",
    const size_t iterations = BENCHMARK_PERFORMANCE_ITERATIONS,
    int8_t prefix_width = BENCHMARK_PREFIX_WIDTH,
    int8_t column_width = BENCHMARK_COLUMN_WIDTH,
    int8_t precision = BENCHMARK_PRECISION)
{
    if (static_cast<double>(iterations) > DBL_MAX)
    {
        throw std::invalid_argument("iterations exceeds bounds of double");
    }

    if (!functionName.empty())
    {
        std::cout << std::setw(prefix_width) << functionName.substr(0, prefix_width) << ": " << std::flush;
    }

    const auto tenth = (iterations >= 10) ? iterations / 10 : 1;

    double minimum_time = DBL_MAX, maximum_time = 0, total_time = 0;

    for (size_t i = 0; i < iterations; ++i)
    {
        const auto single_iter_timer = NOW();

        function();

        const auto single_elapsed = NOW_DIFF(single_iter_timer);

        total_time += single_elapsed;

        if (i % tenth == 0)
        {
            std::cout << "." << std::flush;
        }

        if (single_elapsed > maximum_time)
        {
            maximum_time = single_elapsed;
        }
        else if (single_elapsed < minimum_time && single_elapsed >= 0)
        {
            minimum_time = single_elapsed;
        }
    }

    const auto average_time = total_time / static_cast<double>(iterations);

    std::cout << std::fixed << std::setprecision(precision) << std::setw(column_width) << average_time << std::fixed
              << std::setprecision(precision) << std::setw(column_width) << minimum_time << std::fixed
              << std::setprecision(precision) << std::setw(column_width) << maximum_time << std::fixed
              << std::setprecision(precision) << std::setw(column_width) << total_time << " ms" << std::endl;
}

/**
 * Performs a benchmark of the given function for the number of iterations specified
 *
 * @tparam T
 * @param function
 * @param functionName
 * @param iterations
 * @param prefix_width
 * @param column_width
 * @param precision
 */
template<typename T>
void benchmark_long(
    T &&function,
    const std::string &functionName = "",
    const size_t iterations = BENCHMARK_PERFORMANCE_ITERATIONS_LONG,
    int8_t prefix_width = BENCHMARK_PREFIX_WIDTH,
    int8_t column_width = BENCHMARK_COLUMN_WIDTH,
    int8_t precision = BENCHMARK_PRECISION)
{
    benchmark(function, functionName, iterations, prefix_width, column_width, precision);
}

#endif // ED25519_BENCHMARK_H
