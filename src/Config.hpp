#pragma once
// Hang so chung cua game (di chuyen, quai, gameplay)

constexpr float EYE_HEIGHT       = 1.7f;
constexpr float CROUCH_EYE       = 1.05f;
constexpr float PLAYER_RADIUS    = 0.4f;
constexpr float RUN_SPEED        = 7.4f;
constexpr float WALK_SPEED       = 4.4f;
constexpr float CROUCH_SPEED     = 3.2f;
constexpr float GROUND_ACCEL     = 14.0f;
constexpr float AIR_ACCEL        = 9.0f;
constexpr float GROUND_FRICTION  = 8.0f;
constexpr float JUMP_SPEED       = 7.2f;
constexpr float GRAVITY          = 20.0f;

constexpr float BASE_MOUSE_SENS  = 0.0020f;   // radian / pixel (nhan voi Settings::sensitivity)
constexpr float MAX_RANGE        = 120.0f;
constexpr float FOV_DEG          = 75.0f;

constexpr int   PLAYER_MAX_HP    = 150;
constexpr int   ENEMY_MAX_HP     = 150;
constexpr float ENEMY_RADIUS     = 0.45f;
constexpr int   MAX_ENEMIES      = 14;

constexpr int   M4_KILL_AMMO_REWARD = 1;      // moi lan giet 1 quai -> M4 +3 dan
constexpr int   MAX_BULLET_HOLES    = 40;     // vuot qua -> xoa lo dan cu nhat
