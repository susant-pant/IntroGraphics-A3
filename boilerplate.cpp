// ==========================================================================
// Barebones OpenGL Core Profile Boilerplate
//    using the GLFW windowing system (http://www.glfw.org)
//
// Loosely based on
//  - Chris Wellons' example (https://github.com/skeeto/opengl-demo) and
//  - Camilla Berglund's example (http://www.glfw.org/docs/latest/quick.html)
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <iterator>
#include "GlyphExtractor.h"

// Specify that we want the OpenGL core profile before including GLFW headers
#ifndef LAB_LINUX
	#include <glad/glad.h>
#else
	#define GLFW_INCLUDE_GLCOREARB
	#define GL_GLEXT_PROTOTYPES
#endif
#include <GLFW/glfw3.h>

//STB
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
using namespace std;

int bezierType = 0;

// --------------------------------------------------------------------------
// OpenGL utility and support function prototypes

void QueryGLVersion();
bool CheckGLErrors();

string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint TCSshader, GLuint TESshader, GLuint fragmentShader); 

// --------------------------------------------------------------------------
// Functions to set up OpenGL shader programs for rendering

struct MyShader
{
	// OpenGL names for vertex and fragment shaders, shader program
	GLuint  vertex;
	GLuint  TCS; 
	GLuint  TES; 
	GLuint  fragment;
	GLuint  program;

	// initialize shader and program names to zero (OpenGL reserved value)
	MyShader() : vertex(0), fragment(0), program(0)
	{}
};

MyShader shader;

// load, compile, and link shaders, returning true if successful
bool InitializeShaders(MyShader *shader)
{
	// load shader source from files
	string vertexSource = LoadSource("vertex.glsl");
	string fragmentSource = LoadSource("fragment.glsl");
	string TCSSource = LoadSource("tessControl.glsl");
	string TESSource = LoadSource("tessEval.glsl"); 
	if (vertexSource.empty() || fragmentSource.empty()) return false;

	// compile shader source into shader objects
	shader->vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
	shader->fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource); 
	shader->TCS = CompileShader(GL_TESS_CONTROL_SHADER, TCSSource);
	shader->TES = CompileShader(GL_TESS_EVALUATION_SHADER, TESSource);

	// link shader program
	shader->program = LinkProgram(shader->vertex, shader->TCS, shader->TES, shader->fragment);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate shader-related objects
void DestroyShaders(MyShader *shader)
{
	// unbind any shader programs and destroy shader objects
	glUseProgram(0);
	glDeleteProgram(shader->program);
	glDeleteShader(shader->vertex);
	glDeleteShader(shader->fragment);
	glDeleteShader(shader->TCS); 
	glDeleteShader(shader->TES); 
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing geometry data

struct MyGeometry
{
	// OpenGL names for array buffer objects, vertex array object
	GLuint  vertexBuffer;
	GLuint  textureBuffer;
	GLuint  colourBuffer;
	GLuint  vertexArray;
	GLsizei elementCount;

	// initialize object names to zero (OpenGL reserved value)
	MyGeometry() : vertexBuffer(0), colourBuffer(0), vertexArray(0), elementCount(0)
	{}
};

MyGeometry geomPoints;
MyGeometry geomLines;
MyGeometry geomQuad;
MyGeometry geomCubic;

// create buffers and fill with geometry data, returning true if successful
bool InitializeGeometry(MyGeometry *geometry, GLfloat *points, GLfloat *cols, int elemCount, float scale, float transform){
	GLfloat vertices[elemCount][2];
	GLfloat colours[elemCount][3];
	
	for(int i = 0; i < elemCount; i++){
		vertices[i][0] = (points[2*i] + transform) / scale;
		vertices[i][1] = (points[2*i + 1] + transform) / scale;
		
		colours[i][0] = cols[3*i];
		colours[i][1] = cols[3*i + 1];
		colours[i][2] = cols[3*i + 2];
	}
	geometry->elementCount = elemCount; 

	// these vertex attribute indices correspond to those specified for the
	// input variables in the vertex shader
	const GLuint VERTEX_INDEX = 0;
	const GLuint COLOUR_INDEX = 1;

	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometry->vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// create another one for storing our colours
	glGenBuffers(1, &geometry->colourBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colours), colours, GL_STATIC_DRAW);

	// create a vertex array object encapsulating all our vertex attributes
	glGenVertexArrays(1, &geometry->vertexArray);
	glBindVertexArray(geometry->vertexArray);

	// associate the position array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glVertexAttribPointer(VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(VERTEX_INDEX);

	// assocaite the colour array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glVertexAttribPointer(COLOUR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(COLOUR_INDEX);

	// unbind our buffers, resetting to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate geometry-related objects
void DestroyGeometry(MyGeometry *geometry)
{
	// unbind and destroy our vertex array object and associated buffers
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &geometry->vertexArray);
	glDeleteBuffers(1, &geometry->vertexBuffer);
	glDeleteBuffers(1, &geometry->colourBuffer);
}

bool printPoints = false;
bool printLinear = false;
bool printQuad = false;
bool printCubic = false;

void renderArray(MyGeometry *geometry, MyShader *shader){
	glUseProgram(shader->program);
	GLint loc = glGetUniformLocation(shader->program, "bezierType");
	if (loc != -1)
		glUniform1i(loc, bezierType);
	glBindVertexArray(geometry->vertexArray);
	glDrawArrays(GL_PATCHES, 0, geometry->elementCount);
	glBindVertexArray(0);
	glUseProgram(0);
}

// --------------------------------------------------------------------------
// Rendering function that draws our scene to the frame buffer

void RenderScene(MyShader *shader)
{
	// clear screen to a dark grey colour
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);

	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	// reset state to default (no shader or geometry bound)
	
	if(printLinear) {
		bezierType = 1;
		renderArray(&geomLines, shader);
	}
	if(printQuad) {
		bezierType = 2;
		renderArray(&geomQuad, shader);
	}
	if(printCubic) {
		bezierType = 3;
		renderArray(&geomCubic, shader);
	}
	if(printPoints) {
		bezierType = 0;
		renderArray(&geomPoints, shader);
	}
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}

// --------------------------------------------------------------------------
// GLFW callback functions

GlyphExtractor extractor;
MyGlyph glyph;
MyContour contour;
MySegment segment;

bool scroll = false;
bool awesome = false;
bool text = false;
float scrollFactor = 0.f;
float scrollSpeed = 3.f;
float scrollBound = 0.f;

int lineCount = 0;
int quadraticCount = 0;
int cubicCount = 0;

string fox = "The Quick Brown Fox Jumps Over the Lazy Dog.";
string name = "SUSANT";

void glyphCount() {
	for (unsigned cont = 0; cont < glyph.contours.size(); cont++){
		contour = glyph.contours[cont];
		for (unsigned seg = 0; seg < contour.size(); seg++){
			segment = contour[seg];
			if (segment.degree == 1)
				lineCount += 8;
			else if (segment.degree == 2)
				quadraticCount += 8;
			else if (segment.degree == 3)
				cubicCount += 8;
		}
	}
}

void glyphToGeom(GLfloat *verLines, GLfloat *verQuad, GLfloat *verCub, float scale, float xTrans, float yTrans){
	for (unsigned cont = 0; cont < glyph.contours.size(); cont++){
		contour = glyph.contours[cont];
		for (unsigned seg = 0; seg < contour.size(); seg++){
			segment = contour[seg];
			if (segment.degree == 1){
				verLines[lineCount] = (segment.x[0] + xTrans) * scale; verLines[lineCount + 1] = (segment.y[0] + yTrans) * scale;
				verLines[lineCount + 2] = (segment.x[1] + xTrans) * scale; verLines[lineCount + 3] = (segment.y[1] + yTrans) * scale;
				verLines[lineCount + 4] = (0.f + xTrans) * scale; verLines[lineCount + 5] = (0.f + yTrans) * scale;
				verLines[lineCount + 6] = (0.f + xTrans) * scale; verLines[lineCount + 7] = (0.f + yTrans) * scale;
				lineCount += 8;
			}
			else if (segment.degree == 2){
				verQuad[quadraticCount] = (segment.x[0] + xTrans) * scale; verQuad[quadraticCount + 1] = (segment.y[0] + yTrans) * scale;
				verQuad[quadraticCount + 2] = (segment.x[1] + xTrans) * scale; verQuad[quadraticCount + 3] = (segment.y[1] + yTrans) * scale;
				verQuad[quadraticCount + 4] = (segment.x[2] + xTrans) * scale; verQuad[quadraticCount + 5] = (segment.y[2] + yTrans) * scale;
				verQuad[quadraticCount + 6] = (0.f + xTrans) * scale; verQuad[quadraticCount + 7] = (0.f + yTrans) * scale;
				quadraticCount += 8;
			}
			else if (segment.degree == 3){
				verCub[cubicCount] = (segment.x[0] + xTrans) * scale; verCub[cubicCount + 1] = (segment.y[0] + yTrans) * scale;
				verCub[cubicCount + 2] = (segment.x[1] + xTrans) * scale; verCub[cubicCount + 3] = (segment.y[1] + yTrans) * scale;
				verCub[cubicCount + 4] = (segment.x[2] + xTrans) * scale; verCub[cubicCount + 5] = (segment.y[2] + yTrans) * scale;
				verCub[cubicCount + 6] = (segment.x[3] + xTrans) * scale; verCub[cubicCount + 7] = (segment.y[3] + yTrans) * scale;
				cubicCount += 8;
			}
		}
	}
}

// reports GLFW errors
void ErrorCallback(int error, const char* description)
{
	cout << "GLFW ERROR " << error << ":" << endl;
	cout << description << endl;
}

// handles keyboard input events
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
		printPoints = true;
		printLinear = true;
		printQuad = true;
		printCubic = false;
		
		float transform = 0.f;
		float scale = 3.f;
		int elements = 16;
		
		scroll = false;
		awesome = false;
		text = false;
		
		glUseProgram(shader.program);
		GLint loc = glGetUniformLocation(shader.program, "text");
		if (loc != -1)
			glUniform1i(loc, text);
		
		float verArrayQuad[elements*2] = {
			1.f, 1.f, 2.f, -1.f, 0.f, -1.f, 0.f, 0.f,
			0.f, -1.f, -2.f, -1.f, -1.f, 1.f, 0.f, 0.f,
			-1.f, 1.f, 0.f, 1.f, 1.f, 1.f, 0.f, 0.f,
			1.2f, 0.5f, 2.5f, 1.f, 1.3f, -0.4f, 0.f, 0.f
		};
		float verArrayLines[elements*4];
		int i = 0;
		int j = 0;
		while (i < elements * 2){
			verArrayLines[j] = verArrayQuad[i];		j++;
			verArrayLines[j] = verArrayQuad[i+1];	j++;
			verArrayLines[j] = verArrayQuad[i+2];	j++;
			verArrayLines[j] = verArrayQuad[i+3];	j++;
			for (int temp = 0; temp < 4; temp++){
				verArrayLines[j] = 0.f; j++;
			}
			i+=2;
			if (i%4 == 0)
				i+=4;
		}
		
		float verArrayPoints[elements * 8];
		j = 0;
		int counter = 0;
		for (int i = 0; i < (elements*2)-1; i++){
			float pointX = verArrayQuad[i];		i++;
			float pointY = verArrayQuad[i];
			verArrayPoints[j] = pointX - 0.1f; 	j++;
			verArrayPoints[j] = pointY + 0.1f;	j++;
			verArrayPoints[j] = pointX + 0.1f;	j++;
			verArrayPoints[j] = pointY - 0.05f;	j++;
			verArrayPoints[j] = pointX - 0.1f;	j++;
			verArrayPoints[j] = pointY - 0.05f;	j++;
			verArrayPoints[j] = pointX + 0.1f;	j++;
			verArrayPoints[j] = pointY + 0.1f;	j++;
			counter += 2;
			if ((counter % 6) == 0){
				counter = 0;
				i += 2;
			}
		}
		
		float colsQuad[elements * 12]= {
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
		};
			
		if (!InitializeGeometry(&geomQuad, verArrayQuad, colsQuad, elements, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;
		
		if (!InitializeGeometry(&geomPoints, verArrayPoints, colsQuad, elements*4, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;
		
		if (!InitializeGeometry(&geomLines, verArrayLines, colsQuad, elements*2, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;
	}
	
	if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
		printPoints = true;
		printLinear = true;
		printQuad = false;
		printCubic = true;
		
		float transform = -3.f;
		float scale = 7.f;
		int elements = 28;
		
		scroll = false;
		awesome = false;
		text = false;
		
		glUseProgram(shader.program);
		GLint loc = glGetUniformLocation(shader.program, "text");
		if (loc != -1)
			glUniform1i(loc, text);

		float verArrayCubic[elements*2] = {
			1.f, 1.f, 4.f, 0.f, 6.f, 2.f, 9.f, 1.f,
			8.f, 2.f, 0.f, 8.f, 0.f, -2.f, 8.f, 4.f,
			8.f, 2.f, 7.5f, 2.5f, 7.5f, 3.5f, 8.f, 4.f,
			2.8f, 3.5f, 2.4f, 3.8f, 2.4f, 3.2f, 2.8f, 3.5f,
			3.f, 2.2f, 3.5f, 2.7f, 3.5f, 3.3f, 3.f, 3.8f,
			5.f, 3.f, 3.f, 2.f, 3.f, 3.f, 5.f, 2.f,
			5.f, 3.f, 5.3f, 2.8f, 5.3f, 2.2f, 5.f, 2.f,
		};
		
		float verArrayLines[elements*6];
		int counter = 0;
		int i = 0;
		int j = 0;
		while (i < elements * 2){
			verArrayLines[j] = verArrayCubic[i];	j++;
			verArrayLines[j] = verArrayCubic[i+1];	j++;
			verArrayLines[j] = verArrayCubic[i+2];	j++;
			verArrayLines[j] = verArrayCubic[i+3];	j++;
			for (int temp = 0; temp < 4; temp++){
				verArrayLines[j] = 0.f; j++;
			}
			counter += 2;
			i += 2;
			if (counter%6 == 0){
				counter = 0;
				i+=2;
			}
		}
		
		float verArrayPoints[elements*8];
		j = 0;
		for (int i = 0; i < (elements*2); i++){
			float pointX = verArrayCubic[i];	i++;
			float pointY = verArrayCubic[i];
			verArrayPoints[j] = pointX - 0.1f; 	j++;
			verArrayPoints[j] = pointY + 0.1f;	j++;
			verArrayPoints[j] = pointX + 0.1f;	j++;
			verArrayPoints[j] = pointY - 0.1f;	j++;
			verArrayPoints[j] = pointX - 0.1f;	j++;
			verArrayPoints[j] = pointY - 0.1f;	j++;
			verArrayPoints[j] = pointX + 0.1f;	j++;
			verArrayPoints[j] = pointY + 0.1f;	j++;
		}
		
		float cols[elements*12] = {
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
			0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
			0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
			0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
			0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
			0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
			0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
			0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
			0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
			1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f
		};
		
		if (!InitializeGeometry(&geomCubic, verArrayCubic, cols, elements, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;

		if (!InitializeGeometry(&geomPoints, verArrayPoints, cols, elements*4, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;
		
		if (!InitializeGeometry(&geomLines, verArrayLines, cols, elements*3, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;
	}
	
	if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
		printPoints = false;printLinear = true; printQuad = true; printCubic = false;
		float transform = 0.f; float scale = 1.f;
		scroll = false;
		awesome = false;
		text = true;
		
		glUseProgram(shader.program);
		GLint loc = glGetUniformLocation(shader.program, "text");
		if (loc != -1)
			glUniform1i(loc, text);
			
		extractor.LoadFontFile("Fonts/Lora-Italic.ttf");
		for (unsigned i = 0; i < name.size(); i++){
			glyph = extractor.ExtractGlyph(name[i]);
			glyphCount();
		}
		float verArrayLines[lineCount];
		float verArrayQuad[quadraticCount];
		float verArrayCub[cubicCount];
		float adv = 0.f;
		lineCount = 0;
		quadraticCount = 0;
		cubicCount = 0;
		
		for (unsigned i = 0; i < name.size(); i++){
			glyph = extractor.ExtractGlyph(name[i]);
			glyphToGeom(verArrayLines, verArrayQuad, verArrayCub, 0.55f, -1.8f + adv, -0.39f);
			adv += glyph.advance - 0.08f;
		}
		
		float cols[quadraticCount * 3 / 2];
		for (int i = 0; i < quadraticCount * 3 / 2; i++){
			cols[i] = 1.f;
		}
		
		if (!InitializeGeometry(&geomLines, verArrayLines, cols, lineCount / 2, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;
		if (!InitializeGeometry(&geomQuad, verArrayQuad, cols, quadraticCount / 2, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;
	}
	
	if (key == GLFW_KEY_5 && action == GLFW_PRESS) {
		printPoints = false; printLinear = true; printQuad = true; printCubic = false;
		float transform = 0.f; float scale = 1.f;
		scroll = false;
		awesome = false;
		text = true;
		
		glUseProgram(shader.program);
		GLint loc = glGetUniformLocation(shader.program, "text");
		if (loc != -1)
			glUniform1i(loc, text);
			
		extractor.LoadFontFile("Fonts/Comic_Sans.ttf");
		for (unsigned i = 0; i < name.size(); i++){
			glyph = extractor.ExtractGlyph(name[i]);
			glyphCount();
		}
		
		float verArrayLines[lineCount];
		float verArrayQuad[quadraticCount];
		float verArrayCub[cubicCount];
		float adv = 0.f;
		lineCount = 0;
		quadraticCount = 0;
		cubicCount = 0;
		
		for (unsigned i = 0; i < name.size(); i++){
			glyph = extractor.ExtractGlyph(name[i]);
			glyphToGeom(verArrayLines, verArrayQuad, verArrayCub, 0.5f, -2.f + adv, -0.39f);
			adv += glyph.advance - 0.08f;
		}
		
		float cols[quadraticCount * 3 / 2];
		for (int i = 0; i < quadraticCount * 3 / 2; i++){
			cols[i] = 1.f;
		}
		
		if (!InitializeGeometry(&geomLines, verArrayLines, cols, lineCount / 2, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;
		if (!InitializeGeometry(&geomQuad, verArrayQuad, cols, quadraticCount / 2, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;
	}
	
	if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
		printPoints = false; printLinear = true; printQuad = true; printCubic = true;
		float transform = 0.f; float scale = 1.f;
		scroll = false; awesome = false; text = true;
		
		glUseProgram(shader.program);
		GLint loc = glGetUniformLocation(shader.program, "text");
		if (loc != -1)
			glUniform1i(loc, text);
		
		//Load a font file and extract a glyph
		extractor.LoadFontFile("Fonts/SourceSansPro-ExtraLight.otf");
		for (unsigned i = 0; i < name.size(); i++){
			glyph = extractor.ExtractGlyph(name[i]);
			glyphCount();
		}
		
		float verArrayLines[lineCount];
		float verArrayQuad[quadraticCount];
		float verArrayCub[cubicCount];
		float adv = 0.f;
		lineCount = 0;
		quadraticCount = 0;
		cubicCount = 0;
		
		for (unsigned i = 0; i < name.size(); i++){
			glyph = extractor.ExtractGlyph(name[i]);
			glyphToGeom(verArrayLines, verArrayQuad, verArrayCub, 0.7f, -1.43f + adv, -0.39f);
                        adv += glyph.advance - 0.10f;
		}
		
		float cols[cubicCount * 3 / 2];
		for (int i = 0; i < quadraticCount * 3 / 2; i++){
			cols[i] = 1.f;
		}
		
		if (!InitializeGeometry(&geomLines, verArrayLines, cols, lineCount / 2, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;
		if (!InitializeGeometry(&geomQuad, verArrayQuad, cols, quadraticCount / 2, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;
		if (!InitializeGeometry(&geomCubic, verArrayCub, cols, cubicCount / 2, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;
	}

	if (key == GLFW_KEY_6 && action == GLFW_PRESS) {
		printPoints = false; printLinear = true; printQuad = true; printCubic = false;
		float transform = 0.f; float scale = 1.f;
		scroll = true; awesome = false; text = true;
		scrollBound = -12.f;
		
		glUseProgram(shader.program);
		GLint loc = glGetUniformLocation(shader.program, "text");
		if (loc != -1)
			glUniform1i(loc, text);
		
		//Load a font file and extract a glyph
		extractor.LoadFontFile("Fonts/Comic_Sans.ttf");
		for (unsigned i = 0; i < fox.size(); i++){
			glyph = extractor.ExtractGlyph(fox[i]);
			glyphCount();
		}
		
		float verArrayLines[lineCount];
		float verArrayQuad[quadraticCount];
		float verArrayCub[cubicCount];
		float adv = 0.f;
		lineCount = 0;
		quadraticCount = 0;
		cubicCount = 0;
		
		for (unsigned i = 0; i < fox.size(); i++){
			glyph = extractor.ExtractGlyph(fox[i]);
			glyphToGeom(verArrayLines, verArrayQuad, verArrayCub, 0.5f, 2.f + adv, -0.39f);
			adv += glyph.advance - 0.08f;
		}
		
		float cols[quadraticCount * 3 / 2];
		for (int i = 0; i < quadraticCount * 3 / 2; i++){
			cols[i] = 1.f;
		}
		
		if (!InitializeGeometry(&geomLines, verArrayLines, cols, lineCount / 2, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;
		if (!InitializeGeometry(&geomQuad, verArrayQuad, cols, quadraticCount / 2, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;
	}

	if (key == GLFW_KEY_7 && action == GLFW_PRESS) {
		printPoints = false; printLinear = true; printQuad = true; printCubic = false;
		float transform = 0.f; float scale = 1.f;
		scroll = true; awesome = false; text = true;
		scrollBound = -11.f;
		
		glUseProgram(shader.program);
		GLint loc = glGetUniformLocation(shader.program, "text");
		if (loc != -1)
			glUniform1i(loc, text);
		
		//Load a font file and extract a glyph
		extractor.LoadFontFile("Fonts/AlexBrush-Regular.ttf");
		for (unsigned i = 0; i < fox.size(); i++){
			glyph = extractor.ExtractGlyph(fox[i]);
			glyphCount();
		}
		
		float verArrayLines[lineCount];
		float verArrayQuad[quadraticCount];
		float verArrayCub[cubicCount];
		float adv = 0.f;
		lineCount = 0;
		quadraticCount = 0;
		cubicCount = 0;
		
		for (unsigned i = 0; i < fox.size(); i++){
			glyph = extractor.ExtractGlyph(fox[i]);
			glyphToGeom(verArrayLines, verArrayQuad, verArrayCub, 0.5f, 2.f + adv, -0.39f);
			adv += glyph.advance;
		}
		
		float cols[quadraticCount * 3 / 2];
		for (int i = 0; i < quadraticCount * 3 / 2; i++){
			cols[i] = 1.f;
		}
		
		if (!InitializeGeometry(&geomLines, verArrayLines, cols, lineCount / 2, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;
		if (!InitializeGeometry(&geomQuad, verArrayQuad, cols, quadraticCount / 2, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;
	}
	
	if (key == GLFW_KEY_8 && action == GLFW_PRESS) {
		printPoints = false; printLinear = true; printQuad = true; printCubic = true;
		float transform = 0.f; float scale = 1.f;
		scroll = true; awesome = false; text = true;
		scrollBound = -13.f;
		
		glUseProgram(shader.program);
		GLint loc = glGetUniformLocation(shader.program, "text");
		if (loc != -1)
			glUniform1i(loc, text);
		
		//Load a font file and extract a glyph
		extractor.LoadFontFile("Fonts/Inconsolata.otf");
		for (unsigned i = 0; i < fox.size(); i++){
			glyph = extractor.ExtractGlyph(fox[i]);
			glyphCount();
		}
		
		float verArrayLines[lineCount];
		float verArrayQuad[quadraticCount];
		float verArrayCub[cubicCount];
		float adv = 0.f;
		lineCount = 0;
		quadraticCount = 0;
		cubicCount = 0;
		
		for (unsigned i = 0; i < fox.size(); i++){
			glyph = extractor.ExtractGlyph(fox[i]);
			glyphToGeom(verArrayLines, verArrayQuad, verArrayCub, 0.5f, 2.f + adv, -0.39f);
			adv += glyph.advance;
		}
		
		float cols[quadraticCount * 3 / 2];
		for (int i = 0; i < quadraticCount * 3 / 2; i++){
			cols[i] = 1.f;
		}
		
		if (!InitializeGeometry(&geomLines, verArrayLines, cols, lineCount / 2, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;
		if (!InitializeGeometry(&geomQuad, verArrayQuad, cols, quadraticCount / 2, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;
		if (!InitializeGeometry(&geomCubic, verArrayCub, cols, cubicCount / 2, scale, transform))
			cout << "Program failed to intialize geometry!" << endl;
	}
	
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		if (scroll){
			if (awesome)
				awesome = false;
			else
				awesome = true;
		}
		else if (!text){
			if (printPoints && printLinear) {
				printPoints = false;
				printLinear = false;
			} else {
				printPoints = true;
				printLinear = true;
			}
		}
		glUseProgram(shader.program);
		GLint loc = glGetUniformLocation(shader.program, "awesome");
		if (loc != -1)
			glUniform1i(loc, awesome);
	}
	
	if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
		scrollSpeed *= 1.2f;
	}
	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
		scrollSpeed *= 0.8f;
	}
}

// ==========================================================================
// PROGRAM ENTRY POINT

int main(int argc, char *argv[])
{
	// initialize the GLFW windowing system
	if (!glfwInit()) {
		cout << "ERROR: GLFW failed to initialize, TERMINATING" << endl;
		return -1;
	}
	glfwSetErrorCallback(ErrorCallback);

	// attempt to create a window with an OpenGL 4.1 core profile context
	GLFWwindow *window = 0;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(1024, 1024, "Susant's A3 HALLOWEEN EDITION", 0, 0);
	if (!window) {
		cout << "Program failed to create GLFW window, TERMINATING" << endl;
		glfwTerminate();
		return -1;
	}

	// set keyboard callback function and make our context current (active)
	glfwSetKeyCallback(window, KeyCallback);
	glfwMakeContextCurrent(window);
	
	//Intialize GLAD if not lab linux
	#ifndef LAB_LINUX
	if (!gladLoadGL())
	{
		cout << "GLAD init failed" << endl;
		return -1;
	}
	#endif
	
	// query and print out information about our OpenGL environment
	QueryGLVersion();

	// call function to load and compile shader programs
	if (!InitializeShaders(&shader)) {
		cout << "Program could not initialize shaders, TERMINATING" << endl;
		return -1;
	}
	
	printPoints = false; printLinear = true; printQuad = true; printCubic = false;
	float transform = 0.f; float scale = 1.f;
	scroll = false;
	
	string intro = "Welcome to Susant\"s A3HALLOWEEN EDITION";
	
	glUseProgram(shader.program);
	GLint loc = glGetUniformLocation(shader.program, "text");
	if (loc != -1)
		glUniform1i(loc, true);
	
	//Load a font file and extract a glyph
	extractor.LoadFontFile("Fonts/Dreamscar.ttf");
	for (unsigned i = 0; i < intro.size(); i++){
		glyph = extractor.ExtractGlyph(intro[i]);
		glyphCount();
	}
	
	float verArrayLines[lineCount];
	float verArrayQuad[quadraticCount];
	float verArrayCub[cubicCount];
	float adv = 0.f;
	lineCount = 0;
	quadraticCount = 0;
	cubicCount = 0;
	
	for (unsigned i = 0; i < 22; i++){
		glyph = extractor.ExtractGlyph(intro[i]);
		glyphToGeom(verArrayLines, verArrayQuad, verArrayCub, 0.1f, -5.5f + adv, 1.f);
		adv += glyph.advance;
	}
	
	adv = 0.f;
	
	for (unsigned i = 22; i < intro.size(); i++){
		glyph = extractor.ExtractGlyph(intro[i]);
		glyphToGeom(verArrayLines, verArrayQuad, verArrayCub, 0.17f, -4.5f + adv, -1.f);
		adv += glyph.advance;
	}
	
	float cols[quadraticCount * 3 / 2];
	for (int i = 0; i < quadraticCount * 3 / 2; i++){
		cols[i] = 1.f;
	}
	
	if (!InitializeGeometry(&geomLines, verArrayLines, cols, lineCount / 2, scale, transform))
		cout << "Program failed to intialize geometry!" << endl;
	if (!InitializeGeometry(&geomQuad, verArrayQuad, cols, quadraticCount / 2, scale, transform))
		cout << "Program failed to intialize geometry!" << endl;

	glPatchParameteri(GL_PATCH_VERTICES, 4);

	double lastTime = glfwGetTime();
	int nbFrames = 0;
	double fps = 0.0;

	// run an event-triggered main loop
	while (!glfwWindowShouldClose(window))
	{
		double currentTime = glfwGetTime();
		nbFrames++;
		if (currentTime - lastTime >= 1.0 ){
			fps = double(nbFrames) / (currentTime - lastTime);
			nbFrames = 0;
			lastTime += 1.0;
		}
		if (scroll) {
			scrollFactor -= (scrollSpeed / float(fps));
			if (scrollFactor <= scrollBound)
				scrollFactor = 0.f;
		}
		else
			scrollFactor = 0.f;
		glUseProgram(shader.program);
		GLint loc1 = glGetUniformLocation(shader.program, "scroll");
		if (loc1 != -1)
			glUniform1i(loc1, scroll);
		GLint loc = glGetUniformLocation(shader.program, "scrollFactor");
		if (loc != -1)
			glUniform1f(loc, scrollFactor);
		// call function to draw our scene
		RenderScene(&shader); //render scene with texture
								
		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	// clean up allocated resources before exit
	DestroyGeometry(&geomPoints);
	DestroyGeometry(&geomLines);
	DestroyGeometry(&geomQuad);
	DestroyGeometry(&geomCubic);
	DestroyShaders(&shader);
	glfwDestroyWindow(window);
	glfwTerminate();

	cout << "Goodbye!" << endl;
	return 0;
}

// ==========================================================================
// SUPPORT FUNCTION DEFINITIONS

// --------------------------------------------------------------------------
// OpenGL utility functions

void QueryGLVersion()
{
	// query opengl version and renderer information
	string version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	string glslver = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
	string renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

	cout << "OpenGL [ " << version << " ] "
		<< "with GLSL [ " << glslver << " ] "
		<< "on renderer [ " << renderer << " ]" << endl;
}

bool CheckGLErrors()
{
	bool error = false;
	for (GLenum flag = glGetError(); flag != GL_NO_ERROR; flag = glGetError())
	{
		cout << "OpenGL ERROR:  ";
		switch (flag) {
		case GL_INVALID_ENUM:
			cout << "GL_INVALID_ENUM" << endl; break;
		case GL_INVALID_VALUE:
			cout << "GL_INVALID_VALUE" << endl; break;
		case GL_INVALID_OPERATION:
			cout << "GL_INVALID_OPERATION" << endl; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			cout << "GL_INVALID_FRAMEBUFFER_OPERATION" << endl; break;
		case GL_OUT_OF_MEMORY:
			cout << "GL_OUT_OF_MEMORY" << endl; break;
		default:
			cout << "[unknown error code]" << endl;
		}
		error = true;
	}
	return error;
}

// --------------------------------------------------------------------------
// OpenGL shader support functions

// reads a text file with the given name into a string
string LoadSource(const string &filename)
{
	string source;

	ifstream input(filename.c_str());
	if (input) {
		copy(istreambuf_iterator<char>(input),
			istreambuf_iterator<char>(),
			back_inserter(source));
		input.close();
	}
	else {
		cout << "ERROR: Could not load shader source from file "
			<< filename << endl;
	}

	return source;
}

// creates and returns a shader object compiled from the given source
GLuint CompileShader(GLenum shaderType, const string &source)
{
	// allocate shader object name
	GLuint shaderObject = glCreateShader(shaderType);

	// try compiling the source as a shader of the given type
	const GLchar *source_ptr = source.c_str();
	glShaderSource(shaderObject, 1, &source_ptr, 0);
	glCompileShader(shaderObject);

	// retrieve compile status
	GLint status;
	glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetShaderInfoLog(shaderObject, info.length(), &length, &info[0]);
		cout << "ERROR compiling shader:" << endl << endl;
		cout << source << endl;
		cout << info << endl;
	}

	return shaderObject;
}

// creates and returns a program object linked from vertex and fragment shaders
GLuint LinkProgram(GLuint vertexShader, GLuint TCSshader, GLuint TESshader, GLuint fragmentShader) 
{
	// allocate program object name
	GLuint programObject = glCreateProgram();

	// attach provided shader objects to this program
	if (vertexShader)   glAttachShader(programObject, vertexShader);
	if (TCSshader) glAttachShader(programObject, TCSshader); 
	if (TESshader) glAttachShader(programObject, TESshader); 
	if (fragmentShader) glAttachShader(programObject, fragmentShader);

	// try linking the program with given attachments
	glLinkProgram(programObject);

	// retrieve link status
	GLint status;
	glGetProgramiv(programObject, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetProgramInfoLog(programObject, info.length(), &length, &info[0]);
		cout << "ERROR linking shader program:" << endl;
		cout << info << endl;
	}

	return programObject;
}
