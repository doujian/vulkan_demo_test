package com.vulkandemo

import android.os.Bundle
import android.util.Log
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.appcompat.app.AppCompatActivity

class DemoActivity : AppCompatActivity() {
    
    companion object {
        private const val TAG = "DemoActivity"
    }
    
    private lateinit var surfaceView: SurfaceView
    private var demoName: String = "triangle"
    private var isInitialized = false
    private var renderThread: Thread? = null
    private var isRunning = false
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        demoName = intent.getStringExtra("demo_name") ?: "triangle"
        
        surfaceView = SurfaceView(this)
        surfaceView.holder.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                Log.d(TAG, "Surface created: $demoName")
            }
            
            override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
                Log.d(TAG, "Surface changed: $width x $height")
                if (!isInitialized) {
                    initVulkan(holder.surface)
                }
            }
            
            override fun surfaceDestroyed(holder: SurfaceHolder) {
                Log.d(TAG, "Surface destroyed")
                stopRendering()
                VulkanDemo.nativeSurfaceDestroyed()
            }
        })
        
        setContentView(surfaceView)
    }
    
    private fun initVulkan(surface: android.view.Surface) {
        val success = VulkanDemo.nativeInit(surface, demoName)
        if (success) {
            Log.d(TAG, "Vulkan initialized successfully")
            isInitialized = true
            startRendering()
        } else {
            Log.e(TAG, "Failed to initialize Vulkan")
            finish()
        }
    }
    
    private fun startRendering() {
        isRunning = true
        renderThread = Thread {
            while (isRunning) {
                VulkanDemo.nativeRender()
            }
        }.apply {
            name = "VulkanRenderThread"
            start()
        }
    }
    
    private fun stopRendering() {
        isRunning = false
        renderThread?.join(100)
        renderThread = null
    }
    
    override fun onPause() {
        super.onPause()
        stopRendering()
    }
    
    override fun onResume() {
        super.onResume()
        if (isInitialized && !isRunning) {
            startRendering()
        }
    }
    
    override fun onDestroy() {
        super.onDestroy()
        stopRendering()
        if (isInitialized) {
            VulkanDemo.nativeCleanup()
            isInitialized = false
        }
    }
}