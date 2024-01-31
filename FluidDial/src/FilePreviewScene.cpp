// Copyright (c) 2023 - Barton Dringstarting
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

#include <Arduino.h>
#include "Scene.h"
#include "FileParser.h"

extern Scene menuScene;
extern Scene statusScene;

class FilePreviewScene : public Scene {
    String _filename;
    bool   _needlines;

public:
    FilePreviewScene() : Scene("Preview") {}
    void onEntry(void* arg) {
        char* fname = (char*)arg;
        _filename   = fname;
        _needlines  = true;
        request_file_preview(fname);
    }
    void onFileLines() {
        _needlines = false;
        reDisplay();
    }

    void onDialButtonPress() { activate_scene(&menuScene); }

    void onRedButtonPress() {
        if (state == Idle) {
            pop_scene();
            ackBeep();
        }
    }

    void onDROChange() { reDisplay(); }

    void onGreenButtonPress() {
        if (state == Idle) {
            String command = "$SD/Run=" + dirName + "/" + fileInfo.fileName;
            send_line(command.c_str());
            ackBeep();
        }
    }
    void reDisplay() {
        if (state == Cycle) {
            activate_scene(&statusScene);
            return;
        }
        String grnText, redText = "";

        canvas.createSprite(240, 240);
        drawBackground(BLACK);
        drawMenuTitle(name());
        drawStatusTiny(20);

        if (state == Idle) {
            if (_needlines == false) {
                int y  = 39;
                int tl = 0;
                if (fileLines.size()) {
                    for (auto const& line : fileLines) {
                        text(line, 25, y + tl * 22, WHITE, TINY, top_left);
                        ++tl;
                    }
                } else {
                    text("No Text", 120, 120, WHITE, SMALL, middle_center);
                }
            } else {
                text("Reading File", 120, 120, WHITE, TINY, middle_center);
            }
            grnText = "Run";
            redText = "Back";
        } else {
            centered_text("Invalid State", 105, WHITE, SMALL);
            centered_text("File Preview", 145, WHITE, SMALL);
        }

        drawButtonLegends(redText, grnText, "Menu");
        refreshDisplay();
    }
};
FilePreviewScene filePreviewScene;
