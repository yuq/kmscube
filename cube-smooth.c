/*
 * Copyright (c) 2017 Rob Clark <rclark@redhat.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "common.h"
#include "esUtil.h"


struct {
	struct egl egl;
	GLuint program;
	struct gbm *gbm;
} gl;


GLuint LoadShader(const char *name, GLenum type)
{
	FILE *f;
	int size;
	char *buff;
	GLuint shader;
	GLint compiled;
	const GLchar *source[1];

	assert((f = fopen(name, "r")) != NULL);

	// get file size
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);

	assert((buff = malloc(size)) != NULL);
	assert(fread(buff, 1, size, f) == size);
	source[0] = buff;
	fclose(f);
	shader = glCreateShader(type);
	glShaderSource(shader, 1, source, &size);
	glCompileShader(shader);
	free(buff);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		GLint infoLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char *infoLog = malloc(infoLen);
			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			fprintf(stderr, "Error compiling shader %s:\n%s\n", name, infoLog);
			free(infoLog);
		}
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint InitGLES(const char *vert, const char *frag)
{
	GLint linked;
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint program;
	assert((vertexShader = LoadShader(vert, GL_VERTEX_SHADER)) != 0);
	assert((fragmentShader = LoadShader(frag, GL_FRAGMENT_SHADER)) != 0);
	assert((program = glCreateProgram()) != 0);
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
		GLint infoLen = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char *infoLog = malloc(infoLen);
			glGetProgramInfoLog(program, infoLen, NULL, infoLog);
			fprintf(stderr, "Error linking program:\n%s\n", infoLog);
			free(infoLog);
		}
		glDeleteProgram(program);
		exit(1);
	}

	return program;
}

void CheckFrameBufferStatus(void)
{
  GLenum status;
  status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  switch(status) {
  case GL_FRAMEBUFFER_COMPLETE:
    printf("Framebuffer complete\n");
    break;
  case GL_FRAMEBUFFER_UNSUPPORTED:
    printf("Framebuffer unsuported\n");
    break;
  case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
    printf("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n");
    break;
  case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
    printf("GL_FRAMEBUFFER_MISSING_ATTACHMENT\n");
    break;
  case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
    printf("GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS\n");
    break;
  default:
    printf("Framebuffer error\n");
  }
}

static void draw_cube_smooth(unsigned i)
{
  GLfloat vertex[][9] = {
    {
      -1, -1, 0,
      -1, 1, 0,
      1, 1, 0,
    },
    {
      -1, -1, 0,
      -1, 1, 0,
      1, -1, 0,
    },
    {
      1, -1, 0,
      1, 1, 0,
      -1, 1, 0,
    },
    {
      1, -1, 0,
      1, 1, 0,
      -1, -1, 0,
    },
  };

  EGLint rect[] = { gl.gbm->width/4 - 1, gl.gbm->height/4 - 1, gl.gbm->width/2 + 2, gl.gbm->height/2 + 2};
  assert(gl.egl.eglSetDamageRegionKHR(gl.egl.display, gl.egl.surface, rect, 1) == EGL_TRUE);

  if (!i) {
    //glViewport(0, 0, gl.gbm->width, gl.gbm->height);
    //glScissor(0, 0, gl.gbm->width, gl.gbm->height);

    //glClear(GL_COLOR_BUFFER_BIT);
    assert(glGetError() == GL_NO_ERROR);

    GLint position = glGetAttribLocation(gl.program, "positionIn");
    glEnableVertexAttribArray(position);
    glVertexAttribPointer(position, 3, GL_FLOAT, 0, 0, vertex[0]);
    assert(glGetError() == GL_NO_ERROR);

    glDrawArrays(GL_TRIANGLES, 0, 3);
    assert(glGetError() == GL_NO_ERROR);

    glVertexAttribPointer(position, 3, GL_FLOAT, 0, 0, vertex[1]);
    assert(glGetError() == GL_NO_ERROR);

    glDrawArrays(GL_TRIANGLES, 0, 3);
    assert(glGetError() == GL_NO_ERROR);
  }
  else {
    //glViewport(gl.gbm->width/4, gl.gbm->height/4, gl.gbm->width/2, gl.gbm->height/2);
    //glScissor(gl.gbm->width/4, gl.gbm->height/4, gl.gbm->width/2, gl.gbm->height/2);

    //glClear(GL_COLOR_BUFFER_BIT);
    assert(glGetError() == GL_NO_ERROR);

    GLint position = glGetAttribLocation(gl.program, "positionIn");
    glEnableVertexAttribArray(position);
    glVertexAttribPointer(position, 3, GL_FLOAT, 0, 0, vertex[2]);
    assert(glGetError() == GL_NO_ERROR);

    glDrawArrays(GL_TRIANGLES, 0, 3);
    assert(glGetError() == GL_NO_ERROR);

    glVertexAttribPointer(position, 3, GL_FLOAT, 0, 0, vertex[3]);
    assert(glGetError() == GL_NO_ERROR);

    glDrawArrays(GL_TRIANGLES, 0, 3);
    assert(glGetError() == GL_NO_ERROR);
  }
}

const struct egl * init_cube_smooth(const struct gbm *gbm)
{
	int ret;

	ret = init_egl(&gl.egl, gbm);
	if (ret)
		return NULL;

	gl.program = InitGLES("vert.glsl", "frag.glsl");
	glUseProgram(gl.program);

	glClearColor(0, 0, 0, 0);
	glEnable(GL_SCISSOR_TEST);

	gl.egl.draw = draw_cube_smooth;
	gl.gbm = gbm;

	return &gl.egl;
}
