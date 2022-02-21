
#include <memory>		// shared_ptr, make_shared
#include <chrono>

#include "../utils/SDLWrapper.h"
#include "../utils/FileIOHandler.h"

// global instances
//std::shared_ptr<SDLWrapper> sdl = nullptr;
std::shared_ptr<FileIOHandler> data_writer = nullptr;

#define SIM_FREQUENCY_HZ 50.0

#define WAVE_SIZE 60
sim_data_t IABP_data[WAVE_SIZE];
sim_data_t SAP_data[WAVE_SIZE];

const size_t NUM_VARIABLES = 2;	// IABP and SAP for now


//---------------------------------------------------------------------------------------
void simulate_data(unsigned long it)
{
	std::vector<sim_data_t> data = { IABP_data[it % WAVE_SIZE], 
									 SAP_data[it % WAVE_SIZE] 
							  };
	data_writer->aioWrite(data);
}
//---------------------------------------------------------------------------------------
void main_loop()
{
	bool is_running = true;

	static double frame_time = 1e6 / SIM_FREQUENCY_HZ;
	static long it = 0;

	while (is_running)
	{
		//
		auto t0 = std::chrono::high_resolution_clock::now();

		// simulate data based on tick
		simulate_data(it);

		// enforce an update frequency of SIM_FREQUENCY_HZ
		auto t1 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::micro> dur = t1 - t0;
		double elapsed_us = dur.count();
		double idle_time = frame_time - elapsed_us;
		usleep((useconds_t)idle_time);
		
		auto t2 = std::chrono::high_resolution_clock::now();
		dur = t2 - t0;
		if (it % 100 == 0)
			printf("DATA SIM FREQUNECY = %.5f Hz\n", 1e6 / dur.count());

		it++;
	}
}
//---------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	// tmp -- init data
	for (uint32_t i = 0; i < WAVE_SIZE; i++)
	{
		IABP_data[i] = (sim_data_t)i;
		SAP_data[i] = (sim_data_t)(i * 0.5f);
	}

	// Init SDL
	//printf("--- Initializing SDL ---\n");
	//sdl = std::make_shared<SDLWrapper>();

	// Create async IO stream
	printf("--- Creating data streams ---\n");
	data_writer = newFileIOHandler(NUM_VARIABLES, FileIOMode::WRITE);

	// start generating data
	printf("--- Simulating data ---\n");
	main_loop();

	//
	return 0;
}