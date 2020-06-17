package com.omicron.omicontrol.tools

import javafx.geometry.Point2D
import java.awt.Color
import java.awt.Rectangle
import java.awt.image.BufferedImage
import java.nio.file.Paths
import javax.imageio.ImageIO
import kotlin.math.*

/** A location in the image */
data class Location(val x: Int, val y: Int)

/**
 * Utility program which dewarps a frame from the camera based on the mirror model
 * @author Matt Young
 */
object CameraDewarper {
    private const val MIRROR_RADIUS = 384
    /** size to attempt to constrain dewarp output into a square of */
    private const val OUT_SIZE: Double = MIRROR_RADIUS * 1.0
    private val OUT_DIAGONAL = sqrt(OUT_SIZE.pow(2) + OUT_SIZE.pow(2)) / 2.0

    /**
     * Mirror dewarp model, directly from the calculated regression in the Calibration View in Omicontrol.
     */
    private fun dewarp(x: Double): Double {
        return 2.5874667517468426 * exp(0.012010445463214335 * x)
    }

    @JvmStatic
    fun main(args: Array<String>){
        val image = ImageIO.read(Paths.get("test_data/frame2.jpg").toFile())
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
                // note: uncomment the dewarp() part here to make a cool warp grid, somehow! no idea how that works
                val polarMag = dewarp(vec.magnitude())
                val scaledMag = ((polarMag - minMag) / (maxMag - minMag)) * OUT_DIAGONAL

                // Convert back to cartesian
                val dewarped = Point2D(scaledMag * cos(polarAngle), scaledMag * sin(polarAngle))
                    .add(centre.multiply(0.5))

                // Add dewarped location with current colour to pixels grid
                pixels[Location(dewarped.x.roundToInt(), dewarped.y.roundToInt())] = image.getRGB(x, y)
            }
        }

//        val out = BufferedImage(ceil(OUT_SIZE).toInt() + 64, ceil(OUT_SIZE).toInt() + 64, image.type)
//        println("Image size: ${out.width}x${out.height}")
//        for ((key, value) in pixels.entries){
//            val x = if (key.x == out.width) key.x - 1 else key.x
//            val y = if (key.y == out.height) key.y - 1 else key.y
//            try {
//                out.setRGB(x, y, value)
//            } catch (e: Exception){
//                println("FUCK YOU, COULDN'T SET $x,$y")
//            }
//        }

        // Find largest x value
        val largestX = pixels.keys.maxBy { it.x }!!.x
        val largestY = pixels.keys.maxBy { it.y }!!.y
        val smallestX = pixels.keys.minBy { it.x }!!.x
        val smallestY = pixels.keys.minBy { it.y }!!.y
        println("Largest XY: $largestX,$largestY Smallest XY: $smallestX,$smallestY")

        // Write out the image
        val out = BufferedImage(largestX + 64, largestY + 64, image.type)
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
                if (x > maxX){
                    maxX = x
                } else if (x < minX){
                    minX = x
                } else if (y > maxY){
                    maxY = y
                } else if (y < minY){
                    minY = y
                }
            } catch (e: Exception){
                println("FUCK YOU, COULDN'T SET $x,$y")
            }
        }
        print("Min/max written X: $minX, $maxX")

        out.setRGB(minX, out.height / 2, Color.RED.rgb)
        out.setRGB(maxX, out.height / 2, Color.RED.rgb)
        out.setRGB(out.width / 2, minY, Color.RED.rgb)
        out.setRGB(out.width / 2, maxY, Color.RED.rgb)

        // TODO nearest neighbour interp missed pixels in image - only crop if inside bounding rectangle
        val bounds = Rectangle(minX, minY, maxX - minX, maxY - minY)

        // crop output (don't know why we have to do this, such is life)
        val cropped = out.getSubimage(minX, minY, maxX - minX, maxY - minY)

        ImageIO.write(cropped, "png", Paths.get("test_data/out.png").toFile())
    }
}