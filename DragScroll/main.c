#include <ApplicationServices/ApplicationServices.h>

#define DEFAULT_BUTTON 5
#define DEFAULT_KEYS kCGEventFlagMaskShift
#define DEFAULT_SCALE 3
#define MAX_KEY_COUNT 5
#define EQ(x, y) (CFStringCompare(x, y, kCFCompareCaseInsensitive) == kCFCompareEqualTo)

static int BUTTON;
static int KEYS;
static int SCALE;

static bool BUTTON_ENABLED;
static bool KEY_ENABLED;
static CGPoint POINT;

void maybeSetAndWarp(bool thisEnabled, bool otherEnabled, CGPoint location)
{
    if (!otherEnabled) {
        POINT = location;
        CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
        if (thisEnabled) {
            CGEventSourceSetLocalEventsSuppressionInterval(source, 10.0);
            CGWarpMouseCursorPosition(POINT);
        } else {
            CGEventSourceSetLocalEventsSuppressionInterval(source, 0.0);
            CGWarpMouseCursorPosition(POINT);
            CGEventSourceSetLocalEventsSuppressionInterval(source, 0.25);
        }
        CFRelease(source);
    }
}

CGEventRef callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *userInfo)
{
    if (type == kCGEventMouseMoved && (BUTTON_ENABLED || KEY_ENABLED)) {
        int deltaX = (int)CGEventGetIntegerValueField(event, kCGMouseEventDeltaX);
        int deltaY = (int)CGEventGetIntegerValueField(event, kCGMouseEventDeltaY);
        CGEventRef scrollWheelEvent = CGEventCreateScrollWheelEvent(
            NULL, kCGScrollEventUnitPixel, 2, -SCALE * deltaY, -SCALE * deltaX
        );
        if (KEY_ENABLED)
            CGEventSetFlags(scrollWheelEvent, CGEventGetFlags(event) & ~KEYS);
        CGEventTapPostEvent(proxy, scrollWheelEvent);
        CFRelease(scrollWheelEvent);
        CGWarpMouseCursorPosition(POINT);
        event = NULL;
    } else if (type == kCGEventOtherMouseDown
               && CGEventGetFlags(event) == 0
               && CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber) == BUTTON) {
        BUTTON_ENABLED = !BUTTON_ENABLED;
        maybeSetAndWarp(BUTTON_ENABLED, KEY_ENABLED, CGEventGetLocation(event));
        event = NULL;
    } else if (type == kCGEventFlagsChanged) {
        KEY_ENABLED = (CGEventGetFlags(event) & KEYS) == KEYS;
        maybeSetAndWarp(KEY_ENABLED, BUTTON_ENABLED, CGEventGetLocation(event));
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

bool getIntPreference(CFStringRef key, int *valuePtr)
{
    CFNumberRef number = (CFNumberRef)CFPreferencesCopyAppValue(
        key, kCFPreferencesCurrentApplication
    );
    bool got = false;
    if (number) {
        if (CFGetTypeID(number) == CFNumberGetTypeID())
            got = CFNumberGetValue(number, kCFNumberIntType, valuePtr);
        CFRelease(number);
    }

    return got;
}

bool getArrayPreference(CFStringRef key, CFStringRef *values, int *count, int maxCount)
{
    CFArrayRef array = (CFArrayRef)CFPreferencesCopyAppValue(
        key, kCFPreferencesCurrentApplication
    );
    bool got = false;
    if (array) {
        if (CFGetTypeID(array) == CFArrayGetTypeID()) {
            CFIndex c = CFArrayGetCount(array);
            if (c <= maxCount) {
                CFArrayGetValues(array, CFRangeMake(0, c), (const void **)values);
                *count = (int)c;
                got = true;
            }
        }
        CFRelease(array);
    }

    return got;
}

int main(void)
{
    CFDictionaryRef options = CFDictionaryCreate(
        kCFAllocatorDefault,
        (const void **)&kAXTrustedCheckOptionPrompt, (const void **)&kCFBooleanTrue, 1,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks
    );
    bool trusted = AXIsProcessTrustedWithOptions(options);
    CFRelease(options);
    if (!trusted)
        displayNoticeAndExit(
            CFSTR("Restart DragScroll after granting it access to accessibility features.")
        );

    if (!(getIntPreference(CFSTR("button"), &BUTTON)
          && (BUTTON == 0 || (BUTTON >= 3 && BUTTON <= 32))))
        BUTTON = DEFAULT_BUTTON;

    CFStringRef names[MAX_KEY_COUNT];
    int count;
    if (getArrayPreference(CFSTR("keys"), names, &count, MAX_KEY_COUNT)) {
        KEYS = 0;
        for (int i = 0; i < count; i++) {
            if (EQ(names[i], CFSTR("capslock"))) {
                KEYS |= kCGEventFlagMaskAlphaShift;
            } else if (EQ(names[i], CFSTR("shift"))) {
                KEYS |= kCGEventFlagMaskShift;
            } else if (EQ(names[i], CFSTR("control"))) {
                KEYS |= kCGEventFlagMaskControl;
            } else if (EQ(names[i], CFSTR("option"))) {
                KEYS |= kCGEventFlagMaskAlternate;
            } else if (EQ(names[i], CFSTR("command"))) {
                KEYS |= kCGEventFlagMaskCommand;
            } else {
                KEYS = DEFAULT_KEYS;
                break;
            }
        }
    } else {
        KEYS = DEFAULT_KEYS;
    }

    if (!getIntPreference(CFSTR("scale"), &SCALE))
        SCALE = DEFAULT_SCALE;

    CGEventMask events = CGEventMaskBit(kCGEventMouseMoved);
    if (BUTTON != 0) {
        events |= CGEventMaskBit(kCGEventOtherMouseDown);
        BUTTON--;
    }
    if (KEYS != 0)
        events |= CGEventMaskBit(kCGEventFlagsChanged);
    CFMachPortRef tap = CGEventTapCreate(
        kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault,
        events, callback, NULL
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
