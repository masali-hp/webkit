// To prevent text selections or drag and drop from taking priority over
// a gesture, set HOLD_THRESHOLD.  The mouse has to remain within
// the PAN_THRESHOLD for HOLD_THRESHOLD miliseconds before a selection /
// drag can begin.
// If during the WM_GESTURENOTIFY, the app did not configure any gesture
// recognition, there is no threshold for a drag / text selection.
// To disable this threshold and allow drags / text selection to start
// immediately, set HOLD_THRESHOLD to 0.
#define HOLD_THRESHOLD 500 // miliseconds

// Overpan feedback is a gradient of this color from 0% opacity to 100% opacity.
#define OVERPAN_COLOR makeRGB(0, 0xa5, 0xec)

// The amount of time that the overpan feedback animation is shown.
#define OVERPAN_DURATION 0.25 // seconds

// When dragging your finger, accumulate between 0 - 100% of the distance dragged.
// 0%: don't overpan when dragging
// 50%: rubber band the size of the overpan when dragging
// 100%: match the overpan size to your finger
#define OVERPAN_DRAG 0.5 // 50%

// The percent size the overpan feedback will reduce to before it fades out
// completely.  Because the opacity also animates towards 0, it looks better
// to not animate the size all the way to 0.
#define OVERPAN_REDUCTION 0.5 // reduce by 50%

// The amount the opacity on the non-transparent end of the gradient reduces
// as the overpan fades out.  0 = does not fade out at all.  1.0 = fades out
// completely.
#define OVERPAN_OPACITY_REDUCTION 1.0

// The time from when overpan feedback is shown until it begins fading out.
// This should be small but not too small - at 0 unnecessary flicker occurs.
#define OVERPAN_START 0.1 // seconds

// The number of pixels for the max size of overpan feedback.
// Set to 0 to have no maximum.
#define OVERPAN_MAX_SIZE 80