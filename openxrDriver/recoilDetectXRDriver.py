"""
pyopenxr headless example
using low level OpenXR API
"""

import ctypes
import platform
import time
import xr
# Enumerate the required instance extensions
extensions = [xr.MND_HEADLESS_EXTENSION_NAME]  # Permits use without a graphics display
# Tracking controllers in headless mode requires a way to get the current XrTime
if platform.system() == "Windows":
    extensions.append(xr.KHR_WIN32_CONVERT_PERFORMANCE_COUNTER_TIME_EXTENSION_NAME)
else:  # Linux
    extensions.append(xr.KHR_CONVERT_TIMESPEC_TIME_EXTENSION_NAME)

# Create instance for headless use
instance = xr.create_instance(xr.InstanceCreateInfo(
    enabled_extension_names=extensions,
))
system = xr.get_system(
    instance,
    # Presumably the form factor is irrelevant in headless mode...
    xr.SystemGetInfo(form_factor=xr.FormFactor.HEAD_MOUNTED_DISPLAY),
)
session = xr.create_session(
    instance,
    xr.SessionCreateInfo(
        system_id=system,
        next=None,  # No GraphicsBinding structure is required here in HEADLESS mode
    )
)

if platform.system() == "Windows": # TODO: work on this stuff, but for now i'm priortising Linux
    import ctypes.wintypes
    pc_time = ctypes.wintypes.LARGE_INTEGER()
    kernel32 = ctypes.WinDLL("kernel32")
    pxrConvertWin32PerformanceCounterToTimeKHR = ctypes.cast(
        xr.get_instance_proc_addr(
            instance=instance,
            name="xrConvertWin32PerformanceCounterToTimeKHR",
        ),
        xr.PFN_xrConvertWin32PerformanceCounterToTimeKHR,
    )

    def time_from_perf_counter(instance: xr.Instance,
                               performance_counter: ctypes.wintypes.LARGE_INTEGER) -> xr.Time:
        xr_time = xr.Time()
        result = pxrConvertWin32PerformanceCounterToTimeKHR(
            instance,
            ctypes.pointer(performance_counter),
            ctypes.byref(xr_time),
        )
        result = xr.check_result(result)
        if result.is_exception():
            raise result
        return xr_time
else: # Linux/MacOS
    timespecTime = xr.timespec()
    pxrConvertTimespecTimeToTimeKHR = ctypes.cast(
        xr.get_instance_proc_addr(
            instance=instance,
            name="xrConvertTimespecTimeToTimeKHR",
        ),
        xr.PFN_xrConvertTimespecTimeToTimeKHR,
    )

    def time_from_timespec(instance: xr.Instance, timespec_time: xr.timespec) -> xr.Time:
        xr_time = xr.Time()
        result = pxrConvertTimespecTimeToTimeKHR(
            instance,
            ctypes.pointer(timespec_time),
            ctypes.byref(xr_time),
        )
        result = xr.check_result(result)
        if result.is_exception():
            raise result
        return xr_time
# Set up controller tracking, as one possible legitimate headless activity
# NOTE: this is going to be the place to look to change what we can do/track
action_set = xr.create_action_set(
    instance=instance,
    create_info=xr.ActionSetCreateInfo(
        action_set_name="action_set",
        localized_action_set_name="Action Set",
        priority=0,
    ),
)
controller_paths = (xr.Path * 2)(  # noqa
    xr.string_to_path(instance, "/user/hand/left"), # Can prob disregard left for time being
    xr.string_to_path(instance, "/user/hand/right"),
)
controller_pose_action = xr.create_action(
    action_set=action_set,
    create_info=xr.ActionCreateInfo(
        action_type=xr.ActionType.POSE_INPUT,
        action_name="controller_pose",
        localized_action_name="Controller pose",
        count_subaction_paths=len(controller_paths),
        subaction_paths=controller_paths,
    ),
)
trigger_click_action = xr.create_action( # AI said this, pls work
    action_set=action_set,
    create_info=xr.ActionCreateInfo(
        action_type=xr.ActionType.BOOLEAN_INPUT,
        action_name="trigger_click",
        localized_action_name="Trigger Click",
        count_subaction_paths=len(controller_paths),
        subaction_paths=controller_paths,
    ),
)
suggested_bindings = (xr.ActionSuggestedBinding * 2)(
    # Can ignore this below one - AL
    xr.ActionSuggestedBinding(
        action=controller_pose_action,
        binding=xr.string_to_path(
            instance=instance,
            path_string="/user/hand/left/input/grip/pose",
        ),
    ), # This here is useful to detect trigger clicks - AL
    xr.ActionSuggestedBinding(
        action=trigger_click_action, 
        binding=xr.string_to_path(
            instance=instance,
            # Check the input trigger to see if it's 'pressed', 
            # can change to 'value' instead of 'click' if you want granularity
            path_string="/user/hand/right/input/trigger/value",
        ),
    ),
)
print(xr.string_to_path(instance, "/interaction_profiles/oculus/touch_controller"))
# Suggest Oculus keybindings
xr.suggest_interaction_profile_bindings(
    instance=instance,
    suggested_bindings=xr.InteractionProfileSuggestedBinding(
        interaction_profile=xr.string_to_path(
            instance,
            # I sure hope this is the same for quest, should be
            "/interaction_profiles/oculus/touch_controller",
        ),
        count_suggested_bindings=len(suggested_bindings),
        suggested_bindings=suggested_bindings,
    ),
)

"""
xr.suggest_interaction_profile_bindings(
    instance=instance,
    suggested_bindings=xr.InteractionProfileSuggestedBinding(
        interaction_profile=xr.string_to_path(
            instance,
            "/interaction_profiles/khr/simple_controller",
        ),
        count_suggested_bindings=len(suggested_bindings),
        suggested_bindings=suggested_bindings,
    ),
)

xr.suggest_interaction_profile_bindings(
    instance=instance,
    suggested_bindings=xr.InteractionProfileSuggestedBinding(
        interaction_profile=xr.string_to_path(
            instance,
            "/interaction_profiles/htc/vive_controller",
        ),
        count_suggested_bindings=len(suggested_bindings),
        suggested_bindings=suggested_bindings,
    ),
)
"""
xr.attach_session_action_sets(
    session=session,
    attach_info=xr.SessionActionSetsAttachInfo(
        action_sets=[action_set],
    ),
)
action_spaces = [
    xr.create_action_space(
        session=session,
        create_info=xr.ActionSpaceCreateInfo(
            action=controller_pose_action,
            subaction_path=controller_paths[0],
        ),
    )
]
reference_space = xr.create_reference_space(
    session=session,
    create_info=xr.ReferenceSpaceCreateInfo(
        reference_space_type=xr.ReferenceSpaceType.STAGE,
    ),
)
view_reference_space = xr.create_reference_space(
    session=session,
    create_info=xr.ReferenceSpaceCreateInfo(
        reference_space_type=xr.ReferenceSpaceType.VIEW,
    ),
)
session_state = xr.SessionState.UNKNOWN
# Loop over session frames
for frame_index in range(30):  # Limit number of frames for demo purposes
    # Poll session state changed events
    '''
    while True:
        try:
            event_buffer = xr.poll_event(instance)
            event_type = xr.StructureType(event_buffer.type)
            print(event_type)
            if event_type == xr.StructureType.EVENT_DATA_SESSION_STATE_CHANGED: # This shouldn't maybe exist Since only looks for changes in state
                event = ctypes.cast(
                    ctypes.byref(event_buffer),
                    ctypes.POINTER(xr.EventDataSessionStateChanged)).contents
                session_state = xr.SessionState(event.state)
                print(session_state)
                print(f"OpenXR session state changed to xr.SessionState.{session_state.name}")
                if session_state == xr.SessionState.READY:
                    xr.begin_session(
                        session,
                        xr.SessionBeginInfo(
                            # TODO: zero should be allowed here...
                            primary_view_configuration_type=xr.ViewConfigurationType.PRIMARY_MONO,
                        ),
                    )
                elif session_state == xr.SessionState.STOPPING:
                    xr.destroy_session(session)
                    session = None
        except xr.EventUnavailable:
            break  # There is no event in the queue at this moment
    '''
    if True or session_state == xr.SessionState.FOCUSED: # I think this should be in while true?
        # wait_frame()/begin_frame()/end_frame() are not required in headless mode
        xr.wait_frame(session=session)  # Helps SteamVR show application name better
        # Perform per-frame activities here
        
        # This code is to please my LSP, and maybe python idk - AL
        if (session == None):
            # FIXME: maybe change this print statement :3 - AL
            print("Freak the FUCK out")
            exit()

        if platform.system() == "Windows":
            kernel32.QueryPerformanceCounter(ctypes.byref(pc_time))
            xr_time_now = time_from_perf_counter(instance, pc_time)
        else: # Linux
            time_float = time.clock_gettime(time.CLOCK_MONOTONIC)
            timespecTime.tv_sec = int(time_float)
            timespecTime.tv_nsec = int((time_float % 1) * 1e9)
            xr_time_now = time_from_timespec(instance, timespecTime)

        active_action_set = xr.ActiveActionSet(
            action_set=action_set,
            subaction_path=xr.NULL_PATH,
        )
        # Idk what this does - AL
        xr.sync_actions(
            session=session,
            sync_info=xr.ActionsSyncInfo(
                active_action_sets=[active_action_set],
            ),
        )
        # Below is mostly/all vibed
        # I assume the AI meant 'active_action_sets=' not just 'action_sets='
        sync_info = xr.ActionsSyncInfo(active_action_sets=[active_action_set])
        xr.sync_actions(session, sync_info)
        # 2. Ask for the specific state of our trigger action
        get_info = xr.ActionStateGetInfo(
            action=trigger_click_action, 
            subaction_path=xr.NULL_PATH
        )
        trigger_state = xr.get_action_state_boolean(session, get_info)
        if trigger_state.is_active or trigger_state is True: # Hopefully this is correct, maybe just 'true' might work - AL
            # Current_state is True while the trigger is held down
            if trigger_state.current_state:
                # changed_since_last_sync ensures we only trigger once per distinct pull
                if trigger_state.changed_since_last_sync:
                    print("Trigger was just pulled!")
                    print("We are in business")
                else:
                    pass # Trigger is being held down
        else: # Trigger not held down
            print("Trigger is not being held down")

        # Idk what below does other than just print location of HMD and controllers - AL

        found_count = 0
        hmd_location = xr.locate_space(
            space=view_reference_space,
            base_space=reference_space,
            time=xr_time_now,
        )
        if hmd_location.location_flags & xr.SPACE_LOCATION_POSITION_VALID_BIT:
            print(f"HMD location: {hmd_location.pose}")
            found_count += 1
        for index, space in enumerate(action_spaces):
            space_location = xr.locate_space(
                space=space,
                base_space=reference_space,
                time=xr_time_now,
            )
            if space_location.location_flags & xr.SPACE_LOCATION_POSITION_VALID_BIT:
                print(f"Controller {index + 1}: {space_location.pose}")
                found_count += 1
        if found_count == 0:
            print("no controllers active")
        print("Sleeping for .5.....")
        # Sleep periodically to avoid consuming all available system resources
        time.sleep(0.500)

# Clean up
system = xr.NULL_SYSTEM_ID
xr.destroy_action_set(action_set)
action_set = None
xr.destroy_instance(instance)
instance = None
print("We are done cleaning up, exit")
exit()
