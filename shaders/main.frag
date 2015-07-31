uniform sampler2D texture;
uniform sampler2D secondaryTexture;
varying highp vec2 texc;

void main(void) {
    int index = int(texture2D(texture, texc).r * 255.0);

    int b = int(round(texture2D(secondaryTexture, vec2((3.0 * float(index) + 0.0) / (64.0 * 3.0 - 1.0), 0.0)).r * 255.0));
    int g = int(round(texture2D(secondaryTexture, vec2((3.0 * float(index) + 1.0) / (64.0 * 3.0 - 1.0), 0.0)).r * 255.0));
    int r = int(round(texture2D(secondaryTexture, vec2((3.0 * float(index) + 2.0) / (64.0 * 3.0 - 1.0), 0.0)).r * 255.0));

    gl_FragColor = vec4(float(r)/7.0, float(g)/7.0, float(b)/7.0, 1.0);
}
