#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Uniform for screen dimensions
uniform vec2 screenDims;
uniform vec2 mp;
uniform float time;

// New uniform for metaball positions
#define MAX_METABALLS 10
uniform vec2 metaballPositions[MAX_METABALLS];
uniform int numMetaballs;

// Output fragment color
out vec4 finalColor;

// Metaball function
float metaball(vec2 p, vec2 center, float size)
{
    float r = size * 0.5;
    return r * r / max(0.001, dot(p - center, p - center));
}


float circle(vec2 uv, vec2 p, float r)
{
    float d = distance(p, uv);
    return step(d, r);
}


void main()
{
    // Normalized coordinates
    vec2 uv = gl_FragCoord.xy / screenDims;
    uv.y = 1.0 - uv.y; // fix flipped y-coordinate

    /////////////// SHADER STARTS HERE ///////////////
    float metaballValue = 0.0;
    float threshold = 1.0; // Adjust this value to change the metaball size

    // Calculate metaball value
    // for (int i = 0; i < numMetaballs && i < MAX_METABALLS; i++)
    // {
    //     metaballValue += metaball(uv, metaballPositions[i], 0.1); // Increased size for visibility
    // }

    // Create color based on metaball value
    vec3 color = vec3(0.0); // Black background

    if (metaballValue > threshold)
    {
        color = vec3(1.0); // White metaballs
    }

    // draw red circle at mouse
    color.r += circle(uv, mp, 0.05);

    // draw tiny green circles at each metaball position
    for (int i = 0; i < numMetaballs && i < MAX_METABALLS; i++)
    {
        color.g += circle(uv, metaballPositions[i], 0.01);
    }

    finalColor = vec4(color, 1.0);
}