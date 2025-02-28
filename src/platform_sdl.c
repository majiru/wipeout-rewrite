#include <SDL2/SDL.h>

#include "platform.h"
#include "input.h"
#include "system.h"

static uint64_t perf_freq = 0;
static bool wants_to_exit = false;
static SDL_Window *window;
static SDL_AudioDeviceID audio_device;
static void (*audio_callback)(float *buffer, uint32_t len) = NULL;

void platform_exit() {
	wants_to_exit = true;
}

void platform_pump_events(void) {
	SDL_Event ev;
	while (SDL_PollEvent(&ev)) {
		// Input Keyboard
		if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
			int code = ev.key.keysym.scancode;
			float state = ev.type == SDL_KEYDOWN ? 1.0 : 0.0;
			if (code >= SDL_SCANCODE_LCTRL && code <= SDL_SCANCODE_RALT) {
				int code_internal = code - SDL_SCANCODE_LCTRL + INPUT_KEY_LCTRL;
				input_set_button_state(code_internal, state);
			}
			else if (code > 0 && code < INPUT_KEY_MAX) {
				input_set_button_state(code, state);
			}
		}

		else if (ev.type == SDL_TEXTINPUT) {
			input_textinput(ev.text.text[0]);
		}
		// Mouse buttons
		else if (
			ev.type == SDL_MOUSEBUTTONDOWN ||
			ev.type == SDL_MOUSEBUTTONUP
		) {
			button_t button = INPUT_BUTTON_NONE;
			switch (ev.button.button) {
				case SDL_BUTTON_LEFT: button = INPUT_MOUSE_LEFT; break;
				case SDL_BUTTON_MIDDLE: button = INPUT_MOUSE_MIDDLE; break;
				case SDL_BUTTON_RIGHT: button = INPUT_MOUSE_RIGHT; break;
				default: break;
			}
			if (button != INPUT_BUTTON_NONE) {
				float state = ev.type == SDL_MOUSEBUTTONDOWN ? 1.0 : 0.0;
				input_set_button_state(button, state);
			}
		}

		// Mouse wheel
		else if (ev.type == SDL_MOUSEWHEEL) {
			button_t button = ev.wheel.y > 0 
				? INPUT_MOUSE_WHEEL_UP
				: INPUT_MOUSE_WHEEL_DOWN;
			input_set_button_state(button, 1.0);
			input_set_button_state(button, 0.0);
		}

		// Mouse move
		else if (ev.type == SDL_MOUSEMOTION) {
			input_set_mouse_pos(ev.motion.x, ev.motion.y);
		}

		// Window Events
		if (ev.type == SDL_QUIT) {
			wants_to_exit = true;
		}
		else if (
			ev.type == SDL_WINDOWEVENT &&
			(
				ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
				ev.window.event == SDL_WINDOWEVENT_RESIZED
			)
		) {
			system_resize(platform_screen_size());
		}
	}
}

double platform_now() {
	uint64_t perf_counter = SDL_GetPerformanceCounter();
	return (double)perf_counter / (double)perf_freq;
}

void platform_set_fullscreen(bool fullscreen) {
	if (fullscreen) {
		int32_t display = SDL_GetWindowDisplayIndex(window);
		
		SDL_DisplayMode mode;
		SDL_GetDesktopDisplayMode(display, &mode);
		//SDL_SetWindowDisplayMode(window, &mode);
		//SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
		SDL_ShowCursor(SDL_DISABLE);
	}
	else {
		SDL_SetWindowFullscreen(window, 0);
		SDL_ShowCursor(SDL_ENABLE);
	}
}

void platform_audio_callback(void* userdata, uint8_t* stream, int len) {
	if (audio_callback) {
		audio_callback((float *)stream, len/sizeof(float));
	}
	else {
		memset(stream, 0, len);
	}
}

void platform_set_audio_mix_cb(void (*cb)(float *buffer, uint32_t len)) {
	audio_callback = cb;
	SDL_PauseAudioDevice(audio_device, 0);
}


#if defined(RENDERER_GL) // ----------------------------------------------------
	#define PLATFORM_WINDOW_FLAGS SDL_WINDOW_OPENGL
	SDL_GLContext platform_gl;

	void platform_video_init() {
		#if defined(USE_GLES2)
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
		#endif

		platform_gl = SDL_GL_CreateContext(window);
		SDL_GL_SetSwapInterval(1);
	}

	void platform_prepare_frame() {
		// nothing
	}

	void platform_video_cleanup() {
		SDL_GL_DeleteContext(platform_gl);
	}

	void platform_end_frame() {
		SDL_GL_SwapWindow(window);
	}

	vec2i_t platform_screen_size() {
		int width, height;
		SDL_GL_GetDrawableSize(window, &width, &height);
		return vec2i(width, height);
	}


#elif defined(RENDERER_SOFTWARE) // ----------------------------------------------
	#define PLATFORM_WINDOW_FLAGS 0
	static SDL_Renderer *renderer;
	static SDL_Texture *screenbuffer = NULL;
	static void *screenbuffer_pixels = NULL;
	static int screenbuffer_pitch;
	static vec2i_t screenbuffer_size;
	static vec2i_t screen_size;

	void platform_video_init(void) {
		screenbuffer_size = vec2i(0, 0);
		screen_size = vec2i(0, 0);
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	}

	void platform_video_cleanup(void) {
		
	}

	void platform_prepare_frame(void) {
		if (screen_size.x != screenbuffer_size.x || screen_size.y != screenbuffer_size.y) {
			if (screenbuffer) {
				SDL_DestroyTexture(screenbuffer);
			}
			screenbuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, screen_size.x, screen_size.y);
			screenbuffer_size = screen_size;
		}
		SDL_LockTexture(screenbuffer, NULL, &screenbuffer_pixels, &screenbuffer_pitch);
	}

	void platform_end_frame(void) {
		screenbuffer_pixels = NULL;
		SDL_UnlockTexture(screenbuffer);
		SDL_RenderCopy(renderer, screenbuffer, NULL, NULL);
		SDL_RenderPresent(renderer);
	}

	rgba_t *platform_get_screenbuffer(int32_t *pitch) {
		*pitch = screenbuffer_pitch;
		return screenbuffer_pixels;
	}

	vec2i_t platform_screen_size(void) {
		int width, height;
		SDL_GetWindowSize(window, &width, &height);

		// float aspect = (float)width / (float)height;
		// screen_size = vec2i(240 * aspect, 240);
		screen_size = vec2i(width, height);
		return screen_size;
	}

#else
	#error "Unsupported renderer for platform SDL"
#endif


void global_init(void);

int main(int argc, char *argv[]) {
	global_init();
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);

	SDL_AudioSpec spec = {
		.freq = 44100,
		.format = AUDIO_F32,
		.channels = 2,
		.samples = 1024,
		.callback = platform_audio_callback
	};

	audio_device = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);

	perf_freq = SDL_GetPerformanceFrequency();

	window = SDL_CreateWindow(
		SYSTEM_WINDOW_NAME,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		SYSTEM_WINDOW_WIDTH, SYSTEM_WINDOW_HEIGHT,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | PLATFORM_WINDOW_FLAGS | SDL_WINDOW_ALLOW_HIGHDPI
	);

	platform_video_init();
	system_init();

	while (!wants_to_exit) {
		platform_pump_events();
		platform_prepare_frame();
		system_update();
		platform_end_frame();
	}

	system_cleanup();
	platform_video_cleanup();

	SDL_CloseAudioDevice(audio_device);
	SDL_Quit();
	return 0;
}
