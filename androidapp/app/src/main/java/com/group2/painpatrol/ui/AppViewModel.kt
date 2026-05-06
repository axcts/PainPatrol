package com.group2.painpatrol.ui

import android.app.Application
import android.util.Log
import androidx.lifecycle.AndroidViewModel
import com.group2.painpatrol.data.sendNotification
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.serialization.json.JsonElement

object AppViewModel: AndroidViewModel(application = Application()) {
    private val discomfortMessages = mutableListOf<String>()
    private val uiStateFlow = MutableStateFlow(UiState())
    val uiState = uiStateFlow.asStateFlow()

    internal fun processReadings(readings: Map<String, JsonElement>) {
        val stringifiedReadings: Map<String, String> = readings.mapValues { it.value.toString() }

        uiState.value.thresholds.forEach { (sensor, range) ->
            if ((stringifiedReadings[sensor]?.toFloat() ?: -1.00f) !in range) {
                when(sensor) {
                    "temperature" -> {
                        if (stringifiedReadings[sensor]?.toFloat() ?: -1.00f < range.start)
                            discomfortMessages.add("too cold")
                        else discomfortMessages.add("too warm")
                    }
                    "humidity" -> {
                        if (stringifiedReadings[sensor]?.toFloat() ?: -1.00f < range.start)
                            discomfortMessages.add("too dry")
                        else discomfortMessages.add("too humid")
                    }
                    "lighting" -> {
                        if (stringifiedReadings[sensor]?.toFloat() ?: -1.00f < range.start)
                            discomfortMessages.add("too dark")
                        else discomfortMessages.add("too bright")
                    }
                    "sound" -> {
                        if (stringifiedReadings[sensor]?.toFloat() ?: -1.00f < range.start)
                            discomfortMessages.add("too silent")
                        else discomfortMessages.add("too loud")
                    }
                }
            }
        }

        if (!discomfortMessages.isEmpty()) {
            Log.d("test", discomfortMessages.joinToString())

            sendNotification(getApplication<Application>().applicationContext,discomfortMessages)

            discomfortMessages.clear()
        }


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