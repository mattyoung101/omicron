# Virtual Poster Session Notes
Some thoughts I (Matt) have after observing the other Open leagues teams on the 2020 Virtual Poster Session on the forums.
These are just thoughts, so take them at face value. No mean spirit meant if interpreted that way.

Link: https://junior.forum.robocup.org/c/robocupjunior-soccer

## Stuff we need to improve upon
1. Improve localisation accuracy for field objects (ball drifts when moving).
2. Improve localiser speed (rewrite as non-linear least squares problem and solve using Levenberg-Marquardt).
3. Long-term trajectory planning with arbitrary obstacle avoidance. This is part of the steering behaviour system.
4. Model kinematics of robot better, for better control (already a WIP with mouse sensor).
5. More strategies and choose which strategy we do better.
6. Kalman filter or better filtering on the robots and field objects
7. Handle line over smoother. Resolve instead of bouncing off it.
8. More inter-robot communication over Bluetooth to organise strategies and stuff. Seems not many teams do this right now.
9. Somehow figure out a way to semi accurately detect robots, probably using our little ray system (detect empty spaces in them)
10. Face away from robots more (turtle strat, use aggressively). We may still be the highest resolution camera in the league, 
so we can still try hide the ball.
11. Test the shit out of all of our stuff. It's better to have less, but more tested, than more, but less tested.

## Omicam related stuff
We seem to be still be the highest resolution camera in the league, other SBC teams are at like 480p. We are slower though, 
we are 60fps@720p while some teams are 90fps@480p - however, the improved resolution is still worth it and shouldn't cause
any gameplay problems. Our latency might though, need to look into that.

Our vision-based non-linear optimisation approach still seems to be novel. Other teams use the classic goal maths,
mouse sensor or 360 LiDAR.

However, FESB from Croatia are getting close because they use a 360 LiDAR, Hough transform and apparently tried a particle filter 
but couldn't get it to run in realtime (makes sense since they're trying to do it on just an ESP32) so settled for a 
simpler approach. They also mention the potential use of a Kalman filter, but so far it doesn't seem to be used in production.
They're only a two man team which is super impressive considering what they've tried so far, once they get an SBC to process
their LiDAR data they'll have comparable or better localisation than us.

Transcendence from Singapore say they use various sensors like mouse and single directional LiDAR into some sensor fusion 
algorithm based on reliability. We don't know how this works, but judging on its description we can assume its some hand-crafted 
approach that works reasonably well. I believe they also use a Kalman filter. Given that both them and FESB use a Kalman filter,
we really should investigate using this - and also because we seriously need it to keep our localised field object positions
stable during movement along with the robot.

Our LattePanda is still one of the most powerful SBCs in the league, most other teams use Pis if any SBC at all.
XLC-Innovation use a Google Coral which is for a different specialty (machine learning with its TPU), so its hard to compare, 
but they would be the next most powerful or better, depending on the task. Obviously the LattePanda isn't going to be great
for ML, but we don't use any so it's fine.

Many teams mention robot detection. We cannot yet detect robots, this may be worth looking into. While XLC initially seems
scary, it seems as though they can only detect _their own_ robots which are covered in a special triangle pattern using
the YOLOv3 network they setup. So far I haven't seen any conclusive proof that accurate robot detection of _other teams_
is possible yet. It's interesting they want to _detect_ team mates instead of localising and transmitting their position
data wirelessly, would seem easier to do that (it's our approach as well).

The actual camera we use itself, the e-con Hyperyon, should still be one of the better ones around in terms of coping with
weird venue lighting. Most teams use Amazon USB cameras or Pi Cameras, so they might have issues depending on what 
venue lighting is like. We could mention this more on the poster, though I don't know if it's fair to mention something
that's basically just you can afford it, not something you did.

Final note: a lot of teams are still using OpenMV or Pixies. No wonder because stuff like our setup expensive and complicated as,
but still, only a minority of teams are using an SBC so far. More than last year, but not everyone yet. So it's not like
we're going to be knocked out of the competition right at the start.

## Misc
May be worth looking into a building a simulator, but once the real robot ships it won't be that much use. Especially
with the long term planning stuff we can just look where it's going. Transcendence's simulator uses Pymunk, seems as
though they take a similar approach to that very basic sim I made for the NEAT agent a while ago.