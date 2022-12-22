//「""」はカレント→環境変数 「<>」は環境変数
#include <windows.h>
#include <dinput.h>
#include "Input.h"

//マクロ定義
#define DIRECTINPUT_VERSION 0x0800


//コード指定
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

//構造体宣言
static LPDIRECTINPUT8 g_InputInterface;							//!< DIRECTINPUT8のポインタ
static LPDIRECTINPUTDEVICE8 g_GamePadDevice;					//!< DIRECTINPUTDEVICE8のポインタ
struct DeviceEnumParameter										//ゲームパッドデバイスの作成-デバイス列挙の結果を受け取る構造体
{
	LPDIRECTINPUTDEVICE8* GamePadDevice;
	int FindCount;
};		

// 入力インターフェースの作成
bool CreateInputInterface();
// ゲームパッドデバイスの作成
bool CreateGamePadDevice();

BOOL SetUpCooperativeLevel(LPDIRECTINPUTDEVICE8 device)
{
	//協調モードの設定
	if (FAILED(device->SetCooperativeLevel(
		FindWindow(WINDOW_CLASS_NAME, nullptr),
		DISCL_EXCLUSIVE | DISCL_FOREGROUND)
	))
	{
		return false;
	}

	return true;
}
BOOL StartGamePadControl()
{
	// デバイスが生成されてない
	if (g_GamePadDevice == nullptr)
	{
		return false;
	}

	// 制御開始
	if (FAILED(g_GamePadDevice->Acquire()))
	{
		return false;
	}

	DIDEVCAPS cap;
	g_GamePadDevice->GetCapabilities(&cap);
	// ポーリング判定
	if (cap.dwFlags & DIDC_POLLEDDATAFORMAT)
	{
		DWORD error = GetLastError();
		// ポーリング開始
		/*
			PollはAcquireの前に行うとされていたが、
			Acquireの前で実行すると失敗したので
			後で実行するようにした
		*/
		if (FAILED(g_GamePadDevice->Poll()))
		{
			return false;
		}
	}

	return true;
}
BOOL SetUpGamePadProperty(LPDIRECTINPUTDEVICE8 device)
{
	// 軸モードを絶対値モードとして設定
	DIPROPDWORD diprop;
	ZeroMemory(&diprop, sizeof(diprop));
	diprop.diph.dwSize = sizeof(diprop);
	diprop.diph.dwHeaderSize = sizeof(diprop.diph);
	diprop.diph.dwHow = DIPH_DEVICE;
	diprop.diph.dwObj = 0;
	diprop.dwData = DIPROPAXISMODE_ABS;
	if (FAILED(device->SetProperty(DIPROP_AXISMODE, &diprop.diph)))
	{
		return false;
	}

	// X軸の値の範囲設定
	DIPROPRANGE diprg;
	ZeroMemory(&diprg, sizeof(diprg));
	diprg.diph.dwSize = sizeof(diprg);
	diprg.diph.dwHeaderSize = sizeof(diprg.diph);
	diprg.diph.dwHow = DIPH_BYOFFSET;
	diprg.diph.dwObj = DIJOFS_X;
	diprg.lMin = -1000;
	diprg.lMax = 1000;
	if (FAILED(device->SetProperty(DIPROP_RANGE, &diprg.diph)))
	{
		return false;
	}

	// Y軸の値の範囲設定
	diprg.diph.dwObj = DIJOFS_Y;
	if (FAILED(device->SetProperty(DIPROP_RANGE, &diprg.diph)))
	{
		return false;
	}

	return true;
}


BOOL CALLBACK DeviceFindCallBack(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
	DeviceEnumParameter* parameter = (DeviceEnumParameter*)pvRef;
	LPDIRECTINPUTDEVICE8 device = nullptr;

	// 既に発見しているなら終了
	if (parameter->FindCount >= 1)
	{
		return DIENUM_STOP;
	}

	// デバイス生成
	HRESULT hr = g_InputInterface->CreateDevice(
		lpddi->guidInstance,
		parameter->GamePadDevice,
		NULL);

	if (FAILED(hr))
	{
		return DIENUM_STOP;
	}

	// 入力フォーマットの指定
	device = *parameter->GamePadDevice;
	hr = device->SetDataFormat(&c_dfDIJoystick);

	if (FAILED(hr))
	{
		return DIENUM_STOP;
	}

	// プロパティの設定
	if (SetUpGamePadProperty(device) == false)
	{
		return DIENUM_STOP;
	}

	// 協調レベルの設定
	if (SetUpCooperativeLevel(device) == false)
	{
		return DIENUM_STOP;
	}

	// 発見数をカウント
	parameter->FindCount++;

	return DIENUM_CONTINUE;
}



//①IDirectInput8インターフェイスの作成
bool CreateInputInterface()
{
	//インターフェース作成　[戻り値 成功:S_OK 失敗:S_OK以外]
	HRESULT ret = DirectInput8Create(
		GetModuleHandle(nullptr),	// インスタンスハンドル
		DIRECTINPUT_VERSION,		// DirectInputのバージョン
		IID_IDirectInput8,			// 使用する機能
		(void**)&g_InputInterface,	// 作成されたインターフェース代入用
		NULL						// NULL固定
	);
	//作成できたか確認
	if (FAILED(ret))
	{
		//失敗
		return false;
	}

	return true;
}

//②ゲームパッドデバイスの列挙
bool CreateGamePadDevice()
{
	DeviceEnumParameter parameter;

	parameter.FindCount = 0;
	parameter.GamePadDevice = &g_GamePadDevice;

	// racingwheelを調べる
	g_InputInterface->EnumDevices(
		DI8DEVTYPE_DEVICE,			// 検索するデバイスの種類
		DeviceFindCallBack,			// 発見時に実行する関数
		&parameter,					// 関数に渡す値
		DIEDFL_ATTACHEDONLY			// 検索方法
	);

	// どちらも見つけることが出来なかったら失敗
	if (parameter.FindCount == 0)
	{
		return false;
	}

	int count = 0;
	// 制御開始
	while (StartGamePadControl() == false)
	{
		Sleep(100);
		count++;
		if (count >= 5)
		{
			break;
		}
	}

	return true;
}