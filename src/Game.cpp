#include "Game.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>

// ============================================================
// Khoi tao / vong lap
// ============================================================
bool Game::init() {
    if (!glfwInit()) {
        std::cerr << "Khong the khoi tao GLFW\n";
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4);

    window_ = glfwCreateWindow(1280, 720, "Shooter 3D", nullptr, nullptr);
    if (!window_) {
        std::cerr << "Khong the tao cua so\n";
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    int version = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
    if (version == 0) {
        std::cerr << "Khong the khoi tao GLAD\n";
        return false;
    }
    std::cout << "OpenGL " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << "\n";

    glfwGetFramebufferSize(window_, &fbW_, &fbH_);
    glfwSetWindowUserPointer(window_, &input_);     // Input dung cho callback cuon chuot
    input_.attach(window_);
    glfwSetFramebufferSizeCallback(window_, [](GLFWwindow* w, int fw, int fh) {
        (void)w;
        glViewport(0, 0, fw, fh);
    });

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    settings_.load();
    scores_.load();

    if (!shaders_.init()) std::cerr << "Canh bao: loi shader (xem log o tren)\n";
    r2d_.init(&shaders_.ui);
    ui_.init(&r2d_, &input_);

    // Man hinh loading (nap model GLB mat vai giay)
    glClearColor(0.05f, 0.06f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    r2d_.begin(fbW_, fbH_);
    r2d_.text("LOADING...", fbW_ * 0.5f, fbH_ * 0.5f - 10, 6.0f * std::max(0.5f, fbH_ / 720.0f), {1, 1, 1, 1},
              TextAlign::Center);
    r2d_.end();
    glfwSwapBuffers(window_);

    if (!modelM4_.load(assetPath("model/M4.glb")) || !modelMark23_.load(assetPath("model/Mark23.glb"))) {
        std::cerr << "Khong nap duoc model sung. Kiem tra thu muc assets/model\n";
        return false;
    }
    world_.init();
    audio_.init();
    player_.init(&modelM4_, &modelMark23_, &audio_);

    setState(GameState::Title);
    return true;
}

void Game::run() {
    double last = glfwGetTime();
    while (running_ && !glfwWindowShouldClose(window_)) {
        double now = glfwGetTime();
        float dt = std::min(static_cast<float>(now - last), 0.05f);
        last = now;

        glfwPollEvents();
        glfwGetFramebufferSize(window_, &fbW_, &fbH_);
        input_.update();
        clock_ += dt;

        update(dt);
        render();
        glfwSwapBuffers(window_);
    }
}

void Game::shutdown() {
    if (state_ == GameState::Playing) endRun();
    settings_.save();
    audio_.shutdown();
    world_.destroy();
    modelM4_.destroy();
    modelMark23_.destroy();
    r2d_.destroy();
    shaders_.destroy();
    if (window_) glfwDestroyWindow(window_);
    glfwTerminate();
}

// ============================================================
// Trang thai
// ============================================================
void Game::setState(GameState s) {
    state_ = s;
    if (s == GameState::Playing) {
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        if (glfwRawMouseMotionSupported()) glfwSetInputMode(window_, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    } else {
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    input_.resetMouse();
    ui_.resetFocus();
}

void Game::startRun() {
    world_.reset();
    player_.reset();
    gameTime_ = 0.0f;
    newRecord_ = false;
    runRecorded_ = false;
    setState(GameState::Playing);
}

void Game::endRun() {
    if (runRecorded_) return;
    runRecorded_ = true;
    newRecord_ = scores_.add(player_.score(), player_.kills());
}

// ============================================================
// Cap nhat
// ============================================================
void Game::update(float dt) {
    switch (state_) {
    case GameState::Title:
        if (input_.pressed(GLFW_KEY_ESCAPE)) running_ = false;
        break;

    case GameState::Highscores:
    case GameState::Settings:
        if (input_.pressed(GLFW_KEY_ESCAPE)) {
            if (state_ == GameState::Settings) settings_.save();
            setState(GameState::Title);
        }
        break;

    case GameState::Playing: {
        if (input_.pressed(GLFW_KEY_ESCAPE)) {       // thoat ve menu
            endRun();
            setState(GameState::Title);
            break;
        }
        player_.look(input_.mouseDelta(), BASE_MOUSE_SENS * settings_.sensitivity);
        gameTime_ += dt;
        player_.update(input_, window_, dt, world_);
        int dmg = world_.update(dt, player_.position(), gameTime_);
        player_.takeDamage(dmg);
        if (player_.dead()) {
            endRun();
            setState(GameState::GameOver);
        }
        break;
    }

    case GameState::GameOver:
        player_.updateEffects(dt);
        world_.update(0.0f, player_.position(), gameTime_);   // chi don tia dan, khong di chuyen quai
        if (input_.pressed(GLFW_KEY_ENTER)) startRun();
        else if (input_.pressed(GLFW_KEY_ESCAPE)) setState(GameState::Title);
        break;
    }
}

// ============================================================
// Ve
// ============================================================
void Game::renderScene(const glm::mat4& view, const glm::vec3& eye) {
    glClearColor(World::SKY_COLOR.r, World::SKY_COLOR.g, World::SKY_COLOR.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glm::mat4 proj = glm::perspective(glm::radians(FOV_DEG), static_cast<float>(fbW_) / fbH_, 0.05f, 200.0f);
    world_.render(shaders_, view, proj, eye, clock_, settings_.shaderOn);

    if (state_ == GameState::Playing) {
        player_.drawViewmodel(shaders_, view, proj, settings_.shaderOn);

        if (player_.muzzleFlash() > 0.0f) {          // chop lua dau nong
            Shader& w = shaders_.world;
            w.use();
            w.setM("uView", view);
            w.setM("uProj", proj);
            w.setI("uUnlit", 1);
            glm::mat4 m = glm::inverse(view) *
                          glm::translate(glm::mat4(1.0f), player_.weapon().def().muzzleCam) *
                          glm::rotate(glm::mat4(1.0f), clock_ * 40.0f, glm::vec3(0, 0, 1)) *
                          glm::scale(glm::mat4(1.0f), glm::vec3(0.11f, 0.11f, 0.04f));
            w.setM("uModel", m);
            w.set4("uColor", {1.0f, 0.85f, 0.3f, 0.9f});
            world_.cube().draw();
            w.setI("uUnlit", 0);
        }
    }
}

void Game::render() {
    if (fbW_ <= 0 || fbH_ <= 0) return;
    glViewport(0, 0, fbW_, fbH_);

    if (state_ == GameState::Playing || state_ == GameState::GameOver) {
        renderScene(player_.viewMatrix(), player_.eyePos());
        r2d_.begin(fbW_, fbH_);
        drawHud();
        r2d_.end();
    } else {
        // Menu: camera quay quanh san dau de lam nen
        float a = clock_ * 0.12f;
        glm::vec3 eye(std::cos(a) * 15.0f, 5.5f, std::sin(a) * 15.0f);
        renderScene(glm::lookAt(eye, glm::vec3(0, 1.5f, 0), glm::vec3(0, 1, 0)), eye);
        r2d_.begin(fbW_, fbH_);
        ui_.begin(fbW_, fbH_);
        drawMenuBackdrop();
        if (state_ == GameState::Title) drawTitle();
        else if (state_ == GameState::Highscores) drawHighscores();
        else drawSettings();
        r2d_.end();
    }
}

// ============================================================
// Menu
// ============================================================
void Game::drawMenuBackdrop() {
    r2d_.rect(0, 0, r2d_.width(), r2d_.height(), {0.02f, 0.03f, 0.06f, 0.62f});
}

void Game::drawTitle() {
    const float s = ui_.scale();
    const float W = r2d_.width(), H = r2d_.height();
    float cx = W * 0.5f;

    r2d_.text("SHOOTER 3D", cx + 4 * s, H * 0.14f + 4 * s, 11.0f * s, {0, 0, 0, 0.6f}, TextAlign::Center);
    r2d_.text("SHOOTER 3D", cx, H * 0.14f, 11.0f * s, {1.0f, 0.62f, 0.2f, 1}, TextAlign::Center);

    char buf[48];
    std::snprintf(buf, sizeof(buf), "BEST SCORE: %d", scores_.best());
    r2d_.text(buf, cx, H * 0.14f + 100 * s, 3.0f * s, {0.85f, 0.9f, 1, 0.9f}, TextAlign::Center);

    float bw = 340 * s, bh = 56 * s, gap = 16 * s;
    float y = H * 0.40f;
    if (ui_.button("PLAY", cx, y, bw, bh)) startRun();
    y += bh + gap;
    if (ui_.button("HIGHSCORE", cx, y, bw, bh)) { setState(GameState::Highscores); return; }
    y += bh + gap;
    if (ui_.button("SETTINGS", cx, y, bw, bh)) { setState(GameState::Settings); return; }
    y += bh + gap;
    if (ui_.button("QUIT", cx, y, bw, bh)) running_ = false;

    r2d_.text("MOUSE OR W/S + ENTER", cx, H - 36 * s, 2.0f * s, {0.7f, 0.75f, 0.85f, 0.8f}, TextAlign::Center);
}

void Game::drawHighscores() {
    const float s = ui_.scale();
    const float W = r2d_.width(), H = r2d_.height();
    float cx = W * 0.5f;
    r2d_.text("HIGHSCORE", cx, H * 0.12f, 8.0f * s, {1.0f, 0.62f, 0.2f, 1}, TextAlign::Center);

    float pw = 520 * s, py = H * 0.28f, rowH = 52 * s;
    r2d_.rect(cx - pw * 0.5f, py - 16 * s, pw, rowH * HighScores::MAX_ENTRIES + 32 * s, {0.05f, 0.06f, 0.1f, 0.85f});
    const auto& list = scores_.entries();
    if (list.empty()) {
        r2d_.text("NO SCORES YET", cx, py + 2 * rowH, 3.5f * s, {0.8f, 0.85f, 0.95f, 1}, TextAlign::Center);
    }
    for (size_t i = 0; i < list.size(); ++i) {
        char l[16], m[32], r[32];
        std::snprintf(l, sizeof(l), "%d.", static_cast<int>(i) + 1);
        std::snprintf(m, sizeof(m), "%d", list[i].score);
        std::snprintf(r, sizeof(r), "%d KILLS", list[i].kills);
        float y = py + rowH * i + 8 * s;
        glm::vec4 col = i == 0 ? glm::vec4(1.0f, 0.85f, 0.3f, 1) : glm::vec4(0.9f, 0.93f, 1, 1);
        r2d_.text(l, cx - pw * 0.5f + 24 * s, y, 3.5f * s, col);
        r2d_.text(m, cx - 10 * s, y, 3.5f * s, col, TextAlign::Right);
        r2d_.text(r, cx + pw * 0.5f - 24 * s, y, 3.0f * s, {0.7f, 0.78f, 0.9f, 1}, TextAlign::Right);
    }

    if (ui_.button("BACK", cx, H * 0.80f, 260 * s, 56 * s)) setState(GameState::Title);
}

void Game::drawSettings() {
    const float s = ui_.scale();
    const float W = r2d_.width(), H = r2d_.height();
    float cx = W * 0.5f;
    r2d_.text("SETTINGS", cx, H * 0.12f, 8.0f * s, {1.0f, 0.62f, 0.2f, 1}, TextAlign::Center);

    float bw = 460 * s, bh = 66 * s, gap = 18 * s;
    float y = H * 0.32f;
    ui_.slider("SENSITIVITY", settings_.sensitivity, Settings::MIN_SENS, Settings::MAX_SENS, 0.05f, cx, y, bw, bh);
    y += bh + gap;
    std::string label = std::string("SHADER: ") + (settings_.shaderOn ? "ON" : "OFF");
    if (ui_.button(label, cx, y, bw, 56 * s)) settings_.shaderOn = !settings_.shaderOn;
    y += 56 * s + gap;
    if (ui_.button("BACK", cx, y, bw, 56 * s)) {
        settings_.save();
        setState(GameState::Title);
    }
}

// ============================================================
// HUD
// ============================================================
void Game::drawHud() {
    const float s = std::max(0.5f, fbH_ / 720.0f);
    const float W = static_cast<float>(fbW_), H = static_cast<float>(fbH_);
    const Weapon& w = player_.weapon();
    const WeaponDef& d = w.def();

    // Mau do khi bi danh / game over
    if (player_.hurt() > 0.0f) r2d_.rect(0, 0, W, H, {0.8f, 0.0f, 0.0f, player_.hurt() / 0.3f * 0.35f});
    if (state_ == GameState::GameOver) r2d_.rect(0, 0, W, H, {0.5f, 0.0f, 0.0f, 0.45f});

    // Tam ngam
    if (state_ == GameState::Playing) {
        float cx = W * 0.5f, cy = H * 0.5f;
        glm::vec4 cc = player_.hitMarker() > 0.0f ? glm::vec4(1.0f, 0.2f, 0.2f, 1.0f) : glm::vec4(1.0f, 1.0f, 1.0f, 0.85f);
        float arm = 5.0f * s, th = std::max(1.0f, s);
        float gap = 2.0f * s + player_.spread() * 900.0f * s;
        if (player_.hitMarker() > 0.0f) gap += 2.0f * s;
        r2d_.rect(cx - gap - arm, cy - th * 0.5f, arm, th, cc);
        r2d_.rect(cx + gap, cy - th * 0.5f, arm, th, cc);
        r2d_.rect(cx - th * 0.5f, cy - gap - arm, th, arm, cc);
        r2d_.rect(cx - th * 0.5f, cy + gap, th, arm, cc);
    }

    // Diem / ha guc (tren-trai), quai (tren-phai)
    char buf[64];
    std::snprintf(buf, sizeof(buf), "SCORE %d", player_.score());
    r2d_.text(buf, 20 * s, 18 * s, 3.0f * s, {1, 1, 1, 0.95f});
    std::snprintf(buf, sizeof(buf), "KILLS %d", player_.kills());
    r2d_.text(buf, 20 * s, 18 * s + 30 * s, 3.0f * s, {1, 0.8f, 0.5f, 0.95f});
    std::snprintf(buf, sizeof(buf), "ENEMIES %d", static_cast<int>(world_.enemies().size()));
    r2d_.text(buf, W - 20 * s, 18 * s, 3.0f * s, {1, 0.6f, 0.6f, 0.95f}, TextAlign::Right);

    // Thanh mau (trai duoi)
    float hpFrac = static_cast<float>(player_.hp()) / PLAYER_MAX_HP;
    r2d_.rect(18 * s, H - 42 * s, 224 * s, 22 * s, {0, 0, 0, 0.6f});
    r2d_.rect(20 * s, H - 40 * s, 220.0f * s * hpFrac, 18 * s,
              glm::vec4(glm::mix(glm::vec3(0.9f, 0.15f, 0.15f), glm::vec3(0.2f, 0.85f, 0.3f), hpFrac), 1.0f));
    std::snprintf(buf, sizeof(buf), "HP %d", player_.hp());
    r2d_.text(buf, 20 * s, H - 70 * s, 3.0f * s, {1, 1, 1, 1});

    // Dan (phai duoi)
    float ax = W - 20 * s;
    r2d_.text(d.name, ax, H - 128 * s, 3.0f * s, {0.85f, 0.9f, 1, 1}, TextAlign::Right);
    if (d.infiniteReserve) std::snprintf(buf, sizeof(buf), "%d / INF", w.mag());
    else std::snprintf(buf, sizeof(buf), "%d / %d", w.mag(), w.reserve());
    r2d_.text(buf, ax, H - 100 * s, 5.0f * s,
              (w.mag() == 0 && !w.hasReserve()) ? glm::vec4(1, 0.3f, 0.3f, 1) : glm::vec4(1, 1, 1, 1), TextAlign::Right);

    float frac = w.state() == Weapon::State::Reloading ? w.reloadProgress()
                                                       : static_cast<float>(w.mag()) / d.magSize;
    r2d_.rect(W - 242 * s, H - 42 * s, 224 * s, 22 * s, {0, 0, 0, 0.6f});
    r2d_.rect(W - 240 * s, H - 40 * s, 220.0f * s * frac, 18 * s,
              w.state() == Weapon::State::Reloading ? glm::vec4(1.0f, 0.8f, 0.2f, 1.0f) : glm::vec4(0.9f, 0.9f, 0.95f, 1.0f));
    if (w.state() == Weapon::State::Reloading)
        r2d_.text("RELOADING", W - 130 * s, H - 62 * s, 2.5f * s, {1, 0.85f, 0.3f, 1}, TextAlign::Center);
    else if (w.mag() == 0 && !w.hasReserve() && state_ == GameState::Playing)
        r2d_.text("NO AMMO - PRESS 2 / KILL TO GET 3", W * 0.5f, H * 0.62f, 3.0f * s, {1, 0.4f, 0.3f, 0.95f}, TextAlign::Center);

    // O sung (giua duoi)
    float sw = 150 * s, sh = 40 * s, sgap = 10 * s;
    float sx = W * 0.5f - (sw * WEAPON_COUNT + sgap * (WEAPON_COUNT - 1)) * 0.5f;
    for (int i = 0; i < WEAPON_COUNT; ++i) {
        bool cur = i == player_.currentSlot();
        bool pend = i == player_.pendingSlot();
        float x = sx + i * (sw + sgap), y = H - 56 * s;
        r2d_.rect(x, y, sw, sh, cur ? glm::vec4(0.95f, 0.5f, 0.12f, 0.9f) : glm::vec4(0, 0, 0, 0.55f));
        if (pend) r2d_.rectOutline(x, y, sw, sh, 2 * s, {1, 1, 1, 0.9f});
        std::snprintf(buf, sizeof(buf), "%d %s", i + 1, player_.weaponAt(i).def().name);
        r2d_.text(buf, x + sw * 0.5f, y + (sh - 7 * 2.5f * s) * 0.5f, 2.5f * s,
                  cur ? glm::vec4(0.05f, 0.05f, 0.05f, 1) : glm::vec4(0.9f, 0.9f, 0.95f, 1), TextAlign::Center);
    }

    // Game over
    if (state_ == GameState::GameOver) {
        float cx = W * 0.5f;
        r2d_.text("GAME OVER", cx, H * 0.26f, 10.0f * s, {1, 0.9f, 0.9f, 1}, TextAlign::Center);
        std::snprintf(buf, sizeof(buf), "SCORE %d   KILLS %d", player_.score(), player_.kills());
        r2d_.text(buf, cx, H * 0.26f + 100 * s, 4.0f * s, {1, 1, 1, 1}, TextAlign::Center);
        if (newRecord_) r2d_.text("NEW HIGHSCORE!", cx, H * 0.26f + 150 * s, 4.0f * s, {1, 0.85f, 0.2f, 1}, TextAlign::Center);
        r2d_.text("ENTER - PLAY AGAIN    ESC - MENU", cx, H * 0.26f + 210 * s, 3.0f * s, {1, 1, 1, 0.9f}, TextAlign::Center);
    }
}
