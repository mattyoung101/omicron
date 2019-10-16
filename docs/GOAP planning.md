# GOAP attacker (current)
Planning for if we were going to implement GOAP on our current robots.

## Actions
- GoDirectlyToBall: if ball in front, go and pick it up
- OrbitBall: if ball behind, go and orbit to pick it up
- DriveToGoal: drive with the ball in hand to the goal
- Score: when near the ball, go at YEET speed or kick if we had a kicker
- AvoidLine: do line avoidance

## World model
- HaveBall: if we have the ball in our capture zone
- CanScore: if the robot is in a position and orientation where it is likely to score (i.e. touching the goal with ball in hand)
- TouchingLine: if the robot is touching the line
- BallInFront: whether the ball is in front or behind the robot (whether to orbit or not)

## Goal
Make CanScore true and TouchingLine false (not sure if multiple goals permitted, just make a new variable called
Success or something).

## Example plan
OrbitBall -> DriveToGoal -> AvoidLine -> Score


# GOAP attacker (future)
If we were to implement GOAP in the future using our mad strats.

## Actions
- GoDirectlyToBall: if ball in front, go and pick it up
- OrbitBall: if ball behind, go and orbit to pick it up
- TakeBallToLine: with the ball in hand, goes to the line and starts backdribbling
- LineRunner: run down the line facing back to the goal (backdribbling) if we have ball
- FlickScore: when touching the goalie box line, flick and score into goal
- DriveToGoalNormal: just the old drive to the goal the normal way, no tricks
- ScoreNormal: scores a goal without any flicks. Must have done DriveToGoalNormal first(?)
- AvoidLine: do line avoidance
- LocateBall: if ball is not visible, sit on line to find it (or search for it in another way)

## World model
- HaveBall: if we have the ball in our capture zone and we are/should be dribbling it
- CanScore: if the robot is in a position and orientation where it is likely to score (i.e. near goal w/ ball)
- OutsideLine: whether the robot has left the line and needs to come back in
- TouchingLine: whether the robot is just touching the line or not
- TouchingGoalieBox: if the robot is touching the goalie box (on new ints field)
- TrickshotsEnabled: randomly chosen on boot with 65% chance of yes, whether or not fancy trickshots are allowed
- BallInFront: whether the ball is in front or behind the robot (whether to orbit or not)

## Goal
Make CanScore true and OutsideLine false.

## Example plan
LocateBall -> OrbitBall -> TakeBallToLine -> LineRunner -> FlickScore

LocateBall -> GoDirectlyToBall -> DriveToGoalNormal -> AvoidLine -> ScoreNormal


# GOAP description
## Tasks
GOAP will generate a plan containing different tasks. A task can be in the following states: RUNNING, SUCCESS, FAILURE.
A task will run until it reaches a state where it can't do so any more. If this was intentional, it is counted as a 
success. Otherwise, this is counted as a failure and the GOAP planner will be invoked again to generate a new plan.

## Q&A
**Q:** Why isn't the world model more complicated? It's just like 6 booleans!

**A:** The task code itself will consult robot_state_t when it actually executes the actions like shooting. The planner just needs to know a rough overview of the world to get a basic idea of what to do.