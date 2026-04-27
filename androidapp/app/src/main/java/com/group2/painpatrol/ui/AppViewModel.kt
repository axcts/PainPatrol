package com.group2.painpatrol.ui

import androidx.lifecycle.ViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update

object AppViewModel: ViewModel() {
    private val uiStateFlow = MutableStateFlow(UiState())
    val uiState = uiStateFlow.asStateFlow()

    internal fun processReading(payload: String) {
        if (payload != "N/A") {
            updateAppState(payload)
        }
    }

    private fun updateAppState(payload: String) {
        uiStateFlow.update { currentState ->
            currentState.copy(
                tempPayload = payload
            )
        }
    }
}