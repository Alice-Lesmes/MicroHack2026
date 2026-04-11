#include <Windows.h>
#include <ControllerDriver.h>

const float STOPPED = 0.0;
const float WALK_SPEED = 0.5;
const float RUN_SPEED = 0.95;

EVRInitError ControllerDriver::Activate(uint32_t unObjectId)
{
	driverId = unObjectId;

	PropertyContainerHandle_t props = VRProperties()->TrackedDeviceToPropertyContainer(driverId);
	// Tell SteamVR this device provides inputs but has no physical tracking of its own
	VRProperties()->SetBoolProperty(props, Prop_NeverTracked_Bool, true);

	VRProperties()->SetStringProperty(props, Prop_InputProfilePath_String, "{esp32}/input/controller_profile.json");
	VRProperties()->SetInt32Property(props, Prop_ControllerRoleHint_Int32, ETrackedControllerRole::TrackedControllerRole_OptOut);


	VRDriverInput()->CreateBooleanComponent(props, "/input/trigger/click", &triggerHandle);
	vr::VRServerDriverHost()->TrackedDevicePoseUpdated(driverId, GetPose(), sizeof(DriverPose_t));
	return VRInitError_None;
}

DriverPose_t ControllerDriver::GetPose()
{
	// We don't use pose (for now).
	DriverPose_t pose = { 0 };
	pose.poseIsValid = true;
	pose.result = TrackingResult_Running_OK;
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


void UpdateTriggerState(VRInputComponentHandle_t triggerHandle) {
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
	VRDriverInput()->UpdateBooleanComponent(triggerHandle, isTriggerPressed, 0);
}

void ControllerDriver::RunFrame()
{

	// Update Y Axis according to the keyboard pressed buttons.
	//UpdateYAxis(joystickYHandle, trackpadYHandle, currMovSpeed);
	// Update X Axis according to the keyboard pressed buttons.
	//UpdateXAxis(joystickXHandle, trackpadXHandle, currMovSpeed);
	// 1. PUSH THE TRACKING STATE TO STEAMVR EVERY FRAME
	if (driverId != vr::k_unTrackedDeviceIndexInvalid)
	{
		vr::VRServerDriverHost()->TrackedDevicePoseUpdated(driverId, GetPose(), sizeof(DriverPose_t));
	}
	UpdateTriggerState(triggerHandle);
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