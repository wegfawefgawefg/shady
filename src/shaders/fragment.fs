#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Uniform for screen dimensions
uniform vec2 screenDims;

// Output fragment color
out vec4 finalColor;

void main()
{
    // Normalize coordinates based on screen dimensions
    vec2 nc = gl_FragCoord.xy / screenDims;
    nc.y = 1.0 - nc.y;  // Flip y-coordinate
    
    finalColor = vec4(nc.x, nc.y - 0.5, 0.0, 1.0);
}