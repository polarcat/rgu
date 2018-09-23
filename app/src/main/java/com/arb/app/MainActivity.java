package com.arb.app;

import lib3d.scene;

import android.os.Bundle;
import android.app.Activity;
import android.hardware.Camera;
import android.util.Log;
import android.view.SurfaceView;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.graphics.SurfaceTexture;

import java.io.IOException;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MainActivity extends Activity implements GLSurfaceView.Renderer
{
	private static final String TAG = "arb: ";
	private Camera camera_;
	private GLSurfaceView surface_;
	private SurfaceTexture texture_;

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		surface_ = new GLSurfaceView(this);
		surface_.setPreserveEGLContextOnPause(true);
		surface_.setEGLContextClientVersion(2);
		surface_.setEGLConfigChooser(8, 8, 8, 8, 16, 0);
		surface_.setRenderer(this);
		surface_.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);

		setContentView(surface_);

//		Log.i(TAG, "external storage: " + android.os.Environment.getExternalStorageDirectory());
	}

	@Override
	protected void onResume()
	{
		super.onResume();
		scene.resume(getApplicationContext(), this);
		surface_.onResume();
		startCamera();
	}

	@Override
	protected void onPause()
	{
		super.onPause();
		camera_.stopPreview();
		camera_.release();
		scene.pause();
		surface_.onPause();
	}

	@Override
	protected void onDestroy()
	{
		super.onDestroy();
		camera_.stopPreview();
		camera_.release();
		scene.close();
	}

	private void startCamera()
	{
		synchronized(this) {
			camera_ = Camera.open();
			Camera.Parameters p = camera_.getParameters();
			p.setRotation(90);
			camera_.setParameters(p);

			try {
				camera_.setPreviewTexture(texture_);
			} catch (IOException e) {
				e.printStackTrace();
			}

			camera_.startPreview();
		}
	}

	@Override
	public void onSurfaceCreated(GL10 gl, EGLConfig cfg)
	{
		texture_ = new SurfaceTexture(scene.open(getAssets()));
		startCamera();
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
		texture_.updateTexImage();
		scene.render();
	}
}
