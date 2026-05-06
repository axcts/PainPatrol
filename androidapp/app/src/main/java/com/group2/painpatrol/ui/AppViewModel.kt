package com.group2.painpatrol.ui

import android.app.Application
import android.content.Context
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

    var notifSendTime = System.currentTimeMillis()
    val cooldownTimeMs = 60000L     // 1 minute
    private lateinit var appContext : Context // lateinit allows us to not have to initialised vars
    // ie they get initialised later
    fun init(context: Context){
        appContext = context.applicationContext
    }

    internal fun processReadings(readings: Map<String, JsonElement>) {
        val stringifiedReadings: Map<String, String> = readings.mapValues { it.value.toString() }

        uiState.value.thresholds.forEach { (sensor, range) ->
            if ((stringifiedReadings[sensor]?.toFloat() ?: -1.00f) !in range) {
                when(sensor) {
                    "temperature" -> {
                        if (stringifiedReadings[sensor]?.toFloat() ?: -1.00f < range.start)
                            discomfortMessages.add("cold")
                        else discomfortMessages.add("warm")
                    }
                    "humidity" -> {
                        if (stringifiedReadings[sensor]?.toFloat() ?: -1.00f < range.start)
                            discomfortMessages.add("dry")
                        else discomfortMessages.add("humid")
                    }
                    "lighting" -> {
                        if (stringifiedReadings[sensor]?.toFloat() ?: -1.00f < range.start)
                            discomfortMessages.add("dark")
                        else discomfortMessages.add("bright")
                    }
                    "sound" -> {
                        if (stringifiedReadings[sensor]?.toFloat() ?: -1.00f < range.start)
                            discomfortMessages.add("quiet")
                        else discomfortMessages.add("loud")
                    }
                }
            }
        }

        val unixTime = System.currentTimeMillis()

        if (!discomfortMessages.isEmpty() && (unixTime - notifSendTime >= cooldownTimeMs)) {
            sendNotification(appContext,discomfortMessages)
            notifSendTime = System.currentTimeMillis()
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