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
	GLuint program1;
	GLuint program2;
	GLuint fbid;
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

	glBindFramebuffer(GL_FRAMEBUFFER, gl.fbid);
	glUseProgram(gl.program1);

	GLfloat vertex[] = {
		-1, -1, 0,
		-1, 1, 0,
		1, 1, 0,
		1, -1, 0
	};

	GLint position = glGetAttribLocation(gl.program1, "positionIn");
	glEnableVertexAttribArray(position);
	glVertexAttribPointer(position, 3, GL_FLOAT, 0, 0, vertex);
	assert(glGetError() == GL_NO_ERROR);

	glClear(GL_COLOR_BUFFER_BIT);
	assert(glGetError() == GL_NO_ERROR);

	glDrawArrays(GL_TRIANGLES, 0, 3);
	assert(glGetError() == GL_NO_ERROR);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(gl.program2);

        glClear(GL_COLOR_BUFFER_BIT);

	GLfloat tex[] = {
		1, 1,
		1, 0,
		0, 1,
		0, 0,
	};

	position = glGetAttribLocation(gl.program2, "positionIn");
	glEnableVertexAttribArray(position);
	glVertexAttribPointer(position, 3, GL_FLOAT, 0, 0, vertex);
	assert(glGetError() == GL_NO_ERROR);

	GLint texIn = glGetAttribLocation(gl.program2, "texIn");
	glEnableVertexAttribArray(texIn);
	glVertexAttribPointer(texIn, 2, GL_FLOAT, 0, 0, tex);
	assert(glGetError() == GL_NO_ERROR);

	GLint sample = glGetUniformLocation(gl.program2, "tex");
	glUniform1i(sample, 0);
	assert(glGetError() == GL_NO_ERROR);

	glDrawArrays(GL_TRIANGLES, 0, 3);
	assert(glGetError() == GL_NO_ERROR);
}

static inline int
align(int value, int alignment)
{
   return (value + alignment - 1) & ~(alignment - 1);
}

const struct egl * init_cube_smooth(const struct gbm *gbm)
{
	int ret;

	ret = init_egl(&gl.egl, gbm);
	if (ret)
		return NULL;

	gl.program1 = InitGLES("vert.glsl", "frag.glsl");
	gl.program2 = InitGLES("vert-tex.glsl", "frag-tex.glsl");

	glClearColor(0, 0, 0, 0);
	glViewport(0, 0, gbm->width, gbm->height);

	int drm_fd = gbm_device_get_fd(gbm->dev);
	struct drm_mode_create_dumb arg = {
	  .bpp = 32,
	  .width = align(gbm->width, 16),
	  .height = align(gbm->height, 16),
	};
	assert(!drmIoctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &arg));

	int dma_fd;
	assert(!drmPrimeHandleToFD(drm_fd, arg.handle, 0, &dma_fd));

	printf("pitch = %d fd = %d\n", arg.pitch, dma_fd);

	EGLint attrib_list[] = {
	  EGL_WIDTH, gbm->width,
	  EGL_HEIGHT, gbm->height,
	  EGL_LINUX_DRM_FOURCC_EXT, DRM_FORMAT_RGBA8888,
	  EGL_DMA_BUF_PLANE0_FD_EXT, dma_fd,
	  EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
	  EGL_DMA_BUF_PLANE0_PITCH_EXT, arg.pitch,
	  EGL_NONE
	};
	EGLImageKHR image =
	  gl.egl.eglCreateImageKHR(
		gl.egl.display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT,
		NULL, attrib_list);
	printf("egl error %x\n", eglGetError());
	assert(image != EGL_NO_IMAGE_KHR);

	/*
	struct gbm_bo *bo = gbm_bo_create(
		gbm->dev, gbm->width, gbm->height, 
		GBM_FORMAT_XRGB8888, 0);
		//GBM_BO_USE_LINEAR |
		//GBM_BO_USE_RENDERING |
		//GBM_BO_USE_SCANOUT);
	assert(bo);
	printf("gbm bo width/stride %d/%d\n",
	       gbm_bo_get_width(bo),
	       gbm_bo_get_stride(bo));
	
	EGLImageKHR image = gl.egl.eglCreateImageKHR(
		gl.egl.display, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, bo, NULL);
	assert(image != EGL_NO_IMAGE_KHR);
	*/

	glActiveTexture(GL_TEXTURE0);

	GLuint texid;
	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_2D, texid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl.egl.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);

	GLuint fbid;
	glGenFramebuffers(1, &fbid);
	glBindFramebuffer(GL_FRAMEBUFFER, fbid);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texid, 0);

	CheckFrameBufferStatus();

	gl.fbid = fbid;
	gl.egl.draw = draw_cube_smooth;

	return &gl.egl;
}
