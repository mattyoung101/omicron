#include "fsm.h"
#include "states.h"
#include "utils.h"
#include "DG_dynarr.h"

static const char *TAG = "FSM";
state_machine_t *stateMachine = NULL;

state_machine_t* fsm_new(fsm_state_t *startState){
    state_machine_t* fsm = calloc(1, sizeof(state_machine_t));
    fsm->currentState = &stateGeneralNothing;
    fsm->semaphore = xSemaphoreCreateMutex();
    fsm->updateInProgress = xSemaphoreCreateMutex();
    // change into the start state, to make sure startState->enter is called
    fsm_change_state(fsm, startState); 
    return fsm;
}

void fsm_update(state_machine_t *fsm){
    if (xSemaphoreTake(fsm->updateInProgress, pdMS_TO_TICKS(32))){
        fsm->currentState->update(fsm);
        xSemaphoreGive(fsm->updateInProgress);
    } else {
        ESP_LOGE(TAG, "Failed to unlock updateInProgress, cannot update FSM!");
    }

    // too many states may use up too much RAM/make the FSM slower, so clear it if it gets too hectic
    if (da_count(fsm->stateHistory) >= FSM_MAX_STATES){
        ESP_LOGE(TAG, "Too many states in state history (have: %d)!", da_count(fsm->stateHistory));
        fsm_reset(fsm);
    }
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
    char *currentName = fsm_get_current_state_name(fsm);
    
    ESP_LOGD(TAG, "SWITCHING states from %s to %s", currentName, newState->name);
    fsm_internal_change_state(fsm, newState, true);
    free(currentName);
    currentName = NULL;
}

void fsm_revert_state(state_machine_t *fsm){
    if (da_count(fsm->stateHistory) < 1){
        ESP_LOGE(TAG, "Unable to revert: state history too small, size = %d", da_count(fsm->stateHistory));
        return;
    }
    fsm_state_t *previousState = da_pop(fsm->stateHistory);
    char *currentName = fsm_get_current_state_name(fsm);

    ESP_LOGD(TAG, "REVERTING from state %s to %s", currentName, previousState->name);
    fsm_internal_change_state(fsm, previousState, false);
    free(currentName);
}

bool fsm_in_state(state_machine_t *fsm, char *name){
    char *curName = fsm_get_current_state_name(fsm);
    bool ret = strcmp(curName, name) == 0;
    free(curName);
    return ret;
}

char *fsm_get_current_state_name(state_machine_t *fsm){
    if (xSemaphoreTake(fsm->semaphore, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))){
        char *state = strdup(fsm->currentState->name);
        xSemaphoreGive(fsm->semaphore);
        return state;
    } else {
        ESP_LOGE(TAG, "Failed to unlock FSM semaphore, cannot return state name");
        // use strdup instead of allocating the variable on the stack so it won't cause errors when we use free()
        return strdup("ERROR");
    }
}

void fsm_reset(state_machine_t *fsm){
    ESP_LOGI(TAG, "Resetting FSM");
    
    if (xSemaphoreTake(fsm->semaphore, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))
        && xSemaphoreTake(fsm->updateInProgress, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))){
        // check if resetting would cause errors
        if (da_count(fsm->stateHistory) <= 1){
            ESP_LOGD(TAG, "Nothing to revert (only one state in history)");
            xSemaphoreGive(fsm->semaphore);
            xSemaphoreGive(fsm->updateInProgress);
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
        xSemaphoreGive(fsm->updateInProgress);

        ESP_LOGI(TAG, "FSM reset completed");
    } else {
        ESP_LOGE(TAG, "Failed to unlock FSM semaphore, cannot reset");
    }
}

void fsm_partial_reset(state_machine_t *fsm){
    ESP_LOGD(TAG, "Partially resetting FSM");

    if (xSemaphoreTake(fsm->semaphore, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))){
        // check if resetting would cause errors
        if (da_count(fsm->stateHistory) <= 1){
            ESP_LOGD(TAG, "Nothing to revert (only one state in history)");
            xSemaphoreGive(fsm->semaphore);
            return;
        }

        // clear state history and add back the stateNothing
        fsm_state_t *stateNothing = da_get(fsm->stateHistory, 0);
        da_free(fsm->stateHistory);
        da_add(fsm->stateHistory, stateNothing);
        
        xSemaphoreGive(fsm->semaphore);
    } else {
        ESP_LOGE(TAG, "Failed to unlock FSM semaphore, cannot partially reset");
    }
}

void fsm_dump(state_machine_t *fsm){
    if (xSemaphoreTake(fsm->semaphore, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))){
        printf("BEGIN FSM DUMP\nState history (%d entries):\n", da_count(fsm->stateHistory));
        
        for (size_t i = 0; i < da_count(fsm->stateHistory); i++){
            fsm_state_t *state = da_get(fsm->stateHistory, i);
            printf("\t%d. %s\n", i, state->name);
        }

        printf("Current state: %s\nEND FSM DUMP\n", fsm->currentState->name);
        xSemaphoreGive(fsm->semaphore);
    } else {
        ESP_LOGE(TAG, "Failed to unlock FSM semaphore, cannot dump state history");
    }
}

void fsm_free(state_machine_t *fsm){
    da_free(fsm->stateHistory);
    vSemaphoreDelete(fsm->semaphore);
    free(fsm);
    fsm = NULL;
}