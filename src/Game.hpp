#pragma once
#include "Audio.hpp"
#include "Input.hpp"
#include "Model.hpp"
#include "Player.hpp"
#include "Renderer2D.hpp"
#include "Settings.hpp"
#include "Shader.hpp"
#include "Ui.hpp"
#include "World.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

enum class GameState { Title, Highscores, Settings, Playing, GameOver };

class Game {
public:
    bool init();
    void run();
    void shutdown();

    void onResize(int w, int h) { fbW_ = w; fbH_ = h; }

private:
    void setState(GameState s);
    void startRun();
    void endRun();                    // ghi diem cao
    void update(float dt);
    void render();

    void renderScene(const glm::mat4& view, const glm::vec3& eye);
    void drawHud();
    void drawTitle();
    void drawHighscores();
    void drawSettings();
    void drawMenuBackdrop();

    GLFWwindow* window_ = nullptr;
    int fbW_ = 1280, fbH_ = 720;
    GameState state_ = GameState::Title;
    bool running_ = true;

    Input input_;
    Settings settings_;
    HighScores scores_;
    Audio audio_;
    Shaders shaders_;
    Renderer2D r2d_;
    Ui ui_;
    Model modelM4_, modelMark23_;
    World world_;
    Player player_;

    float gameTime_ = 0.0f;       // thoi gian van choi
    float clock_ = 0.0f;          // dong ho tong (animation shader / camera menu)
    bool newRecord_ = false;
    bool runRecorded_ = true;
};
