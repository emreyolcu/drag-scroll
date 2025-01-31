#include <ApplicationServices/ApplicationServices.h>

#define DEFAULT_BUTTON 3
#define DEFAULT_KEYS kCGEventFlagMaskShift
#define DEFAULT_SPEED 2
#define MAX_KEY_COUNT 5
#define HOLD_THRESHOLD 0.3
#define EQ(x, y) (CFStringCompare(x, y, kCFCompareCaseInsensitive) == kCFCompareEqualTo)

static CFStringRef AX_NOTIFICATION;
static bool TRUSTED;

static int BUTTON;
static int KEYS;
static int SPEED;

static bool BUTTON_ENABLED;
static bool KEY_ENABLED;
static CGPoint POINT;

typedef struct
{
    CGPoint location;
    bool isHolding;
    CFAbsoluteTime pressStartTime;
    int lastDeltaX;
    int lastDeltaY;
} MouseState;

static MouseState mouseState = {0};
static bool legacyMode = false;

static void maybeSetPointAndWarpMouse(bool thisEnabled, bool otherEnabled,
                                      CGEventRef event)
{
    if (!otherEnabled)
    {
        POINT = CGEventGetLocation(event);

        CGEventSourceRef source =
            CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);

        if (thisEnabled)
        {
            CGEventSourceSetLocalEventsSuppressionInterval(source, 10.0);
            CGWarpMouseCursorPosition(POINT);
        }
        else
        {
            CGEventSourceSetLocalEventsSuppressionInterval(source, 0.0);
            CGWarpMouseCursorPosition(POINT);
            CGEventSourceSetLocalEventsSuppressionInterval(source, 0.25);
        }

        CFRelease(source);
    }
}

static CGEventRef handleMouseDrag(CGEventTapProxy proxy, CGEventRef event)
{
    mouseState.isHolding = true;
    mouseState.lastDeltaX = (int)CGEventGetIntegerValueField(event, kCGMouseEventDeltaX);
    mouseState.lastDeltaY = (int)CGEventGetIntegerValueField(event, kCGMouseEventDeltaY);

    CGEventRef scrollEvent = CGEventCreateScrollWheelEvent(
        NULL, kCGScrollEventUnitPixel, 2,
        -SPEED * mouseState.lastDeltaY,
        -SPEED * mouseState.lastDeltaX);

    if (scrollEvent)
    {
        CGEventSetLocation(scrollEvent, mouseState.location);
        CGEventTapPostEvent(proxy, scrollEvent);
        CFRelease(scrollEvent);
    }

    CGWarpMouseCursorPosition(mouseState.location);
    return NULL;
}

static CGEventRef tapCallback(CGEventTapProxy proxy, CGEventType type,
                              CGEventRef event, void *userInfo)
{
    const char *eventName;

    switch (type)
    {
    case kCGEventLeftMouseDown:
        eventName = "LeftMouseDown";
        break;
    case kCGEventLeftMouseUp:
        eventName = "LeftMouseUp";
        break;
    case kCGEventRightMouseDown:
        eventName = "RightMouseDown";
        break;
    case kCGEventRightMouseUp:
        eventName = "RightMouseUp";
        break;
    case kCGEventMouseMoved:
        eventName = "MouseMoved";
        break;
    case kCGEventLeftMouseDragged:
        eventName = "LeftMouseDragged";
        break;
    case kCGEventRightMouseDragged:
        eventName = "RightMouseDragged";
        break;
    case kCGEventOtherMouseDown:
        eventName = "OtherMouseDown";
        break;
    case kCGEventOtherMouseUp:
        eventName = "OtherMouseUp";
        break;
    case kCGEventOtherMouseDragged:
        eventName = "OtherMouseDragged";
        break;
    case kCGEventScrollWheel:
        eventName = "ScrollWheel";
        break;
    case kCGEventFlagsChanged:
        eventName = "FlagsChanged";
        break;
    default:
        eventName = "Other";
        break;
    }

    if (legacyMode)
    {
        if (type == kCGEventOtherMouseDown &&
            CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber) == BUTTON)
        {
            BUTTON_ENABLED = !BUTTON_ENABLED;
            maybeSetPointAndWarpMouse(BUTTON_ENABLED, KEY_ENABLED, event);
            return NULL;
        }
    }
    else
    {
        if (type == kCGEventOtherMouseDown &&
            CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber) == BUTTON)
        {
            BUTTON_ENABLED = true;
            maybeSetPointAndWarpMouse(true, KEY_ENABLED, event);
            POINT = CGEventGetLocation(event);
            return event;
        }
        else if (type == kCGEventOtherMouseUp &&
                 CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber) == BUTTON)
        {
            BUTTON_ENABLED = false;
            maybeSetPointAndWarpMouse(false, KEY_ENABLED, event);
            return event;
        }
    }

    if ((type == kCGEventMouseMoved || type == kCGEventOtherMouseDragged) && (BUTTON_ENABLED || KEY_ENABLED))
    {
        CGPoint currentLocation = CGEventGetLocation(event);
        int deltaX = (int)CGEventGetIntegerValueField(event, kCGMouseEventDeltaX);
        int deltaY = (int)CGEventGetIntegerValueField(event, kCGMouseEventDeltaY);

        CGEventRef scrollEvent = CGEventCreateScrollWheelEvent(
            NULL, kCGScrollEventUnitPixel, 2,
            -SPEED * deltaY,
            -SPEED * deltaX);

        CGEventSetLocation(scrollEvent, currentLocation);

        if (KEY_ENABLED)
        {
            CGEventSetFlags(scrollEvent, CGEventGetFlags(event) & ~KEYS);
        }

        CGEventTapPostEvent(proxy, scrollEvent);
        CFRelease(scrollEvent);

        return NULL;
    }
    else if (type == kCGEventFlagsChanged)
    {
        KEY_ENABLED = (CGEventGetFlags(event) & KEYS) == KEYS;
        maybeSetPointAndWarpMouse(KEY_ENABLED, BUTTON_ENABLED, event);
    }

    return event;
}

static void displayNoticeAndExit(CFStringRef alertHeader)
{
    CFUserNotificationDisplayNotice(0, kCFUserNotificationCautionAlertLevel,
                                    NULL, NULL, NULL, alertHeader, NULL, NULL);

    exit(EXIT_FAILURE);
}

static void notificationCallback(CFNotificationCenterRef center, void *observer,
                                 CFNotificationName name, const void *object,
                                 CFDictionaryRef userInfo)
{
    static bool previouslyTrusted = false;

    if (!previouslyTrusted)
    {
        bool isTrusted = AXIsProcessTrusted();
        if (isTrusted)
        {
            previouslyTrusted = true;
            CFRunLoopStop(CFRunLoopGetCurrent());
        }
    }
}

static bool getIntPreference(const CFStringRef key, int *valuePtr)
{
    CFNumberRef number = (CFNumberRef)CFPreferencesCopyAppValue(
        key, kCFPreferencesCurrentApplication);
    bool got = false;
    if (number)
    {
        if (CFNumberGetValue(number, kCFNumberIntType, valuePtr))
        {
            got = true;
        }
        CFRelease(number);
    }
    return got;
}

static bool getBoolPreference(const CFStringRef key, bool *valuePtr)
{
    CFBooleanRef boolRef = (CFBooleanRef)CFPreferencesCopyAppValue(key, kCFPreferencesCurrentApplication);
    bool got = false;
    if (boolRef)
    {
        if (CFGetTypeID(boolRef) == CFBooleanGetTypeID())
        {
            *valuePtr = CFBooleanGetValue(boolRef);
            got = true;
        }
        CFRelease(boolRef);
    }
    return got;
}

static bool getArrayPreference(CFStringRef key, CFStringRef *values, int *count,
                               int maxCount)
{
    if (!values || !count)
        return false;

    CFArrayRef array = (CFArrayRef)CFPreferencesCopyAppValue(
        key, kCFPreferencesCurrentApplication);
    bool got = false;

    if (array && CFGetTypeID(array) == CFArrayGetTypeID())
    {
        CFIndex c = CFArrayGetCount(array);
        if (c <= maxCount)
        {
            CFArrayGetValues(array, CFRangeMake(0, c), (const void **)values);
            *count = (int)c;
            got = true;
        }
    }

    if (array)
        CFRelease(array);
    return got;
}

int main(void)
{
    AX_NOTIFICATION = CFSTR("com.apple.accessibility.api");
    CFNotificationCenterRef center = CFNotificationCenterGetDistributedCenter();
    static char observer;

    CFNotificationCenterAddObserver(
        center, &observer, notificationCallback, AX_NOTIFICATION, NULL,
        CFNotificationSuspensionBehaviorDeliverImmediately);

    CFDictionaryRef options = CFDictionaryCreate(
        kCFAllocatorDefault, (const void **)&kAXTrustedCheckOptionPrompt,
        (const void **)&kCFBooleanTrue, 1, &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);
    TRUSTED = AXIsProcessTrustedWithOptions(options);
    CFRelease(options);

    if (!TRUSTED)
        CFRunLoopRun();
    CFNotificationCenterRemoveObserver(center, &observer, AX_NOTIFICATION,
                                       NULL);

    if (!(getIntPreference(CFSTR("button"), &BUTTON) &&
          (BUTTON == 0 || (BUTTON >= 3 && BUTTON <= 32))))
        BUTTON = DEFAULT_BUTTON;

    CFStringRef keyNames[MAX_KEY_COUNT];
    int keyCount;
    if (getArrayPreference(CFSTR("keys"), keyNames, &keyCount, MAX_KEY_COUNT))
    {
        KEYS = 0;
        for (int i = 0; i < keyCount; i++)
        {
            if (EQ(keyNames[i], CFSTR("capslock")))
            {
                KEYS |= kCGEventFlagMaskAlphaShift;
            }
            else if (EQ(keyNames[i], CFSTR("shift")))
            {
                KEYS |= kCGEventFlagMaskShift;
            }
            else if (EQ(keyNames[i], CFSTR("control")))
            {
                KEYS |= kCGEventFlagMaskControl;
            }
            else if (EQ(keyNames[i], CFSTR("option")))
            {
                KEYS |= kCGEventFlagMaskAlternate;
            }
            else if (EQ(keyNames[i], CFSTR("command")))
            {
                KEYS |= kCGEventFlagMaskCommand;
            }
            else
            {
                KEYS = DEFAULT_KEYS;
                break;
            }
        }
    }
    else
    {
        KEYS = DEFAULT_KEYS;
    }

    if (!getBoolPreference(CFSTR("legacy_button_hold_behaviour"), &legacyMode))
        legacyMode = false;

    if (!getIntPreference(CFSTR("speed"), &SPEED))
        SPEED = DEFAULT_SPEED;

    CGEventMask events = CGEventMaskBit(kCGEventMouseMoved);
    if (BUTTON != 0)
    {
        events |= CGEventMaskBit(kCGEventOtherMouseDown) |
                  CGEventMaskBit(kCGEventOtherMouseUp) |
                  CGEventMaskBit(kCGEventOtherMouseDragged) |
                  CGEventMaskBit(kCGEventLeftMouseDown) |
                  CGEventMaskBit(kCGEventLeftMouseUp) |
                  CGEventMaskBit(kCGEventLeftMouseDragged) |
                  CGEventMaskBit(kCGEventRightMouseDown) |
                  CGEventMaskBit(kCGEventRightMouseUp) |
                  CGEventMaskBit(kCGEventRightMouseDragged);
        BUTTON--;
    }
    if (KEYS != 0)
        events |= CGEventMaskBit(kCGEventFlagsChanged);
    CFMachPortRef tap =
        CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap,
                         kCGEventTapOptionDefault, events, tapCallback, NULL);
    if (!tap)
        displayNoticeAndExit(
            CFSTR("DragScroll could not create an event tap."));
    CFRunLoopSourceRef source =
        CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0);
    if (!source)
        displayNoticeAndExit(
            CFSTR("DragScroll could not create a run loop source."));
    CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopDefaultMode);
    CFRelease(tap);
    CFRelease(source);
    CFRunLoopRun();

    return EXIT_SUCCESS;
}
