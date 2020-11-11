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

import org.apache.commons.math3.fitting.WeightedObservedPoint

/** Encapsulates an approach to calculating a mirror model, for example by using a polynomial or by an exponential. */
interface ModelApproach {
    /**
     * Performs a least squares regression to optimise the coefficients for the specified function.
     * @return the list of coefficients for the specified model
     */
    fun calculateModel(dataPoints: Collection<WeightedObservedPoint>): DoubleArray

    /**
     * Formats the list of coefficients for the function in a string that can later be evaluated by a mathematical expression
     * parser.
     *
     * Must be valid for both exp4j (the once we use in Omicontrol) and tinyexpr (the one we use in Omicam).
     */
    fun formatFunction(coefficients: DoubleArray): String

    /**
     * Evaluates the model function at the specified point
     */
    fun evaluate(x: Double, coefficients: DoubleArray): Double
}