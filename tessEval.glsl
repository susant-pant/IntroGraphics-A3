//TES transforms new vertices. Runs once for all output vertices

#version 410
layout(isolines) in; // Controls how tesselator creates new geometry

in vec3 teColour[]; // input colours

out vec3 Colour; // colours to fragment shader

uniform int bezierType = 2;
uniform bool text = false;

float u = gl_TessCoord.x;
float b0 = 1.0-u;
float b1 = u;
vec3 pink = vec3(1.f, 0.3f, 0.7f);
vec3 orange = vec3(1.f, 0.7f, 0.f);

vec4 linear(vec4 p0, vec4 p1) {
	vec4 lerp = b0 * p0 +
				b1 * p1 ;
	return lerp;
}

vec4 quadratic(vec4 p0, vec4 p1, vec4 p2) {
	vec4 lerp = pow(b0,2) * p0 +
				2 * b0 * b1 * p1 +
				pow(b1,2) * p2 ;
	return lerp;
}

vec4 cubic(vec4 p0, vec4 p1, vec4 p2, vec4 p3) {
	vec4 lerp = pow(b0,3) * p0 +
				3 * pow(b0,2) * b1 * p1 +
				3 * b0 * pow(b1,2) * p2 +
				pow(b1,3) * p3 ;
	return lerp;
}
	
vec3 linColour(vec3 p0, vec3 p1) {
	vec3 lerp = b0 * p0 +
				b1 * p1 ;
	return lerp;
}

void main() {
	switch(bezierType) {
		case 0 :
			gl_Position = cubic(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position, gl_in[3].gl_Position);
			Colour = teColour[0];
			break;
		case 1 :
			gl_Position = linear(gl_in[0].gl_Position, gl_in[1].gl_Position);
			Colour = vec3(0.3f, 0.3f, 0.3f);
			break;
		case 2 :
			gl_Position = quadratic(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position);
			Colour = linColour(pink, orange);
			break;
		case 3 :
			gl_Position = cubic(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position, gl_in[3].gl_Position);
			Colour = linColour(pink, orange);
			break;
    }
    if (text)
		Colour = vec3(1.f, 1.f, 1.f);
}



