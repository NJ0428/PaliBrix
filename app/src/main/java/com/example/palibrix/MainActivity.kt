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
import android.media.MediaPlayer

class MainActivity : AppCompatActivity() {

    private lateinit var glSurfaceView: GLSurfaceView
    private lateinit var scoreText: TextView
    private lateinit var linesText: TextView
    private lateinit var levelText: TextView
    private lateinit var gameOverLayout: android.widget.RelativeLayout
    private lateinit var pauseLayout: android.widget.RelativeLayout
    private var isPaused = false
    private lateinit var vibrator: Vibrator
    private var currentLevel = 1
    private var baseDropSpeed = 150L // 기본 속도 (ms)
    private var backgroundMusic: MediaPlayer? = null
    private lateinit var soundPool: SoundPool
    private var soundMap: HashMap<String, Int> = HashMap()
    private var gameOverSoundPlayed = false
    
    private val updateHandler = Handler(Looper.getMainLooper())
    
    // UI update timer (100ms)
    private val uiUpdateRunnable = object : Runnable {
        override fun run() {
            updateUI()
            updateHandler.postDelayed(this, 100) // Update every 100ms
        }
    }
    
    // Game drop timer (dynamic speed based on level)
    private val gameDropRunnable = object : Runnable {
        override fun run() {
            if (!isPaused) {
                nativeUpdate() // Call native game update for automatic dropping
                val dropSpeed = calculateDropSpeed()
                updateHandler.postDelayed(this, dropSpeed) // Drop speed based on level
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Initialize UI elements
        scoreText = findViewById(R.id.tv_score)
        linesText = findViewById(R.id.tv_lines)
        levelText = findViewById(R.id.tv_level)
        gameOverLayout = findViewById(R.id.game_over_layout)
        pauseLayout = findViewById(R.id.pause_layout)
        vibrator = getSystemService(Context.VIBRATOR_SERVICE) as Vibrator

        // 효과음 초기화
        val audioAttributes = AudioAttributes.Builder()
            .setUsage(AudioAttributes.USAGE_GAME)
            .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
            .build()

        soundPool = SoundPool.Builder()
            .setMaxStreams(5)
            .setAudioAttributes(audioAttributes)
            .build()

        soundMap["rotate"] = soundPool.load(this, R.raw.re, 1)
        soundMap["gameover"] = soundPool.load(this, R.raw.fail, 1)
        soundMap["click"] = soundPool.load(this, R.raw.click, 1)
        soundMap["hold"] = soundPool.load(this, R.raw.hole, 1)
        soundMap["lock"] = soundPool.load(this, R.raw.down, 1)
        soundMap["clear"] = soundPool.load(this, R.raw.del, 1)
        
        // 배경음악 초기화
        initBackgroundMusic()
        
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
            playSoundEffect("click")
            togglePause()
        }

        findViewById<Button>(R.id.resume_button).setOnClickListener {
            playSoundEffect("click")
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
            nativeHold() // Y: 홀드
            playSoundEffect("hold")
        }

        findViewById<android.view.View>(R.id.btn_x_left).setOnClickListener {
            nativeHold() // X (왼쪽): 홀드
            playSoundEffect("hold")
        }

        findViewById<android.view.View>(R.id.btn_a).setOnClickListener {
            nativeRotate() // A: 우회전 (시계방향)
            playSoundEffect("rotate") // 회전 효과음 재생
        }

        findViewById<android.view.View>(R.id.btn_b).setOnClickListener {
            nativeRotateLeft() // B: 좌회전 (반시계방향)
            playSoundEffect("rotate") // 회전 효과음 재생
        }

        findViewById<Button>(R.id.restart_button).setOnClickListener {
            playSoundEffect("click")
            nativeReset()
            currentLevel = 1 // 레벨 초기화
            gameOverSoundPlayed = false // 게임 오버 효과음 플래그 초기화
            gameOverLayout.visibility = android.view.View.GONE
            startBackgroundMusic() // 게임 재시작 시 배경음악 재생
        }
    }

    private fun calculateDropSpeed(): Long {
        // 레벨마다 10ms씩 빨라짐 (최소 50ms)
        val speed = baseDropSpeed - (currentLevel - 1) * 10
        return if (speed < 50) 50 else speed
    }

    private fun initBackgroundMusic() {
        try {
            backgroundMusic = MediaPlayer.create(this, R.raw.background)
            backgroundMusic?.apply {
                isLooping = true // 반복 재생 설정
                setVolume(0.5f, 0.5f) // 볼륨 설정 (0.0 ~ 1.0)
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun startBackgroundMusic() {
        try {
            backgroundMusic?.let { player ->
                if (!player.isPlaying) {
                    player.start()
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun pauseBackgroundMusic() {
        try {
            backgroundMusic?.let { player ->
                if (player.isPlaying) {
                    player.pause()
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun stopBackgroundMusic() {
        try {
            backgroundMusic?.let { player ->
                if (player.isPlaying) {
                    player.stop()
                    player.prepare() // 다시 재생 가능하도록 준비
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun playSoundEffect(soundName: String) {
        try {
            soundMap[soundName]?.let { soundId ->
                soundPool.play(soundId, 1.0f, 1.0f, 1, 0, 1.0f)
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun updateUI() {
        // Get score and lines from native code
        val score = nativeGetScore()
        val lines = nativeGetLines()
        val isGameOver = nativeIsGameOver()

        // 레벨 계산 (10줄마다 레벨 업)
        val newLevel = (lines / 10) + 1
        if (newLevel != currentLevel) {
            currentLevel = newLevel
            // 레벨이 올라갔을 때 진동 효과
            vibrator.vibrate(100)
        }

        scoreText.text = "Score: $score"
        linesText.text = "Lines: $lines"
        levelText.text = "Level: $currentLevel"

        if (isGameOver) {
            gameOverLayout.visibility = android.view.View.VISIBLE
            updateHandler.removeCallbacks(gameDropRunnable)
            stopBackgroundMusic() // 게임 오버 시 음악 정지
            if (!gameOverSoundPlayed) {
                playSoundEffect("gameover") // 게임 오버 효과음 재생
                gameOverSoundPlayed = true
            }
        } else {
            gameOverLayout.visibility = android.view.View.GONE
            gameOverSoundPlayed = false // 게임 오버 상태가 아니면 플래그 초기화
        }
    }

    private fun togglePause() {
        isPaused = !isPaused
        if (isPaused) {
            updateHandler.removeCallbacks(gameDropRunnable)
            pauseLayout.visibility = android.view.View.VISIBLE
            pauseBackgroundMusic() // 일시정지 시 음악 일시정지
        } else {
            updateHandler.post(gameDropRunnable)
            pauseLayout.visibility = android.view.View.GONE
            startBackgroundMusic() // 재개 시 음악 재생
        }
    }

    override fun onResume() {
        super.onResume()
        glSurfaceView.onResume()
        isPaused = false
        pauseLayout.visibility = android.view.View.GONE
        updateHandler.post(gameDropRunnable)
        startBackgroundMusic() // 게임 재개 시 음악 재생
    }

    override fun onPause() {
        super.onPause()
        glSurfaceView.onPause()
        isPaused = true
        updateHandler.removeCallbacks(gameDropRunnable)
        pauseBackgroundMusic() // 게임 일시정지 시 음악 일시정지
    }

    override fun onDestroy() {
        super.onDestroy()
        updateHandler.removeCallbacks(uiUpdateRunnable)
        updateHandler.removeCallbacks(gameDropRunnable)
        
        // 배경음악 정리
        backgroundMusic?.let { player ->
            if (player.isPlaying) {
                player.stop()
            }
            player.release()
        }
        backgroundMusic = null

        // 효과음 정리
        soundPool.release()
        
        // It's important to call the native cleanup function
        nativeOnDestroy()
    }

    fun vibrate(milliseconds: Long) {
        vibrator.vibrate(milliseconds)
    }

    fun playLockSound() {
        playSoundEffect("lock")
    }

    fun playLineClearSound() {
        playSoundEffect("clear")
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