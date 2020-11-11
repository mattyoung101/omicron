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

package com.omicron.omicontrol.field

import javafx.geometry.Point2D
import javafx.scene.control.Label

// TODO make this inherit from FieldObject
data class Robot(var id: Int,
                 var position: Point2D = Point2D(0.0, 0.0),
                 var orientation: Float = 0.0f,
                 var isPositionKnown: Boolean = false,
                 var fsmState: String = "Unknown",
                 var positionLabel: Label? = null) {
}