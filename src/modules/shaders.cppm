export module shaders;

export constexpr const char *SPIRAL_VS = R"(#version 330

// Macros
#define COLOR_CALCULATED 0
#define COLOR_BREATHING  1
#define COLOR_STATIC     2
#define COLOR_GRADIENT   3

layout(location = 0) in vec2 vertexPosition; // (x, y)

// Uniforms
uniform mat4 mvp;
uniform float u_time;
uniform float u_pointSize;
uniform float u_maxP;
uniform int u_colorMode;
uniform vec4 u_customStatic;
uniform vec4 u_gradientCenter;
uniform vec4 u_gradientEdge;
uniform vec4 u_globalBreath;

out vec4 fragColor;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 applyRipple(vec2 pos, float dist, float time){
  if (dist <= 0.0) return pos;
  float wave = sin(dist * 0.005 - time * 4.0);
  return pos + (pos / dist) * (wave * (dist * 0.08));
}

vec4 calculateColor(int mode, float pVal, float maxP) {
    float distRatio = (maxP > 0.0) ? clamp(pVal / maxP, 0.0, 1.0) : 0.0;

    switch (mode) {
        case COLOR_CALCULATED: {
            float hue = mod(pVal * 0.05, 360.0) / 360.0;
            return vec4(hsv2rgb(vec3(hue, 0.8, 1.0)), 1.0);
        }
        case COLOR_BREATHING:
            return u_globalBreath;

        case COLOR_STATIC:
            return u_customStatic;

        case COLOR_GRADIENT:
            return mix(u_gradientCenter, u_gradientEdge, distRatio);

        default:
            return vec4(1.0); // White fallback
    }
}

void main() {
    vec2 pos = vertexPosition.xy;
    float pVal = length(pos);

  // Ripple
    pos = applyRipple(pos, pVal, u_time);
  
    // Camera Projection and point size
    gl_Position = mvp * vec4(pos, 0.0, 1.0);
    gl_PointSize = u_pointSize;

    // Color
    fragColor = calculateColor(u_colorMode, pVal, u_maxP);
}
)";

export constexpr const char *SPIRAL_FS = R"(#version 330

in vec4 fragColor;
out vec4 finalColor;

void main() {
    finalColor = fragColor;
}
)";
