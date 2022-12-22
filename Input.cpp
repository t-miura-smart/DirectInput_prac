//�u""�v�̓J�����g�����ϐ� �u<>�v�͊��ϐ�
#include <windows.h>
#include <dinput.h>
#include "Input.h"

//�}�N����`
#define DIRECTINPUT_VERSION 0x0800


//�R�[�h�w��
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

//�\���̐錾
static LPDIRECTINPUT8 g_InputInterface;							//!< DIRECTINPUT8�̃|�C���^
static LPDIRECTINPUTDEVICE8 g_GamePadDevice;					//!< DIRECTINPUTDEVICE8�̃|�C���^
struct DeviceEnumParameter										//�Q�[���p�b�h�f�o�C�X�̍쐬-�f�o�C�X�񋓂̌��ʂ��󂯎��\����
{
	LPDIRECTINPUTDEVICE8* GamePadDevice;
	int FindCount;
};		

// ���̓C���^�[�t�F�[�X�̍쐬
bool CreateInputInterface();
// �Q�[���p�b�h�f�o�C�X�̍쐬
bool CreateGamePadDevice();

BOOL SetUpCooperativeLevel(LPDIRECTINPUTDEVICE8 device)
{
	//�������[�h�̐ݒ�
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
	// �f�o�C�X����������ĂȂ�
	if (g_GamePadDevice == nullptr)
	{
		return false;
	}

	// ����J�n
	if (FAILED(g_GamePadDevice->Acquire()))
	{
		return false;
	}

	DIDEVCAPS cap;
	g_GamePadDevice->GetCapabilities(&cap);
	// �|�[�����O����
	if (cap.dwFlags & DIDC_POLLEDDATAFORMAT)
	{
		DWORD error = GetLastError();
		// �|�[�����O�J�n
		/*
			Poll��Acquire�̑O�ɍs���Ƃ���Ă������A
			Acquire�̑O�Ŏ��s����Ǝ��s�����̂�
			��Ŏ��s����悤�ɂ���
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
	// �����[�h���Βl���[�h�Ƃ��Đݒ�
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

	// X���̒l�͈̔͐ݒ�
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

	// Y���̒l�͈̔͐ݒ�
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

	// ���ɔ������Ă���Ȃ�I��
	if (parameter->FindCount >= 1)
	{
		return DIENUM_STOP;
	}

	// �f�o�C�X����
	HRESULT hr = g_InputInterface->CreateDevice(
		lpddi->guidInstance,
		parameter->GamePadDevice,
		NULL);

	if (FAILED(hr))
	{
		return DIENUM_STOP;
	}

	// ���̓t�H�[�}�b�g�̎w��
	device = *parameter->GamePadDevice;
	hr = device->SetDataFormat(&c_dfDIJoystick);

	if (FAILED(hr))
	{
		return DIENUM_STOP;
	}

	// �v���p�e�B�̐ݒ�
	if (SetUpGamePadProperty(device) == false)
	{
		return DIENUM_STOP;
	}

	// �������x���̐ݒ�
	if (SetUpCooperativeLevel(device) == false)
	{
		return DIENUM_STOP;
	}

	// ���������J�E���g
	parameter->FindCount++;

	return DIENUM_CONTINUE;
}



//�@IDirectInput8�C���^�[�t�F�C�X�̍쐬
bool CreateInputInterface()
{
	//�C���^�[�t�F�[�X�쐬�@[�߂�l ����:S_OK ���s:S_OK�ȊO]
	HRESULT ret = DirectInput8Create(
		GetModuleHandle(nullptr),	// �C���X�^���X�n���h��
		DIRECTINPUT_VERSION,		// DirectInput�̃o�[�W����
		IID_IDirectInput8,			// �g�p����@�\
		(void**)&g_InputInterface,	// �쐬���ꂽ�C���^�[�t�F�[�X����p
		NULL						// NULL�Œ�
	);
	//�쐬�ł������m�F
	if (FAILED(ret))
	{
		//���s
		return false;
	}

	return true;
}

//�A�Q�[���p�b�h�f�o�C�X�̗�
bool CreateGamePadDevice()
{
	DeviceEnumParameter parameter;

	parameter.FindCount = 0;
	parameter.GamePadDevice = &g_GamePadDevice;

	// racingwheel�𒲂ׂ�
	g_InputInterface->EnumDevices(
		DI8DEVTYPE_DEVICE,			// ��������f�o�C�X�̎��
		DeviceFindCallBack,			// �������Ɏ��s����֐�
		&parameter,					// �֐��ɓn���l
		DIEDFL_ATTACHEDONLY			// �������@
	);

	// �ǂ���������邱�Ƃ��o���Ȃ������玸�s
	if (parameter.FindCount == 0)
	{
		return false;
	}

	int count = 0;
	// ����J�n
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