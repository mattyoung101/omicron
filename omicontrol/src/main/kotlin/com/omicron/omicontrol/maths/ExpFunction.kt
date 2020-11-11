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

import org.apache.commons.math3.analysis.ParametricUnivariateFunction
import org.apache.commons.math3.analysis.differentiation.DerivativeStructure
import org.apache.commons.math3.util.FastMath

/**
 * Implementation of a standard exponential function:
 * f(x) = a * exp(b * x)
 */
class ExpFunction : ParametricUnivariateFunction {
    override fun value(x: Double, vararg parameters: Double): Double {
        // most other Apache implementations of functions use FastMath, so we will as well
        val a = parameters[0]
        val b = parameters[1]
        return a * FastMath.exp(b * x)
    }

    override fun gradient(x: Double, vararg parameters: Double): DoubleArray {
        val a = parameters[0]
        val b = parameters[1]

        // return or derivatives with respect to parameters: dy/da, dy/db
        // thanks to riley bowyer for help with this
        val dyda = FastMath.exp(b * x)
        val dydb = a * x * FastMath.exp(b * x)

        return doubleArrayOf(dyda, dydb)
    }
}