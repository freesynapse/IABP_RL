
#include <memory>
#include <chrono>

#include "../utils/SDLWrapper.h"
#include "../utils/AsyncIOHandler.h"

//
#define READ_DATA_FREQUENCY 200.0 // in milliseconds
const int NUM_VARIABLES = 2;

std::shared_ptr<SDLWrapper> sdl_wrapper = nullptr;
std::shared_ptr<AsyncIOHandler> data_reader = nullptr;

//
int main(int argc, char* argv[])
{
    /*
    float* f;
    int sz = 20;
    int idx = 0;
    f = new float[sz];
    memset(f, 0, sizeof(float)*sz);
    for (int i = 0; i < sz; i++)
        f[i] = (float)i;

    for (int n = 0; n < 10; n++)
    {
        printf("%d -- ", n);
        for (int i = idx; i < idx + sz; i++)
            printf("%.1f ", f[i % sz]);
        printf("\n");
        idx++;
    }

    delete[] f;
    exit(0);
    */

    // instantiate an async IO handler in reading mode
    printf("--- Reading data stream ---\n");
    data_reader = newAsyncIOHandler(NUM_VARIABLES, AsyncIOMode::READ);

    // initalize SDL
    printf("--- Initializing SDL ---\n");
    sdl_wrapper = newSDLWrapper(640, 480, data_reader->getDataBlockSize(), 2);

    //
    bool is_running = true;
    double frame_time_us = 1000.0 / READ_DATA_FREQUENCY * 1e3;
    long it = 0;

    size_t data_block_size = data_reader->getDataBlockSize(); 
    sim_data_t* data = new sim_data_t[data_block_size];
    float fps = 0.0f;

    while (is_running)
    {
        auto t0 = std::chrono::high_resolution_clock::now();

        // start the SDL layer this frame
        sdl_wrapper->beginFrame();
        

        // read the data
        size_t read_bytes = data_reader->aioRead(data);
        if (read_bytes > 0)
        {
            printf("%ld: read %lu bytes: [ ", it, read_bytes);
            for (int i = 0; i < data_block_size; i++)
                printf("%.1f ", data[i]);
            printf("]\n");
            sdl_wrapper->addData(data, data_reader->getDataBlockSize());
        }

        // render lines
        sdl_wrapper->renderChannelData();

        // end SDL frame 
        sdl_wrapper->endFrame(fps);
        // exit conditions from SDL wrapper (based on SDL events)
        is_running = sdl_wrapper->isRunning();

        // timing stuff
        auto t1 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> dur = t1 - t0;
        double elapsed_us = dur.count();
        double idle_time = frame_time_us - elapsed_us;
        if (idle_time > 0)
            usleep((useconds_t)idle_time);
        auto t2 = std::chrono::high_resolution_clock::now();
        dur = t2 - t0;
        // calculate FPS
        if (it % (int)READ_DATA_FREQUENCY == 0)
            fps = 1e6 / dur.count();

        it++;
    }

    delete[] data;

    return 0;
}