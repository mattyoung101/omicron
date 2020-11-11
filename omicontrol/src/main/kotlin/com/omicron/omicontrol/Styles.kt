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

package com.omicron.omicontrol

import javafx.scene.paint.Color
import javafx.scene.text.FontWeight
import tornadofx.Stylesheet
import tornadofx.box
import tornadofx.cssclass
import tornadofx.px

class Styles : Stylesheet() {
    companion object {
        val appMenuButton by cssclass()
        val titleLabel by cssclass()
        val rect by cssclass()
        val paddedBox by cssclass()
        val bigLabel by cssclass()
        val boldLabel by cssclass()
    }

    init {
        label {
            fontSize = 18.px
        }

        boldLabel {
            fontWeight = FontWeight.BOLD
        }

        bigLabel {
            fontSize = 22.px
            fontWeight = FontWeight.BOLD
        }

        button {
            fontSize = 20.px
            padding = box(12.px)
            minWidth = 128.px
        }

        paddedBox {
            padding = box(5.px)
        }

        titleLabel {
            fontSize = 36.px
            fontWeight = FontWeight.EXTRA_BOLD
        }

        progressIndicator {
            // TODO doesn't work, want to fill it in white
            fill = Color.WHITE
        }

        rect {
            stroke = Color.BLACK
            strokeWidth = 5.px
            fill = Color.TRANSPARENT
        }

        appMenuButton {
            fontSize = 25.px
            padding = box(40.px)
            minWidth = 256.px
            tabMinWidth = 256.px
        }
    }
}