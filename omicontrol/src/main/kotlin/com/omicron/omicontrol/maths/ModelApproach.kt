package com.omicron.omicontrol.maths

import org.apache.commons.math3.fitting.WeightedObservedPoint

/** Encapsulates an approach to calculating a mirror model */
interface ModelApproach {
    /**
     * @return the list of coefficients for the specified model
     */
    fun calculateModel(dataPoints: Collection<WeightedObservedPoint>): DoubleArray
}