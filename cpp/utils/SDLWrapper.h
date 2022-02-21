
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <X11/Xlib.h>

#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <chrono>
#include <assert.h>

#include "AsyncIOHandler.h"

// ease-of use stuff
#define MAX(a,b) ((a)>(b)?(a):(b))
typedef std::chrono::high_resolution_clock::time_point time_point_t;

// wrapper macro for IntelliSense
class SDLWrapper;
inline std::shared_ptr<SDLWrapper> newSDLWrapper(int _width=640, int _height=480, int _channels=2, int _x_res=1)
{ return std::make_shared<SDLWrapper>(_width, _height, _channels, _x_res); }

//
class SDLWrapper
{
private:
	//-----------------------------------------------------------------------------------
	// SDL window and rendering pointers
	SDL_Window* m_window = nullptr;
	SDL_Renderer* m_renderer = nullptr;
	TTF_Font* m_font = nullptr;
	Screen* m_x11screen = nullptr;
	int m_width = 0;
	int m_height = 0;
	// timing stuff
	time_point_t m_frameStart;
	time_point_t m_frameEnd;
	uint64_t m_frameCount = 0;
	float m_FPS = 0.0f;
	float m_frameTime = 0.0f;
	bool m_isRunning = true;
	// render colors
	const SDL_Color s_textColor = { 0, 0, 0, 255 };
	const SDL_Color s_renderColor = { 0, 0, 0, 255 };
	const SDL_Color s_clearColor = { 255, 255, 255, 255 };
	// data
	int m_channelCount = 0;		// the number of variables updates each timestep (of floats)
	int m_xScale = 0;			// the scale of the x axis; 1 cooresponds to x increase of 1 for every m_width etc.
	int m_xRes = 0;
	sim_data_t** m_data = nullptr;
	uint64_t m_currentIndex = 0;

public:
	//-----------------------------------------------------------------------------------
	SDL_Window* getWindowPtr() { return m_window; }
	SDL_Renderer* getRendererPtr() { return m_renderer; }
	const float getFPS() const { return m_FPS; }
	const uint64_t getFrameCount() const { return m_frameCount; }
	const bool isRunning() const { return m_isRunning; }

public:
	//-----------------------------------------------------------------------------------
	SDLWrapper(int _width, int _height, int _channels, int _x_res) :
		m_width(_width),
		m_height(_height),
		m_channelCount(_channels),
		m_xRes(_x_res)
	{ 
		// find screen resolution for X11 for positioning of window
		Display* display = XOpenDisplay(NULL);
		m_x11screen = DefaultScreenOfDisplay(display);
		
		// sanity check
		if (m_width > m_x11screen->width ||  m_height > m_x11screen->height)
		{
			m_width = m_x11screen->width;
			m_height = m_x11screen->height;
		}

		assert(_x_res > 0);
		m_xScale = (int)(m_width / _x_res);

		// now init SDL
		if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		{
			printf("Error initializing SDL: %s\n", SDL_GetError());
			exit(-1);
		}

		m_window = SDL_CreateWindow("IABP_RL data simulator", 
									MAX(m_x11screen->width - m_width - 5, 0), 
									0,
									m_width, 
									m_height, 
									SDL_WINDOW_SHOWN);
		if (!m_window)
		{
			printf("Error creating window: %s\n", SDL_GetError());
			exit(-1);
		}

		m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
		if (!m_renderer)
		{
			printf("Error creating renderer: %s\n", SDL_GetError());
			exit(-1);
		}

		if (TTF_Init() < 0)
		{
			printf("Error initializing SDL TTF library: %s\n", SDL_GetError());
			exit(-1);
		}

		m_font = TTF_OpenFont("/usr/share/fonts/JetBrainsMono-Medium.ttf", 14);
		if (!m_font)
		{
			printf("Error loading font: %s\n", SDL_GetError());
			exit(-1);
		}

		// dimension data structures
		m_data = new sim_data_t*[m_channelCount];
		for (int c = 0; c < m_channelCount; c++)
			m_data[c] = new sim_data_t[m_xScale];
	}
	//-----------------------------------------------------------------------------------
	~SDLWrapper() 
	{
		// release data
		for (int c = 0; c < m_channelCount; c++)
			delete[] m_data[c];
		delete[] m_data;

		// release various SDL components
		TTF_CloseFont(m_font);
		TTF_Quit();

		if (m_renderer != nullptr)
			SDL_DestroyRenderer(m_renderer);
		
		if (m_window != nullptr)
			SDL_DestroyWindow(m_window);
		SDL_Quit();	
	}
	//-----------------------------------------------------------------------------------
	void beginFrame()
	{
		
		m_frameStart = std::chrono::high_resolution_clock::now();

		SDL_SetRenderDrawColor(m_renderer, 
							   s_clearColor.r, 
							   s_clearColor.g, 
							   s_clearColor.b, 
							   s_clearColor.a);
		SDL_RenderClear(m_renderer);
	}
	//-----------------------------------------------------------------------------------
	double endFrame(float _fps)
	{
		// process events
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
			case SDL_QUIT: 
				m_isRunning = false;
				break;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE)
					m_isRunning = false;
				break; 
			}
		}

		// render fps counter
		m_FPS = _fps;
		renderFPS();

		
		// flip
		SDL_RenderPresent(m_renderer);

		m_frameEnd = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::micro> dur = m_frameEnd - m_frameStart;
		return dur.count();
	}
	//-----------------------------------------------------------------------------------
	void addData(sim_data_t *_data, size_t _block_size)
	{
		// Updates one float per channel at a time (for now)
		//
		assert(_block_size == m_channelCount);
		for (size_t c = 0; c < _block_size; c++)
			m_data[c][m_currentIndex % m_xScale] = _data[c];
		m_currentIndex++;
	}
	//-----------------------------------------------------------------------------------
	void renderChannelData()
	{
		SDL_SetRenderDrawColor(m_renderer, s_renderColor.r, s_renderColor.g, s_renderColor.b, s_renderColor.a);
		
		for (int c = 0; c < m_channelCount; c++)
		{
			int x = m_xRes;
			for (uint64_t i = m_currentIndex; i < m_currentIndex + m_xScale; i++)
			{
				SDL_RenderDrawLine(m_renderer, x-m_xRes, m_data[c][i % m_xScale], x, m_data[c][(i-1) % m_xScale]);
				x += m_xRes;
			}
		}
	}

private:
	//-----------------------------------------------------------------------------------
	void renderFPS()
	{
		static SDL_Surface* text;
		static SDL_Texture* text_texture;
		static SDL_Rect dest;

		std::stringstream sstr;
		sstr << std::fixed << std::setprecision(2) << m_FPS;

		text = TTF_RenderText_Solid(m_font, sstr.str().c_str(), s_textColor);
		if (!text)
			printf("Error generating surface from text: %s\n", SDL_GetError());

		text_texture = SDL_CreateTextureFromSurface(m_renderer, text);
		dest = { m_width - text->w, 0, text->w, text->h };
		SDL_RenderCopy(m_renderer, text_texture, NULL, &dest);
		SDL_DestroyTexture(text_texture);
		SDL_FreeSurface(text);
	}

};



