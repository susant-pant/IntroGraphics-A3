// ==========================================================================
// Vertex program for barebones GLFW boilerplate
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================
#version 410

// location indices for these attributes correspond to those specified in the
// InitializeGeometry() function of the main program
layout(location = 0) in vec2 VertexPosition;
layout(location = 1) in vec3 VertexColour;

// output to be interpolated between vertices and passed to the fragment stage
out vec3 tcColour;

uniform bool scroll = false;
uniform float scrollFactor;

void main()
{
	vec2 newPos = VertexPosition;
	if (scroll){
		float xPos = VertexPosition.x + scrollFactor;
		float yPos = VertexPosition.x;
		newPos = vec2((VertexPosition.x - 2.f / -xPos) + (scrollFactor / 5), (VertexPosition.y * yPos));
	}
    // assign vertex position without modification
    gl_Position = vec4(newPos, 0.0, 1.0);

    // assign output colour to be interpolated
    tcColour = VertexColour;
}
