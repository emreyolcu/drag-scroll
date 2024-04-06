#include <ApplicationServices/ApplicationServices.h>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#define SCALE 3
#define BUTTON 4

bool enabled = false;
CGPoint savedPoint;

CGEventRef cgEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
    switch (type) {
        case kCGEventOtherMouseDown:
            if ((int32_t)CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber) == BUTTON) {
                enabled = !enabled;
                savedPoint = CGEventGetLocation(event);
                
                if (enabled) {
                    CGSetLocalEventsSuppressionInterval(10.0);
                    CGWarpMouseCursorPosition(savedPoint);
                }
                else {
                    CGSetLocalEventsSuppressionInterval(0.0);
                    CGWarpMouseCursorPosition(savedPoint);
                    CGSetLocalEventsSuppressionInterval(0.25);
                }
                
                return NULL;
            }
            
            break;
            
        case kCGEventMouseMoved:
            if (enabled) {
                int32_t deltaX = (int32_t)CGEventGetIntegerValueField(event, kCGMouseEventDeltaX);
                int32_t deltaY = (int32_t)CGEventGetIntegerValueField(event, kCGMouseEventDeltaY);
                
                CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
                CGEventRef scrollEvent = CGEventCreateScrollWheelEvent(source, kCGScrollEventUnitPixel, 2, -SCALE * deltaY, -SCALE * deltaX);
                
                CGEventPost(kCGSessionEventTap, scrollEvent);
                
                CFRelease(scrollEvent);
                CFRelease(source);
                
                CGWarpMouseCursorPosition(savedPoint);
                
                return event;
            }
            
            break;
            
        default:
            break;
    }
    
    return event;
}

int main(void)
{
    CFMachPortRef eventTap;
    CFRunLoopSourceRef runLoopSource;
    
    CGEventMask mask = CGEventMaskBit(kCGEventMouseMoved) | CGEventMaskBit(kCGEventOtherMouseDown);
    
    eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, 0, mask, cgEventCallback, NULL);
    runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
    CFRunLoopRun();
    
    CFRelease(eventTap);
    CFRelease(runLoopSource);
    
    return 0;
}
