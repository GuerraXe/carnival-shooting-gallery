// Carnival Shooting Gallery — world post-process (ARCHITECTURE §5.3, brief §7.5).
//
// Applied to world layers only (D-014). One cheap pass:
//   1. warm, faded grade: slightly lifted warm blacks, warm midtones, a touch
//      of desaturation ("softly remembered", never a heavy filter)
//   2. vignette: analytic lens falloff, ~20 % at the corners, ~2 % at the
//      lane ends (brief §7.5: must not hide ducks)
//   3. film grain: sampled from a SCANNED grain plate (OV_GRAIN, D-022).
//      When the scan is absent grainStrength is 0 and no grain is drawn;
//      there is deliberately no procedural fallback.
//   Before the vignette, the booth's light level (D-031): the incandescent
//   bulbs' slight sags and the warm exposure lift of a shot. (1,1,1) = steady.
//
// GLSL 1.10 (SFML's compatibility context).

uniform sampler2D texture;
uniform sampler2D grain;
uniform float grainStrength;   // 0 disables grain
uniform vec2 grainScale;       // target pixels / grain texture pixels
uniform vec2 grainOffset;      // random per frame, in [0,1)
uniform float vignetteStrength;
uniform float gradeStrength;   // 0 = untouched, 1 = full grade
uniform vec3 light;            // per-channel exposure: flicker and shot flash

float luma(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

void main() {
    vec2 uv = gl_TexCoord[0].xy;
    vec4 src = texture2D(texture, uv);
    vec3 c = src.rgb;

    // 1. Grade.
    vec3 graded = c;
    float l = luma(graded);
    graded = mix(vec3(l), graded, 0.93);                                  // gentle fade of saturation
    float mid = 4.0 * l * (1.0 - l);                                      // 0 at black/white, 1 at mid-grey
    graded *= mix(vec3(1.0), vec3(1.045, 1.0, 0.93), mid);                // tungsten midtones
    vec3 lift = vec3(0.030, 0.022, 0.014);                                // warm, never pure black
    graded = lift + graded * (1.0 - lift);
    c = mix(c, graded, gradeStrength);

    // Light level of the booth (flicker, shot flash).
    c *= light;

    // 2. Vignette (elliptical, wider than tall like a lens on a 16:9 frame).
    vec2 d = (uv - 0.5) * vec2(1.0, 0.85);
    float r = length(d) / 0.7;
    c *= 1.0 - vignetteStrength * smoothstep(0.45, 1.1, r);

    // 3. Scanned film grain, strongest in the midtones like real emulsion.
    if (grainStrength > 0.0) {
        float g = texture2D(grain, uv * grainScale + grainOffset).r - 0.5;
        float response = 0.35 + 0.65 * mid;
        c += g * grainStrength * response;
    }

    gl_FragColor = vec4(clamp(c, 0.0, 1.0), 1.0);
}
