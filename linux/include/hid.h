#ifndef HID_H
#define HID_H

#include <stdint.h>

#define BIT(n) (1 << (n))
#define HID_EVENT_COUNT (14)
#define HID_HAS_KEY(state, key) (((state) & (key)) != 0)

enum {
    HID_KEY_A            = BIT(0),  ///< A
    HID_KEY_B            = BIT(1),  ///< B
    HID_KEY_SELECT       = BIT(2),  ///< Select
    HID_KEY_START        = BIT(3),  ///< Start
    HID_KEY_DRIGHT       = BIT(4),  ///< D-Pad Right
    HID_KEY_DLEFT        = BIT(5),  ///< D-Pad Left
    HID_KEY_DUP          = BIT(6),  ///< D-Pad Up
    HID_KEY_DDOWN        = BIT(7),  ///< D-Pad Down
    HID_KEY_R            = BIT(8),  ///< R
    HID_KEY_L            = BIT(9),  ///< L
    HID_KEY_X            = BIT(10), ///< X
    HID_KEY_Y            = BIT(11), ///< Y
    HID_KEY_ZL           = BIT(14), ///< ZL (New 3DS only)
    HID_KEY_ZR           = BIT(15), ///< ZR (New 3DS only)
    HID_KEY_TOUCH        = BIT(20), ///< Touch (Not actually provided by HID)
    HID_KEY_CSTICK_RIGHT = BIT(24), ///< C-Stick Right (New 3DS only)
    HID_KEY_CSTICK_LEFT  = BIT(25), ///< C-Stick Left (New 3DS only)
    HID_KEY_CSTICK_UP    = BIT(26), ///< C-Stick Up (New 3DS only)
    HID_KEY_CSTICK_DOWN  = BIT(27), ///< C-Stick Down (New 3DS only)
    HID_KEY_CPAD_RIGHT   = BIT(28), ///< Circle Pad Right
    HID_KEY_CPAD_LEFT    = BIT(29), ///< Circle Pad Left
    HID_KEY_CPAD_UP      = BIT(30), ///< Circle Pad Up
    HID_KEY_CPAD_DOWN    = BIT(31), ///< Circle Pad Down

    // Generic catch-all directions
    /// D-Pad Up or Circle Pad Up
    HID_KEY_UP = HID_KEY_DUP | HID_KEY_CPAD_UP,
    /// D-Pad Down or Circle Pad Down
    HID_KEY_DOWN = HID_KEY_DDOWN | HID_KEY_CPAD_DOWN,
    /// D-Pad Left or Circle Pad Left
    HID_KEY_LEFT = HID_KEY_DLEFT | HID_KEY_CPAD_LEFT,
    /// D-Pad Right or Circle Pad Right
    HID_KEY_RIGHT = HID_KEY_DRIGHT | HID_KEY_CPAD_RIGHT,
};

struct circlepos {
    int16_t dx;
    int16_t dy;
};

struct touchpos {
    uint16_t px;
    uint16_t py;
};

struct gyrorate {
    int16_t x;
    int16_t y;
    int16_t z;
};

struct accelrate {
    int16_t x;
    int16_t y;
    int16_t z;
};

struct hidinfo {
    uint16_t version;
    struct {
        uint32_t up;
        uint32_t down;
        uint32_t held;
    } keys;
    struct circlepos circlepad;
    struct circlepos cstick;
    struct touchpos touchscreen;
    struct gyrorate gyro;
    struct accelrate accel;
};

#endif /* ----- #ifndef HID_H  ----- */
