package com.group2.painpatrol.data

import kotlinx.serialization.Serializable
import kotlinx.serialization.json.JsonElement

@Serializable
data class Settings(val readingsHistory: Map<String, JsonElement>, val sensorBounds: Map<String, List<Float>>)
