# Open source release

_Introduction by Matt Young._

## Introduction

Hi, and thanks for reading this! This year, 2020, has been a long one for everyone, RoboCup Jr teams included.
Regardless of where you're from, COVID-19 is bound to have affected you in one way or another. At Team Omicron, it
effected us quite significantly.  The 2019 Internationals competition had left all team members with a lot to be
desired, with many bugs to fix and new things to add. Thus, we all set our sights to the 2020 competition. We undertook
many new research & development projects in the hope of finally bringing something (hopefully) game-changing to soccer.
Our plan was, of course, to deploy this all at the 2020 RoboCup Jr Internationals in Bordeaux, France. Unfortunately,
however, this competition was tragically cancelled due to the COVID-19 restrictions, and it's unlikely we'll will have
the chance to go back next year in 2021 (should it even go ahead). This left us feeling extremely disappointed, and 
thus development and testing of the robot slowed down quite a lot.

Although we never got a chance to make our difference in the official competitions, we at Team Omicron still want to
give back to the community somehow, or do _something_ with the hundreds of hours we've individually spent designing,
programming, testing and writing. As such, we've decided to open source our entire project! This means that every
single last bit of code, hardware (both structural & electrical), all formal documentation and even internal testing
documents/notes will be released under permissive open source licences. We provide documentation below and on GitHub
for how to get started.

We're really excited about doing this, because we believe it's the largest scale open-source release of a relatively
successful and long-running RoboCup Jr team. We hope that it's really beneficial to the community and allows teams
new and old to grow, adapt and improve on their current setups. Although Omicron ourselves will never have the chance to
deploy our new technology at a competition, we hope that others will enjoy the same honour, and all of our team members
look forward to seeing what people come up with. 

If you have any questions or comments, shoot them our way - see the [About page](about.md)
for contact details. For myself and everyone else on Team Omicron, it's been an amazing experience participating in
RoboCup Jr over the years, even despite this year's many disasters. Anyway, that's it from me. Thanks, and have fun!

## Getting started
To get started, simply install Git and clone **[our GitHub repo](TODO add link)**. 

This repo contains all our projects and heaps of documentation to get you started. Please read the README.md file
all the way through, and branch out to individual projects from there.

### License
#### Code
All code written by Team Omicron is released under the **Mozilla Public License 2.0** (see LICENSE.txt in each
directory). You will be able to tell which code is ours due to the presence of Omicron copyright notices at the top of
the file.

For information on this licence, please [read this FAQ](https://www.mozilla.org/en-US/MPL/2.0/FAQ/), and [this
question](https://opensource.stackexchange.com/a/8832). Simply put, if you are building a robot based on Team Omicron's
code, the MPL requires that you disclose that your robot uses open-source software from Team Omicron, and where anyone
can obtain it (this repo). _A great place to do so would be in your poster and presentation._ If you modify any files with
the MPL licence header, the licence requires that you release your improvements under the MPL 2.0 as well. New files
that you create can be under any licence.

We have decided to use the MPL because we believe it balances freedom of usage, while making sure that any improvements
are released back to the RoboCup community for future teams to benefit from.

#### Designs
All hardware designs and PCBs produced by us are licenced under
the **[Creative Commons Attribution-ShareAlike 4.0 International License](https://creativecommons.org/licenses/by-sa/4.0/)**.

This licence basically means you can use and change the designs, as long as you give us appropriate credit and also distribute
your modifications under the same licence. See the linked document for more info (it's not too complicated). A great place
to attribute us, if you use our designs, is in your presentation and poster.

## Some final thoughts
While it's certainly possible to use, say, our camera project on your robot by simply downloading and compiling it,
I personally think it's much more educational to use our design ideas and code as a _baseline for your own projects_.
For example, we think that our novel localisation algorithm works pretty well. While it's certainly possible to use
our implementation in Omicam on your robot directly (and there are instructions for doing so), I think it's much
more impressive and educational to use that code as a method for creating _your own_ implementation of our algorithms -
or creating better algorithms that run faster and more accurately! That's the spirit of RoboCup, after all :)
