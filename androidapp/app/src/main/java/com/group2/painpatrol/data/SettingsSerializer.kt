package com.group2.painpatrol.data

import androidx.datastore.core.CorruptionException
import androidx.datastore.core.Serializer
import kotlinx.serialization.SerializationException
import kotlinx.serialization.json.Json
import java.io.InputStream
import java.io.OutputStream


object SettingsSerializer : Serializer<Settings> {
    override val defaultValue: Settings = Settings(mapOf(), mapOf())
    override suspend fun readFrom(input: InputStream): Settings =
        try {
            Json.decodeFromString<Settings>(input.readBytes().decodeToString())
        } catch (serialization: SerializationException) {
            throw CorruptionException("Unable to read Settings", serialization)
        }

    override suspend fun writeTo(settings: Settings, output: OutputStream) {
        output.write(
            Json.encodeToString(settings).encodeToByteArray()
        )
    }
}