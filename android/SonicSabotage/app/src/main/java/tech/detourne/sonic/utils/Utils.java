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

package tech.detourne.sonic.utils;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.provider.MediaStore;
import android.provider.Settings;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.util.Log;

import org.apache.commons.io.FileUtils;
import org.apache.commons.io.IOUtils;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.InetAddress;
import java.net.Socket;
import java.net.SocketException;
import java.net.URL;
import java.net.URLConnection;
import java.net.UnknownHostException;
import java.security.MessageDigest;
import java.util.List;

public class Utils {
	private static final String PREFS_LOCATION_NOT_REQUIRED = "location_not_required";
	private static final String PREFS_PERMISSION_REQUESTED = "permission_requested";

	/**
	 * Checks whether Bluetooth is enabled.
	 * @return true if Bluetooth is enabled, false otherwise.
	 */
	public static boolean isBleEnabled() {
		final BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
		return adapter != null && adapter.isEnabled();
	}

	/**
	 * Checks for required permissions.
	 *
	 * @return true if permissions are already granted, false otherwise.
	 */
	public static boolean isLocationPermissionsGranted(final Context context) {
		return ContextCompat.checkSelfPermission(context, Manifest.permission.ACCESS_COARSE_LOCATION) == PackageManager.PERMISSION_GRANTED;
	}

	/**
	 * Returns true if location permission has been requested at least twice and
	 * user denied it, and checked 'Don't ask again'.
	 * @param activity the activity
	 * @return true if permission has been denied and the popup will not come up any more, false otherwise
	 */
	public static boolean isLocationPermissionDeniedForever(final Activity activity) {
		final SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(activity);

		return !isLocationPermissionsGranted(activity) // Location permission must be denied
				&& preferences.getBoolean(PREFS_PERMISSION_REQUESTED, false) // Permission must have been requested before
				&& !ActivityCompat.shouldShowRequestPermissionRationale(activity, Manifest.permission.ACCESS_COARSE_LOCATION); // This method should return false
	}

	/**
	 * On some devices running Android Marshmallow or newer location services must be enabled in order to scan for Bluetooth LE devices.
	 * This method returns whether the Location has been enabled or not.
	 *
	 * @return true on Android 6.0+ if location mode is different than LOCATION_MODE_OFF. It always returns true on Android versions prior to Marshmallow.
	 */
	public static boolean isLocationEnabled(final Context context) {
		if (isMarshmallowOrAbove()) {
			int locationMode = Settings.Secure.LOCATION_MODE_OFF;
			try {
				locationMode = Settings.Secure.getInt(context.getContentResolver(), Settings.Secure.LOCATION_MODE);
			} catch (final Settings.SettingNotFoundException e) {
				// do nothing
			}
			return locationMode != Settings.Secure.LOCATION_MODE_OFF;
		}
		return true;
	}

	/**
	 * Location enabled is required on some phones running Android Marshmallow or newer (for example on Nexus and Pixel devices).
	 *
	 * @param context the context
	 * @return false if it is known that location is not required, true otherwise
	 */
	public static boolean isLocationRequired(final Context context) {
		final SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
		return preferences.getBoolean(PREFS_LOCATION_NOT_REQUIRED, isMarshmallowOrAbove());
	}

	/**
	 * When a Bluetooth LE packet is received while Location is disabled it means that Location
	 * is not required on this device in order to scan for LE devices. This is a case of Samsung phones, for example.
	 * Save this information for the future to keep the Location info hidden.
	 * @param context the context
	 */
	public static void markLocationNotRequired(final Context context) {
		final SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
		preferences.edit().putBoolean(PREFS_LOCATION_NOT_REQUIRED, false).apply();
	}

	/**
	 * The first time an app requests a permission there is no 'Don't ask again' checkbox and
	 * {@link ActivityCompat#shouldShowRequestPermissionRationale(Activity, String)} returns false.
	 * This situation is similar to a permission being denied forever, so to distinguish both cases
	 * a flag needs to be saved.
	 * @param context the context
	 */
	public static void markLocationPermissionRequested(final Context context) {
		final SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
		preferences.edit().putBoolean(PREFS_PERMISSION_REQUESTED, true).apply();
	}

	public static boolean isMarshmallowOrAbove() {
		return Build.VERSION.SDK_INT >= Build.VERSION_CODES.M;
	}

	/*
	public static void downloadTask(final String file_url, final String internal_filename) {
		Thread thread = new Thread(new Runnable(){
			@Override
			public void run() {
				try {
					URL url = new URL(file_url);
					URLConnection urlConnection = url.openConnection();
					InputStream in = urlConnection.getInputStream();
					File file = new File(Environment.getExternalStorageDirectory(),
							Environment.DIRECTORY_DOWNLOADS + "/" + internal_filename);
					//Uri.fromFile(file);
					//new FileOutputStream(Uri.fromFile(file));
					FileOutputStream outputStream = new FileOutputStream(file);

					if (!file.exists()) {
						file.createNewFile();
					}

					IOUtils.copy(in,outputStream);
					outputStream.close();
					in.close();


				} catch (Exception e) {
					Log.e("download task fail", e.getMessage());
				}
			}
		});
		thread.start();
	}
	*/

	public static String getHashString(final String testFile) {
		try {
			byte[] buffer = new byte[8192];
			String computerHash;
			int count;
			MessageDigest digest = MessageDigest.getInstance("SHA-256");
			BufferedInputStream bis =
					new BufferedInputStream(new FileInputStream(new File(testFile)));
			while ((count = bis.read(buffer)) > 0) {
				digest.update(buffer, 0, count);
			}
			bis.close();

			computerHash = new String(bytesToHex(digest.digest())).trim();
			return computerHash;
		} catch (Exception e) {
			Log.e("checkHash", "exception", e);
			return null;
		}
	}

	public static String getRealPathFromURI(Context ctx, Uri contentURI) {
		String result;
		Cursor cursor = ctx.getContentResolver().query(contentURI, null, null, null, null);
		if (cursor == null) { // Source is Dropbox or other similar local file path
			result = contentURI.getPath();
		} else {
			cursor.moveToFirst();
			int idx = cursor.getColumnIndex(MediaStore.Images.ImageColumns.DATA);
			result = cursor.getString(idx);
			cursor.close();
		}
		return result;
	}

	/*
	public static boolean checkHash(final String testFile, final String knownHashFile) {
		try {
			String goodHash =
					FileUtils.readFileToString(new File(knownHashFile), "UTF-8").trim();
			String computerHash = Utils.getHashString(testFile);
			Log.d("b64compute", computerHash + " " + computerHash.length());
			Log.d("b64known",goodHash + " " + goodHash.length());
			return goodHash.equals(computerHash);
		} catch (Exception e) {
			Log.e("checkHash", "exception", e);
			return false;
		}
	}
	*/

	private static final char[] hexDigit = "0123456789abcdef".toCharArray();

	private static String bytesToHex(byte[] bytes) {
		char[] hexChars = new char[bytes.length * 2];
		for (int i = 0; i < bytes.length; ++i) {
			int b = bytes[i] & 0xFF;
			hexChars[i * 2] = hexDigit[b >>> 4];
			hexChars[i * 2 + 1] = hexDigit[b & 0x0F];
		}
		return new String(hexChars);
	}

	public static void connectToAP(final Context ctx,
								   final String networkSSID, final String networkPass) {

		Thread thread = new Thread(new Runnable() {
			@Override
			public void run() {
				try {

					Log.d("network:", "ssid: " + networkSSID + " " + networkSSID.length() +
							" pass: " + networkPass + " " + networkPass.length());
					Thread.sleep(5000);
					WifiConfiguration conf = new WifiConfiguration();
					conf.SSID = "\"" + networkSSID + "\"";
					conf.preSharedKey = "\"" + networkPass + "\"";
					WifiManager wifiManager =
							(WifiManager) ctx.getSystemService(Context.WIFI_SERVICE);
					wifiManager.addNetwork(conf);
					List<WifiConfiguration> list = wifiManager.getConfiguredNetworks();
					for (WifiConfiguration i : list) {
						if (i.SSID != null && i.SSID.equals("\"" + networkSSID + "\"")) {
							Log.d("ap connected", "foundone");
							boolean status1 = wifiManager.disconnect();
							boolean status2 = wifiManager.enableNetwork(i.networkId, true);
							boolean status3 = wifiManager.reconnect();
							Log.d("ap stats", " " + status1 + " " + status2 + " " + status3);

							break;
						}
					}
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
		});
		thread.start();
	}

	public static boolean checkWifiConn(final Context ctx) {
		WifiManager wifiMgr = (WifiManager) ctx.getSystemService(Context.WIFI_SERVICE);
		if (wifiMgr.isWifiEnabled()) { // Wi-Fi adapter is ON
			WifiInfo wifiInfo = wifiMgr.getConnectionInfo();
			if( wifiInfo == null || wifiInfo.getNetworkId() == -1 ||
					!wifiInfo.getSSID().equals("DTCAP"))
			{
				Log.d("sendBytes","failed1");
				return false;
			}
			Log.d("sendBytes","succeed");
			return true;
		}
		Log.d("sendBytes","failed2");
		return false;
	}

	public static void sendBytesToAP(final Context ctx, final String fname) {
		Thread thread = new Thread(new Runnable() {
			@Override
			public void run() {
				Socket socket;
				int tryCount = 0;
				int maxTries = 3;
				while (true) {
					try {
						Thread.sleep(4000);
						InetAddress serverAddr = InetAddress.getByName("192.168.1.1"); // user pref
						socket = new Socket(serverAddr, 5000);
						socket.setReuseAddress(true);

						File file = new File(fname);
						long length = file.length();
						Log.d("filelentag", "file is len " + length);
						byte[] bytes = new byte[8 * 1024];
						InputStream in = new FileInputStream(file);

						Thread.sleep(1000);

						//OutputStream os = socket.getOutputStream();
						DataOutputStream outToServer = new DataOutputStream(socket.getOutputStream());
						outToServer.writeLong(length);

						int count;
						while ((count = in.read(bytes)) > 0) {
							outToServer.write(bytes, 0, count);
						}

						//outToServer.write(data,0,data.length);
						in.close();
						outToServer.close();
						socket.close();
						return;
					} catch (UnknownHostException e) {
						e.printStackTrace();
					} catch (IOException e) {
						e.printStackTrace();
						if (++tryCount == maxTries) {
							Log.d("socketexception", "e");
							return;
						}
					} catch (InterruptedException e) {
						e.printStackTrace();
					}
				}
			}
		});
		thread.start();
	}

	public static String getCurrentSSID(Context context) {
		String ssid = null;
		ConnectivityManager connManager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
		NetworkInfo networkInfo = connManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
		if (networkInfo.isConnected()) {
			final WifiManager wifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
			final WifiInfo connectionInfo = wifiManager.getConnectionInfo();
			if (connectionInfo != null) {
				ssid = connectionInfo.getSSID();
			}
		}
		return ssid;
	}
}
