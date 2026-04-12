#include <Windows.h>
#include <ControllerDriver.h>

const float STOPPED = 0.0;
const float WALK_SPEED = 0.5;
const float RUN_SPEED = 0.95;

EVRInitError ControllerDriver::Activate(uint32_t unObjectId)
{
	driverId = unObjectId;

	PropertyContainerHandle_t props = VRProperties()->TrackedDeviceToPropertyContainer(driverId);

	// 1. Set this device as a Left or Right controller
	vr::VRProperties()->SetInt32Property(props, vr::Prop_ControllerRoleHint_Int32, vr::TrackedControllerRole_OptOut);

	// 2. Create the Trigger components (Boolean for click, Scalar for axis)
	vr::VRDriverInput()->CreateBooleanComponent(props, "/input/trigger/click", &m_compTriggerClick);

	// Notice the VRScalarType_Absolute and NormalizedOneSided (0.0 to 1.0)
	vr::VRDriverInput()->CreateScalarComponent(props, "/input/trigger/value", &m_compTriggerValue, vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedOneSided);

	return VRInitError_None;
}

DriverPose_t ControllerDriver::GetPose()
{
	// We don't use pose (for now).
	DriverPose_t pose = { 0 };
	pose.poseIsValid = false;
	pose.result = TrackingResult_Uninitialized;
	pose.deviceIsConnected = true;

	HmdQuaternion_t quat;
	quat.w = 1;
	quat.x = 0;
	quat.y = 0;
	quat.z = 0;

	pose.qWorldFromDriverRotation = quat;
	pose.qDriverFromHeadRotation = quat;

	return pose;
}


void UpdateTriggerState(VRInputComponentHandle_t clickHandle, VRInputComponentHandle_t valueHandle) {
	// Read the global hardware state of F10
	bool isTriggerPressed = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;

	// Track the previous state to log only on changes
	static bool lastState = false;
	if (isTriggerPressed != lastState) {
		if (isTriggerPressed) {
			vr::VRDriverLog()->Log("SUCCESS: ESP32 F10 PRESSED!");
		}
		else {
			vr::VRDriverLog()->Log("SUCCESS: ESP32 F10 RELEASED!");
		}
		lastState = isTriggerPressed;
	}

	// Send the boolean state to SteamVR
	float triggerAxisValue = isTriggerPressed ? 1.0f : 0.0f;

	vr::VRDriverInput()->UpdateBooleanComponent(clickHandle, isTriggerPressed, 0);
	vr::VRDriverInput()->UpdateScalarComponent(valueHandle, triggerAxisValue, 0);
}

void ControllerDriver::RunFrame()
{

	// Update Y Axis according to the keyboard pressed buttons.
	//UpdateYAxis(joystickYHandle, trackpadYHandle, currMovSpeed);
	// Update X Axis according to the keyboard pressed buttons.
	//UpdateXAxis(joystickXHandle, trackpadXHandle, currMovSpeed);
	//UpdateTriggerState(triggerHandle);
	UpdateTriggerState(m_compTriggerClick, m_compTriggerValue);
}

void ControllerDriver::Deactivate()
{
	driverId = k_unTrackedDeviceIndexInvalid;
}

void* ControllerDriver::GetComponent(const char* pchComponentNameAndVersion)
{
	if (strcmp(IVRDriverInput_Version, pchComponentNameAndVersion) == 0)
	{
		return this;
	}
	return NULL;
}

void ControllerDriver::EnterStandby() {}

void ControllerDriver::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize)
{
	if (unResponseBufferSize >= 1)
	{
		pchResponseBuffer[0] = 0;
	}
}