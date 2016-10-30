#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif
#include <cassert>
#include <cstring>
#include <GL/glew.h>
#include "utils/shader_builder.h"
#include "utils/gl_utils.h"
#include "utils/vec.h"
#include "utils/camera.h"
#include "utils/gl_render_constants.h"
#include "utils/Image.h"

uint32_t vec4_to_uint32(const vec4& v) {

    uint32_t x = clamp(v.x, 0.0f, 1.0f) * 255.0f;
    uint32_t y = clamp(v.y, 0.0f, 1.0f) * 255.0f;
    uint32_t z = clamp(v.z, 0.0f, 1.0f) * 255.0f;
    uint32_t w = clamp(v.w, 0.0f, 1.0f) * 255.0f;

    uint32_t res = x | (y<<8) | (z<<16) | (w<<24);
    return res;
}

// maybe just 
// header: extern uint32_t TF_R8
// cpp: TF_R8 = GL_R8 
// ?
//
const GLint textureFormats[TF_COUNT] = {
    0,
    GL_RED,
    GL_RG,
    GL_RGB,
    GL_RGBA,

    GL_R,
    GL_RG,
    GL_RGB,
    GL_RGBA,
};

const GLint textureInternalFormats[TF_COUNT] = {
    0,
    GL_R8,
    GL_RG8,
    GL_RGB8,
    GL_RGBA8,

    GL_R32F,
    GL_RG32F,
    GL_RGB32F,
    GL_RGBA32F,
};


const int textureFormatNumChannels[TF_COUNT] = {
    0,
    1,
    2,
    3,
    4,
    1,
    2,
    3,
    4
};

const GLint textureFormatChannelType[TF_COUNT] = {
    0,
    GL_UNSIGNED_BYTE,
    GL_UNSIGNED_BYTE,
    GL_UNSIGNED_BYTE,
    GL_UNSIGNED_BYTE,
    GL_FLOAT,
    GL_FLOAT,
    GL_FLOAT,
    GL_FLOAT,
};

const static uint32_t textureFormatChannelSize[TF_COUNT] = {
    0,
    1,
    1,
    1,
    1,
    4,
    4,
    4,
    4,
};

uint32_t
getTexFormatPixelSize(TexFormat fmt) {

    assert(sizeof(textureFormatChannelSize)/sizeof(textureFormatChannelSize[0]) == TF_COUNT);
    assert(sizeof(textureFormatNumChannels)/sizeof(textureFormatNumChannels[0]) == TF_COUNT);

    return textureFormatChannelSize[fmt] * textureFormatNumChannels[fmt];
}

Texture create2DTexture(int w, int h, TexFormat fmt, const uint8_t* texdata)
{
    
	GLuint texID;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, textureInternalFormats[fmt], 
            w, h, 0, textureFormats[fmt], textureFormatChannelType[fmt], texdata);
    CHECK_GL_ERROR

	Texture t;
	t.id = texID;
	t.w = w;
	t.h = h;
	t.fmt_ = fmt;
    t.format = (GLenum)-1;

	return t;
}


Texture createDynamicTexture(int w, int h, TexFormat fmt)
{
    
	GLuint texID;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, textureInternalFormats[fmt], 
            w, h, 0, textureFormats[fmt], textureFormatChannelType[fmt], NULL);
    CHECK_GL_ERROR

	Texture t;
	t.id = texID;
	t.w = w;
	t.h = h;
	t.fmt_ = fmt;
    t.format = (GLenum)-1;

	return t;
}

Texture create3DTextureF(int w, int h, int depth)
{
	GLuint texID;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_3D, texID);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, w, h, depth, 0, GL_RED, GL_FLOAT, NULL);

	Texture t;
	t.id = texID;
	t.w = w;
	t.h = h;
	t.depth = depth;
	t.format = GL_RED;

	return t;
}


Texture createPBO(int w, int h, GLenum fmt, int el_size)
{
	GLuint pbo;
	glGenBuffers(1, &pbo);
	glBindBuffer(GL_ARRAY_BUFFER, pbo);
	glBufferData(GL_ARRAY_BUFFER,	w*h*4*el_size,	NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	Texture t;
	t.id = pbo;
	t.w = w;
	t.h = h;
	t.format = fmt;

	return t;
}

void
updateTexture(const Texture& t , char* pdata) {

    glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, t.id);
    if(t.fmt_ != TF_NONE) {
        // TODO: keep this all in platform dependent data of texture
        GLint int_fmt = textureInternalFormats[t.fmt_];
        GLenum fmt = textureFormats[t.fmt_];
        GLenum ch_type = textureFormatChannelType[t.fmt_];
	    glTexImage2D(GL_TEXTURE_2D, 0, int_fmt, t.w, t.h, 0, fmt, ch_type, pdata);
	    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t.w, t.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pdata);
        //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, t.w, t.h, GL_RGBA, GL_UNSIGNED_BYTE, pdata);
    } else {
#pragma message("Deprecated!!!")
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t.w, t.h, 0, t.format, GL_UNSIGNED_BYTE, pdata);
    }
	glBindTexture(GL_TEXTURE_2D, 0);
    CHECK_GL_ERROR
}

void draw_quad(float x0, float y0, float x1, float y1)
{
	glBegin(GL_QUADS);
	
	glTexCoord2f(0,0);
	glVertex2f(x0,y0);
		
	glTexCoord2f(1,0);
	glVertex2f(x1, y0);
		
	glTexCoord2f(1,1);
	glVertex2f(x1, y1);
		
	glTexCoord2f(0,1);
	glVertex2f(x0, y1);

	glEnd();
}


void applyTexture(glsl_program* program, int unit, const char* name, GLuint texid)
{
    assert(program->samplers_.count( name ));

	if(program->samplers_.count( name ))
	{
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, texid);
		glUniform1i(program->samplers_[ name ]->index_, unit);
        CHECK_GL_ERROR
	}
}

void applyPBO(glsl_program* program, int unit, const char* name, const Texture pbo, const Texture tex)
{
	if(program->samplers_.count( name ))
	{
		assert(pbo.h==tex.h && pbo.w==tex.w && pbo.format==tex.format);
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, tex.id);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo.id);
		glTexImage2D(GL_TEXTURE_2D, 0, pbo.format, pbo.w, pbo.h, 0, pbo.format, GL_UNSIGNED_BYTE, 0);
		glUniform1i(program->samplers_[ name ]->index_, unit);
	}
}

void draw_in_2d(int w, int h, render_func_t prenderfunc, void* puserdata)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
    mat4 ortho_proj = orthoMatrix(0, w, h, 0, -1.0f, 1.0f, false);
	//gluOrtho2D(0, w, 0, h);
    glLoadTransposeMatrixf((const float*)ortho_proj);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	prenderfunc(w, h, puserdata);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}


GLuint makeBuffer(GLenum target, const GLvoid* buffer_data, GLsizei buffer_size, GLenum type)
{
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(target, buffer);
	glBufferData(target, buffer_size, buffer_data, type);
	glBindBuffer(target, 0);
	return buffer;
}

void updateBuffer(GLuint buf, GLenum target, const GLvoid* buffer_data, GLsizei buffer_size)
{
	assert(buf && buffer_data);
	glBindBuffer(target, buf);
	glBufferSubData(target, 0, buffer_size, buffer_data);
	glBindBuffer(target, 0);
}


void updateBuffer(GLuint buf, GLenum target, const GLvoid* buffer_data, GLsizei buffer_size, GLenum type)
{
	assert(buf && buffer_data);
	glBindBuffer(target, buf);
	glBufferData(target, buffer_size, buffer_data, type);
	glBindBuffer(target, 0);
}



///////////////////////////////////////////////////////////////////////////////
// draw a textured cube with GL_TRIANGLES
///////////////////////////////////////////////////////////////////////////////
void draw_textured_cube(GLuint textureId)
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureId);

	glColor4f(1, 1, 1, 1);
	glBegin(GL_TRIANGLES);
	// front faces
	glNormal3f(0,0,1);
	// face v0-v1-v2
	glTexCoord2f(1,1);  glVertex3f(1,1,1);
	glTexCoord2f(0,1);  glVertex3f(-1,1,1);
	glTexCoord2f(0,0);  glVertex3f(-1,-1,1);
	// face v2-v3-v0
	glTexCoord2f(0,0);  glVertex3f(-1,-1,1);
	glTexCoord2f(1,0);  glVertex3f(1,-1,1);
	glTexCoord2f(1,1);  glVertex3f(1,1,1);

	// right faces
	glNormal3f(1,0,0);
	// face v0-v3-v4
	glTexCoord2f(0,1);  glVertex3f(1,1,1);
	glTexCoord2f(0,0);  glVertex3f(1,-1,1);
	glTexCoord2f(1,0);  glVertex3f(1,-1,-1);
	// face v4-v5-v0
	glTexCoord2f(1,0);  glVertex3f(1,-1,-1);
	glTexCoord2f(1,1);  glVertex3f(1,1,-1);
	glTexCoord2f(0,1);  glVertex3f(1,1,1);

	// top faces
	glNormal3f(0,1,0);
	// face v0-v5-v6
	glTexCoord2f(1,0);  glVertex3f(1,1,1);
	glTexCoord2f(1,1);  glVertex3f(1,1,-1);
	glTexCoord2f(0,1);  glVertex3f(-1,1,-1);
	// face v6-v1-v0
	glTexCoord2f(0,1);  glVertex3f(-1,1,-1);
	glTexCoord2f(0,0);  glVertex3f(-1,1,1);
	glTexCoord2f(1,0);  glVertex3f(1,1,1);

	// left faces
	glNormal3f(-1,0,0);
	// face  v1-v6-v7
	glTexCoord2f(1,1);  glVertex3f(-1,1,1);
	glTexCoord2f(0,1);  glVertex3f(-1,1,-1);
	glTexCoord2f(0,0);  glVertex3f(-1,-1,-1);
	// face v7-v2-v1
	glTexCoord2f(0,0);  glVertex3f(-1,-1,-1);
	glTexCoord2f(1,0);  glVertex3f(-1,-1,1);
	glTexCoord2f(1,1);  glVertex3f(-1,1,1);

	// bottom faces
	glNormal3f(0,-1,0);
	// face v7-v4-v3
	glTexCoord2f(0,0);  glVertex3f(-1,-1,-1);
	glTexCoord2f(1,0);  glVertex3f(1,-1,-1);
	glTexCoord2f(1,1);  glVertex3f(1,-1,1);
	// face v3-v2-v7
	glTexCoord2f(1,1);  glVertex3f(1,-1,1);
	glTexCoord2f(0,1);  glVertex3f(-1,-1,1);
	glTexCoord2f(0,0);  glVertex3f(-1,-1,-1);

	// back faces
	glNormal3f(0,0,-1);
	// face v4-v7-v6
	glTexCoord2f(0,0);  glVertex3f(1,-1,-1);
	glTexCoord2f(1,0);  glVertex3f(-1,-1,-1);
	glTexCoord2f(1,1);  glVertex3f(-1,1,-1);
	// face v6-v5-v4
	glTexCoord2f(1,1);  glVertex3f(-1,1,-1);
	glTexCoord2f(0,1);  glVertex3f(1,1,-1);
	glTexCoord2f(0,0);  glVertex3f(1,-1,-1);
	glEnd();

	if(textureId)
		glBindTexture(GL_TEXTURE_2D, 0);
}


Texture load_texture_from_file(const char* texName)
{
	Texture t;

	Image* fontTexture_ = new Image();
	if( false == fontTexture_->loadFromFile(texName) )
		return t;
	
	// support only 32bit textures :P

	int bpp = getBytesPerPixel(fontTexture_->getFormat());
	assert(bpp == 3 || bpp == 4);
	GLenum format = bpp==3 ? GL_RGB : GL_RGBA;

	glGenTextures(1, &t.id);
	glBindTexture(GL_TEXTURE_2D, t.id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, format, fontTexture_->getWidth(), 
		fontTexture_->getHeight(), 0, format, GL_UNSIGNED_BYTE, 
		fontTexture_->getPixels());


	t.w = fontTexture_->getWidth();
	t.h = fontTexture_->getHeight();
	t.format = format;
	t.depth = 1;

	glBindTexture(GL_TEXTURE_2D, 0);
	
	delete fontTexture_;

	return t;
}

/*
glMesh<SIMPLE_VERTEX_PTN>* make_mesh_from_file(const char* filepath)
{
	assert(filepath);
	COMMON_MESH* pcomesh = load_mesh_from_binary(filepath);
	assert(pcomesh);

	if(pcomesh->va.stride != sizeof(SIMPLE_VERTEX_PTN))
		return 0;
	if(pcomesh->vd.decl.size() !=3)
		return 0;
	if(pcomesh->vd.decl[0].comp_type!=TYPE_FLOAT || pcomesh->vd.decl[0].num_comp!=3)
		return 0;
	if(pcomesh->vd.decl[1].comp_type!=TYPE_FLOAT || pcomesh->vd.decl[1].num_comp!=2)
		return 0;
	if(pcomesh->vd.decl[2].comp_type!=TYPE_FLOAT || pcomesh->vd.decl[2].num_comp!=3)
		return 0;
	
	glMesh<SIMPLE_VERTEX_PTN>* pmesh = new glMesh<SIMPLE_VERTEX_PTN>();
	pmesh->num_indices_ = 0;
	pmesh->num_vertices_ = pcomesh->va.num_vertices;
	pmesh->prim_type_ = GL_TRIANGLES;
	pmesh->pvertices_ = new SIMPLE_VERTEX_PTN[pcomesh->va.num_vertices];
	memcpy(pmesh->pvertices_, pcomesh->va.data, pcomesh->va.num_vertices*sizeof(SIMPLE_VERTEX_PTN));

	delete pcomesh;

	pmesh->gen_hw(GL_STATIC_DRAW);
	return pmesh;
}
*/