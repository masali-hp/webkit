// To prevent text selections or drag and drop from taking priority over
// a gesture, set HOLD_THRESHOLD.  The mouse has to remain within
// the PAN_THRESHOLD for HOLD_THRESHOLD miliseconds before a selection /
// drag can begin.
// If during the WM_GESTURENOTIFY, the app did not configure any gesture
// recognition, there is no threshold for a drag / text selection.
// To disable this threshold and allow drags / text selection to start
// immediately, set HOLD_THRESHOLD to 0.
#define HOLD_THRESHOLD 500 // miliseconds