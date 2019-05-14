package rgs.circles;

import lib3d.scene;

import android.content.pm.ActivityInfo;
import android.content.res.AssetFileDescriptor;
import android.os.Bundle;
import android.app.Activity;
import android.util.Log;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.graphics.SurfaceTexture;
import android.view.SurfaceView;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.media.SoundPool;
import android.media.AudioAttributes;
import android.media.AudioManager;

import java.io.IOException;
import java.nio.ByteBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MainActivity extends Activity implements GLSurfaceView.Renderer
{
	/* keep these in sync with native code */

	private static final int SND_BUTTON_TOUCH = 0;
	private static final int SND_PLAYER_MOVE = 1;
	private static final int SND_PLAYER_BOUNCE = 2;
	private static final int SND_TARGET_BOUNCE = 3;
	private static final int SND_TARGET_GROUND = 4;
	private static final int SND_TARGET_HIDE = 5;
	private static final int SND_MAX = 6;

	private int sounds_[];
	private static final String TAG = "rgs: ";
	private GLSurfaceView surface_;
	private SoundPool audio_;

	private void setFullscreen()
	{
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
		  WindowManager.LayoutParams.FLAG_FULLSCREEN);

		final int flags = View.SYSTEM_UI_FLAG_LAYOUT_STABLE
		  | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
		  | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
		  | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
		  | View.SYSTEM_UI_FLAG_FULLSCREEN
		  | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;

		getWindow().getDecorView().setSystemUiVisibility(flags);
	}

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		setFullscreen();
		setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

		surface_ = new GLSurfaceView(this);
		surface_.setPreserveEGLContextOnPause(true);
		surface_.setEGLContextClientVersion(2);
		surface_.setEGLConfigChooser(8, 8, 8, 8, 16, 0);
		surface_.setRenderer(this);
		surface_.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);

		setContentView(surface_);
		initSounds();
	}

	@Override
	protected void onResume()
	{
		super.onResume();

		getWindow().getDecorView().setSystemUiVisibility(
		    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
		  | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
		  | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
		  | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
		  | View.SYSTEM_UI_FLAG_FULLSCREEN
		  | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
		setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

		scene.resume(getApplicationContext(), this);
		surface_.onResume();
	}

	@Override
	protected void onPause()
	{
		super.onPause();
		scene.pause();
		surface_.onPause();
	}

	@Override
	protected void onDestroy()
	{
		super.onDestroy();
		audio_.release();
		scene.close();
	}

	@Override
	public void onSurfaceCreated(GL10 gl, EGLConfig cfg)
	{
		scene.open(getAssets());
	}

	@Override
	public void onSurfaceChanged(GL10 gl, int w, int h)
	{
		scene.resize(w, h);
		scene.rotate(getWindowManager().getDefaultDisplay().getRotation());
	}

	@Override
	public void onDrawFrame(GL10 gl)
	{
		int sounds = scene.render();
	}

	@Override
	public boolean onTouchEvent(MotionEvent e)
	{
		switch (e.getAction()) {
		case MotionEvent.ACTION_MOVE: /* fall through */
		case MotionEvent.ACTION_DOWN:
			if (e.getAction() == MotionEvent.ACTION_DOWN)
				playSound(0);

			scene.input(e.getX(), e.getY());
			break;
		}

		return true;
	}

	private void initSounds()
	{
		Log.i(TAG, "init sounds");

		AudioAttributes attrs = new AudioAttributes.Builder()
		  .setUsage(AudioAttributes.USAGE_GAME)
		  .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
		  .build();
		audio_ = new SoundPool.Builder()
		  .setMaxStreams(SND_MAX)
		  .setAudioAttributes(attrs)
		  .build();

		audio_.setOnLoadCompleteListener(new SoundPool.OnLoadCompleteListener() {
			@Override
			public void onLoadComplete(SoundPool sp, int id, int status) {
				Log.i(TAG, "loaded sound " + id + " | status " + status);
			}
		});

		setVolumeControlStream(AudioManager.STREAM_MUSIC);
		sounds_ = new int[SND_MAX];

		AssetFileDescriptor afd;

//		sounds_[SND_BUTTON_TOUCH] = audio_.load("/sdcard/sounds/touch_btn.ogg", 1);

		try {
			afd = getAssets().openFd("sounds/touch_btn.ogg");
			sounds_[SND_BUTTON_TOUCH] = audio_.load(afd, 1);
		}  catch (IOException e) {
			e.printStackTrace();
		}
	}

	private void playSound(int id)
	{
		if (id < 0 || id >= SND_MAX)
			return;

		Log.i(TAG, "play sound " + id);
		audio_.play(sounds_[id], 1, 1, 1, 0, 1.f);
	}
}
