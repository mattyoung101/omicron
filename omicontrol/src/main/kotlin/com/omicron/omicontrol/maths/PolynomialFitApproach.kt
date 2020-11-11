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

package com.omicron.omicontrol.maths

import org.apache.commons.math3.analysis.polynomials.PolynomialFunction
import org.apache.commons.math3.fitting.PolynomialCurveFitter
import org.apache.commons.math3.fitting.WeightedObservedPoint
import org.apache.commons.math3.util.FastMath
import org.tinylog.kotlin.Logger
import java.text.DecimalFormat

/**
 * Calculates a mirror model by doing a least-squares curve fit on an N-th order polynomial using Apache Commons Math
 * @param order the order of the polynomial (number of coefficients)
 */
class PolynomialFitApproach(private val order: Int) : ModelApproach {
    private val fitter = PolynomialCurveFitter.create(order)
    // https://stackoverflow.com/a/20937683/5007892
    private val decimalFormat = DecimalFormat("#.#").apply {
        maximumFractionDigits = 16
    }

    override fun calculateModel(dataPoints: Collection<WeightedObservedPoint>): DoubleArray {
        Logger.debug("Calculating a polynomial model for ${dataPoints.size} points, order of $order")
        return fitter.fit(dataPoints)
    }

    private fun coeffToString(coeff: Double): String {
        // this is also just a copy and paste of PolynomialFunction.toString() except that it uses the DecimalFormat
        // to try and avoid using scientific notation which may confuse parsers
        // may not be strictly required to use the decimal formatter since most parsers can handle it fine, in which
        // case, call Double.toString()
        return decimalFormat.format(coeff)
    }

    override fun formatFunction(coefficients: DoubleArray): String {
        // note: this is just a copy and paste of PolynomialFunction.toString()
        // except that it adds multiply symbols between the x terms so it works with tinyexpr and other parsers

        val s = StringBuilder()
        if (coefficients[0] == 0.0) {
            if (coefficients.size == 1) {
                return "0"
            }
        } else {
            s.append(coeffToString(coefficients[0]))
        }

        for (i in 1 until coefficients.size) {
            if (coefficients[i] != 0.0) {
                if (s.isNotEmpty()) {
                    if (coefficients[i] < 0) {
                        s.append(" - ")
                    } else {
                        s.append(" + ")
                    }
                } else {
                    if (coefficients[i] < 0) {
                        s.append("-")
                    }
                }
                val absAi = FastMath.abs(coefficients[i])
                if (absAi - 1 != 0.0) {
                    s.append(coeffToString(absAi))
                    s.append(" * ")
                }
                s.append("x")
                if (i > 1) {
                    s.append('^')
                    s.append(i.toString())
                }
            }
        }

        return s.toString()
    }

    override fun evaluate(x: Double, coefficients: DoubleArray): Double {
        return PolynomialFunction(coefficients).value(x)
    }
}