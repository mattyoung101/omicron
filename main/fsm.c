#define DG_DYNARR_IMPLEMENTATION
#include "fsm.h"
#include "states.h"
#include "utils.h"

static const char *TAG = "FSM";

state_machine_t* fsm_new(fsm_state_t *startState){
    state_machine_t* fsm = calloc(1, sizeof(state_machine_t));
    fsm->currentState = &stateGeneralNothing;
    fsm->semaphore = xSemaphoreCreateMutex();
    // change into the start state, to make sure startState->enter is called
    fsm_change_state(fsm, startState); 
    return fsm;
}

void fsm_update(state_machine_t *fsm){
    // we could add back the state history size check but if never seen it go off so I'm removing it for now
    fsm->currentState->update(fsm);
}

// function which handles the actual logic of changing states
static void fsm_internal_change_state(state_machine_t *fsm, fsm_state_t *newState, bool pushToStack){
    if (xSemaphoreTake(fsm->semaphore, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))){
        if (pushToStack) da_push(fsm->stateHistory, fsm->currentState);
        fsm->currentState->exit(fsm);
        
        fsm->currentState = newState;
        fsm->currentState->enter(fsm);

        log_once_reset();
        xSemaphoreGive(fsm->semaphore);
    } else {
        ESP_LOGE(TAG, "Failed to unlock FSM semaphore, cannot change states.");
    }
}

void fsm_change_state(state_machine_t *fsm, fsm_state_t *newState){
    if (fsm->currentState == newState) return;
    ESP_LOGI(TAG, "Switching states from %s to %s", fsm->currentState->name, newState->name);    
    fsm_internal_change_state(fsm, newState, true);
}

void fsm_revert_state(state_machine_t *fsm){
    if (da_count(fsm->stateHistory) < 1){
        ESP_LOGE(TAG, "Unable to revert: state history too small, size = %d", da_count(fsm->stateHistory));
        return;
    }
    fsm_state_t *previousState = da_pop(fsm->stateHistory);
    ESP_LOGI(TAG, "Reverting from state %s to %s", fsm->currentState->name, previousState->name);
    fsm_internal_change_state(fsm, previousState, false);
}

bool fsm_in_state(state_machine_t *fsm, char *name){
    char *curName = fsm_get_current_state_name(fsm);
    bool ret = strcmp(curName, name) == 0;
    // free string allocated with strdup
    free(curName);
    curName = NULL;
    return ret;
}

char *fsm_get_current_state_name(state_machine_t *fsm){
    if (xSemaphoreTake(fsm->semaphore, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))){
        char *state = strdup(fsm->currentState->name);
        xSemaphoreGive(fsm->semaphore);
        return state;
    } else {
        ESP_LOGE(TAG, "Failed to unlock FSM semaphore, cannot return state name");
        return "ERROR";
    }
}

void fsm_reset(state_machine_t *fsm){
    ESP_LOGD(TAG, "Resetting FSM");
    if (xSemaphoreTake(fsm->semaphore, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))){
        // check if resetting would cause errors
        if (da_count(fsm->stateHistory) <= 1){
            ESP_LOGD(TAG, "Nothing to revert (only one state in history)");
            xSemaphoreGive(fsm->semaphore);
            return;
        }

        // grab the first two elements, INCLUDING StateNothing, so we don't cause issues with reverting
        fsm_state_t *stateNothing = da_get(fsm->stateHistory, 0);
        fsm_state_t *initialState = da_get(fsm->stateHistory, 1);

        // change states to the initial state, giving the semaphore as fsm_internal_change_state requires it
        xSemaphoreGive(fsm->semaphore);
        fsm_internal_change_state(fsm, initialState, false);
        if (!xSemaphoreTake(fsm->semaphore, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))){
            ESP_LOGE(TAG, "Cannot re-acquire semaphore after changing states!");
            return;
        }
        
        // now delete the array
        da_free(fsm->stateHistory);

        // and add back the nothing state, but not the initialState since we've already changed into it
        da_add(fsm->stateHistory, stateNothing);
        xSemaphoreGive(fsm->semaphore);
    } else {
        ESP_LOGE(TAG, "Failed to unlock FSM semaphore, cannot reset");
    }
}

void fsm_dump(state_machine_t *fsm){
    if (xSemaphoreTake(fsm->semaphore, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))){
        printf("BEGIN FSM DUMP\nState history (%d entries):\n", da_count(fsm->stateHistory));
        
        for (int i = 0; i < da_count(fsm->stateHistory); i++){
            fsm_state_t *state = da_get(fsm->stateHistory, i);
            printf("\t%d. %s\n", i, state->name);
        }

        printf("Current state: %s\nEND FSM DUMP\n", fsm->currentState->name);
        xSemaphoreGive(fsm->semaphore);
    } else {
        ESP_LOGE(TAG, "Failed to unlock FSM semaphore, cannot dump state history");
    }
}