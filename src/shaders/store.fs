// background grid of squares function
float grid_of_squares(vec2 uv, float gridSize, float gridSquareSize, float gridCornerRadius)
{
    vec2 gridUV = fract(uv / gridSize);
    vec2 gridID = floor(uv / gridSize);

    // Calculate rotation angle using frequencyPulse
    float rotationSpeed = 3.14159 / 4.0; // Quarter rotation in one second
    float freq = 2.0;
    float rotationPulse = frequencyPulse(0.5); // Alternates every 2 seconds
    float rotationAngle = rotationSpeed * time * 2.0; // Full rotation in 4 seconds

    // Determine if this is an odd or even square
    bool isOdd = mod(gridID.x + gridID.y, 2.0) == 1.0;

    // Apply rotation to gridUV for alternating squares
    if ((isOdd && rotationPulse == 1.0) || (!isOdd && rotationPulse == 0.0)) {
        gridUV = rotate(gridUV - 0.5, rotationAngle) + 0.5;
    }

    return rounded_square(gridUV, vec2(0.5), gridSquareSize, gridCornerRadius);
}

c += vec4(0.1, 0.1, 0.1, 1.0) * grid_of_squares(uv, 0.1, 0.4, 0.2);



// Updated utility function for frequency-based float
float frequencyPulse(float frequency)
{
    return step(0.5, mod(time * frequency, 1.0));
}

float circle(vec2 uv, vec2 p, float r)
{
    float d = distance(p, uv);
    return step(d, r);
}

float rounded_square(vec2 uv, vec2 p, float size, float radius)
{
    vec2 d = abs(uv - p) - size + radius;
    return 1.0 - smoothstep(0.0, 0.001, length(max(d, 0.0)) - radius);
}
