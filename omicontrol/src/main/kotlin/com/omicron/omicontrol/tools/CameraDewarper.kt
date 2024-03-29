/*
 * This file is part of the Omicontrol project.
 * Copyright (c) 2019-2020 Team Omicron. All rights reserved.
 *
 * Team Omicron members: Lachlan Ellis, Tynan Jones, Ethan Lo,
 * James Talkington, Matt Young.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package com.omicron.omicontrol.tools

import javafx.geometry.Point2D
import java.awt.Color
import java.awt.Rectangle
import java.awt.image.BufferedImage
import java.nio.file.Paths
import javax.imageio.ImageIO
import kotlin.math.*

/** A location in the image (used instead of Point2D because we want integer coords) */
data class Location(val x: Int, val y: Int)

/**
 * Utility program which dewarps a frame from the camera based on the mirror model
 * @author Matt Young (Team Omicron), 2020
 */
object CameraDewarper {
    /** mirror radius, same as in omicam.ini */
    private const val MIRROR_RADIUS = 384
    /** size to attempt to constrain dewarp output into a square of */
    private const val OUT_SIZE: Double = MIRROR_RADIUS * 2.0
    private val OUT_DIAGONAL = sqrt(OUT_SIZE.pow(2) + OUT_SIZE.pow(2)) / 2.0
    private const val FILE_PATH = "test_data/frame8.jpg"

    /**
     * Mirror dewarp model, directly from the calculated regression in the Calibration View in Omicontrol.
     */
    private fun dewarp(x: Double): Double {
        return -13.095322188640901 + 0.1934477257439889 * x + 0.0004631806201315 * x.pow(2) - 0.0000050016525762 *
                x.pow(3) + 0.0000000034877769 * x.pow(4) + 0.0000000000585543 * x.pow(5)
    //         return 2.5874667517468426 * exp(0.012010445463214335 * x)
    }

    /**
     * Returns the RGB of the pixel if it exists in the image, otherwise black
     */
    private fun safeGetRGB(image: BufferedImage, x: Int, y: Int): Int {
        return try {
            image.getRGB(x, y)
        } catch (e: Exception){
            Color.BLACK.rgb
        }
    }

    /**
     * Returns the average colour for a list of colours from image.getRGB()
     */
    private fun averageColours(colours: IntArray): Int {
        var totalR = 0
        var totalG = 0
        var totalB = 0
        for (item in colours){
            val colour = Color(item)
            totalR += colour.red
            totalG += colour.green
            totalB += colour.blue
        }
        return Color(totalR / colours.size, totalG / colours.size, totalB / colours.size).rgb
    }

    @JvmStatic
    fun main(args: Array<String>){
        val image = ImageIO.read(Paths.get(FILE_PATH).toFile())
        val centre = Point2D(image.width / 2.0, image.height / 2.0)
        val pixels = hashMapOf<Location, Int>()

        println("Out diagonal: $OUT_DIAGONAL")

        println("Finding magnitude range...")
        var minMag = 9999.0
        var maxMag = -9999.0
        for (y in 0 until image.height){
            for (x in 0 until image.width){
                // Make a vector from the centre
                val vec = Point2D(x.toDouble(), y.toDouble()).subtract(centre)

                // If the magnitude of the vector from the centre is outside the mirror, skip
                if (vec.magnitude() > MIRROR_RADIUS) continue

                // Calculate dewarped magnitude
                val mag = dewarp(vec.magnitude())
                if (mag > maxMag){
                    maxMag = mag
                } else if (mag < minMag){
                    minMag = mag
                }
            }
        }
        println("Min dewarped magnitude: $minMag, Max dewarped magnitude: $maxMag")

        println("Dewarping...")
        for (y in 0 until image.height){
            for (x in 0 until image.width){
                // Make a vector from the centre
                val vec = Point2D(x.toDouble(), y.toDouble()).subtract(centre)

                // If the magnitude of the vector from the centre is outside the mirror, skip
                if (vec.magnitude() > MIRROR_RADIUS) continue

                // Convert to polar and scale magnitude
                val polarAngle = atan2(vec.y, vec.x)
                // note: uncomment the dewarp() part here to make a cool warp grid, somehow. no idea how that works ^^
                val polarMag = dewarp(vec.magnitude())
                val scaledMag = ((polarMag - minMag) / (maxMag - minMag)) * OUT_DIAGONAL

                // Convert back to cartesian
                val dewarped = Point2D(scaledMag * cos(polarAngle), scaledMag * sin(polarAngle))
                    .add(centre.multiply(1.0))

                // Add dewarped location with current colour to pixels grid
                pixels[Location(dewarped.x.roundToInt(), dewarped.y.roundToInt())] = image.getRGB(x, y)
            }
        }

        // Find largest x value
        val largestX = pixels.keys.maxBy { it.x }!!.x
        val largestY = pixels.keys.maxBy { it.y }!!.y
        val smallestX = pixels.keys.minBy { it.x }!!.x
        val smallestY = pixels.keys.minBy { it.y }!!.y
        println("Largest XY: $largestX,$largestY Smallest XY: $smallestX,$smallestY")

        // Write out the image
        val out = BufferedImage(largestX + 64, largestY + 64, image.type)
        println("Image size: ${out.width}x${out.height}")
        var minX = 999
        var maxX = -999
        var minY = 999
        var maxY = -99
        for ((key, value) in pixels.entries){
            val x = if (key.x == largestX) key.x - 1 else key.x
            val y = if (key.y == largestY) key.y - 1 else if (key.y < 0) 32 else key.y + 32

            try {
                out.setRGB(x, y, value)

                // we do this to find the bounding box of the mirror for cropping
                when {
                    x > maxX -> {
                        maxX = x
                    }
                    x < minX -> {
                        minX = x
                    }
                    y > maxY -> {
                        maxY = y
                    }
                    y < minY -> {
                        minY = y
                    }
                }
            } catch (e: Exception){
                // this is so stupid i swear to god
                println("whatever, couldn't set $x,$y ($e)")
            }
        }
        println("Min/max written X: $minX, $maxX")

//        out.setRGB(minX, out.height / 2, Color.RED.rgb)
//        out.setRGB(maxX, out.height / 2, Color.RED.rgb)
//        out.setRGB(out.width / 2, minY, Color.RED.rgb)
//        out.setRGB(out.width / 2, maxY, Color.RED.rgb)

        // crop output (don't know why we have to do this, such is life)
        val bounds = Rectangle(minX, minY, maxX - minX, maxY - minY)
        val cropped = out.getSubimage(bounds.x, bounds.y, bounds.width, bounds.height)

        // nearest neighbour interpolation
        println("Interpolating...")
        for (y in 0 until cropped.height){
            for (x in 0 until cropped.width){
                if (cropped.getRGB(x, y) == Color.BLACK.rgb){
                    val north = safeGetRGB(cropped, x, y + 1)
                    val northEast = safeGetRGB(cropped, x + 1, y + 1)
                    val east = safeGetRGB(cropped, x + 1, y)
                    val southEast = safeGetRGB(cropped, x - 1, y - 1)
                    val south = safeGetRGB(cropped, x, y - 1)
                    val southWest = safeGetRGB(cropped, x - 1, y - 1)
                    val west = safeGetRGB(cropped, x - 1, y)
                    val northWest = safeGetRGB(cropped, x - 1, y + 1)

                    val neighbours = intArrayOf(north, northEast, east, southEast, south, southWest, west, northWest).filter { it != Color.BLACK.rgb }.toIntArray()
                    if (neighbours.isEmpty()) continue
                    val avg = averageColours(neighbours)

                    cropped.setRGB(x, y, avg)
                }
            }
        }

        // TODO delete pixels that are not in our circle???
        // TODO draw circle on output for debug

        ImageIO.write(cropped, "png", Paths.get("test_data/out.png").toFile())
    }
}