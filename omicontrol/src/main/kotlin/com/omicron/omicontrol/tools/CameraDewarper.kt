package com.omicron.omicontrol.tools

import net.objecthunter.exp4j.ExpressionBuilder

/**
 * Utility program which dewarps a frame from the camera based on the mirror model
 */
object CameraDewarper {
    private const val MIRROR_MODEL = "2.5874667517468426 * exp(0.012010445463214335 * x)"
    private const val OUTPUT_WIDTH = 1280
    private const val OUTPUT_HEIGHT = 720

    @JvmStatic
    fun main(args: Array<String>){
        val model = ExpressionBuilder(MIRROR_MODEL).variables("x").build()

        // the largest and smallest domains of the mirror dewarp function should be from the centre to the corners
        // of the frame
    }
}