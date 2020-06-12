#include "replay.h"
#include "log/log.h"
#include "utils.h"
#include <inttypes.h>
#include <time.h>
#include <errno.h>
#include "rpa_queue.h"
#include "DG_dynarr.h"
#include "pb.h"
#include "nanopb/pb_encode.h"
#include "nanopb/pb_decode.h"
#include "protobuf/Replay.pb.h"

// This file handles writing replay data to disk. Omicam replays are timestamped Protobuf messages containing stuff like
// the localised robot position and orientation. Vision data is recorded separately, that's handled in computer_vision.cpp.

DA_TYPEDEF(ReplayFrame, replay_frame_list_t)

/** true if a replay is currently being recorded */
static _Atomic replay_status_t replayStatus = REPLAY_NONE;
/** System time in microseconds at which Omicam launched successfully */
static uint64_t lastFrameTime;
/** the numeric id this replay file is assigned */
static _Atomic time_t recordingId = 0;
/** the formatted path used for the replay file */
static char filename[256] = {0};
/** file handle to omirec replay file */
static FILE *outFile = NULL;
/** this contains a linked list of DebugFrames which will be flushed to the Protobuf message on disk */
static replay_frame_list_t frameList = {0};

/** DECODE for ReplayFile.frames */
static bool frames_decode(pb_istream_t *stream, const pb_field_t *field, void **arg){
    // the docs are really unclear, but i gather we want to try and read in the sub message
    // and that this function is called for each ReplayFrame in the repeated Frames element
    return false;
}

/** ENCODE for ReplayFile.frames */
static bool frames_encode(pb_ostream_t *stream, const pb_field_t *field, void * const *arg){
    // parse the tag, docs are unclear but it seems you just have to do this
    if (!pb_encode_tag_for_field(stream, field)){
        return false;
    }
    // iterate over the recorded replay frames and write them out to the stream
    for (ReplayFrame *frame = da_begin(frameList), *end = da_end(frameList); frame != end; ++frame){
        if (!pb_encode_submessage(stream, ReplayFrame_fields, frame)){
            return false;
        }
    }
    return true;
}

/** flushes all the currently recorded ReplayFrames to a ReplayFile message on disk */
static void flush_frames(void){
    ReplayFile replayFile = ReplayFile_init_zero;
    replayFile.frameWidth = 1280;
    replayFile.frameHeight = 720;

    // we do this here because using the curly brace initialiser outside causes some warnings, not sure
    static pb_callback_t framesCallbacks = {0};
    framesCallbacks.funcs.decode = frames_decode;
    framesCallbacks.funcs.encode = frames_encode;
    replayFile.frames = framesCallbacks;

    // figure out how large our replay is to allocate a buffer later
    // technically this encodes the message twice which may be problematic for performance, but flush_frames() isn't
    // called that often
    size_t msgSize = 0;
    if (!pb_get_encoded_size(&msgSize, ReplayFile_fields, &replayFile)){
        log_error("Failed to get replay Protobuf size! Cannot encode!");
        return;
    }
    printf("Expected replay message size: %zu\n", msgSize);

    // now we allocate the buffer (with some extra crap on the end just in case)
    uint8_t *buf = malloc(msgSize + 64);
    pb_ostream_t stream = pb_ostream_from_buffer(buf, msgSize + 64);
    if (!pb_encode(&stream, ReplayFile_fields, &replayFile)){
        log_error("Failed to encode replay Protobuf message: %s", PB_GET_ERROR(&stream));
        free(buf);
        return;
    }

    free(buf);
}

void replay_record(void){
    if (replayStatus == REPLAY_RECORDING){
        log_warn("A replay recording is already in progress. Will not start a new one.");
        return;
    }

    replayStatus = REPLAY_RECORDING;
    recordingId = time(NULL);
    snprintf(filename, 256, "../recordings/omicam_replay_%ld.omirec", recordingId);
    log_debug("Writing replay file to: %s", filename);
    outFile = fopen(filename, "wb");
    if (outFile == NULL){
        log_error("Failed to open replay file: %s", strerror(errno));
        return;
    }

    lastFrameTime = (uint64_t) round(utils_time_micros());
    log_trace("Start micros set to %" PRIu64, lastFrameTime);
}

void replay_close(void){
    if (replayStatus == REPLAY_RECORDING) {
        log_debug("Ending replay that was being written to: %s", filename);
        flush_frames();
        fclose(outFile);
        da_free(frameList);
    } else if (replayStatus == REPLAY_LOADING){
        // we would basically just free the frames list here
    }
}

replay_status_t replay_get_status(void){
    return replayStatus;
}

time_t replay_get_id(){
    return recordingId;
}