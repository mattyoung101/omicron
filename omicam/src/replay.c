#include "replay.h"
#include "log/log.h"
#include "utils.h"
#include <inttypes.h>
#include <time.h>

// This file handles writing replay data to disk. Omicam replays are timestamped Protobuf messages containing stuff like
// the localised robot position and orientation. Vision data is recorded separately, that's handled in computer_vision.cpp.
// This file is brought to you by the following message:
// shawty's like a melody in my head got me goin like na na na na everyday like an ipod stuck on replay

/** true if a replay is currently being recorded */
static _Atomic bool isRecording = false;
/** System time in microseconds at which Omicam launched successfully */
static uint64_t lastFrameTime;
static _Atomic time_t recordingId = 0;
static char filename[256] = {0};

void replay_begin(){
    if (isRecording){
        log_warn("A replay recording is already in progress. Will not start a new one.");
        return;
    }

    isRecording = true;
    recordingId = time(NULL);
    snprintf(filename, 256, "../recordings/omicam_replay_%ld.omirec", recordingId);
    log_debug("Writing replay file to: %s", filename);

    lastFrameTime = (uint64_t) round(utils_time_micros());
    log_trace("Start micros set to %" PRIu64, lastFrameTime);
}

void replay_end(){
    if (!isRecording) return;
    log_debug("Ending replay that was being written to: %s", filename);
}

void replay_dispose(){
    // don't know if we actually need this
}

bool replay_is_recording(){
    return isRecording;
}

time_t replay_get_id(){
    return recordingId;
}