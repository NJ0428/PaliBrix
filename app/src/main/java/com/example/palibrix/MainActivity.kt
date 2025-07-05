package com.example.palibrix

import android.content.Context
import android.opengl.GLSurfaceView
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.MotionEvent
import android.widget.Button
import android.widget.FrameLayout
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

import android.os.Vibrator
import android.media.SoundPool
import android.media.AudioAttributes

class MainActivity : AppCompatActivity() {

    private lateinit var glSurfaceView: GLSurfaceView
    private lateinit var scoreText: TextView
    private lateinit var linesText: TextView
    private lateinit var gameOverLayout: android.widget.RelativeLayout
    private lateinit var pauseLayout: android.widget.RelativeLayout
    private var isPaused = false
    private lateinit var vibrator: Vibrator
    // private lateinit var soundPool: SoundPool
    // private var soundMap: HashMap<String, Int> = HashMap()
    
    private val updateHandler = Handler(Looper.getMainLooper())
    
    // UI update timer (100ms)
    private val uiUpdateRunnable = object : Runnable {
        override fun run() {
            updateUI()
            updateHandler.postDelayed(this, 100) // Update every 100ms
        }
    }
    
    // Game drop timer (300ms for fast dropping)
    private val gameDropRunnable = object : Runnable {
        override fun run() {
            if (!isPaused) {
                nativeUpdate() // Call native game update for automatic dropping
                updateHandler.postDelayed(this, 300) // Drop every 300ms (fast speed)
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Initialize UI elements
        scoreText = findViewById(R.id.tv_score)
        linesText = findViewById(R.id.tv_lines)
        gameOverLayout = findViewById(R.id.game_over_layout)
        pauseLayout = findViewById(R.id.pause_layout)
        vibrator = getSystemService(Context.VIBRATOR_SERVICE) as Vibrator

        /*
        val audioAttributes = AudioAttributes.Builder()
            .setUsage(AudioAttributes.USAGE_GAME)
            .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
            .build()

        soundPool = SoundPool.Builder()
            .setMaxStreams(5)
            .setAudioAttributes(audioAttributes)
            .build()

        soundMap["lock"] = soundPool.load(this, R.raw.lock, 1)
        soundMap["clear"] = soundPool.load(this, R.raw.clear, 1)
        soundMap["gameover"] = soundPool.load(this, R.raw.gameover, 1)
        */
        
        // Create and add GLSurfaceView to the container
        glSurfaceView = GameSurfaceView(this)
        val container = findViewById<FrameLayout>(R.id.gl_surface_container)
        container.addView(glSurfaceView)

        // Setup button listeners
        setupControlButtons()

        // Call the native C++ setup function
        nativeOnCreate()
        
        // Start both update loops
        updateHandler.post(uiUpdateRunnable)
        updateHandler.post(gameDropRunnable)

        findViewById<Button>(R.id.pause_button).setOnClickListener {
            togglePause()
        }

        findViewById<Button>(R.id.resume_button).setOnClickListener {
            togglePause()
        }
    }

    private fun setupControlButtons() {
        val moveHandler = Handler(Looper.getMainLooper())
        var moveRunnable: Runnable? = null

        val moveLeft = findViewById<android.view.View>(R.id.btn_left)
        val moveRight = findViewById<android.view.View>(R.id.btn_right)
        val softDrop = findViewById<android.view.View>(R.id.btn_down)

        val onTouchListener = { direction: Int ->
            object : android.view.View.OnTouchListener {
                override fun onTouch(v: android.view.View?, event: MotionEvent?): Boolean {
                    when (event?.action) {
                        MotionEvent.ACTION_DOWN -> {
                            moveRunnable = object : Runnable {
                                override fun run() {
                                    nativeMove(direction)
                                    moveHandler.postDelayed(this, 100) // 100ms 간격으로 반복
                                }
                            }
                            moveHandler.post(moveRunnable!!)
                        }
                        MotionEvent.ACTION_UP -> {
                            moveRunnable?.let { moveHandler.removeCallbacks(it) }
                        }
                    }
                    return true
                }
            }
        }

        moveLeft.setOnTouchListener(onTouchListener(-1))
        moveRight.setOnTouchListener(onTouchListener(1))

        softDrop.setOnTouchListener(object : android.view.View.OnTouchListener {
            override fun onTouch(v: android.view.View?, event: MotionEvent?): Boolean {
                when (event?.action) {
                    MotionEvent.ACTION_DOWN -> {
                        moveRunnable = object : Runnable {
                            override fun run() {
                                nativeSoftDrop()
                                moveHandler.postDelayed(this, 100)
                            }
                        }
                        moveHandler.post(moveRunnable!!)
                    }
                    MotionEvent.ACTION_UP -> {
                        moveRunnable?.let { moveHandler.removeCallbacks(it) }
                    }
                }
                return true
            }
        })

        findViewById<android.view.View>(R.id.btn_up).setOnClickListener {
            nativeHardDrop() // Hard drop on up
        }

        // Action buttons in diamond pattern
        findViewById<android.view.View>(R.id.btn_y).setOnClickListener {
            nativeRotateLeft() // Y: 왼쪽 돌리기 (반시계방향)
        }

        findViewById<android.view.View>(R.id.btn_x_left).setOnClickListener {
            nativeHold() // X (왼쪽): 홀드
        }

        findViewById<android.view.View>(R.id.btn_a).setOnClickListener {
            nativeRotate() // A: 오른쪽 돌리기 (시계방향)
        }

        findViewById<android.view.View>(R.id.btn_b).setOnClickListener {
            nativeHardDrop() // B: 빨리 내리기
        }

        findViewById<Button>(R.id.restart_button).setOnClickListener {
            nativeReset()
            gameOverLayout.visibility = android.view.View.GONE
        }
    }

    private fun updateUI() {
        // Get score and lines from native code
        val score = nativeGetScore()
        val lines = nativeGetLines()
        val isGameOver = nativeIsGameOver()

        scoreText.text = "Score: $score"
        linesText.text = "Lines: $lines"

        if (isGameOver) {
            gameOverLayout.visibility = android.view.View.VISIBLE
            updateHandler.removeCallbacks(gameDropRunnable)
        } else {
            gameOverLayout.visibility = android.view.View.GONE
        }
    }

    private fun togglePause() {
        isPaused = !isPaused
        if (isPaused) {
            updateHandler.removeCallbacks(gameDropRunnable)
            pauseLayout.visibility = android.view.View.VISIBLE
        } else {
            updateHandler.post(gameDropRunnable)
            pauseLayout.visibility = android.view.View.GONE
        }
    }

    override fun onResume() {
        super.onResume()
        glSurfaceView.onResume()
        isPaused = false
        pauseLayout.visibility = android.view.View.GONE
        updateHandler.post(gameDropRunnable)
    }

    override fun onPause() {
        super.onPause()
        glSurfaceView.onPause()
        isPaused = true
        updateHandler.removeCallbacks(gameDropRunnable)
    }

    override fun onDestroy() {
        super.onDestroy()
        updateHandler.removeCallbacks(uiUpdateRunnable)
        updateHandler.removeCallbacks(gameDropRunnable)
        // It's important to call the native cleanup function
        nativeOnDestroy()
    }

    fun vibrate(milliseconds: Long) {
        vibrator.vibrate(milliseconds)
    }

    // --- Native Methods ---
    private external fun nativeOnCreate()
    private external fun nativeOnSurfaceCreated()
    private external fun nativeOnSurfaceChanged(width: Int, height: Int)
    private external fun nativeOnDrawFrame()
    private external fun nativeOnDestroy()
    private external fun nativeUpdate()
    private external fun nativeMove(direction: Int)
    private external fun nativeRotate()
    private external fun nativeRotateLeft()
    private external fun nativeSoftDrop()
    private external fun nativeHardDrop()
    private external fun nativeHold()
    private external fun nativeReset()
    private external fun nativeGetScore(): Int
    private external fun nativeGetLines(): Int
    private external fun nativeIsGameOver(): Boolean

    companion object {
        init {
            System.loadLibrary("palibrix")
        }
    }

    // Inner class to access MainActivity's native methods
    internal inner class GameSurfaceView(context: Context) : GLSurfaceView(context) {
        private val renderer: GameRenderer

        init {
            setEGLContextClientVersion(3)
            renderer = GameRenderer()
            setRenderer(renderer)
            renderMode = RENDERMODE_CONTINUOUSLY
        }
    }

    internal inner class GameRenderer : GLSurfaceView.Renderer {
        override fun onSurfaceCreated(gl: GL10, config: EGLConfig) {
            nativeOnSurfaceCreated()
        }

        override fun onSurfaceChanged(gl: GL10, width: Int, height: Int) {
            nativeOnSurfaceChanged(width, height)
        }

        override fun onDrawFrame(gl: GL10) {
            nativeOnDrawFrame()
        }
    }
}

/* Duplicate top-level classes commented out because inner classes are already defined inside MainActivity

class GameSurfaceView(context: Context) : GLSurfaceView(context) {
    private val renderer: GameRenderer
    private val activity: MainActivity = context as MainActivity

    init {
        setEGLContextClientVersion(3)
        renderer = GameRenderer(activity)
        setRenderer(renderer)
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        if (event.action == MotionEvent.ACTION_DOWN) {
            val x = event.x
            val y = event.y
            val width = this.width.toFloat()
            val height = this.height.toFloat()

            if (y < height * 0.7f && x >= width * 0.2f && x <= width * 0.8f) {
                when {
                    y < height * 0.3f -> activity.nativeRotate()
                    else -> activity.nativeSoftDrop()
                }
                return true
            }
        }
        return super.onTouchEvent(event)
    }
}

class GameRenderer(private val activity: MainActivity) : GLSurfaceView.Renderer {
    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        activity.nativeOnSurfaceCreated()
    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        activity.nativeOnSurfaceChanged(width, height)
    }

    override fun onDrawFrame(gl: GL10?) {
        activity.nativeOnDrawFrame()
    }
}

*/