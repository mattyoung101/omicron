package com.github.ajalt.colormath

import kotlin.math.roundToInt

data class CMYK(val c: Int, val m: Int, val y: Int, val k: Int, val a: Float = 1f) : ConvertibleColor {
    init {
        require(c in 0..100) { "c must be in range [0, 100] in $this" }
        require(m in 0..100) { "m must be in range [0, 100] in $this" }
        require(y in 0..100) { "y must be in range [0, 100] in $this" }
        require(k in 0..100) { "k must be in range [0, 100] in $this" }
        require(a in 0f..1f) { "a must be in range [0, 1] in $this" }
    }

    companion object : ColourSpace {
        override val friendlyName = "CMYK"
        override val minRange = intArrayOf(0, 0, 0, 0)
        override val maxRange = intArrayOf(100, 100, 100, 100)
    }

    override val alpha: Float get() = a

    override fun toRGB(): RGB {
        val c = this.c / 100.0
        val m = this.m / 100.0
        val y = this.y / 100.0
        val k = this.k / 100.0
        val r = 255 * (1 - c) * (1 - k)
        val g = 255 * (1 - m) * (1 - k)
        val b = 255 * (1 - y) * (1 - k)
        return RGB(r.roundToInt(), g.roundToInt(), b.roundToInt(), alpha)
    }

    override fun toCMYK() = this
}
