#include <ApplicationServices/ApplicationServices.h>

#define DEFAULT_BUTTON 5
#define DEFAULT_KEYS kCGEventFlagMaskShift
#define DEFAULT_SCALE 3
#define MAX_KEY_COUNT 5
#define EQ(x, y) (CFStringCompare(x, y, kCFCompareCaseInsensitive) == kCFCompareEqualTo)

static const CFStringRef AX_NOTIFICATION = CFSTR("com.apple.accessibility.api");
static bool TRUSTED;

static CFMachPortRef TAP;
static CFRunLoopSourceRef SOURCE;

static int BUTTON;
static int KEYS;
static int SCALE;

static bool BUTTON_ENABLED;
static bool KEY_ENABLED;
static CGPoint POINT;

static void maybeSetPointAndWarpMouse(bool thisEnabled, bool otherEnabled, CGEventRef event)
{
    if (!otherEnabled) {
        POINT = CGEventGetLocation(event);
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

static CGEventRef tapCallback(CGEventTapProxy proxy,
                              CGEventType type, CGEventRef event, void *userInfo)
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
        maybeSetPointAndWarpMouse(BUTTON_ENABLED, KEY_ENABLED, event);
        event = NULL;
    } else if (type == kCGEventFlagsChanged) {
        KEY_ENABLED = (CGEventGetFlags(event) & KEYS) == KEYS;
        maybeSetPointAndWarpMouse(KEY_ENABLED, BUTTON_ENABLED, event);
    }

    return event;
}

static void displayNoticeAndExit(CFStringRef alertHeader)
{
    CFUserNotificationDisplayNotice(
        0, kCFUserNotificationCautionAlertLevel,
        NULL, NULL, NULL,
        alertHeader, NULL, NULL
    );

    exit(EXIT_FAILURE);
}

static void notificationCallback(CFNotificationCenterRef center, void *observer,
                                 CFNotificationName name, const void *object,
                                 CFDictionaryRef userInfo)
{
    if (CFStringCompare(name, AX_NOTIFICATION, 0) == kCFCompareEqualTo) {
        CFRunLoopRef runLoop = CFRunLoopGetCurrent();
        CFRunLoopPerformBlock(
            runLoop, kCFRunLoopDefaultMode, ^{
                bool previouslyTrusted = TRUSTED;
                if ((TRUSTED = AXIsProcessTrusted()) != previouslyTrusted) {
                    CFRunLoopStop(runLoop);
                    if (SOURCE && CFRunLoopContainsSource(runLoop, SOURCE, kCFRunLoopDefaultMode)) {
                        CGEventTapEnable(TAP, TRUSTED);
                        CFRunLoopRun();
                    } else if (!TRUSTED) {
                        CFRunLoopRun();
                    }
                }
            }
        );
    }
}

static bool getIntPreference(CFStringRef key, int *valuePtr)
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

static bool getArrayPreference(CFStringRef key, CFStringRef *values, int *count, int maxCount)
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
    CFNotificationCenterAddObserver(
        CFNotificationCenterGetDistributedCenter(), NULL,
        notificationCallback, AX_NOTIFICATION, NULL,
        CFNotificationSuspensionBehaviorDeliverImmediately
    );
    CFDictionaryRef options = CFDictionaryCreate(
        kCFAllocatorDefault,
        (const void **)&kAXTrustedCheckOptionPrompt, (const void **)&kCFBooleanTrue, 1,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks
    );
    TRUSTED = AXIsProcessTrustedWithOptions(options);
    CFRelease(options);
    if (!TRUSTED)
        CFRunLoopRun();

    if (!(getIntPreference(CFSTR("button"), &BUTTON)
          && (BUTTON == 0 || (BUTTON >= 3 && BUTTON <= 32))))
        BUTTON = DEFAULT_BUTTON;

    CFStringRef keyNames[MAX_KEY_COUNT];
    int keyCount;
    if (getArrayPreference(CFSTR("keys"), keyNames, &keyCount, MAX_KEY_COUNT)) {
        KEYS = 0;
        for (int i = 0; i < keyCount; i++) {
            if (EQ(keyNames[i], CFSTR("capslock"))) {
                KEYS |= kCGEventFlagMaskAlphaShift;
            } else if (EQ(keyNames[i], CFSTR("shift"))) {
                KEYS |= kCGEventFlagMaskShift;
            } else if (EQ(keyNames[i], CFSTR("control"))) {
                KEYS |= kCGEventFlagMaskControl;
            } else if (EQ(keyNames[i], CFSTR("option"))) {
                KEYS |= kCGEventFlagMaskAlternate;
            } else if (EQ(keyNames[i], CFSTR("command"))) {
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
    TAP = CGEventTapCreate(
        kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault,
        events, tapCallback, NULL
    );
    if (!TAP)
        displayNoticeAndExit(CFSTR("DragScroll could not create an event tap."));
    SOURCE = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, TAP, 0);
    if (!SOURCE)
        displayNoticeAndExit(CFSTR("DragScroll could not create a run loop source."));
    CFRunLoopAddSource(CFRunLoopGetCurrent(), SOURCE, kCFRunLoopDefaultMode);
    CFRunLoopRun();

    return EXIT_SUCCESS;
}
