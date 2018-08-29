/*
 * Copyright (c) 2015, Nordic Semiconductor
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 *  Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 *  Neither the name of copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package tech.detourne.sonic;

import android.app.Activity;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.arch.lifecycle.ViewModel;
import android.arch.lifecycle.ViewModelProviders;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.graphics.PorterDuff;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;
import android.provider.OpenableColumns;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import org.apache.commons.io.IOUtils;

import java.io.BufferedInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.Socket;
import java.net.URL;
import java.net.URLConnection;
import java.net.UnknownHostException;

import tech.detourne.sonic.adapter.ExtendedBluetoothDevice;
import tech.detourne.sonic.profile.OnTaskCompleted;
import tech.detourne.sonic.utils.Utils;
import tech.detourne.sonic.viewmodels.DetourneViewModel;

@SuppressWarnings("ConstantConditions")
public class DetourneActivity extends AppCompatActivity implements OnTaskCompleted {
	public static final String TAG = "Sonic";
	public static final String EXTRA_DEVICE = "tech.detourne.sonic.EXTRA_DEVICE";
	private static final int READ_REQUEST_CODE = 42;
	private ProgressDialog pDialog;
	public static final int progress_bar_type = 0;
	private WifiReceiver mWifiReceiver;
	IntentFilter mWifiIntentFilter;
	private int connCount;

	@Override
	protected void onCreate(final Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_blinky);

		final Intent intent = getIntent();
		final ExtendedBluetoothDevice device = intent.getParcelableExtra(EXTRA_DEVICE);
		final String deviceName = device.getName();
		final String deviceAddress = device.getAddress();

		/* instantiate intent filter */
		mWifiIntentFilter = new IntentFilter();
		/* add action indicating there was a change on the state of wifi */
		mWifiIntentFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
		mWifiReceiver = new WifiReceiver();
		connCount = 0;


		final Toolbar toolbar = findViewById(R.id.toolbar);
		setSupportActionBar(toolbar);
		getSupportActionBar().setTitle(deviceName);
		getSupportActionBar().setSubtitle(deviceAddress);
		getSupportActionBar().setDisplayHomeAsUpEnabled(true);

		// Configure the view model
		final DetourneViewModel viewModel =
				ViewModelProviders.of(this).get(DetourneViewModel.class);
		viewModel.connect(device);

		// Set up views
		final TextView buzzState = findViewById(R.id.led_state);
		final Switch buzzSwitch = findViewById(R.id.led_switch);
		//final TextView buttonState = findViewById(R.id.button_state);
		final LinearLayout progressContainer = findViewById(R.id.progress_container);
		final TextView connectionState = findViewById(R.id.connection_state);
		final View content = findViewById(R.id.device_container);
		final SeekBar fixedInt = findViewById(R.id.fixedint_seekbar);
		final TextView fixedIntText = findViewById(R.id.fixedint_text);
		final SeekBar randInt = findViewById(R.id.randomint_seekbar);
		final TextView randIntText = findViewById(R.id.randomint_text);
		final SeekBar solarMin = findViewById(R.id.solarthresh_seekbar);
		final TextView solarMinText = findViewById(R.id.solarthresh_text);
		final SeekBar rssiMin = findViewById(R.id.rssithresh_seekbar);
		final TextView rssiMinText = findViewById(R.id.rssithresh_text);
		final ImageView fwStatusView = findViewById(R.id.download_status);
		final TextView rssiValText = findViewById(R.id.rssival_view);
		final TextView solarValText = findViewById(R.id.solarval_view);

		final Button dlButton = findViewById(R.id.button_id);
		final Button upButton = findViewById(R.id.button_id2);
		final Button chButton = findViewById(R.id.button_id3);

		final TextView fname_text = findViewById(R.id.fname_text);
		final TextView fnamesha_text = findViewById(R.id.fnamesha_text);
		final TextView fnamesize_text = findViewById(R.id.fname_size);

		fwStatusView.setVisibility(View.GONE);

		buzzSwitch.setOnClickListener(view -> viewModel.toggleBuzz(buzzSwitch.isChecked()));

		dlButton.setOnClickListener(view -> {
			Toast.makeText(this, R.string.downloading, Toast.LENGTH_SHORT).show();
			fwStatusView.setVisibility(View.GONE);
			final String remoteBase = "https://detourne.tech/fw";
			final String binFileName = "dbuzz.bin";
			final String shaFileName = "dbuzz.sha256";

			DownloadFileFromURL dlfu1 = new DownloadFileFromURL(DetourneActivity.this);
			dlfu1.execute(remoteBase + "/" + binFileName);
			//DownloadFileFromURL dlfu2 = new DownloadFileFromURL(DetourneActivity.this);
			//dlfu2.execute(remoteBase + "/" + shaFileName);

		});
		upButton.setOnClickListener(view -> {
			viewModel.flashTask(this);
		});
		chButton.setOnClickListener(view -> {
			Intent intent_open = new Intent(Intent.ACTION_OPEN_DOCUMENT);
			intent_open.addCategory(Intent.CATEGORY_OPENABLE);
			intent_open.setType("*/*");

			startActivityForResult(intent_open, READ_REQUEST_CODE);
		});

		viewModel.isDeviceReady().observe(this, deviceReady -> {
			progressContainer.setVisibility(View.GONE);
			content.setVisibility(View.VISIBLE);
		});

		viewModel.getConnectionState().observe(this, connectionState::setText);

		viewModel.isConnected().observe(this, connected -> {
			if (!connected) {
				Toast.makeText(this, R.string.state_disconnected, Toast.LENGTH_SHORT).show();
				finish();
			}
		});

		viewModel.isSupported().observe(this, supported -> {
			if (!supported) {
				Toast.makeText(this, R.string.state_not_supported, Toast.LENGTH_SHORT).show();
			}
		});

		viewModel.getFileName().observe(this, fname -> {
			fname_text.setText(getString(R.string.fname_holder, fname));

		});

		viewModel.getSHA256String().observe(this, sha -> {
			fnamesha_text.setText(getString(R.string.fnamesha_holder, sha));
		});

		viewModel.getFWSize().observe(this,mysize -> {
			fnamesize_text.setText(getString(R.string.fnamesize_holder,mysize));
		});

		viewModel.getFirmwareDownloadState().observe( this, good -> {
			fwStatusView.setVisibility(View.VISIBLE);
			if (good) {
				Toast.makeText(this, R.string.download_good, Toast.LENGTH_SHORT).show();
				fwStatusView.setColorFilter(ContextCompat.getColor(this, android.R.color.holo_green_dark),
						PorterDuff.Mode.MULTIPLY);
			} else {
				Toast.makeText(this, R.string.download_fail, Toast.LENGTH_SHORT).show();
				fwStatusView.setColorFilter(ContextCompat.getColor(this, android.R.color.holo_red_dark),
						PorterDuff.Mode.MULTIPLY);
			}
		});

		viewModel.getBuzzState().observe(this, isOn -> {
			buzzState.setText(isOn ? R.string.turn_on : R.string.turn_off);
			buzzSwitch.setChecked(isOn);
		});
		//viewModel.getButtonState().observe(this, pressed ->
		// buttonState.setText(pressed ? R.string.button_pressed : R.string.button_released));


		viewModel.getRSSIVal().observe(this, val -> {
			rssiValText.setText(getString(R.string.rssi_reading,val));
		});

		viewModel.getSolarVal().observe(this, val -> {
			solarValText.setText(getString(R.string.solar_reading,val));
		});

		viewModel.getFixedIntVal().observe(this, val -> {
			fixedIntText.setText(getString(R.string.fixed_int_indicator,val));
		});

		fixedInt.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

			public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
				byte val = (byte)progress;
				//put in loop mode if slider is left somewhere > 0, idle mode if left at 0
				byte newMode = (val == (byte)0x00) ? (byte)0x00 : (byte)0x02;
				viewModel.updateMode(newMode); //mode loop
				viewModel.updateFixedIntVal(val);
			}

			public void onStartTrackingTouch(SeekBar arg0) {}

			public void onStopTrackingTouch(SeekBar seekBar) {
				viewModel.sendModeVal();
				viewModel.sendFixedIntVal();
			}
		});

		viewModel.getRandIntVal().observe(this, val -> {
			randIntText.setText(getString(R.string.rand_int_indicator,val));
		});

		// connect to AP as soon as password arrives from device
		viewModel.getPassVal().observe(this, pass ->{
			String networkSSID = "DTCAP"; //user pref
			Utils.connectToAP(this,networkSSID, pass);
		});

		randInt.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

			public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
				byte val = (byte)progress;
				//put in loop mode if slider is left somewhere > 0, idle mode if left at 0
				byte newMode = (val == (byte)0x00) ? (byte)0x00 : (byte)0x02;
				viewModel.updateMode(newMode); //mode loop
				viewModel.updateRandIntVal(val);
			}

			public void onStartTrackingTouch(SeekBar arg0) {
				//do something
			}

			public void onStopTrackingTouch(SeekBar seekBar) {
				viewModel.sendModeVal();
				viewModel.sendRandomIntVal();
			}
		});

		viewModel.getSolarMinVal().observe(this, val -> {
			solarMinText.setText(getString(R.string.solar_indicator,val));
		});

		solarMin.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

			public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
				byte val = (byte)progress;
				//put in loop mode if slider is left somewhere > 0, idle mode if left at 0
				byte newMode = (val == (byte)0x00) ? (byte)0x00 : (byte)0x02;
				viewModel.updateMode(newMode); //mode loop
				viewModel.updateSolarMinVal(val);
			}

			public void onStartTrackingTouch(SeekBar arg0) {
				//do something
			}

			public void onStopTrackingTouch(SeekBar seekBar) {
				viewModel.sendModeVal();
				viewModel.sendSolarMinVal();
			}
		});

		viewModel.getRSSIMinVal().observe(this, val -> {
			rssiMinText.setText(getString(R.string.rssi_indicator,val));
		});

		rssiMin.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

			public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
				byte val = (byte)progress;
				//put in loop mode if slider is left somewhere > 0, idle mode if left at 0
				byte newMode = (val == (byte)0x00) ? (byte)0x00 : (byte)0x02;
				viewModel.updateMode(newMode); //mode loop
				viewModel.updateRSSIMinVal(val);
			}

			public void onStartTrackingTouch(SeekBar arg0) {
				//do something
			}

			public void onStopTrackingTouch(SeekBar seekBar) {
				viewModel.sendModeVal();
				viewModel.sendRSSIMinVal();
			}
		});

	}

	@Override
	protected void onResume() {
		super.onResume();
		/* register our receiver */
		registerReceiver(mWifiReceiver, mWifiIntentFilter);
	}

	@Override
	protected void onPause() {
		super.onPause();
		unregisterReceiver(mWifiReceiver);
	}


	@Override
	public boolean onOptionsItemSelected(final MenuItem item) {
		switch (item.getItemId()) {
			case android.R.id.home:
				onBackPressed();
				return true;
		}
		return false;
	}

	@Override
	public void onActivityResult(int requestCode, int resultCode,
								 Intent resultData) {
		if (requestCode == READ_REQUEST_CODE && resultCode == Activity.RESULT_OK) {
			Uri uri = null;
			if (resultData != null) {
				final DetourneViewModel viewModel =
						ViewModelProviders.of(this).get(DetourneViewModel.class);
				try {

					Uri returnUri = resultData.getData();
					String path = Utils.getRealPathFromURI(this, returnUri);

					viewModel.setFWFile(path);
					Log.i(TAG, "Path: " + path);
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
		}
	}

	@Override
	public void onDownloadTaskCompleted(String response) {
		Log.i("task completed", "file:" + response);
		final DetourneViewModel viewModel =
				ViewModelProviders.of(this).get(DetourneViewModel.class);
		viewModel.setFWFile(response);
	}

	@Override
	public void onUploadTaskCompleted(String response) {
		Log.i("task completed", "file:" + response);
		final DetourneViewModel viewModel =
				ViewModelProviders.of(this).get(DetourneViewModel.class);

	}

	@Override
	protected Dialog onCreateDialog(int id) {
		switch (id) {
			case progress_bar_type: // we set this to 0
				pDialog = new ProgressDialog(this);
				pDialog.setMessage(getString(R.string.update_processing_message));
				pDialog.setIndeterminate(false);
				pDialog.setMax(100);
				pDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
				pDialog.setCancelable(true);
				pDialog.show();
				return pDialog;
			default:
				return null;
		}
	}

	/**
	 * Background Async Task to download file
	 * */
	public class DownloadFileFromURL extends AsyncTask<String, String, String> {

		String s = "";
		private OnTaskCompleted taskCompleted;

		public DownloadFileFromURL(OnTaskCompleted activityContext){
			this.taskCompleted = activityContext;
		}

		/**
		 * Before starting background thread Show Progress Bar Dialog
		 * */
		@Override
		protected void onPreExecute() {
			super.onPreExecute();
			publishProgress("" + (int) (0));
			showDialog(progress_bar_type);
		}

		/**
		 * Downloading file in background thread
		 * */
		@Override
		protected String doInBackground(String... f_url) {
			int count;
			try {
				URL url = new URL(f_url[0]);
				URLConnection connection = url.openConnection();
				connection.connect();

				int lengthOfFile = connection.getContentLength();

				// download the file
				InputStream input = new BufferedInputStream(url.openStream(),
						8192);

				String urlStr = url.toString();
				String fileName = urlStr.substring( urlStr.lastIndexOf('/')+1, urlStr.length() );
				String path = Environment.getExternalStorageDirectory().toString() + "/" +
						Environment.DIRECTORY_DOWNLOADS;

				OutputStream output = new FileOutputStream( path + "/" + fileName);
				s = path + "/" + fileName;

				byte data[] = new byte[1024];
				long total = 0;
				while ((count = input.read(data)) != -1) {
					total += count;
					publishProgress("" + (int) ((total * 100) / lengthOfFile));
					output.write(data, 0, count);
				}

				output.flush();
				output.close();
				input.close();

			} catch (Exception e) {
				Log.e("Error: ", e.getMessage());
			}

			return null;
		}

		/**
		 * Updating progress bar
		 * */
		protected void onProgressUpdate(String... progress) {
			// setting progress percentage
			pDialog.setProgress(Integer.parseInt(progress[0]));
		}

		/**
		 * After completing background task Dismiss the progress dialog
		 * **/
		@Override
		protected void onPostExecute(String file_url) {
			// dismiss the dialog after the file was downloaded
			dismissDialog(progress_bar_type);
			taskCompleted.onDownloadTaskCompleted(s);
		}
	}

	/**
	 * Background Async Task to upload file
	 * */
	public class UploadFileToDevice extends AsyncTask<String, String, String> {

		String s = "";
		private OnTaskCompleted taskCompleted;

		public UploadFileToDevice(OnTaskCompleted activityContext){
			this.taskCompleted = activityContext;
		}

		/**
		 * Before starting background thread Show Progress Bar Dialog
		 * */
		@Override
		protected void onPreExecute() {
			super.onPreExecute();
			publishProgress("" + (int) (0));
			showDialog(progress_bar_type);
		}

		/**
		 * Uploading file in background thread
		 * */
		@Override
		protected String doInBackground(String... f_path) {
			int byteCount;
			Socket socket;
			int tryCount = 0;
			int maxTries = 3;
			while (true) {
				try {
					Thread.sleep(8000);
					InetAddress serverAddr = InetAddress.getByName("192.168.1.1"); // user pref
					socket = new Socket(serverAddr, 5000);
					socket.setReuseAddress(true);

					File file = new File(f_path[0]);
					long length = file.length();
					Log.d("filelentag", "file is len " + length);
					byte[] bytes = new byte[8 * 1024];
					InputStream in = new FileInputStream(file);
					Thread.sleep(1000);

					//OutputStream os = socket.getOutputStream();
					DataOutputStream outToServer = new DataOutputStream(socket.getOutputStream());
					outToServer.writeLong(length);

					long total = 0;
					while ((byteCount = in.read(bytes)) > 0) {
						total += byteCount;
						publishProgress("" + (int) ((total * 100) / length));
						outToServer.write(bytes, 0, byteCount);
					}

					//outToServer.write(data,0,data.length);
					in.close();
					outToServer.close();
					socket.close();
					return null;

				} catch (UnknownHostException e) {
					e.printStackTrace();
				} catch (IOException e) {
					e.printStackTrace();
					if (++tryCount == maxTries) {
						Log.d("socketexception", "e");
						return null;
					}
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			}
		}

		/**
		 * Updating progress bar
		 * */
		protected void onProgressUpdate(String... progress) {
			// setting progress percentage
			pDialog.setProgress(Integer.parseInt(progress[0]));
		}

		/**
		 * After completing background task Dismiss the progress dialog
		 * **/
		@Override
		protected void onPostExecute(String file_url) {
			// dismiss the dialog after the file was downloaded
			dismissDialog(progress_bar_type);
			taskCompleted.onUploadTaskCompleted(s);
		}
	}

	public class WifiReceiver extends BroadcastReceiver {
		public void onReceive(Context c, Intent intent) {
			if (intent.getAction().equals(WifiManager.NETWORK_STATE_CHANGED_ACTION)) {
				NetworkInfo networkInfo = intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
				NetworkInfo.State state = networkInfo.getState();
				if (ConnectivityManager.TYPE_WIFI == networkInfo.getType()) {
					if (state == NetworkInfo.State.CONNECTED) {
						WifiManager wifiManager = (WifiManager) getApplicationContext().
								getSystemService(Context.WIFI_SERVICE);
						WifiInfo info = wifiManager.getConnectionInfo();
						String ssid = info.getSSID();
						//changeUIToConnected();
						Log.d(TAG, "someone connected: " + ssid);
						boolean ours = ssid.contains("DTCAP");
						if(ours)
							DetourneActivity.this.connCount += 1;

						//TODO connect events come in pairs
						if (ours && DetourneActivity.this.connCount % 2 == 0) {
							try {
								Thread.sleep(1000);
								Log.d(TAG, "Connected to DTC!");
								final DetourneViewModel viewModel =
										ViewModelProviders.of(DetourneActivity.this)
												.get(DetourneViewModel.class);
								UploadFileToDevice uf2d =
										new UploadFileToDevice(DetourneActivity.this);
								Log.d(TAG,"putting: " + viewModel.getFileName().getValue());
								uf2d.execute(viewModel.getFileName().getValue());
							} catch (InterruptedException e) {
								e.printStackTrace();
							}

						} else if (state == NetworkInfo.State.DISCONNECTED) {
							//if (!tryReconnect) {
							Log.d(TAG, "Network disconnected");

						}
						// Check if connection to the right network (osselet) during connection
						else if (state == NetworkInfo.State.DISCONNECTING) {
							Log.d(TAG, "Disconnecting to " + networkInfo.toString());
						}
					}
				}
			}
		}
	}

}
