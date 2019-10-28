precision mediump float;

// Based on libgdx's default SpriteBatch shader:
// https://github.com/libgdx/libgdx/blob/master/gdx/src/com/badlogic/gdx/graphics/g2d/SpriteBatch.java (Apache 2)

attribute vec3 a_position;
attribute vec2 a_texCoord;
varying vec2 v_texCoords;

void main(){
    v_texCoords = a_texCoord;
    gl_Position =  vec4(a_position, 1.0);
}