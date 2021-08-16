#include "rsw_math.h"

#define MANGLE_TYPES
#define PORTABLEGL_IMPLEMENTATION
#include "GLObjects.h"


#include <iostream>
#include <stdio.h>


#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#define WIDTH 640
#define HEIGHT 480

using namespace std;

using rsw::vec4;
using rsw::mat4;

vec4 Red(1.0f, 0.0f, 0.0f, 0.0f);
vec4 Green(0.0f, 1.0f, 0.0f, 0.0f);
vec4 Blue(0.0f, 0.0f, 1.0f, 0.0f);

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;

u32* bbufpix;

glContext the_Context;

typedef struct My_Uniforms
{
	mat4 mvp_mat;
} My_Uniforms;

int depth_test;


void cleanup();
void setup_context();
void setup_gl_data();

void smooth_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void smooth_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

int main(int argc, char** argv)
{
	setup_context();

	GLenum smooth[4] = { SMOOTH, SMOOTH, SMOOTH, SMOOTH };

#define NUM_TRIANGLES 4

	float points[] = { -1, 1, 0.9,
	                   -1, -1, 0.9,
	                    1, -1, -0.9,

	                    -0.8, -0.7, -2,
                        -0.8, -0.9, -2,
	                    -0.6, -0.9, -2,

	                    -0.8, 0.9, 2,
                        -0.8, 0.7, 2,
	                    -0.6, 0.7, 2,

                        1, 1, 0.9,
	                    -1, -1, -0.9,
                        1, -1, 0.9 };

	float color_array[] = { 1.0, 0.0, 0.0, 0.0,
	                        1.0, 0.0, 0.0, 0.0,
	                        1.0, 0.0, 0.0, 0.0,

	                        0.0, 1.0, 0.0, 0.0,
	                        0.0, 1.0, 0.0, 0.0,
	                        0.0, 1.0, 0.0, 0.0,

	                        0.0, 1.0, 0.0, 0.0,
	                        0.0, 1.0, 0.0, 0.0,
	                        0.0, 1.0, 0.0, 0.0,

	                        0.0, 0.0, 1.0, 0.0,
                            0.0, 0.0, 1.0, 0.0,
	                        0.0, 0.0, 1.0, 0.0 };

	mat4 identity;
	My_Uniforms the_uniforms;

	make_orthographic_matrix(identity, -1, 2, -1, 0.9, -0.3, 3);
	//make_orthographic_matrix(identity, -0.8, -0.6, -0.7, -0.9, -2.1, -1.9);

	Buffer triangle(1);
	triangle.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*9*NUM_TRIANGLES, points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	Buffer colors(1);
	colors.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*12*NUM_TRIANGLES, color_array, GL_STATIC_DRAW);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glEnable(GL_DEPTH_TEST);
	depth_test = 1;
	//glClearDepth(0);
	//glDepthFunc(GL_GREATER);

	//There's no reason to use a vec4 and vary the alpha in this case.  It'll just make
	//it marginally slower.  I can't think of very many cases where'd you want that effect.
	//In any case I don't support alpha/blending yet and I'm not sure what SDL will do or what
	//options it has.  TODO finish alpha blending and look into SDL's behavior/options
	//Note, I can't use SDL2 for my blending because my library has'd to be self contained
	//and fulfill the specs on it's own but I should know what SDL2 can do anyway
	GLuint myshader = pglCreateProgram(smooth_vs, smooth_fs, 4, smooth, GL_TRUE);
	glUseProgram(myshader);

	pglSetUniform(&the_uniforms);

	the_uniforms.mvp_mat = identity;



	SDL_Event e;
	bool quit = false;

	unsigned int old_time = 0, new_time=0, counter = 0;
	unsigned int frame_cap_old = 0, frame_cap_new = 0, last_frame = 0;
	float frame_time = 0;

	while (!quit) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT)
				quit = true;
			if (e.type == SDL_KEYDOWN)
				switch (e.key.keysym.sym) {
				case SDLK_d:
					if (depth_test)
						glDisable(GL_DEPTH_TEST);
					else
						glEnable(GL_DEPTH_TEST);
					depth_test = !depth_test;
					break;
				default:
					quit = true;
				}
			if (e.type == SDL_MOUSEBUTTONDOWN)
				quit = true;
		}

		new_time = SDL_GetTicks();
		frame_time = (new_time - last_frame)/1000.0f;
		last_frame = new_time;

		counter++;
		if (!(counter % 300)) {
			printf("%d  %f FPS\n", new_time-old_time, 300000/((float)(new_time-old_time)));
			old_time = new_time;
			counter = 0;
		}


		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, 3*NUM_TRIANGLES);

		//Render the scene
		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));

		//SDL_RenderClear(ren); //necessary?
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}

	cleanup();

	return 0;
}

void smooth_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	((vec4*)vs_output)[0] = ((vec4*)vertex_attribs)[4]; //color

	*(vec4*)&builtins->gl_Position = *((mat4*)uniforms) * ((vec4*)vertex_attribs)[0];
}

void smooth_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	//builtins->gl_FragDepth = 1.0 - builtins->gl_FragCoord.z;
	*(vec4*)&builtins->gl_FragColor = ((vec4*)fs_input)[0];
}


void setup_context()
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO)) {
		cout << "SDL_Init error: " << SDL_GetError() << "\n";
		exit(0);
	}

	window = SDL_CreateWindow("zbuf_learn", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!window) {
		cerr << "Failed to create window\n";
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000)) {
		puts("Failed to initialize glContext");
		exit(0);
	}
	set_glContext(&the_Context);
}

void cleanup()
{
	free_glContext(&the_Context);

	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(window);

	SDL_Quit();
}

