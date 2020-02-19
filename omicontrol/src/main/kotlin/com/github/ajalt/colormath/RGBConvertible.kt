package com.github.ajalt.colormath

/** Interface to instantiate this colour from RGB values **/
interface RGBConvertible {
    fun fromRGB(rgb: RGB): ConvertibleColor
}