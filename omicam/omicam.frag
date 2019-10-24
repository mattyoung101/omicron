precision mediump float;

uniform vec4 color;
uniform vec3 minBall, maxBall, minLine, maxLine, minYellow, maxYellow, minBlue, maxBlue;
varying vec2 v_texCoords;
uniform sampler2D u_texture;

// Partial source: https://github.com/libgdx/libgdx/blob/master/gdx/src/com/badlogic/gdx/graphics/g2d/SpriteBatch.java (Apache 2)

bool inRange(vec3 value, vec3 min, vec3 max){
    bool r = value.r >= min.r && value.r <= max.r;
    bool g = value.g >= min.g && value.g <= max.r;
    bool b = value.b >= min.r && value.b <= max.r;
    return r && g && b;
}

void main() {
    vec3 texCol = texture2D(u_texture, v_texCoords).rgb;
    gl_FragColor.r = inRange(texCol, minBall, maxBall) ? 1.0 : 0.0;
    gl_FragColor.g = inRange(texCol, minLine, maxLine) ? 1.0 : 0.0;
    gl_FragColor.b = inRange(texCol, minBlue, maxBlue) || inRange(texCol, minYellow, maxYellow) ? 1.0 : 0.0;

    // so the output of this fragment shader is three bitmasks:
    // red channel = ball pixels, green channel = line pixels, b channel = any goal pixels
}