package com.group2.painpatrol.data

import androidx.datastore.core.DataStore
import kotlinx.coroutines.flow.map
import kotlinx.serialization.json.JsonElement
import kotlinx.serialization.json.contentOrNull
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

class SettingsRepository(private val dataStore: DataStore<Settings>) {

    internal suspend fun saveThresholds(thresholds: Map<String, ClosedFloatingPointRange<Float>>) {
        dataStore.updateData { currentBounds ->
            currentBounds.copy(sensorBounds = thresholds.mapValues {
                listOf(it.value.start, it.value.endInclusive)
            })
        }
    }

    internal suspend fun saveReadings(readings: Map<String, JsonElement>) {
        /*
            since timestamp is a string, we have to treat it as a primitive, not jsonobject
         */
        val timestamp = readings["timestamp"]?.jsonPrimitive?.contentOrNull
        val readings = readings["readings"]?.jsonObject

        if (readings.isNullOrEmpty() || timestamp == null) return
        dataStore.updateData { currentReadings ->
            currentReadings.copy(readingsHistory = currentReadings.readingsHistory
                                                    + (timestamp to readings))
        }
    }

    internal fun readThresholds() = dataStore.data.map { settings -> settings.sensorBounds }
    internal fun readReadings() = dataStore.data.map { settings -> settings.readingsHistory }
}