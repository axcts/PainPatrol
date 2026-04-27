package com.group2.painpatrol.data

import com.group2.painpatrol.ui.AppViewModel
import com.hivemq.client.mqtt.MqttClient

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
                println("MQTT failed to connect")
            } else {
                client.subscribeWith()
                    .topicFilter("painpatrol/readings")
                    .callback {
                        publish ->
                            payload = (publish.payloadAsBytes).toString(Charsets.UTF_8)
                            AppViewModel.processReading(payload ?: "N/A")
                    }
                    .send()
            }
        }

    }
}