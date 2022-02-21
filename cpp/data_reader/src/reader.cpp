
#include <memory>
#include <chrono>

#include "../utils/SDLWrapper.h"
#include "../utils/FileIOHandler.h"

#define READ_DATA_FREQUENCY 200.0 // in milliseconds

std::shared_ptr<FileIOHandler> data_reader = nullptr;
const int NUM_VARIABLES = 2;

int main(int argc, char* argv[])
{
    // instantiate a IO handler in reading mode
    data_reader = newFileIOHandler(NUM_VARIABLES, FileIOMode::READ);

    bool is_running = true;
    double frame_time_us = 1000.0 / READ_DATA_FREQUENCY * 1e3;
    long it = 0;

    size_t data_block_size = data_reader->getDataBlockSize(); 
    sim_data_t* data = new sim_data_t[data_block_size];

    while (is_running)
    {
        auto t0 = std::chrono::high_resolution_clock::now();

        // read the data
        size_t read_bytes = data_reader->aioRead(data);
        if (read_bytes > 0)
        {
            printf("%ld: read %lu bytes: [ ", it, read_bytes);
            for (int i = 0; i < data_block_size; i++)
                printf("%u ", data[i]);
            printf("]\n");
        }

        // timing stuff
        auto t1 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> dur = t1 - t0;
        double elapsed_us = dur.count();
        double idle_time = frame_time_us - elapsed_us;
        if (idle_time > 0)
            usleep((useconds_t)idle_time);
        auto t2 = std::chrono::high_resolution_clock::now();
        dur = t2 - t0;
        if (it % 1000 == 0)
            printf("DATA READ FREQUENCY = %.5f Hz\n", 1e6 / dur.count());

        it++;
    }

    delete[] data;

    return 0;
}