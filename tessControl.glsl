//Executes once for each vertex passed in. for each patch.
//Determines the amount of tessellation that a patch should have
// **Every TCS invocation for an input patch has access to the same input data, save for gl_InvocationID​** (That is, it gets to see all the patch data in gl_in)

//gl_TessLevelInner and gl_TessLevelOuter
// The inner tessellation level controls the number of “nested” primitives,
// and the outer tessellation level controls the number of times to subdivide each edge.

//Isolines contain no inner value and only two outer values
// outer 0 is how many isolines to draw
// outer 1 is how much to subdivide each line

// gl_in incoming array of structs holding some data coming into the shader
// gl_out outgoing array of structs holding updated data

// gl_PatchVerticesIn is the number of vertices per patch and also the length of gl_in[]

// per vertex out, use "out <type> <name>"
// per patch out, use  "patch out <type> <name>"

#version 410
layout(vertices = 4) out; //How long gl_out[] should be

in vec3 tcColour[];
out vec3 teColour[];

void main()
{
    // gl_InvocationID tells you what input vertex you are working on
    if (gl_InvocationID == 0) {   // only needs to be set once
        gl_TessLevelOuter[0] = 1; // only need to draw one line
        gl_TessLevelOuter[1] = 100; // how much to subdivide each line
    }
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;	// pass control points to TES
    teColour[gl_InvocationID] = tcColour[gl_InvocationID]; 						// pass colours to TES
}

