package com.group2.painpatrol.data

import android.util.Log
import com.group2.painpatrol.ui.AppViewModel
import com.hivemq.client.mqtt.MqttClient
import kotlinx.serialization.json.Json
import kotlinx.serialization.json.jsonObject

internal object MQTTSubscriber {
    private val identifier: String = "pain-patrol"
    private val serverHost: String = "broker.hivemq.com"
    private val serverPort: Int = 1883
    var payload: String? = null
    // so that MQTT client can connect and work asynchronously
    var client = MqttClient.builder()
        .useMqttVersion5()
        .identifier(identifier)
        .serverHost(serverHost)
        .serverPort(serverPort)
        .buildAsync()

    fun connect() {
        client.connect().whenComplete { connAck, throwable ->
            if (throwable != null) {
                Log.e("MQTT", throwable.toString())
                Log.e("MQTT", "MQTT failed to connect")
            } else {
                client.subscribeWith()
                    .topicFilter("painpatrol/readings")
                    .callback {
                        publish ->
                            payload = (publish.payloadAsBytes).toString(Charsets.UTF_8)
                            val mappedPayload = Json.parseToJsonElement(payload ?: "{}")

                            if ( !mappedPayload.jsonObject.isEmpty() ) {

                                val readings = mappedPayload.jsonObject["readings"] ?:
                                    Json.parseToJsonElement("{}")

                                if ( !readings.jsonObject.isEmpty()) {
                                    AppViewModel.processReadings(readings
                                                                            .jsonObject.toMap())

                                    // TODO: possibly save payload json to local storage
                                    // saving would be delegated to a diff class
                                }
                            }
                    }
                    .send()
            }
        }

    }
}