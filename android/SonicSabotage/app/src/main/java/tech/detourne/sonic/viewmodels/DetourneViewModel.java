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

package tech.detourne.sonic.viewmodels;

import android.app.Application;
import android.arch.lifecycle.AndroidViewModel;
import android.arch.lifecycle.LiveData;
import android.arch.lifecycle.MutableLiveData;
import android.arch.lifecycle.ViewModel;
import android.arch.lifecycle.ViewModelProvider;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.os.Environment;
import android.os.Handler;
import android.support.annotation.NonNull;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;

import org.apache.commons.io.IOUtils;

import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.net.InetAddress;
import java.net.Socket;
import java.net.URI;
import java.net.URL;
import java.net.URLConnection;
import java.net.UnknownHostException;
import java.util.List;

import tech.detourne.sonic.DetourneActivity;
import tech.detourne.sonic.R;
import tech.detourne.sonic.adapter.ExtendedBluetoothDevice;
import tech.detourne.sonic.profile.DetourneManager;
import tech.detourne.sonic.profile.DetourneManagerCallbacks;
import no.nordicsemi.android.log.ILogSession;
import no.nordicsemi.android.log.LogSession;
import no.nordicsemi.android.log.Logger;
import tech.detourne.sonic.profile.DetourneManager;
import tech.detourne.sonic.profile.DetourneManagerCallbacks;
import tech.detourne.sonic.utils.Utils;

public class DetourneViewModel extends AndroidViewModel implements DetourneManagerCallbacks {

	private final DetourneManager mDetourneManager;

	// Connection states Connecting, Connected, Disconnecting, Disconnected etc.
	private final MutableLiveData<String> mConnectionState = new MutableLiveData<>();

	// Flag to determine if the device is connected
	private final MutableLiveData<Boolean> mIsConnected = new MutableLiveData<>();

	// Flag to determine if the device has required services
	private final SingleLiveEvent<Boolean> mIsSupported = new SingleLiveEvent<>();

	// Flag to determine if the device is ready
	private final MutableLiveData<Void> mOnDeviceReady = new MutableLiveData<>();

	// Flag that holds the on off state of the Buzzer. On is true, Off is False
	private final MutableLiveData<Boolean> mBuzzState = new MutableLiveData<>();
	private final MutableLiveData<Byte> mFixedIntVal = new MutableLiveData<>();
	private final MutableLiveData<Byte> mRandIntVal = new MutableLiveData<>();
	private final MutableLiveData<Byte> mSolarMinVal = new MutableLiveData<>();
	private final MutableLiveData<Byte> mRSSIMinVal = new MutableLiveData<>();
	private final MutableLiveData<Byte> mSolarVal = new MutableLiveData<>();
	private final MutableLiveData<Byte> mRSSIVal = new MutableLiveData<>();
	private final MutableLiveData<Boolean> mDownloadState = new MutableLiveData<>();
	private final MutableLiveData<String> mPass = new MutableLiveData<>();
	private final MutableLiveData<String> mFWFile = new MutableLiveData<>();
	private final MutableLiveData<String> mSHA256 = new MutableLiveData<>();
	private final MutableLiveData<Long> mFWSize = new MutableLiveData<>();
	private final MutableLiveData<Byte> mMode = new MutableLiveData<>();


	// Flag that holds the pressed released state of the button on the devkit. Pressed is true, Released is False
	private final MutableLiveData<Boolean> mButtonState = new MutableLiveData<>();
	public LiveData<Void> isDeviceReady() {
		return mOnDeviceReady;
	}
	public LiveData<String> getConnectionState() {
		return mConnectionState;
	}
	public LiveData<Boolean> isConnected() {
		return mIsConnected;
	}
	public LiveData<Boolean> getButtonState() {
		return mButtonState;
	}
	public LiveData<Boolean> getBuzzState() {
		return mBuzzState;
	}
	public LiveData<Boolean> isSupported() {
		return mIsSupported;
	}
	public LiveData<Byte> getFixedIntVal() {
		return mFixedIntVal;
	}
	public LiveData<Byte> getRandIntVal() {
		return mRandIntVal;
	}
	public LiveData<Byte> getSolarMinVal() {
		return mSolarMinVal;
	}
	public LiveData<Byte> getRSSIMinVal() {
		return mRSSIMinVal;
	}
	public LiveData<Byte> getSolarVal() {
		return mSolarVal;
	}
	public LiveData<Byte> getRSSIVal() {
		return mRSSIVal;
	}
	public LiveData<String> getPassVal() { return mPass; }
	public LiveData<String> getFileName() { return mFWFile; }
	public LiveData<String> getSHA256String() { return mSHA256; }
	public LiveData<Long> getFWSize() { return mFWSize; }
	public LiveData<Boolean> getFirmwareDownloadState() { return mDownloadState; }
	public LiveData<Byte> getMode() {
		return mMode;
	}



	public DetourneViewModel(@NonNull final Application application) {
		super(application);

		// Initialize the manager
		mDetourneManager = new DetourneManager(getApplication());
		mDetourneManager.setGattCallbacks(this);
	}

	/**
	 * Connect to peripheral
	 */
	public void connect(final ExtendedBluetoothDevice device) {
		final LogSession logSession = Logger.newSession(getApplication(), null, device.getAddress(), device.getName());
		mDetourneManager.setLogger(logSession);
		mDetourneManager.connect(device.getDevice());
	}

	/**
	 * Disconnect from peripheral
	 */
	private void disconnect() {
		mDetourneManager.disconnect();
	}

	public void setFWFile(final String fwName) {
		mFWFile.setValue(fwName);
		mFWSize.setValue(new File(fwName).length());
		mSHA256.setValue(Utils.getHashString(fwName));
	}

	public void flashTask(Context ctx) {

		Log.d("flash task:","time to start flashing to board");
		//read wifi pass characteristic off of device
		mDetourneManager.readPassCharacteristic();
		//send mode: fw update characteristic
		mDetourneManager.startFlashMode();
		//wait 5 seconds for AP to start

		Log.d("flash task:","connect to AP");
		String networkSSID = "DTCAP"; //user pref
		String networkPass = getPassVal().getValue(); //user pref?
		Utils.connectToAP(ctx,networkSSID, networkPass);

	}


	public void toggleBuzz(final boolean onOff) {
		byte val = onOff ? (byte)1 : (byte)0;
		mDetourneManager.sendByte(DetourneManager.LBS_UUID_BUZZ_CHAR, val);
		mBuzzState.setValue(onOff);
	}

	public void updateFixedIntVal(final byte val) {
		mFixedIntVal.setValue(val);
		//Log.i("updating","a fixed int val");
	}

	public void updateRandIntVal(final byte val) {
		mRandIntVal.setValue(val);
		//Log.i("updating","a fixed int val");
	}

	public void updateSolarMinVal(final byte val) {
		mSolarMinVal.setValue(val);
		//Log.i("updating","a fixed int val");
	}

	public void updateRSSIMinVal(final byte val) {
		mRSSIMinVal.setValue(val);
		//Log.i("updating","a fixed int val");
	}

	public void updateMode(final byte val) {
		mMode.setValue(val);
		//Log.i("updating","a fixed int val");
	}

	@Override
	public void onSolarReceived(final byte val) {
		mSolarVal.postValue(val);
		//Log.i("updating","a fixed int val");
	}

	@Override
	public void onRSSIReceived(final byte val) {
		mRSSIVal.postValue(val);
		//Log.i("updating","a fixed int val");
	}

	public void sendFixedIntVal() {
		Log.i("sending","a fixed int val");
		mDetourneManager.sendByte(DetourneManager.LBS_UUID_FIXEDINT_CHAR,
											getFixedIntVal().getValue());
	}

	public void sendRandomIntVal() {
		Log.i("sending","a rand int val");
		mDetourneManager.sendByte(DetourneManager.LBS_UUID_RANDINT_CHAR,
											getRandIntVal().getValue());
	}

	public void sendSolarMinVal() {
		Log.i("sending","a solar min val");
		mDetourneManager.sendByte(DetourneManager.LBS_UUID_SOLARMIN_CHAR,
											getSolarMinVal().getValue());
	}

	public void sendRSSIMinVal() {
		Log.i("sending","an RSSI min val");
		mDetourneManager.sendByte(DetourneManager.LBS_UUID_RSSIMIN_CHAR,
											getRSSIMinVal().getValue());
	}

	public void sendModeVal() {
		Log.i("sending","a mode val");
		mDetourneManager.sendByte(DetourneManager.LBS_UUID_MODE,
				getMode().getValue());
	}

	@Override
	protected void onCleared() {
		super.onCleared();
		if (mDetourneManager.isConnected()) {
			disconnect();
		}
		Log.i("STATECHECK","onCleared");

	}

	@Override
	public void onDataReceived(final boolean state) {
		//mButtonState.postValue(state);
		Log.i("STATECHECK","DataRecv");

	}

	@Override
	public void onPassReceived(final String pass) {
		mPass.postValue(pass); //post vs. set ???
	}

	@Override
	public void onDataSent(final boolean state) {
		mBuzzState.postValue(state);
		Log.i("STATECHECK","onDataSent");

	}

	@Override
	public void onDeviceConnecting(final BluetoothDevice device) {
		mConnectionState.postValue(getApplication().getString(R.string.state_connecting));
		Log.i("STATECHECK","OnDeviceConnecting");

	}

	@Override
	public void onDeviceConnected(final BluetoothDevice device) {
		mIsConnected.postValue(true);
		mConnectionState.postValue(getApplication().getString(R.string.state_discovering_services));
		Log.i("STATECHECK","OnDeviceConnected");
	}

	@Override
	public void onDeviceDisconnecting(final BluetoothDevice device) {
		mIsConnected.postValue(false);
		Log.i("STATECHECK","OnDeviceDisconnecting");
	}

	@Override
	public void onDeviceDisconnected(final BluetoothDevice device) {
		mIsConnected.postValue(false);
		Log.i("STATECHECK","OnDeviceDisconnected");
	}

	@Override
	public void onLinklossOccur(final BluetoothDevice device) {
		mIsConnected.postValue(false);
		Log.i("STATECHECK","OnLinkLoss");


	}

	@Override
	public void onServicesDiscovered(final BluetoothDevice device, final boolean optionalServicesFound) {
		mConnectionState.postValue(getApplication().getString(R.string.state_initializing));
		Log.i("STATECHECK","OnServiceDisco");

	}

	@Override
	public void onDeviceReady(final BluetoothDevice device) {
		mIsSupported.postValue(true);
		mConnectionState.postValue(getApplication().getString(R.string.state_discovering_services_completed, device.getName()));
		mOnDeviceReady.postValue(null);
		Log.i("STATECHECK","OnDeviceReady");

	}

	@Override
	public boolean shouldEnableBatteryLevelNotifications(final BluetoothDevice device) {
		// no Battery Service
		return false;
	}

	@Override
	public void onBatteryValueReceived(final BluetoothDevice device, final int value) {
		// no Battery Service
	}

	@Override
	public void onBondingRequired(final BluetoothDevice device) {
		Log.i("STATECHECK","OnBondingReq");

	}

	@Override
	public void onBonded(final BluetoothDevice device) {
		Log.i("STATECHECK","OnBonded");
		//mDetourneManager.connect(device);
	}

	@Override
	public void onError(final BluetoothDevice device, final String message, final int errorCode) {
		Log.i("STATECHECK","OnError");
		if(errorCode == BluetoothGatt.GATT_INSUFFICIENT_AUTHENTICATION) {
			Log.i("STATECHECK","insuffauth");
			Log.i("STATECHECK", message );
			mIsConnected.postValue(false);
			try {
				Method m = device.getClass().getMethod("removeBond", (Class[]) null);
				m.invoke(device, (Object[]) null);
			} catch (Exception e) { Log.e("STATECHECK", e.getMessage()); }
		}

	}


	@Override
	public void onDeviceNotSupported(final BluetoothDevice device) {
		mIsSupported.postValue(false);
		Log.i("STATECHECK","OnDeviceNotSupported");

	}
}
