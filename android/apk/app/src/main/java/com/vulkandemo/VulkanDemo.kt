package com.vulkandemo

import android.view.Surface

object VulkanDemo {
    init {
        System.loadLibrary("vulkan_demo")
    }

    external fun nativeInit(surface: Surface, demoName: String): Boolean
    external fun nativeRender()
    external fun nativeCleanup()
    external fun nativeSurfaceDestroyed()
    external fun nativeGetDemoNames(): Array<String>
    external fun nativeGetDemoDescription(demoName: String): String
}