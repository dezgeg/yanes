#version 120

uniform sampler2D texture;
uniform sampler2D secondaryTexture;
uniform sampler2D xAxisGrid;
uniform sampler2D yAxisGrid;
varying highp vec2 texc;

void main(void) {
    float xc = texture2D(xAxisGrid, vec2(texc.x, 0)).r;
    float yc = texture2D(yAxisGrid, vec2(texc.y, 0)).r;

    int tileX = int(texc.x * 16.0);
    int tileY = int(texc.y * 32.0);
    int bitX = int(round(xc * 7));
    int bitY = int(round(yc * 7));

    int offs = 16 * (16 * tileY + tileX);

    int lsbs = int(round(texture2D(secondaryTexture, vec2((offs + bitY) / 8191.0, 0.0)).r * 255.0));
    int msbs = int(round(texture2D(secondaryTexture, vec2((offs + bitY + 8) / 8191.0, 0.0)).r * 255.0));

    int b0 = int(mod(int(lsbs / exp2(7 - bitX)), 2.0));
    int b1 = int(mod(int(msbs / exp2(7 - bitX)), 2.0));

    float grayScale = 1.0 - float(2 * b1 + b0)/3.0;

    if (xc == 1.0 || yc == 1.0) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        gl_FragColor = vec4(grayScale, grayScale, grayScale, 1.0);
    }
}
