/**
 * @author Saleh Elkaza
 */



#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <tuple>
#include <complex>
#include <chrono>
#include <atomic>
#include <thread>

#include "mandelbrot-helpers.hpp"

using namespace std;


/**
 * @brief Computes Mandelbrot set using static work allocation.
 * 
 * @param[inout] image        - The image to which Mandelbrot values will be stored.
 * @param[in]    thread_id    - The ID of the current worker thread.
 * @param[in]    num_threads  - The total number of threads used.
 * @param[inout] pixels_inside- Atomic counter for pixels inside the Mandelbrot set.
 */
void worker_static(Image &image, int thread_id, int num_threads,std::atomic<int>& pixels_inside /*ADDED <--*/ /*int& pixels_inside*/)
{
    // Get image dimensions
    int rows = image.height;
    int cols = image.width;
    
    // Initialize pixel as black (0,0,0)
    std::array pixel = {0, 0, 0}; 

    // Initialize a complex number variable
    complex<double> c;

    
    // Generate a color based on the thread ID
    int color = (255*thread_id)/num_threads; // 0 means { 0, 0, 0 } -> black

    int start_row = (rows / num_threads) * thread_id;  
    int end_row = (thread_id == num_threads - 1) ? rows : start_row + (rows / num_threads);

    for (int row = start_row; row < end_row; row++) 
    {
        for (int col = 0; col < cols; col++) 
        {
            // Map pixel coordinates to the complex plane
            double dx = ((double)col / cols - 0.75) * 2;
            double dy = ((double)row / rows - 0.5) * 2;

            // Assign coordinates to the complex number
            c = complex<double>(dx, dy);

            // Calculate Mandelbrot set for the current complex number
            if (mandelbrot_kernel(c, pixel, color)) // the actual mandelbrot kernel, 0 means black color for mandelbrot pixels
                // Atomic increment
                pixels_inside.fetch_add(1, std::memory_order_relaxed); 

            image[row][col] = pixel; 
        }
    }
}


/**
 * @brief Computes Mandelbrot set using dynamic work allocation.
 * 
 * @param[inout] image        - The image to which Mandelbrot values will be stored.
 * @param[in]    thread_id    - The ID of the current worker thread.
 * @param[in]    num_threads  - The total number of threads used.
 * @param[inout] next_row     - Atomic counter for the next row to be processed.
 * @param[inout] pixels_inside- Atomic counter for pixels inside the Mandelbrot set.
 */
void worker_dynamic(Image &image, int thread_id, int num_threads, std::atomic<int>& next_row, std::atomic<int>& pixels_inside){
    int cols = image.width;

    // pixel to be passed to the mandelbrot function
    array pixel = {0, 0, 0}; 
    complex<double> c;
    int color = (255*thread_id)/num_threads; 

    int row;
    while ((row = next_row++) < image.height) {
        for (int col = 0; col < cols; col++) {
            double dx = ((double)col / cols - 0.75) * 2;
            double dy = ((double)row / image.height - 0.5) * 2;

            c = complex<double>(dx, dy);

            if (mandelbrot_kernel(c, pixel, color))
                pixels_inside++;

            image[row][col] = pixel;
        }
    }
}


int main(int argc, char **argv)
{

    //int num_threads = std::thread::hardware_concurrency();
    int num_threads = 32;  
    std::string work_allocation = "dynamic";
    int print_level = 2; 

    // Default image dimensions
    // height and width of the output image
    int width = 960, height = 720;

    // Parse input arguments
    parse_args(argc, argv, num_threads, work_allocation, height, width, print_level);

    // Variable to store computation time
    double time;

    // Variable to count number of pixels inside the Mandelbrot set
    int pixels_inside2 = 0;
    std::atomic<int> pixels_inside(0); 

    // Generate Mandelbrot set in this image
    Image image(height, width); // Initialize an image with specified dimensions

    // Start the timer
    auto t1 = chrono::high_resolution_clock::now();

    if ( work_allocation.compare("static") == 0 ) {
            std::vector<std::thread> threads;
            for (int tid = 0; tid < num_threads; ++tid) {
                    threads.push_back(std::thread(worker_static, std::ref(image), tid, num_threads, std::ref(pixels_inside)));
            }

            for (auto &t : threads) {
                    t.join();
            }
    } 
  
    std::atomic<int> next_row(0); 
    if ( work_allocation.compare("dynamic") == 0 ) {
        // spawn threads and pass parameters 
         std::vector<std::thread> threads;
            for (int tid = 0; tid < num_threads; ++tid) {
                threads.push_back(std::thread(worker_dynamic, std::ref(image), tid, num_threads, std::ref(next_row), std::ref(pixels_inside)));
            }

            for (auto &t : threads) {
                t.join();
            }
    }

    // Stop the timer
    auto t2 = chrono::high_resolution_clock::now();

    // save image
    image.save_to_ppm("mandelbrot-32-for-dynamic.ppm"); // Save the generated image to a file

    // Print results based on verbosity level
    if ( print_level >= 2) cout << "Work allocation: " << work_allocation << endl;
    if ( print_level >= 1 ) cout << "Total Mandelbrot pixels: " << pixels_inside << endl;

    // Print computation time
    cout << chrono::duration<double>(t2 - t1).count() << endl;

    return 0;
}
