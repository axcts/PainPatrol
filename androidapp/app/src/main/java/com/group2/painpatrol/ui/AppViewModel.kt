package com.group2.painpatrol.ui

import androidx.lifecycle.ViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.serialization.json.JsonElement

object AppViewModel: ViewModel() {
    private val uiStateFlow = MutableStateFlow(UiState())
    val uiState = uiStateFlow.asStateFlow()

    internal fun processReadings(readings: Map<String, JsonElement>) {
        val stringifiedReadings: Map<String, String> = readings.mapValues { it.value.toString() }
        updateAppState(stringifiedReadings)
    }

    private fun updateAppState(payload: Map<String, String>) {
        uiStateFlow.update { currentState ->
            currentState.copy(
                readings = payload
            )
        }
    }
}