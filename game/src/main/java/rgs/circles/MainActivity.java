package rgs.circles;

import lib3d.scene;

import android.content.pm.ActivityInfo;
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

import java.io.IOException;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MainActivity extends Activity implements GLSurfaceView.Renderer
{
	private static final String TAG = "rgs: ";
	private GLSurfaceView surface_;

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
		scene.render();
	}

	@Override
	public boolean onTouchEvent(MotionEvent e)
	{
		switch (e.getAction()) {
		case MotionEvent.ACTION_MOVE: /* fall through */
		case MotionEvent.ACTION_DOWN:
			scene.input(e.getX(), e.getY());
			break;
		}

		return true;
	}
}
