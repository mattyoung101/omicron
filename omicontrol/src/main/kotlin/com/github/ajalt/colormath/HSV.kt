package com.github.ajalt.colormath

import java.lang.IllegalArgumentException
import kotlin.math.floor
import kotlin.math.max
import kotlin.math.roundToInt

/**
 * A color in the Hue-Saturation-Value color space.
 *
 * @property h The hue, as degrees in the range `[0, 360]`
 * @property s The saturation, as a percent in the range `[0, 100]`
 * @property v The value, as a percent in the range `[0, 100]`
 * @property a The alpha, as a fraction in the range `[0, 1]`
 */
data class HSV(override val h: Int, val s: Int, val v: Int, val a: Float = 1f) : ConvertibleColor, HueColor {
    init {
        require(h in 0..360) { "h must be in range [0, 360]" }
        require(s in 0..100) { "s must be in range [0, 100]" }
        require(v in 0..100) { "v must be in range [0, 100]" }
        require(a in 0f..1f) { "a must be in range [0, 1] in $this" }
    }

    companion object : ColourSpace {
        override val friendlyName = "HSV"
        override val minRange = intArrayOf(0, 0, 0)
        override val maxRange = intArrayOf(360, 100, 100)
    }

    override val alpha: Float get() = a

    override fun toRGB(): RGB {
        val h = h.toDouble() / 60
        val s = s.toDouble() / 100
        var v = v.toDouble() / 100
        val hi = floor(h) % 6

        val f = h - floor(h)
        val p = 255 * v * (1 - s)
        val q = 255 * v * (1 - (s * f))
        val t = 255 * v * (1 - (s * (1 - f)))
        v *= 255

        val (r, g, b) = when (hi.roundToInt()) {
            0 -> Triple(v, t, p)
            1 -> Triple(q, v, p)
            2 -> Triple(p, v, t)
            3 -> Triple(p, q, v)
            4 -> Triple(t, p, v)
            else -> Triple(v, p, q)
        }
        return RGB(r.roundToInt(), g.roundToInt(), b.roundToInt(), alpha)
    }

    override fun toHSL(): HSL {
        val h = h.toDouble()
        val s = s.toDouble() / 100
        val v = v.toDouble() / 100
        val vmin = max(v, 0.01)

        val l = ((2 - s) * v) / 2
        val lmin = (2 - s) * vmin
        val sl = if(lmin == 2.0) 0.0 else (s * vmin) / (if (lmin <= 1) lmin else 2 - lmin)

        return HSL(h.roundToInt(), (sl * 100).roundToInt(), (l * 100).roundToInt(), alpha)
    }

    override fun toHSV() = this

    override fun getChannel(idx: Int): Int {
        return when (idx){
            0 -> h
            1 -> s
            2 -> v
            3 -> a.toInt()
            else -> throw IllegalArgumentException("Illegal colour channel: $idx")
        }
    }
}
