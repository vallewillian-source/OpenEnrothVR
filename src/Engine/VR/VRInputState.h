#pragma once

struct VRInputState {
    float moveX = 0.0f;
    float moveY = 0.0f;
    float turnX = 0.0f;
    float turnY = 0.0f;
    bool jump = false;
    bool attack = false;
    bool castReady = false;
    bool interact = false;
    bool esc = false;
    bool combat = false;
    bool cast = false;
    bool flyUp = false;
    bool flyDown = false;
    bool quest = false;
    bool pass = false;
};
