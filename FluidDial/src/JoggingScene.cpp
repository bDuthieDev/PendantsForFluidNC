// Copyright (c) 2023 - Barton Dring
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

#include "Config.h"

#ifndef USE_MULTI_JOG
#    include <Arduino.h>
#    include "Scene.h"

class JoggingScene : public Scene {
private:
    static const int MAX_INC = 5;
    static const int n_axes  = 3;

    int  _active_setting = 0;  // Dist or Rate
    int  _selection      = 0;
    bool _continuous     = false;

    int _axis = 0;  // the axis currently being jogged

    int _cont_speed[2][3] = { { 1000, 1000, 1000 }, { 40, 40, 40 } };

    // Saved to NVS
    int _inc_level[2][3]  = { { 3, 3, 2 }, { 2, 2, 1 } };  // exponent 0=0.001, 1=0.01, 2=0.1 3=1 4=10 5 = 100.00
    int _rate_level[2][3] = { { 1000, 1000, 100 }, { 40, 40, 4 } };

    float _increment() { return pow(10.0, abs(_inc_level[inInches][_axis])) / 1000.0; }

    void feedRateRotator(int& rate, bool up) {
        if (up) {
            if (rate < 10) {
                rate += 1;
            } else if (rate < 100) {
                rate += 10;
            } else if (rate < 1000) {
                rate += 100;
            } else {
                rate += 1000;
            }
        } else {
            if (rate > 1000) {
                rate -= 1000;
            } else if (rate > 100) {
                rate -= 100;
            } else if (rate > 10) {
                rate -= 10;
            } else if (rate > 2) {
                rate -= 1;
            }
        }
    }
    void cancelJog() {
        if (state == Jog) {
            fnc_realtime(JogCancel);
        }
    }

public:
    JoggingScene() : Scene("MPG Jog", 4) {}

    void onEntry(void* arg) {
        if (initPrefs()) {
            for (size_t axis = 0; axis < n_axes; axis++) {
                getPref(inInches ? "InchIncLevel" : "IncLevel", axis, &_inc_level[inInches][axis]);
                getPref(inInches ? "InchRateLevel" : "RateLevel", axis, &_rate_level[inInches][axis]);
            }
        }
    }
    void onExit() { cancelJog(); }

    void onDialButtonPress() { pop_scene(); }

    static void jogCommand(pos_t speed, int axis, float distance, int digits) {
        String cmd = "$J=G91";
        cmd += inInches ? "G20" : "G21";
        cmd += "F";
        cmd += floatToString(speed, 0);
        cmd += axisNumToString(axis);
        cmd += floatToString(distance, digits);
        log_println(cmd);
        send_line(cmd);
    }

    void onGreenButtonPress() {
        if (state == Idle) {
            if (_continuous) {
                // $J=G91F1000X10000
                jogCommand(_cont_speed[inInches][_axis], _axis, 10000.0, 0);
            } else {
                if (_active_setting == 0) {
                    if (_inc_level[inInches][_axis] != MAX_INC) {
                        _inc_level[inInches][_axis]++;
                        setPref(inInches ? "InchIncLevel" : "IncLevel", _axis, _inc_level[inInches][_axis]);
                    }
                } else {
                    feedRateRotator(_rate_level[inInches][_axis], true);
                    setPref(inInches ? "InchRateLevel" : "RateLevel", _axis, _rate_level[inInches][_axis]);
                }
            }
            reDisplay();

            return;
        }
        cancelJog();
    }
    void onGreenButtonRelease() {
        if (_continuous) {
            cancelJog();
        }
    }

    void onRedButtonPress() {
        if (state == Idle) {
            if (_continuous) {
                // $J=G91F1000X-10000
                jogCommand(_cont_speed[inInches][_axis], _axis, -10000.0, 0);
            } else {
                if (_active_setting == 0) {
                    if (_inc_level[inInches][_axis] > 0) {
                        _inc_level[inInches][_axis]--;
                        setPref("IncLevel", _axis, _inc_level[inInches][_axis]);
                    }
                } else {
                    feedRateRotator(_rate_level[inInches][_axis], false);
                    setPref("RateLevel", _axis, _rate_level[inInches][_axis]);
                }
            }
            reDisplay();
            return;
        }
        if (state == Jog || state == Cycle) {
            if (!_continuous) {
                fnc_realtime(Reset);
            }
            return;
        }
    }
    void onRedButtonRelease() {
        if (_continuous) {
            cancelJog();
        }
    }

    void onTouchRelease(int x, int y) {
        //Use dial to break out of continuous mode
        if (state == Idle) {
            if (y < 70) {
                _continuous = !_continuous;
            } else if (y < 105) {
                rotateNumberLoop(_axis, 1, 0, 2);
            } else if (y < 140) {
                String cmd = "G10L20P0" + axisNumToString(_axis) + "0";
                log_println(cmd);
                send_line(cmd);
            } else {
                rotateNumberLoop(_active_setting, 1, 0, 1);
            }
            ackBeep();
        }
        reDisplay();
    }

    void onDROChange() { reDisplay(); }
    void onLimitsChange() { reDisplay(); }
    void onAlarm() { reDisplay(); }

    void onEncoder(int delta) {
        if (state == Idle && _continuous) {
            feedRateRotator(_cont_speed[inInches][_axis], delta > 0);
        } else {
            // $J=G91F200Z5.0
            float increment = delta > 0 ? _increment() : -_increment();
            jogCommand(_rate_level[inInches][_axis], _axis, increment, num_digits());
        }
        reDisplay();
    }

    void reDisplay() {
        drawBackground(BLACK);
        String legend;

        legend = _continuous ? "Bttn Jog" : "MPG Jog";
        centered_text(legend, 12);

        drawStatus();

        if (state == Idle || state == Jog) {
            int x      = 14;
            int y      = 67;
            int width  = display.width() - x * 2;
            int height = 38;

            DRO dro(x, y, width, 35);
            dro.draw(_axis, true);

            x = 60;
            y += height + 5;
            width  = display.width() - x * 2;
            height = 48;

            if (state == Idle) {
                Stripe stripe(x, y, width, height, TINY);
                legend = "Zero " + String("XYZ").substring(_axis, _axis + 1) + " Axis";
                stripe.draw(legend, true);
            }

            if (_continuous) {
                legend = "Rate: " + floatToString(_cont_speed[inInches][_axis], 0);
                centered_text(legend, 183);
            } else {
                legend = "Increment: " + floatToString(_increment(), num_digits());
                centered_text(legend, 174, _active_setting == 0 ? WHITE : DARKGREY);
                legend = "Rate: " + floatToString(_rate_level[inInches][_axis], 2);
                centered_text(legend, 194, _active_setting == 1 ? WHITE : DARKGREY);
            }

            const char* back = "Back";
            switch (state) {
                case Idle:
                    if (_continuous) {
                        if (_selection % 2) {
                            drawButtonLegends("", "Zero " + axisNumToString(_axis), back);
                        } else {
                            drawButtonLegends("Jog-", "Jog+", back);
                        }
                    } else {
                        if (_selection % 2) {  // if zro selected
                            drawButtonLegends("", "Zero " + axisNumToString(_axis), back);
                        } else {
                            drawButtonLegends("Dec", "Inc", back);
                        }
                    }
                    break;
                case Jog:
                    if (_continuous) {
                        drawButtonLegends("Jog-", "Jog+", back);
                    } else {
                        drawButtonLegends("E-Stop", "Cancel", back);
                    }
                    break;
                case Alarm:
                    drawButtonLegends("", "", back);
                    break;
            }
        } else {
            centered_text("Invalid State", 105, WHITE, MEDIUM);
            centered_text("For Jogging", 145, WHITE, MEDIUM);

            drawButtonLegends("E-Stop", "", "Menu");
        }

        refreshDisplay();
    }
};
JoggingScene joggingScene;
#endif
