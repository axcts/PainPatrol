package com.group2.painpatrol.data

import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.os.Build

// As we will only have one type of notification,
// this function works specifically for creating the Notif Channel
// for Discomfort Alerts
private fun createNotificationChannel(context: Context) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        val channel = NotificationChannel(
            "painpatrol_discomfort_alerts",
            "Discomfort alerts",
            NotificationManager.IMPORTANCE_DEFAULT
        ).apply { description = "Alerts when values are outside of comfort range." }


        val notificationManager: NotificationManager = context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.createNotificationChannel(channel)
    }
}
