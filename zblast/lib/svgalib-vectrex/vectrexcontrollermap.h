/*****Vectrex Controller Button Mapping to PC Keyboard Keys*****/

#ifndef VECTREXCONTROLLERMAP_H
#define VECTREXCONTROLLERMAP_H

#include "vectrextokeyboard.h"

/* Digital Joystick States
 * Duplicate assignments will be ORed.
 * Direction keys are converted to diagonals (which can also be assigned directly)
 * where appropriate, and should be correctly affected by TRANSLATE_DIAGONAL.
 * Other keyboard_translatekeys() masks aren't used because there's not much point.
 */

#define JOY1UP		SCANCODE_CURSORBLOCKUP
#define JOY1DOWN	SCANCODE_CURSORBLOCKDOWN
#define JOY1LEFT	SCANCODE_CURSORBLOCKLEFT
#define JOY1RIGHT	SCANCODE_CURSORBLOCKRIGHT

/* Button States */

#define PORT1BUT1	SCANCODE_F /* Changed from Esc to make Calibration easier */
#define PORT1BUT2	SCANCODE_P
#define PORT1BUT3	SCANCODE_ENTER
#define PORT1BUT4	SCANCODE_SPACE
#define PORT2BUT1	SCANCODE_ESCAPE
#define PORT2BUT2	SCANCODE_P
#define PORT2BUT3	SCANCODE_ENTER
#define PORT2BUT4	SCANCODE_SPACE

#endif /* VECTREXCONTROLLERMAP_H */
