// Copyright (c) 2023 Mitch Bradley
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

#include "Drawing.h"
#include "alarm.h"
#include <map>
#include <LittleFS.h>

void drawBackground(int color) {
    canvas.fillSprite(color);
}

void drawFilledCircle(int x, int y, int radius, int fillcolor) {
    canvas.fillCircle(x, y, radius, fillcolor);
}
void drawFilledCircle(Point xy, int radius, int fillcolor) {
    Point dispxy = xy.to_display();
    drawFilledCircle(dispxy.x, dispxy.y, radius, fillcolor);
}

void drawCircle(int x, int y, int radius, int thickness, int outlinecolor) {
    for (int i = 0; i < thickness; i++) {
        canvas.drawCircle(x, y, radius - i, outlinecolor);
    }
}
void drawCircle(Point xy, int radius, int thickness, int outlinecolor) {
    Point dispxy = xy.to_display();
    drawCircle(dispxy.x, dispxy.y, radius, thickness, outlinecolor);
}

void drawOutlinedCircle(int x, int y, int radius, int fillcolor, int outlinecolor) {
    canvas.fillCircle(x, y, radius, fillcolor);
    canvas.drawCircle(x, y, radius, outlinecolor);
}
void drawOutlinedCircle(Point xy, int radius, int fillcolor, int outlinecolor) {
    Point dispxy = xy.to_display();
    drawOutlinedCircle(dispxy.x, dispxy.y, radius, fillcolor, outlinecolor);
}

void drawRect(int x, int y, int width, int height, int radius, int bgcolor) {
    canvas.fillRoundRect(x, y, width, height, radius, bgcolor);
}
void drawRect(Point xy, int width, int height, int radius, int bgcolor) {
    Point offsetxy = { width / 2, -height / 2 };
    Point dispxy   = (xy - offsetxy).to_display();
    drawRect(dispxy.x, dispxy.y, width, height, radius, bgcolor);
}
void drawRect(Point xy, Point wh, int radius, int bgcolor) {
    drawRect(xy, wh.x, wh.y, radius, bgcolor);
}

void drawOutlinedRect(int x, int y, int width, int height, int bgcolor, int outlinecolor) {
    canvas.fillRoundRect(x, y, width, height, 5, bgcolor);
    canvas.drawRoundRect(x, y, width, height, 5, outlinecolor);
}
void drawOutlinedRect(Point xy, int width, int height, int bgcolor, int outlinecolor) {
    Point dispxy = xy.to_display();
    drawOutlinedRect(dispxy.x, dispxy.y, width, height, bgcolor, outlinecolor);
}

void drawPngFile(const String& filename, int x, int y) {
    // When datum is middle_center, the origin is the center of the canvas and the
    // +Y direction is down.
    canvas.drawPngFile(LittleFS, filename, x, -y, 0, 0, 0, 0, 1.0f, 1.0f, datum_t::middle_center);
}
void drawPngFile(const String& filename, Point xy) {
    drawPngFile(filename, xy.x, xy.y);
}
void drawPngBackground(const String& filename) {
    drawPngFile(filename, 0, 0);
}

// clang-format off
std::map<state_t, int> stateColors = {
    { Idle,        WHITE },
    { Alarm,       RED },
    { CheckMode,   WHITE },
    { Homing,      CYAN },
    { Cycle,       GREEN },
    { Hold,        YELLOW },
    { Jog,         CYAN },
    { SafetyDoor,  WHITE },
    { Sleep,       WHITE },
    { ConfigAlarm, WHITE },
    { Critical,    WHITE },
    { Disconnected, RED },
};
// clang-format on

void drawStatus() {
    static constexpr int x      = 100;
    static constexpr int y      = 24;
    static constexpr int width  = 140;
    static constexpr int height = 36;

    canvas.fillRoundRect((display.width() - width) / 2, y, width, height, 5, stateColors[state]);
    if (state == Alarm) {
        centered_text(stateString, y + height / 2 - 4, BLACK, SMALL);
        centered_text(alarm_name[lastAlarm], y + height / 2 + 12, BLACK);
    } else {
        centered_text(stateString, y + height / 2 + 3, BLACK, MEDIUM);
    }
}

void drawStatusTiny(int y) {
    static constexpr int width  = 90;
    static constexpr int height = 20;

    canvas.fillRoundRect((display.width() - width) / 2, y, width, height, 5, stateColors[state]);
    centered_text(stateString, y + height / 2 + 3, BLACK, TINY);
}

Stripe::Stripe(int x, int y, int width, int height, fontnum_t font) : _x(x), _y(y), _width(width), _height(height), _font(font) {}

void Stripe::draw(const String& left, const String& right, bool highlighted, int left_color) {
    drawOutlinedRect(_x, _y, _width, _height, highlighted ? BLUE : NAVY, WHITE);
    if (left.length()) {
        text(left, text_left_x(), text_middle_y(), left_color, _font, middle_left);
    }
    if (right.length()) {
        text(right, text_right_x(), text_middle_y(), WHITE, _font, middle_right);
    }
    advance();
}
void Stripe::draw(const String& center, bool highlighted) {
    drawOutlinedRect(_x, _y, _width, _height, highlighted ? BLUE : NAVY, WHITE);
    text(center, text_center_x(), text_middle_y(), WHITE, _font, middle_center);
    advance();
}

#define PUSH_BUTTON_LINE 212
#define DIAL_BUTTON_LINE 228

// This shows on the display what the button currently do.
void drawButtonLegends(const String& red, const String& green, const String& orange) {
    text(red, 80, PUSH_BUTTON_LINE, RED);
    text(green, 160, PUSH_BUTTON_LINE, GREEN);
    centered_text(orange, DIAL_BUTTON_LINE, ORANGE);
}

void putDigit(int& n, int x, int y, int color) {
    char txt[2] = { '\0', '\0' };
    txt[0]      = "0123456789"[n % 10];
    n /= 10;
    text(txt, x, y, color, MEDIUM, middle_right);
}
void fancyNumber(float n, int n_decimals, int hl_digit, int x, int y, int text_color, int hl_text_color) {
    fontnum_t font     = SMALL;
    int       n_digits = n_decimals + 1;
    size_t    i;
    bool      isneg = n < 0;
    if (isneg) {
        n = -n;
    }
    for (i = 0; i < n_decimals; i++) {
        n *= 10;
    }
    const int char_width = 20;

    int ni = (int)n;
    for (i = 0; i < n_decimals; i++) {
        putDigit(ni, x, y, i == hl_digit ? hl_text_color : text_color);
        x -= char_width;
    }
    if (n_decimals) {
        text(".", x - 10, y, text_color, MEDIUM, middle_center);
        x -= char_width;
    }
    do {
        putDigit(ni, x, y, i++ == hl_digit ? hl_text_color : text_color);
        x -= char_width;
    } while (ni || i <= hl_digit);
    if (isneg) {
        text("-", x, y, text_color, MEDIUM, middle_right);
    }
}

void DRO::draw(int axis, int hl_digit, bool highlight) {
    text(axisNumToString(axis), text_left_x(), text_middle_y(), highlight ? GREEN : WHITE, MEDIUM, middle_left);
    fancyNumber(myAxes[axis], num_digits(), hl_digit, text_right_x(), text_middle_y(), WHITE, highlight ? RED : YELLOW);
    advance();
}

void DRO::draw(int axis, bool highlight) {
    Stripe::draw(axisNumToString(axis), floatToString(myAxes[axis], num_digits()), highlight, myLimitSwitches[axis] ? GREEN : WHITE);
}

void LED::draw(bool highlighted) {
    drawOutlinedCircle(_x, _y, _radius, (highlighted) ? GREEN : DARKGREY, WHITE);
    _y += _gap;
}

void drawMenuTitle(const String& name) {
    centered_text(name, 12);
}

void refreshDisplay() {
    display.startWrite();
    canvas.pushSprite(0, 0);
    display.endWrite();
}

void drawMenuView(std::vector<String> labels, int start, int selected) {}

void showImageFile(const char* name, int x, int y, int width, int height) {
    auto file = LittleFS.open(name);
    if (!file) {
        log_println("Can't open logo_img.bin");
        return;
    }
    auto      len   = file.size();
    uint16_t* buf   = (uint16_t*)malloc(len);
    auto      nread = file.read((uint8_t*)buf, len);
    display.pushImage(0, 70, width, height, buf, true);
    free(buf);
}

void showError() {
    if ((milliseconds() - errorExpire) < 0) {
        canvas.fillCircle(120, 120, 95, RED);
        drawCircle(120, 120, 95, 5, WHITE);
        centered_text("Error", 95, WHITE, MEDIUM);
        centered_text(decode_error_number(lastError), 140, WHITE, TINY);
    }
}
