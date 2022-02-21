
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <X11/Xlib.h>

#include <sstream>
#include <iomanip>
#include <unistd.h>


#define WIDTH 640
#define HEIGHT 480
#define SIM_FREQ 50.0f


class SDLWrapper
{
private:
	SDL_Window* m_window = nullptr;
	SDL_Renderer* m_renderer = nullptr;
	TTF_Font* m_font = nullptr;
	Screen* m_x11screen = nullptr;
	uint64_t m_frameStart = 0;
	uint64_t m_frameEnd = 0;
	uint64_t m_frameCount = 0;
	float m_FPS = 0.0f;
	float m_frameTime = 0.0f;
	const SDL_Color s_textColor = { 0, 0, 0, 255 };
	const SDL_Color s_clearColor = { 255, 255, 255, 255 }; 

public:
	//-----------------------------------------------------------------------------------
	SDL_Window* getWindowPtr() { return m_window; }
	SDL_Renderer* getRendererPtr() { return m_renderer; }
	const float getFPS() const { return m_FPS; }
	const uint64_t getFrameCount() const { return m_frameCount; }
public:
	//-----------------------------------------------------------------------------------
	SDLWrapper(bool _init=true) { if (_init) init(); }
	~SDLWrapper() { shutdown(); }
	//-----------------------------------------------------------------------------------
	void init()
	{
		// find screen resolution for X11 for positioning of window
		Display* display = XOpenDisplay(NULL);
		m_x11screen = DefaultScreenOfDisplay(display);

		// now init SDL
		if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		{
			printf("Error initializing SDL: %s\n", SDL_GetError());
			exit(-1);
		}

		m_window = SDL_CreateWindow("IABP_RL data simulator", 
									m_x11screen->width - WIDTH - 5, 
									0,
									WIDTH, 
									HEIGHT, 
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
	}
	//-----------------------------------------------------------------------------------
	void shutdown()
	{
		TTF_CloseFont(m_font);
		TTF_Quit();

		if (m_renderer != nullptr)
			SDL_DestroyRenderer(m_renderer);
		
		if (m_window != nullptr)
			SDL_DestroyWindow(m_window);
		SDL_Quit();	
	}
	//-----------------------------------------------------------------------------------
	void startFrame()
	{
		m_frameStart = SDL_GetPerformanceCounter();

		SDL_SetRenderDrawColor(m_renderer, 
							   s_clearColor.r, 
							   s_clearColor.g, 
							   s_clearColor.b, 
							   s_clearColor.a);
		SDL_RenderClear(m_renderer);
	}
	//-----------------------------------------------------------------------------------
	bool endFrame()
	{
		static float time_res_inv = 1.0f / SDL_GetPerformanceFrequency();
		static float delta_t = (1000.0f / SIM_FREQ) * 1e3;

		bool is_running = true;

		// process events
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
			case SDL_QUIT: 
				is_running = false;
				break;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE)
					is_running = false;
				break; 
			}
		}

		// render fps counter
		renderFPS();

		// compute frame-time and idle-time
		m_frameEnd = SDL_GetPerformanceCounter();

		float elapsed_us = (float)(m_frameEnd - m_frameStart) * time_res_inv * 1e6;
		if (elapsed_us < delta_t)
			usleep((delta_t - elapsed_us));
		
		uint64_t t2 = SDL_GetPerformanceCounter();
		m_frameTime = (float)(t2 - m_frameStart) * time_res_inv * 1e3;

		// update fps
		if (m_frameCount % 50 == 0)
			m_FPS = 1000.0f / m_frameTime;

		m_frameCount++;

		// flip
		SDL_RenderPresent(m_renderer);

		return is_running;
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
		dest = { WIDTH - text->w, 0, text->w, text->h };
		SDL_RenderCopy(m_renderer, text_texture, NULL, &dest);
		SDL_DestroyTexture(text_texture);
		SDL_FreeSurface(text);
	}

};



