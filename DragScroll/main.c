#include <ApplicationServices/ApplicationServices.h>

static int BUTTON;
static int SCALE;

static bool ENABLED = false;
static CGPoint POINT;

CGEventRef callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *userInfo)
{
    if (type == kCGEventOtherMouseDown
        && CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber) == BUTTON) {
        ENABLED = !ENABLED;
        POINT = CGEventGetLocation(event);

        if (ENABLED) {
            CGWarpMouseCursorPosition(POINT);
        } else {
            CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
            CGEventSourceSetLocalEventsSuppressionInterval(source, 0.0);
            CGWarpMouseCursorPosition(POINT);
            CGEventSourceSetLocalEventsSuppressionInterval(source, 0.25);
            CFRelease(source);
        }

        event = NULL;
    } else if (type == kCGEventMouseMoved && ENABLED) {
        int64_t deltaX = CGEventGetIntegerValueField(event, kCGMouseEventDeltaX);
        int64_t deltaY = CGEventGetIntegerValueField(event, kCGMouseEventDeltaY);
        CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
        CGEventRef scrollWheelEvent = CGEventCreateScrollWheelEvent(
            source,
            kCGScrollEventUnitPixel, 2, -SCALE * (int32_t)deltaY, -SCALE * (int32_t)deltaX
        );
        CGEventPost(kCGHeadInsertEventTap, scrollWheelEvent);
        CFRelease(source);
        CFRelease(scrollWheelEvent);

        CGWarpMouseCursorPosition(POINT);

        event = NULL;
    }

    return event;
}

void displayNoticeAndExit(CFStringRef alertHeader)
{
    CFUserNotificationDisplayNotice(
        0, kCFUserNotificationCautionAlertLevel,
        NULL, NULL, NULL,
        alertHeader, NULL, NULL
    );

    exit(EXIT_FAILURE);
}

void setIntPreference(CFStringRef key, int *valuePtr, int defaultValue)
{
    CFNumberRef value = (CFNumberRef)CFPreferencesCopyAppValue(
        key, kCFPreferencesCurrentApplication
    );
    Boolean got = false;
    if (value) {
        got = CFNumberGetValue(value, kCFNumberIntType, valuePtr);
        CFRelease(value);
    }
    if (!got)
        *valuePtr = defaultValue;
}

int main(void)
{
    CFDictionaryRef options = CFDictionaryCreate(
        kCFAllocatorDefault,
        (const void **)&kAXTrustedCheckOptionPrompt, (const void **)&kCFBooleanTrue, 1,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks
    );
    Boolean trusted = AXIsProcessTrustedWithOptions(options);
    CFRelease(options);
    if (!trusted)
        displayNoticeAndExit(
            CFSTR("Restart DragScroll after granting it access to accessibility features.")
        );

    setIntPreference(CFSTR("button"), &BUTTON, 4);
    if (BUTTON < 2 || BUTTON > 31)
        displayNoticeAndExit(
            CFSTR("DragScroll supports up to 32 mouse buttons. "
                  "Set \"button\" to a value between 2 and 31.")
        );
    setIntPreference(CFSTR("scale"), &SCALE, 3);

    CFMachPortRef tap = CGEventTapCreate(
        kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault,
        CGEventMaskBit(kCGEventOtherMouseDown) | CGEventMaskBit(kCGEventMouseMoved), callback, NULL
    );
    if (!tap)
        displayNoticeAndExit(CFSTR("DragScroll could not create an event tap."));
    CFRunLoopSourceRef source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0);
    if (!source)
        displayNoticeAndExit(CFSTR("DragScroll could not create a run loop source."));
    CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopDefaultMode);
    CFRelease(tap);
    CFRelease(source);
    CFRunLoopRun();

    return EXIT_SUCCESS;
}
