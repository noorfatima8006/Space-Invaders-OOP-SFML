#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
using namespace std;


// Window & grid 
const int   WIN_W = 1200;
const int   WIN_H = 720;
const int   WIDTH = 60;
const int   HEIGHT = 24;
const float CW = (float)WIN_W / WIDTH;
const float CH = (float)WIN_H / HEIGHT;

const float BOSS_HP_X = 320.f;
const float BOSS_HP_Y = 12.f;

inline float px(float gx) { return gx * CW; }
inline float py(float gy) { return gy * CH; }
inline float cx(float gx) { return gx * CW + CW * 0.5f; }
inline float cy(float gy) { return gy * CH + CH * 0.5f; }


const int MAX_BULLETS = 100;
const int MAX_ENEMY_BULLETS = 100;
const int MAX_DRONES = 48;
const int MAX_VIPERS = 24;
const int MAX_SEEKERS = 6;
const int MAX_POWERUPS = 20;
const int MAX_ASTEROIDS = 15;
const int MAX_STARS = 120;
int* asteroidRespawnTimer = nullptr;

const float DASH_DISTANCE = 6.f;
const float DASH_COOLDOWN_TIME = 180.f;
const float DASH_INVINCIBLE_TIME = 30.f;

int frameCount = 0;
sf::Font gFont;
bool gFontLoaded = false;

int highScore = 0;

static float gShakeTimer = 0;
static float playerHitFlash = 0.f;
static float shieldBreakFlash = 0.f;

struct PrettyStar {
    float x, y;
    float size;
    float speed;
    float phase; 
};
const int MAX_PRETTY_STARS = 35;
PrettyStar* prettyStars = nullptr;

//  Draw helpers 
//draws a polygon arouund the pngs 
static void drawPoly(sf::RenderWindow& w, float gx, float gy,
    const sf::Vector2f* pts, int n,
    sf::Color fill, sf::Color outline = sf::Color::Transparent, float outThick = 0.f)
{
    sf::ConvexShape s(n);
    float ox = cx(gx), oy = cy(gy);
    for (int i = 0; i < n; i++) s.setPoint(i, { ox + pts[i].x, oy + pts[i].y });
    s.setFillColor(fill);
    s.setOutlineColor(outline);
    s.setOutlineThickness(outThick);
    w.draw(s);
}

static void drawGlow(sf::RenderWindow& w, float px_, float py_, float r, sf::Color col, int layers = 3)
{
    for (int i = layers; i >= 1; i--) {
        sf::CircleShape c(r + i * 2.5f);
        c.setOrigin(r + i * 2.5f, r + i * 2.5f);
        c.setPosition(px_, py_);
        c.setFillColor(sf::Color(col.r, col.g, col.b, (sf::Uint8)(40 / i)));
        w.draw(c);
    }
    sf::CircleShape c(r);
    c.setOrigin(r, r);
    c.setPosition(px_, py_);
    c.setFillColor(col);
    w.draw(c);
}

//used to group related variables together into one custom data type
struct Particle { float x, y, vx, vy, life, maxLife; sf::Color col; bool alive = false; };
const int MAX_PARTICLES = 700;
static Particle* gParts = nullptr;


static void updateParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!gParts[i].alive) continue;
        gParts[i].x += gParts[i].vx; gParts[i].y += gParts[i].vy;
        gParts[i].vx *= 0.94f;       gParts[i].vy *= 0.94f;
        gParts[i].life--;
        if (gParts[i].life <= 0) gParts[i].alive = false;
    }
}
static void drawParticles(sf::RenderWindow& w) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!gParts[i].alive) continue;
        float t = gParts[i].life / gParts[i].maxLife;
        sf::CircleShape c(1.8f * t + 0.5f);
        c.setOrigin(c.getRadius(), c.getRadius());
        c.setPosition(gParts[i].x, gParts[i].y);
        sf::Color col = gParts[i].col;
        col.a = (sf::Uint8)(t * 220);
        c.setFillColor(col);
        w.draw(c);
    }
}

// Screen shake
static void triggerShake(float d = 8.f) { if (gShakeTimer < d) gShakeTimer = d; }
static sf::Vector2f getShakeOffset() {
    if (gShakeTimer <= 0) return { 0,0 };
    float m = gShakeTimer * 0.6f;
    return {
        (float)(rand() % 2 ? 1 : -1) * (rand() % 100) / 100.f * m,
        (float)(rand() % 2 ? 1 : -1) * (rand() % 100) / 100.f * m
    };
}

// used to create named choices,identifies the type
enum ObjectType { PLAYER, ENEMY, BULLET, POWERUP, ASTEROID, ENEMY_BULLET, BOSS, CRUISER, TURRET, TWINCANNONS, MOTHERSHIP };
enum PowerUpType { PU_SPREAD, PU_PIERCING, PU_SHIELD, PU_EMP };
enum WeaponType { NORMAL, W_SPREAD, W_PIERCING };
enum GameState { MENU, MODE_SELECT, ARCADE, SURVIVAL, CONTROLS, PAUSED, GAMEOVER, WIN, LEVEL_TRANSITION };

class SoundManager {
private:
    sf::SoundBuffer shootBuf, explosionBuf, powerupBuf, hitBuf, menuBuf, bossWarningBuf;
    sf::Sound shootSound, explosionSound, powerupSound, hitSound, menuSound, bossWarningSound;

public:
    bool load() {
        bool allSoundsLoaded = true;
        if (!shootBuf.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/shoot.mp3")) {
            cout << "shoot.mp3 failed\n";
            allSoundsLoaded = false;
        }

        if (!explosionBuf.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/explosion.wav")) {
            cout << "explosion.wav failed\n";
            allSoundsLoaded = false;
        }

        if (!powerupBuf.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/powerUp.mp3")) {
            cout << "powerUp.mp3 failed\n";
            allSoundsLoaded = false;
        }

        if (!hitBuf.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/playerdamage.wav")) {
            cout << "playerdamage.wav failed\n";
            allSoundsLoaded = false;
        }

        if (!menuBuf.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/menu.mp3")) {
            cout << "menu.mp3 failed\n";
            allSoundsLoaded = false;
        }

        if (!bossWarningBuf.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/warning.mp3")) {
            cout << "warning.mp3 failed\n";
            allSoundsLoaded = false;
        }

        shootSound.setBuffer(shootBuf);
        explosionSound.setBuffer(explosionBuf);
        powerupSound.setBuffer(powerupBuf);
        hitSound.setBuffer(hitBuf);
        menuSound.setBuffer(menuBuf);
        bossWarningSound.setBuffer(bossWarningBuf);
        setVolumes();
        return allSoundsLoaded;
    }
    void setVolumes() {
        shootSound.setVolume(10.f);
        explosionSound.setVolume(10.f);
        powerupSound.setVolume(40.f);
        hitSound.setVolume(45.f);
        menuSound.setVolume(15.f);
        bossWarningSound.setVolume(50.f);
        cout << "Volumes set successfully\n";

    }

    void playShoot() {
        shootSound.play();
    }

    void playExplosion() {
        explosionSound.play();
    }

    void playPowerup() {
        powerupSound.play();
    }

    void playHit() {
        hitSound.play();
    }

    void playMenu() {
        menuSound.play();
    }

    void playBossWarning() {
        bossWarningSound.play();
    }
};
SoundManager* gSounds = nullptr;

static void spawnExplosionSilent(float gx, float gy, sf::Color col, int count = 18) {
    int sp = 0;
    for (int i = 0; i < MAX_PARTICLES && sp < count; i++) {
        if (!gParts[i].alive) {
            gParts[i].alive = true;
            gParts[i].x = cx(gx); gParts[i].y = cy(gy);
            float ang = (rand() % 360) * 3.14159f / 180.f;
            float spd = 1.5f + (rand() % 30) / 10.f;
            gParts[i].vx = cosf(ang) * spd; gParts[i].vy = sinf(ang) * spd;
            gParts[i].life = 25.f + rand() % 20;
            gParts[i].maxLife = gParts[i].life;
            gParts[i].col = col;
            sp++;
        }
    }
}

// CLASSES
class GameObject {
public:
    virtual ~GameObject() {}
    virtual void update() = 0;
    virtual void onCollision(GameObject* o) = 0;
    virtual void draw(sf::RenderWindow& w) = 0;
    virtual ObjectType getType() = 0;
};
//In C++, a class becomes abstract if it has any pure virtual
// function that is not implemented, 
// even if that pure virtual function came from the parent class.
class Entity : public GameObject {
protected:
    float x, y, vx, vy, width, height; bool alive;
public:
    Entity() : x(0), y(0), vx(0), vy(0), width(1), height(1), alive(true) {}
    Entity(float x, float y, float vx, float vy, float w, float h)
        : x(x), y(y), vx(vx), vy(vy), width(w), height(h), alive(true) {
    }
    float getx()     const { return x; }
    float gety()     const { return y; }
    float getwidth() const { return width; }
    float getheight()const { return height; }
    bool  Isalive()  const { return alive; }
    void  setPosition(float px_, float py_) { x = px_; y = py_; }
    void  setAlive(bool v) { alive = v; }
};

class EnemyBullet;

class Enemy : public Entity {
protected:
    int health, score;
    float fire_timer, fire_cooldown;
public:
    Enemy(float x, float y, float vx, float vy, float w, float h,
        int hp = 1, int sc = 100, float ft = 0, float cd = 10)
        : Entity(x, y, vx, vy, w, h),
        health(hp), score(sc), fire_timer(ft), fire_cooldown(cd) {
    }
    virtual ~Enemy() {}
    virtual void shoot(EnemyBullet b[], int n) = 0;
    void update() override { x += vx; y += vy; fire_timer++; }
    bool canshoot() { return fire_timer >= fire_cooldown; }
    int  getHealth() { return health; }
    int  getScore() { return score; }
    void resetFireTimer() { fire_timer = 0; }
    void setFireCooldown(float cd) { fire_cooldown = cd; }
    virtual void takeDamage(int dmg) { health -= dmg; if (health <= 0) alive = false; }
    void onCollision(GameObject*) override {}
    ObjectType getType() override { return ENEMY; }
    void draw(sf::RenderWindow&)  override {}
    float getPixelX() const { return cx(x); }
    float getPixelY() const { return cy(y); }
    float getRadius()  const { return CW * 0.8f; }
};

//PLAYER BULLET 
class Bullet : public Entity {
    bool piercing;
public:
    Bullet() : Entity() { alive = false; piercing = false; }
    void setPiercing(bool p) { piercing = p; }
    bool isPiercing() const { return piercing; }
    void shoot(float px_, float py_)
    { 
        x = px_; 
        y = py_ - 1;
        alive = true;
    }
    void shootSpread(float px_, float py_, float off) 
    { 
        x = px_ + off; 
        y = py_ - 1; 
        alive = true; 
    }
    void update() override {
        if (alive) {
            y -= 0.65f;
            if (y < 0) alive = false;
        }
    }
    void onCollision(GameObject* o) override {
        if (o->getType() == ENEMY || o->getType() == BOSS) {
            Enemy* e = (Enemy*)o;
            e->takeDamage(piercing ? 2 : 1);
            if (!piercing) alive = false;
        }
        else if (o->getType() == ASTEROID) {
            alive = false;
        }
    }
    ObjectType getType() override { return BULLET; }
    void draw(sf::RenderWindow& w) override {
        if (!alive) return;

        float bx = cx(x), by = cy(y);

        // Outer glow (color depends on weapon)
        sf::Color glowCol = piercing ? sf::Color(180, 0, 255, 120)
            : sf::Color(0, 200, 255, 120);

        drawGlow(w, bx, by, CW * 0.25f, glowCol, 2);

        // Main laser body
        sf::RectangleShape bolt(sf::Vector2f(5.f, CH * 0.9f));
        bolt.setOrigin(2.5f, CH * 0.45f);
        bolt.setPosition(bx, by);
        bolt.setFillColor(piercing ? sf::Color(180, 0, 255)
            : sf::Color(0, 230, 255));
        w.draw(bolt);

        // Bright tip (bigger + aligned)
        sf::CircleShape tip(3.5f);
        tip.setOrigin(3.5f, 3.5f);
        tip.setPosition(bx, by - CH * 0.45f);
        tip.setFillColor(sf::Color::White);
        w.draw(tip);
    }
};


//  ENEMY BULLET
class EnemyBullet : public Entity {
public:
    EnemyBullet() { alive = false; }
    void shoot(float px_, float py_) { x = px_; y = py_ + 1; alive = true; }
    void update() override { if (alive) { y += 0.20f; if (y >= HEIGHT) alive = false; } }
    void onCollision(GameObject* o) override 
    { if (o->getType() == PLAYER)
        alive = false;
    }
    ObjectType getType() override { return ENEMY_BULLET; }
    void draw(sf::RenderWindow& w) {
        if (!alive) return;

        float bx = cx(x), by = cy(y);

        // outer glow (helps visibility)
        drawGlow(w, bx, by, CW * 0.35f, sf::Color(255, 100, 40, 120), 2);

        // main bullet 
        sf::CircleShape bullet(CW * 0.28f);  
        bullet.setOrigin(CW * 0.28f, CW * 0.28f);
        bullet.setPosition(bx, by);
        bullet.setFillColor(sf::Color(255, 120, 60));
        bullet.setOutlineColor(sf::Color(255, 220, 150));
        bullet.setOutlineThickness(1.5f);
        w.draw(bullet);

        // bright core
        sf::CircleShape core(CW * 0.12f);
        core.setOrigin(CW * 0.12f, CW * 0.12f);
        core.setPosition(bx, by);
        core.setFillColor(sf::Color(255, 240, 200));
        w.draw(core);
    }
};

//  POWERUP
class PowerUp : public Entity {
    PowerUpType type; float angle;
public:
    PowerUp() : Entity(0, 0, 0, 0, 1, 1), type(PU_SHIELD), angle(0) { alive = false; }
    PowerUp(float x, float y, float vx, float vy, float w, float h, PowerUpType t)
        : Entity(x, y, vx, vy, w, h), type(t), angle(0) {
    }
    void update() override {
        y += vy;
        angle += 4.f; 
        if (y > HEIGHT) 
            alive = false;
    }
    void onCollision(GameObject* o) override {
        if (o->getType() == PLAYER) 
            alive = false;
    }
    ObjectType getType() override { return POWERUP; }
    PowerUpType getPowerType() { return type; }
    void draw(sf::RenderWindow& w) override {
        if (!alive) return;

        sf::Color col;
        char lbl = ' ';

        switch (type) {
        case PU_SPREAD:   col = sf::Color(255, 210, 0);   lbl = 'S'; break;
        case PU_PIERCING: col = sf::Color(0, 180, 255);   lbl = 'P'; break;
        case PU_SHIELD:   col = sf::Color(0, 255, 110);   lbl = 'E'; break;
        case PU_EMP:      col = sf::Color(200, 0, 255);   lbl = 'N'; break;
        }

        float bx = cx(x), by = cy(y);

        float pulse = 0.9f + sinf(frameCount * 0.12f) * 0.15f;

        // bigger glow
        drawGlow(w, bx, by, CW * 0.55f, sf::Color(col.r, col.g, col.b, 80), 3);

        // bigger rotating gem
        sf::RectangleShape gem(sf::Vector2f(CW * 0.9f, CW * 0.9f));
        gem.setOrigin(CW * 0.45f, CW * 0.45f);
        gem.setRotation(angle);
        gem.setScale(pulse, pulse);
        gem.setPosition(bx, by);
        gem.setFillColor(col);
        gem.setOutlineThickness(2.f);
        gem.setOutlineColor(sf::Color::White);
        w.draw(gem);

        // small bright centre
        sf::CircleShape core(CW * 0.18f);
        core.setOrigin(CW * 0.18f, CW * 0.18f);
        core.setPosition(bx, by);
        core.setFillColor(sf::Color(255, 255, 255, 130));
        w.draw(core);

        if (gFontLoaded) {
            sf::Text t;
            t.setFont(gFont);
            t.setString(string(1, lbl));
            t.setCharacterSize(16);
            t.setFillColor(sf::Color::White);

            auto b2 = t.getLocalBounds();
            t.setOrigin(b2.left + b2.width / 2, b2.top + b2.height / 2);
            t.setPosition(bx, by);
            w.draw(t);
        }
    }
};

//  PLAYER
class Player : public Entity {
    int lives, shieldHits, EMPcount;
    WeaponType current_weapon;
    float dash_cooldown, invincibilityTimer, fire_timer, fire_cooldown, engineFlicker;
    sf::Sprite sprite;

public:
    Player() : Entity((float)(WIDTH / 2), (float)(HEIGHT - 2), 0, 0, 1, 1),
        lives(15), shieldHits(0), EMPcount(21), current_weapon(NORMAL),
        dash_cooldown(0), invincibilityTimer(0), fire_timer(0),
        fire_cooldown(6),   
        engineFlicker(0) {
    }
    void moveLeft() {
        x -= 0.30f;
        if (x < 1.5f) 
            x = 1.5f;
    }
    void moveRight() {
        x += 0.30f;

        if (x > WIDTH - 2.7f)   
            x = WIDTH - 2.7f;
    }

    void moveUp() {
        y -= 0.30f;
        if (y < 1.f)
            y = 1.f;
    }

    void moveDown() {
        y += 0.30f;
        if (y > HEIGHT - 2.0f)  
            y = HEIGHT - 2.0f;
    }
    void dash(int dir) {
        if (dash_cooldown <= 0) {
            //store player's current position before dashing
            float oldX = x;

            x += dir * DASH_DISTANCE;

            float leftLimit = 1.5f;
            float rightLimit = WIDTH - 2.5f;

            x = max(leftLimit, min(x, rightLimit));

            // Only use dash if player actually moved
            if (x != oldX) {
                dash_cooldown = 40;
                invincibilityTimer = DASH_INVINCIBLE_TIME;
            }
        }
    }
    void resetPosition(float mothershipX = -1, float mothershipDirX = 0) {
        // default center
        x = (float)(WIDTH / 2);

        // if mothership laser active, spawn on safe side
        if (mothershipX > 0) {
            if (mothershipDirX < 0) // laser on right side
                x = mothershipX * 0.4f; // spawn left
            else // laser on left side
                x = mothershipX + (WIDTH - mothershipX) * 0.6f; // spawn right
        }

        y = (float)(HEIGHT - 2);
        vx = 0;
        vy = 0;
    }

    float getPixelX() const { return cx(x); }
    float getPixelY() const { return cy(y); }
    float getRadius() const { return CW * 0.75f; } // tweak to match sprite sizee
    void update() override {
        if (invincibilityTimer > 0) invincibilityTimer--;
        if (dash_cooldown > 0) dash_cooldown--;
        if (fire_timer > 0) fire_timer--;
        engineFlicker += 0.3f;
    }
    bool canShoot() { return fire_timer <= 0; }
    void resetFireTimer() { fire_timer = fire_cooldown; }

    void onCollision(GameObject* o) override {
        if (o->getType() == POWERUP) {
            //converting gameobject pointer into powerUp ptr
            PowerUp* p = (PowerUp*)o;
            if (p->getPowerType() == PU_SHIELD)   shieldHits = 2;
            else if (p->getPowerType() == PU_EMP) {
                if (EMPcount < 3)
                    EMPcount++;
            }
            else if (p->getPowerType() == PU_SPREAD) {
                current_weapon = W_SPREAD;
                fire_timer = fire_cooldown; // force full cooldown reset
            }
            else if (p->getPowerType() == PU_PIERCING) {
                current_weapon = W_PIERCING;
                fire_timer = fire_cooldown;
            }

        }
        else if (o->getType() == ASTEROID || o->getType() == ENEMY_BULLET) {
            takeDamage();
        }
    }
    void takeDamage() {
        if (invincibilityTimer > 0) return;

        playerHitFlash = 8.f;

        if (shieldHits > 0) {
            shieldHits--;
            invincibilityTimer = 60;
            triggerShake(5.f);
            spawnExplosionSilent(x, y, sf::Color(0, 220, 255), 20);
            if (gSounds) gSounds->playHit();

            if (shieldHits == 0) {
                shieldBreakFlash = 18.f;
                triggerShake(12.f);
                spawnExplosionSilent(x, y, sf::Color(120, 240, 255), 55);
            }
        }
        else {
            if (lives > 0) {
                lives--;

                current_weapon = NORMAL;
            }

            invincibilityTimer = 90;
            triggerShake(9.f);
            spawnExplosionSilent(x, y, sf::Color(255, 80, 40), 25);
            if (gSounds) gSounds->playHit();
        }
    }
    int  getLives() { return lives; }
    int  getShieldHits() { return shieldHits; }
    WeaponType getWeapon() { return current_weapon; }
    void activateEMP() { EMPcount--; }
    bool hasEMP() { return EMPcount > 0; }
    int  getEMPCount() { return EMPcount; }
    float getDashCooldown() { return dash_cooldown; }
    int  getIntX() { return (int)x; }
    int  getY() { return (int)y; }
    ObjectType getType() override { return PLAYER; }

    void setTexture(sf::Texture& tex) {
        sprite.setTexture(tex);
        sprite.setOrigin(tex.getSize().x / 2.f, tex.getSize().y / 2.f);
        sprite.setScale(0.08f, 0.08f); // adjust size
    }

    void draw(sf::RenderWindow& w) override {
        if (invincibilityTimer > 0 && (int)invincibilityTimer % 4 < 2)
            return;

        float bx = cx(x);
        float by = cy(y);

        // engine glow under player
        float ef = 0.55f + sinf(engineFlicker) * 0.35f;
        drawGlow(w, bx, by + CH * 0.55f, CW * 0.25f * ef,
            sf::Color(0, 120, 255, 140), 3);

        // player PNG
        sprite.setPosition(bx, by);
        w.draw(sprite);

        // weapon visual indicator on player ship
        if (current_weapon == W_SPREAD) {
            // wider spread barrels - left and right
            sf::RectangleShape leftBarrel(sf::Vector2f(5.f, CH * 0.65f));
            leftBarrel.setOrigin(2.5f, CH * 0.65f);
            leftBarrel.setPosition(bx - CW * 0.35f, by - CH * 0.35f);
            leftBarrel.setRotation(-18.f);
            leftBarrel.setFillColor(sf::Color(255, 220, 0));
            w.draw(leftBarrel);

            sf::RectangleShape rightBarrel(sf::Vector2f(5.f, CH * 0.65f));
            rightBarrel.setOrigin(2.5f, CH * 0.65f);
            rightBarrel.setPosition(bx + CW * 0.35f, by - CH * 0.35f);
            rightBarrel.setRotation(18.f);
            rightBarrel.setFillColor(sf::Color(255, 220, 0));
            w.draw(rightBarrel);

            drawGlow(w, bx, by - CH * 0.45f, CW * 0.25f,
                sf::Color(255, 210, 0, 120), 2);
        }
        else if (current_weapon == W_PIERCING) {
            sf::RectangleShape cannon(sf::Vector2f(7.f, CH * 0.85f));
            cannon.setOrigin(3.5f, CH * 0.85f);
            cannon.setPosition(bx, by - CH * 0.35f);
            cannon.setFillColor(sf::Color(180, 0, 255));
            w.draw(cannon);

            drawGlow(w, bx, by - CH * 0.55f, CW * 0.3f,
                sf::Color(180, 0, 255, 130), 2);
        }
        else {
            sf::RectangleShape barrel(sf::Vector2f(4.f, CH * 0.5f));
            barrel.setOrigin(2.f, CH * 0.5f);
            barrel.setPosition(bx, by - CH * 0.30f);
            barrel.setFillColor(sf::Color(0, 220, 255));
            w.draw(barrel);
        }

        // shield visible + cracked effect
        if (shieldHits > 0) {
            float shieldRadius = CW * 1.1f;

            sf::CircleShape sh(shieldRadius);
            sh.setOrigin(shieldRadius, shieldRadius);
            sh.setPosition(bx, by);

            if (shieldHits == 2) {
                // full shield
                sh.setFillColor(sf::Color(0, 200, 255, 35));
                sh.setOutlineColor(sf::Color(0, 220, 255, 210));
            }
            else {
                // damaged shield
                sh.setFillColor(sf::Color(0, 150, 255, 45));
                sh.setOutlineColor(sf::Color(255, 80, 80, 255));
            }

            sh.setOutlineThickness(3.f);
            w.draw(sh);

            // outer glow ring
            sf::CircleShape outerGlow(shieldRadius + 5.f);
            outerGlow.setOrigin(shieldRadius + 5.f, shieldRadius + 5.f);
            outerGlow.setPosition(bx, by);
            outerGlow.setFillColor(sf::Color::Transparent);
            outerGlow.setOutlineColor(sf::Color(0, 220, 255, 140));
            outerGlow.setOutlineThickness(2.f);
            w.draw(outerGlow);

            // helper function to draw thick cracks
            auto drawCrack = [&](sf::Vector2f a, sf::Vector2f b) {
                sf::Vector2f diff = b - a;
                float length = sqrt(diff.x * diff.x + diff.y * diff.y);
                float angle = atan2(diff.y, diff.x) * 180.f / 3.14159f;

                // black shadow behind the crack
                sf::RectangleShape shadow;
                shadow.setSize(sf::Vector2f(length, 7.f));
                shadow.setOrigin(0.f, 3.5f);
                shadow.setPosition(a);
                shadow.setRotation(angle);
                shadow.setFillColor(sf::Color(0, 0, 0, 230));
                w.draw(shadow);

                // bright crack line
                sf::RectangleShape line;
                line.setSize(sf::Vector2f(length, 4.f));
                line.setOrigin(0.f, 2.f);
                line.setPosition(a);
                line.setRotation(angle);
                line.setFillColor(sf::Color(255, 255, 255, 255));
                w.draw(line);
                };

            // cracks show only after shield has taken 1 hit
            if (shieldHits == 1) {
                drawCrack(
                    sf::Vector2f(bx - CW * 0.25f, by - CH * 0.95f),
                    sf::Vector2f(bx - CW * 0.05f, by - CH * 0.55f)
                );

                drawCrack(
                    sf::Vector2f(bx - CW * 0.05f, by - CH * 0.55f),
                    sf::Vector2f(bx - CW * 0.28f, by - CH * 0.20f)
                );

                drawCrack(
                    sf::Vector2f(bx - CW * 0.28f, by - CH * 0.20f),
                    sf::Vector2f(bx + CW * 0.10f, by + CH * 0.15f)
                );

                drawCrack(
                    sf::Vector2f(bx + CW * 0.55f, by - CH * 0.55f),
                    sf::Vector2f(bx + CW * 0.25f, by - CH * 0.18f)
                );

                drawCrack(
                    sf::Vector2f(bx + CW * 0.25f, by - CH * 0.18f),
                    sf::Vector2f(bx + CW * 0.48f, by + CH * 0.25f)
                );

                drawCrack(
                    sf::Vector2f(bx - CW * 0.65f, by + CH * 0.20f),
                    sf::Vector2f(bx - CW * 0.32f, by + CH * 0.40f)
                );

                drawCrack(
                    sf::Vector2f(bx - CW * 0.32f, by + CH * 0.40f),
                    sf::Vector2f(bx - CW * 0.50f, by + CH * 0.70f)
                );
            }
        }
    }
};


//  DRONE 
class Drone : public Enemy {
    sf::Sprite sprite;
public:
    Drone() : Enemy(0, 0, 0, 0.015f, 1, 1, 1, 100, 0, 35) { alive = false; }
    Drone(float startX, float startY, sf::Texture& tex)
        : Enemy(startX, startY, 0, 0.015f, 1, 1, 1, 100, 0, 35)
    {
        alive = true;

        sprite.setTexture(tex);
        sprite.setOrigin(tex.getSize().x / 2.f, tex.getSize().y / 2.f);
        sprite.setScale(0.12f, 0.12f);
    }

    void spawn(float px_, float py_)
    { 
        x = px_; 
        y = py_;
        alive = true;
        health = 1; 
        fire_timer = 0; 
    }
    // for survival, to increase speed every wave
    void spawn(float px_, float py_, float spd) {
        x = px_; y = py_; alive = true; health = 1; fire_timer = 0; vy = spd;
    }
    void shoot(EnemyBullet b[], int n) override {
        for (int i = 0; i < n; i++) {
            // finds an inactive bullet and shoot it
            if (!b[i].Isalive())
            {
                b[i].shoot(x, y);
                break;
            }
        }
    }
    void update() override { Enemy::update(); if (y >= HEIGHT) alive = false; }
    ObjectType getType() override { return ENEMY; }
    void setTexture(sf::Texture& tex) {
        sprite.setTexture(tex);
        sprite.setOrigin(tex.getSize().x / 2.f, tex.getSize().y / 2.f);
        sprite.setScale(0.06f, 0.06f);
    }
    void draw(sf::RenderWindow& window) override {
        if (!alive) return;

        sprite.setPosition(cx(x), cy(y));
        window.draw(sprite);
    }

};

//  VIPER 
class Viper : public Enemy {
    float baseX, angle, rowOffset, syncY;
    sf::Sprite sprite;
public:
    static float globalAngle;
    Viper() : Enemy(0, 0, 0, 0.015f, 1, 1, 1, 150, 0, 45),
        baseX(0), angle(0), rowOffset(0), syncY(0) {
        alive = true;
    }
    Viper(float startX, float startY, sf::Texture& tex)
        : Enemy(startX, startY, 0, 0.015f, 1, 1, 1, 150, 0, 45),
        baseX(startX), angle(0), rowOffset(0), syncY(startY)
    {
        alive = true;

        sprite.setTexture(tex);
        sprite.setOrigin(tex.getSize().x / 2.f, tex.getSize().y / 2.f);
        sprite.setScale(0.12f, 0.12f);
    }
    void setPosition(float px_, float py_) {
        x = px_;
        baseX = px_;
        y = py_; 
    }
    void setRowOffset(float o) { rowOffset = o; }
    void setSyncY(float sy) { syncY = sy; }
    void shoot(EnemyBullet b[], int n) override {
        for (int i = 0; i < n; i++) if (!b[i].Isalive()) { b[i].shoot(x, y); break; }
    }
    void update() override {
        y = syncY + rowOffset; fire_timer++;
        angle = globalAngle;
        x = baseX + sinf(angle) * 8.f;
        if (x < 0) x = 0; if (x > WIDTH - 1) x = WIDTH - 1;
        if (y >= HEIGHT) alive = false;
    }
    ObjectType getType() override { return ENEMY; }
    void setTexture(sf::Texture& tex) {
        sprite.setTexture(tex);
        sprite.setOrigin(tex.getSize().x / 2.f, tex.getSize().y / 2.f);
        sprite.setScale(0.06f, 0.06f);
    }
    void draw(sf::RenderWindow& window) override {
        if (!alive) return;
        sprite.setPosition(cx(x), cy(y));
        window.draw(sprite);
    }

};
float Viper::globalAngle = 0;


//  SEEKER – kamikaze dart
class Seeker : public Enemy {
    float lockedX, speed; bool locked;
    sf::Sprite sprite;
public:
    Seeker() : Enemy(0, 0, 0, 0, 1, 1, 2, 200, 0, 999), locked(false), speed(0.1f) { alive = true; }
    Seeker(float startX, float startY, sf::Texture& tex)
        : Enemy(startX, startY, 0, 0, 1, 1, 2, 200, 0, 999),
        locked(false), speed(0.1f)
    {
        alive = true;

        sprite.setTexture(tex);
        sprite.setOrigin(tex.getSize().x / 2.f, tex.getSize().y / 2.f);
        sprite.setScale(0.12f, 0.12f);
    }
    void reset(float px_, float py_) { x = px_; y = py_; locked = false; speed = 0.1f; alive = true; health = 2; }
    void setPosition(float px_, float py_) { x = px_; y = py_; }
    void lockPlayer(float playerX) { if (!locked) { lockedX = playerX; locked = true; } }
    void shoot(EnemyBullet*, int) override {}
    void update() override {
        if (!locked) return;
        x += (lockedX - x) * 0.05f;
        speed += 0.02f; y += speed;
        if (y >= HEIGHT) alive = false;
    }
    ObjectType getType() override { return ENEMY; }
    void setTexture(sf::Texture& tex) {
        sprite.setTexture(tex);
        sprite.setOrigin(tex.getSize().x / 2.f, tex.getSize().y / 2.f);
        sprite.setScale(0.06f, 0.06f);
    }
    void draw(sf::RenderWindow& window) override {
        if (!alive) return;
        sprite.setPosition(cx(x), cy(y));
        window.draw(sprite);
    }

};


//  ASTEROID
class Asteroid : public Entity {
    int size;
    float rot, rotSpeed;
    sf::Vector2f pts[8];

public:
    Asteroid()
        : Entity(0, 0, 0, 1, 1, 1), size(1), rot(0), rotSpeed(1.f) {
        alive = false;
    }

    Asteroid(float x, float y, float vx, float vy, float w, float h, int s = 1)
        : Entity(x, y, vx, vy, w, h), size(s), rot(0) {

        if (size < 1) size = 1;
        if (size > 5) size = 5;   

        rotSpeed = 0.4f + float(rand() % 8) / 10.f;

        for (int i = 0; i < 8; i++) {
            float ang = i * 3.14159f * 2 / 8;

            float base = CW * (0.30f + size * 0.12f);
            float r = base + float(rand() % int(CW * 0.2f + 1));

            pts[i] = { cosf(ang) * r, sinf(ang) * r };
        }
    }

    int getSize() const { return size; }
    void update() override {
        y += vy;
        rot += rotSpeed;

        if (y > HEIGHT) alive = false;
    }

    void onCollision(GameObject* o) override {
        if (o->getType() == BULLET) {
            ((Bullet*)o)->setAlive(false);
        }
        else if (o->getType() == ENEMY_BULLET) {
            ((EnemyBullet*)o)->setAlive(false);
        }
        else if (o->getType() == PLAYER) {
            ((Player*)o)->takeDamage();
        }
    }
    float getPixelX() const { return cx(x); }
    float getPixelY() const { return cy(y); }
    float getPixelRadius() const { return CW * (0.30f + size * 0.12f); }
    ObjectType getType() override {
        return ASTEROID;
    }

    void draw(sf::RenderWindow& w) override {
        if (!alive) return;

        float bx = cx(x), by = cy(y);
        float rad = rot * 3.14159f / 180.f;

        drawGlow(w, bx, by, CW * (0.25f + size * 0.08f),
            sf::Color(80, 60, 40, 45), 2);

        sf::ConvexShape ash(8);

        for (int i = 0; i < 8; i++) {
            float rx = pts[i].x * cosf(rad) - pts[i].y * sinf(rad);
            float ry = pts[i].x * sinf(rad) + pts[i].y * cosf(rad);

            ash.setPoint(i, { bx + rx, by + ry });
        }

        ash.setFillColor(sf::Color(90, 80, 70));
        ash.setOutlineColor(sf::Color(180, 160, 140));
        ash.setOutlineThickness(2.f);
        w.draw(ash);

        sf::CircleShape highlight(CW * 0.14f);
        highlight.setOrigin(CW * 0.14f, CW * 0.14f);
        highlight.setPosition(bx - CW * 0.18f, by - CH * 0.18f);
        highlight.setFillColor(sf::Color(200, 180, 150, 80));
        w.draw(highlight);
    }
};

//  BOSS BASE
class Boss : public Enemy {
protected:
    int maxHp, phase;
public:
    Boss(int x, int y, float vx, float vy, float w, float h, int hp, int sc, float cd)
        : Enemy(x, y, vx, vy, w, h, hp, sc, 0, cd), maxHp(hp), phase(1) {
    }
    virtual void drawHealthBar(sf::RenderWindow&, sf::Font&) = 0;
    void takeDamage(int dmg) override {
        Enemy::takeDamage(dmg);
       /* if (gSounds) {
            gSounds->playExplosion();
        }*/
        if (alive && health <= maxHp / 2 && phase == 1) phase = 2;
    }
    int getPhase() { return phase; }
    ObjectType getType() override { return BOSS; }
    virtual void onCollision(GameObject*) override = 0;
    virtual void specialAttack(Player&) = 0;
};


//  CRUISER BOSS
class Cruiser : public Boss {
    int gapX, laserTimer, idleDuration, warningDuration, laserDuration;
    bool warningActive, laserActive, hasDamagedPlayer, phaseSpeedBoosted;
    sf::Sprite sprite;

    // One shared gap size for drawing 
    int getGapWidth() const {
        return (phase == 1) ? 6 : 5;
    }

    void regenerateGap() {
        int gw = getGapWidth();
        gapX = rand() % (WIDTH - gw);
    }

public:
    Cruiser(int x, int y)
        : Boss(x, y, 0.3f, 0, 3, 1, 30, 500, 20),
        laserTimer(0),
        idleDuration(60),
        warningDuration(60),
        laserDuration(25),
        warningActive(false),
        laserActive(false),
        hasDamagedPlayer(false),
        phaseSpeedBoosted(false)
    {
        regenerateGap();
    }

    void update() override {
        if (!alive) return;

        // Phase 2 upgrade
        if (phase == 2 && !phaseSpeedBoosted) {
            if (vx > 0) {
                vx = 0.6f;
            }
            else {
                vx = -0.6f;
            }

            idleDuration = 50;
            warningDuration = 50;
            laserDuration = 30;

            laserTimer = 0;
            phaseSpeedBoosted = true;
          /*  regenerateGap();*/
        }

        // Move only once per frame
        x += vx;

        float leftLimit = 5.f;
        float rightLimit = WIDTH - 5.f;

        if (x < leftLimit) {
            x = leftLimit;
            vx = fabs(vx);
        }

        if (x > rightLimit) {
            x = rightLimit;
            vx = -fabs(vx);
        }

        // Laser timing cycle
        laserTimer++;
        //During the idle stage, no warning and no laser are active.
        if (laserTimer < idleDuration) {
            warningActive = false;
            laserActive = false;
        }
        else if (laserTimer < idleDuration + warningDuration) {
            warningActive = true;
            laserActive = false;
        }
        else if (laserTimer < idleDuration + warningDuration + laserDuration) {
            warningActive = false;
            laserActive = true;
        }
        else {
            warningActive = false;
            laserActive = false;

            laserTimer = 0;
            hasDamagedPlayer = false;

            regenerateGap();
        }
    }

    void specialAttack(Player& p) override {
        if (laserActive && !hasDamagedPlayer) {
            int gw = getGapWidth();

            // Checks player's body/wings, not just centre point
            float playerLeft = p.getx() - 0.7f;
            float playerRight = p.getx() + 0.7f;

            float gapLeft = (float)gapX;
            float gapRight = (float)(gapX + gw);

            bool fullyInsideGap = playerLeft >= gapLeft && playerRight <= gapRight;

            if (!fullyInsideGap) {
                p.takeDamage();
                hasDamagedPlayer = true;
            }
        }
    }

    void shoot(EnemyBullet*, int) override {
        // Cruiser does not shoot normal bullets
    }

    void onCollision(GameObject* o) override {
        if (o->getType() == BULLET) {
            Bullet* b = (Bullet*)o;
            if (!b->isPiercing()) b->setAlive(false);  // piercing stays alive
            takeDamage(1);
        }
    }

    ObjectType getType() override {
        return CRUISER;
    }

    void setTexture(sf::Texture& tex) {
        sprite.setTexture(tex);
        sprite.setOrigin(tex.getSize().x / 2.f, tex.getSize().y / 2.f);
        sprite.setScale(0.20f, 0.20f);
    }

    void draw(sf::RenderWindow& w) override {
        if (!alive) return;

        float by = cy(y);
        int gw = getGapWidth();

        float beamStartY = by + CH * 3.2f;
        float beamEndY = WIN_H - 35.f;
        float beamHeight = beamEndY - beamStartY;

        if (beamHeight < 0) beamHeight = 0;

        // Warning beams
        if (warningActive) {
            for (int col = 0; col < WIDTH; col++) {
                if (col >= gapX && col < gapX + gw) continue;

                sf::RectangleShape warning(sf::Vector2f(CW * 0.45f, beamHeight));
                warning.setPosition(px(col) + CW * 0.275f, beamStartY);

                int alpha = (frameCount % 20 < 10) ? 65 : 25;
                warning.setFillColor(sf::Color(255, 180, 0, alpha));

                w.draw(warning);
            }
        }

        // Actual laser beams
        if (laserActive) {
            for (int col = 0; col < WIDTH; col++) {
                if (col >= gapX && col < gapX + gw) continue;

                sf::RectangleShape glow(sf::Vector2f(CW * 0.75f, beamHeight));
                glow.setPosition(px(col) + CW * 0.125f, beamStartY);
                glow.setFillColor(sf::Color(255, 80, 0, 45));
                w.draw(glow);

                sf::RectangleShape beam(sf::Vector2f(CW * 0.35f, beamHeight));
                beam.setPosition(px(col) + CW * 0.325f, beamStartY);
                beam.setFillColor(sf::Color(255, 40, 0, 190));
                w.draw(beam);
            }
        }

        sprite.setPosition(cx(x), cy(y));
        w.draw(sprite);
    }

    void drawHealthBar(sf::RenderWindow& w, sf::Font& font) override {
        float bw = 300.f;
        float bh = 14.f;
        float bx = 20.f;
        float by_ = 62.f;

        sf::RectangleShape bg(sf::Vector2f(bw, bh));
        bg.setPosition(bx, by_);
        bg.setFillColor(sf::Color(40, 40, 60, 220));
        w.draw(bg);

        sf::RectangleShape bar(sf::Vector2f(bw * (float)health / maxHp, bh));
        bar.setPosition(bx, by_);
        bar.setFillColor(
            phase == 2
            ? sf::Color(255, 80, 0)
            : sf::Color(80, 160, 255)
        );
        w.draw(bar);

        sf::Text t;
        t.setFont(font);
        t.setString(
            "CRUISER  " + to_string(health) + "/" + to_string(maxHp) +
            (phase == 2 ? "  [PHASE 2]" : "")
        );
        t.setCharacterSize(10);
        t.setFillColor(sf::Color::White);
        t.setPosition(bx + 4, by_ + 1);

        w.draw(t);
    }
};

//  TURRET 
class Turret {
    float x, y, health, maxHealth;
    bool alive;
    bool falling;
    bool hasDamagedPlayer;

    float fireTimer, fireCoolDown;
    sf::Sprite sprite;

    // falling animation variables
    float fallX, fallY;
    float fallVX, fallVY;
    float fallRot;
    float fallRotSpeed;

public:
    Turret(float x, float y, int hp, float cd)
        : x(x), y(y), health(hp), maxHealth(hp), alive(true), falling(false),
        hasDamagedPlayer(false),
        fireTimer(0), fireCoolDown(cd),
        fallX(0), fallY(0), fallVX(0), fallVY(0), fallRot(0), fallRotSpeed(0) {
    }

    void update() {
        if (alive) {
            fireTimer++;
        }

        // falling turret animation
        if (falling) {
            fallX += fallVX;
            fallY += fallVY;

            fallVY += 0.18f;      // gravity
            fallRot += fallRotSpeed;

            // stop drawing once it falls off screen
            if (fallY > WIN_H + 120.f) {
                falling = false;
            }
        }
    }
    void shoot(EnemyBullet b[], int n, float spawnX, float spawnY) {
        if (!alive) return;

        if (fireTimer >= fireCoolDown) {
            fireTimer = 0;

            for (int i = 0; i < n; i++) {
                if (!b[i].Isalive()) {
                    b[i].shoot(spawnX, spawnY);
                    break;
                }
            }
        }
    }
    void startFalling(bool Side) {
        alive = false;
        falling = true;
        hasDamagedPlayer = false;

        fallX = cx(x);
        fallY = cy(y);

        if (Side) {
            fallVX = -1.2f;       // left turret falls left
            fallRotSpeed = -6.f;
        }
        else {
            fallVX = 1.2f;        // right turret falls right
            fallRotSpeed = 6.f;
        }

        fallVY = 1.0f;
        fallRot = 0.f;
    }
    void takeDamage(int d, bool leftSide) {
        if (!alive) return;

        health -= d;

        if (health <= 0) {
            health = 0;
            startFalling(leftSide);
        }
    }

    bool isAlive() const { return alive; }
    bool isFalling() const { return falling; }

    float getX() const { return x; }
    float getY() const { return y; }
    sf::FloatRect getSpriteBounds() {
        sprite.setRotation(0.f);
        sprite.setPosition(cx(x), cy(y));
        return sprite.getGlobalBounds();
    }

    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }

    void checkPlayerCollision(Player& p) {
        if (!falling || hasDamagedPlayer) return;

        float px_ = cx(p.getx());
        float py_ = cy(p.gety());
        float pr = p.getRadius();

        // Turret radius approx CW * 0.65f
        float tr = CW * 0.65f;

        float dx = fallX - px_;
        float dy = fallY - py_;
        float distSq = dx * dx + dy * dy;
        float minDist = pr + tr;

        if (distSq < minDist * minDist) {
            p.takeDamage();
            hasDamagedPlayer = true;
        }
    }
    void setPosition(float px_, float py_) {
        x = px_;
        y = py_;
    }
    void drawAt(sf::RenderWindow& w, float px, float py) {
        if (alive) {
            sprite.setRotation(0.f);
            sprite.setPosition(px, py);
            w.draw(sprite);
        }
        else if (falling) {
            sprite.setPosition(fallX, fallY);
            sprite.setRotation(fallRot);
            w.draw(sprite);
        }
    }

    void setTexture(sf::Texture& tex) {
        sprite.setTexture(tex);
        sprite.setOrigin(tex.getSize().x / 2.f, tex.getSize().y / 2.f);
        sprite.setScale(0.13f, 0.13f);
    }

    void draw(sf::RenderWindow& w) {
        if (alive) {
            sprite.setRotation(0.f);
            sprite.setPosition(cx(x), cy(y));
            w.draw(sprite);
        }
        else if (falling) {
            sprite.setPosition(fallX, fallY);
            sprite.setRotation(fallRot);
            w.draw(sprite);
        }
    }
};

//  TWIN CANNONS BOSS
class TwinCannons : public Boss {
    Turret leftTurret, rightTurret;

    bool phaseSpeedBoosted;
    bool coreShouldFire;

    bool coreLaserActive;
    bool coreLaserDamagedPlayer;

    float coreFireTimer;
    float coreCooldown;

    float coreLaserTimer;
    float coreLaserDuration;

    static const int TURRET_OFFSET = 4;
    float coreVulnerableTimer;
    float CORE_VULNERABLE_DELAY;

    sf::Sprite bodySprite;

public:
    TwinCannons(int x, int y)
        : Boss(x, y, 0.2f, 0, 5, 1, 60, 1000, 25),
        leftTurret(x - TURRET_OFFSET, y + 1, 15, 20),
        rightTurret(x + TURRET_OFFSET, y + 1, 15, 20),
        phaseSpeedBoosted(false),
        coreShouldFire(false),
        coreLaserActive(false),
        coreLaserDamagedPlayer(false),
        coreFireTimer(0),
        coreCooldown(150),
        coreLaserTimer(0),
        coreLaserDuration(20), coreVulnerableTimer(0.f),
        CORE_VULNERABLE_DELAY(60.f)
    {
    }

    void update() override {
        if (!alive) return;

        // Phase 2 speed boost
        if (phase == 2 && !phaseSpeedBoosted) {
            vx = (vx > 0) ? 0.6f : -0.6f;
            phaseSpeedBoosted = true;
            coreCooldown = 60;
        }

        // Move only ONCE per frame
        x += vx;

        float leftLimit = 5.f;
        float rightLimit = WIDTH - 5.f;

        if (x < leftLimit) {
            x = leftLimit;
            vx = fabs(vx);
        }

        if (x > rightLimit) {
            x = rightLimit;
            vx = -fabs(vx);
        }
        leftTurret.setPosition(
            max(0.f, x - TURRET_OFFSET),
            min((float)HEIGHT - 1.f, y + 2.1f)
        );

        rightTurret.setPosition(
            min((float)WIDTH - 1.f, x + TURRET_OFFSET),
            min((float)HEIGHT - 1.f, y + 2.1f)
        );
        leftTurret.update();
        rightTurret.update();



        bool turretsDown = !leftTurret.isAlive() && !rightTurret.isAlive();
        if (turretsDown && coreVulnerableTimer < CORE_VULNERABLE_DELAY)
            coreVulnerableTimer++;
        bool coreVulnerable = turretsDown && coreVulnerableTimer >= CORE_VULNERABLE_DELAY;

        if (coreVulnerable) {
            if (!coreLaserActive) {
                coreFireTimer++;

                if (coreFireTimer >= coreCooldown) {
                    coreFireTimer = 0;
                    coreLaserActive = true;
                    coreLaserTimer = 0;
                    coreLaserDamagedPlayer = false;
                }
            }
            else {
                coreLaserTimer++;

                if (coreLaserTimer >= coreLaserDuration) {
                    coreLaserActive = false;
                    coreLaserTimer = 0;
                    coreLaserDamagedPlayer = false;
                }
            }
        }
        else {
            coreFireTimer = 0;
            coreLaserTimer = 0;
            coreLaserActive = false;
            coreLaserDamagedPlayer = false;
        }
    }

    void shoot(EnemyBullet b[], int n) override {
        if (!alive) return;

        if (leftTurret.isAlive()) {
            leftTurret.shoot(b, n, x - TURRET_OFFSET, y + 2.1f);
        }

        if (rightTurret.isAlive()) {
            rightTurret.shoot(b, n, x + TURRET_OFFSET, y + 2.1f);
        }
    }

    void specialAttack(Player& p) override {
        leftTurret.checkPlayerCollision(p);
        rightTurret.checkPlayerCollision(p);

        bool coreVulnerable = !leftTurret.isAlive() && !rightTurret.isAlive();

        if (!coreVulnerable) return;
        if (!coreLaserActive) return;

        float playerLeft = p.getx() - 1.15f;
        float playerRight = p.getx() + 1.15f;
        float playerTop = p.gety() - 1.05f;
        float playerBottom = p.gety() + 1.05f;

        float laserLeft = x - 0.6f;
        float laserRight = x + 0.6f;
        float laserTop = y + 0.5f;
        float laserBottom = HEIGHT;

        bool overlapX =
            playerRight >= laserLeft &&
            playerLeft <= laserRight;

        bool overlapY =
            playerBottom >= laserTop &&
            playerTop <= laserBottom;

        if (overlapX && overlapY) {
            p.takeDamage();
        }
    }
    void onCollision(GameObject* o) override {
        if (o->getType() == BULLET) {
            Bullet* b = (Bullet*)o;

            // Bullet position in PIXELS, because PNG bounds are also pixels
            float bulletPX = cx(b->getx());
            float bulletPY = cy(b->gety());

            // Make bullet hitbox similar to its visual laser shape
            sf::FloatRect bulletBounds(
                bulletPX - 4.f,
                bulletPY - CH * 0.55f,
                8.f,
                CH * 1.1f
            );

            bool coreVulnerable = !leftTurret.isAlive() && !rightTurret.isAlive();

            // Actual PNG bounds of turrets
            sf::FloatRect leftBounds = leftTurret.getSpriteBounds();
            sf::FloatRect rightBounds = rightTurret.getSpriteBounds();

            // Actual PNG bounds of the main TwinCannons body/core
            bodySprite.setPosition(cx(x), cy(y));
            sf::FloatRect coreBounds = bodySprite.getGlobalBounds();

            // Make core hitbox taller so bullets register properly
            coreBounds.top -= CH * 0.8f;        // extends hitbox upward
            coreBounds.height += CH * 1.2f;     // increases total height

            bool hitLeftTurret =
                leftTurret.isAlive() &&
                bulletBounds.intersects(leftBounds);

            bool hitRightTurret =
                rightTurret.isAlive() &&
                bulletBounds.intersects(rightBounds);

            bool hitCore =
                coreVulnerable &&
                bulletBounds.intersects(coreBounds);

            if (hitLeftTurret) {
                leftTurret.takeDamage(b->isPiercing() ? 2 : 1, true);

                if (!b->isPiercing())
                    b->setAlive(false);
            }
            else if (hitRightTurret) {
                rightTurret.takeDamage(b->isPiercing() ? 2 : 1, false);

                if (!b->isPiercing())
                    b->setAlive(false);
            }
            else if (hitCore) {
                takeDamage(b->isPiercing() ? 2 : 1);

                if (!b->isPiercing())
                    b->setAlive(false);
            }
        }
    }
    ObjectType getType() override {
        return TWINCANNONS;
    }

    void setTextures(sf::Texture& bodyTex, sf::Texture& leftTex, sf::Texture& rightTex) {
        bodySprite.setTexture(bodyTex);
        bodySprite.setOrigin(bodyTex.getSize().x / 2.f, bodyTex.getSize().y / 2.f);
        bodySprite.setScale(0.20f, 0.20f);

        leftTurret.setTexture(leftTex);
        rightTurret.setTexture(rightTex);
    }

    void draw(sf::RenderWindow& w) override {
        if (!alive) return;

        float bx = cx(x);
        float by = cy(y);

        bool coreVulnerable = !leftTurret.isAlive() && !rightTurret.isAlive();

        // Main body
        bodySprite.setPosition(bx, by);
        w.draw(bodySprite);

        // Turrets
        float turretY = by + CH * 2.1f;
        float leftX = bx - CW * TURRET_OFFSET;
        float rightX = bx + CW * TURRET_OFFSET;

        if (leftTurret.isAlive() || leftTurret.isFalling()) {
            leftTurret.drawAt(w, leftX, turretY);
        }

        if (rightTurret.isAlive() || rightTurret.isFalling()) {
            rightTurret.drawAt(w, rightX, turretY);
        }

        // Core glow after both turrets destroyed
        if (coreVulnerable) {
            drawGlow(w, bx, by, CW * 0.5f, sf::Color(255, 0, 0, 100), 2);
        }

        // Core laser visual - synced with specialAttack()
        if (coreVulnerable && coreLaserActive) {
            float laserStartY = by + CH * 0.5f;
            float laserHeight = WIN_H - laserStartY;

            // Outer laser
            sf::RectangleShape laser(sf::Vector2f(CW * 0.8f, laserHeight));
            laser.setPosition(bx - CW * 0.4f, laserStartY);
            laser.setFillColor(sf::Color(255, 80, 0, 150));
            w.draw(laser);

            // Bright center
            sf::RectangleShape core(sf::Vector2f(CW * 0.25f, laserHeight));
            core.setPosition(bx - CW * 0.125f, laserStartY);
            core.setFillColor(sf::Color(255, 240, 200, 230));
            w.draw(core);
        }
    }
    void drawHealthBar(sf::RenderWindow& w, sf::Font& font) override {
        if (!gFontLoaded) return;
        float bw = 300.f;
        float bh = 14.f;
        float bx = 20.f;
        float by_ = 62.f;

        // MAIN CORE HEALTH BAR
        sf::RectangleShape bg(sf::Vector2f(bw, bh));
        bg.setPosition(bx, by_);
        bg.setFillColor(sf::Color(40, 40, 60, 220));
        w.draw(bg);

        sf::RectangleShape bar(sf::Vector2f(bw * (float)health / maxHp, bh));
        bar.setPosition(bx, by_);
        bar.setFillColor(
            phase == 2
            ? sf::Color(255, 80, 0)
            : sf::Color(255, 160, 40)
        );
        w.draw(bar);

        sf::Text title;
        title.setFont(font);
        title.setString(
            "TWIN CANNONS CORE: " + to_string(health) + "/" + to_string(maxHp) +
            (phase == 2 ? "  [PHASE 2]" : "")
        );
        title.setCharacterSize(10);
        title.setFillColor(sf::Color::White);
        title.setPosition(bx + 4, by_ + 1);
        w.draw(title);

        // TURRET HEALTH BAR SETTINGS
        float smallW = 135.f;
        float smallH = 8.f;
        float smallY = by_ + 18.f;

        // LEFT TURRET BACKGROUND
        sf::RectangleShape leftBg(sf::Vector2f(smallW, smallH));
        leftBg.setPosition(bx, smallY);
        leftBg.setFillColor(sf::Color(35, 35, 45, 220));
        w.draw(leftBg);

        float leftRatio = 0.f;

        if (leftTurret.getMaxHealth() > 0) {
            leftRatio = (float)leftTurret.getHealth() / leftTurret.getMaxHealth();
        }

        sf::RectangleShape leftBar(sf::Vector2f(smallW * leftRatio, smallH));
        leftBar.setPosition(bx, smallY);
        leftBar.setFillColor(leftTurret.isAlive() ? sf::Color(80, 180, 255) : sf::Color(80, 80, 80));
        w.draw(leftBar);

        sf::Text leftText;
        leftText.setFont(font);
        leftText.setString(
            "L: " + to_string(leftTurret.getHealth()) + "/" + to_string(leftTurret.getMaxHealth())
        );
        leftText.setCharacterSize(9);
        leftText.setFillColor(sf::Color::White);
        leftText.setPosition(bx + 4, smallY - 1.f);
        w.draw(leftText);

        // RIGHT TURRET BACKGROUND
        sf::RectangleShape rightBg(sf::Vector2f(smallW, smallH));
        rightBg.setPosition(bx + bw - smallW, smallY);
        rightBg.setFillColor(sf::Color(35, 35, 45, 220));
        w.draw(rightBg);

        float rightRatio = 0.f;

        if (rightTurret.getMaxHealth() > 0) {
            rightRatio = (float)rightTurret.getHealth() / rightTurret.getMaxHealth();
        }

        sf::RectangleShape rightBar(sf::Vector2f(smallW * rightRatio, smallH));
        rightBar.setPosition(bx + bw - smallW, smallY);
        rightBar.setFillColor(rightTurret.isAlive() ? sf::Color(255, 120, 120) : sf::Color(80, 80, 80));
        w.draw(rightBar);

        sf::Text rightText;
        rightText.setFont(font);
        rightText.setString(
            "R: " + to_string(rightTurret.getHealth()) + "/" + to_string(rightTurret.getMaxHealth())
        );
        rightText.setCharacterSize(9);
        rightText.setFillColor(sf::Color::White);
        rightText.setPosition(bx + bw - smallW + 4, smallY - 1.f);
        w.draw(rightText);
    }
};

//  MOTHERSHIP BOSS
class Mothership : public Boss {
    float spawnTimer, spawnCooldown;
    float laserTimer, idleDuration, warningDuration, laserDuration;

    bool warningActive;
    bool laserActive;
    bool hasDamagedPlayer;
    bool phaseSpeedBoosted;

    float bulletFireTimer, bulletCooldown;
    float moveTimer;
    float baseY;

    float dirX, dirY;

    float leftLimit;
    float rightLimit;
    float topLimit;
    float bottomLimit;
    float laserDamageCooldown;

    sf::Sprite sprite;
    sf::Sprite laserSprite;

public:
    Mothership(int x, int y)
        : Boss(x, y, -0.18f, 0, 5, 1, 100, 1500, 20),
        spawnTimer(0),
        spawnCooldown(40),
        laserTimer(0),
        idleDuration(20),       
        warningDuration(18),  
        laserDuration(160),      
        warningActive(false),
        laserActive(false),
        hasDamagedPlayer(false),
        phaseSpeedBoosted(false),
        bulletFireTimer(0),
        bulletCooldown(15),
        moveTimer(0),
        baseY(1.f),
        dirX(-1.f),
        dirY(1.f),
        leftLimit(3.f),
        rightLimit(57.f),
        topLimit(1.f),
        bottomLimit(22.f),
        laserDamageCooldown(0)
    {
        this->x = 57.f;
        this->y = 1.f;

        baseY = 1.f;
        dirX = -1.f;
        dirY = 1.f;
    }

    void update() override {
        if (!alive) return;
        if (laserDamageCooldown > 0)
            laserDamageCooldown--;
        moveTimer += 1.0f;

        float hSpeed = (phase == 1) ? 0.18f : 0.28f;
        float vSpeed = (phase == 1) ? 0.04f : 0.06f;

        // Diagonal movement
        x += hSpeed * dirX;
        baseY += vSpeed * dirY;

        if (x <= leftLimit) {
            x = leftLimit; 
            dirX = 1.f;
        }
        if (x >= rightLimit) {
            x = rightLimit; 
            dirX = -1.f;
        }
        if (baseY >= bottomLimit) {
            baseY = bottomLimit; 
            dirY = -1.f;
        }
        if (baseY <= topLimit) {
            baseY = topLimit;
            dirY = 1.f;
        }

        // Hover effect
        float hover = sinf(moveTimer * 0.08f) * 0.4f;
        y = baseY + hover;

        // Phase 2 boost
        if (phase == 2 && !phaseSpeedBoosted) {
            idleDuration = 10;
            warningDuration = 10;
            laserDuration = 220;
            bulletCooldown = 10;
            laserTimer = 0;
            phaseSpeedBoosted = true;
        }
        // Laser cycle
        laserTimer++;

        if (laserTimer < idleDuration) {
            warningActive = false;
            laserActive = false;
            bulletFireTimer++;
        }
        else if (laserTimer < idleDuration + warningDuration) {
            warningActive = true;
            laserActive = false;
            bulletFireTimer = 0;
        }
        else if (laserTimer < idleDuration + warningDuration + laserDuration) {
            warningActive = false;
            laserActive = true;
        }
        else {
            warningActive = false;
            laserActive = false;
            laserTimer = 0;
            hasDamagedPlayer = false;
            bulletFireTimer = 0;
        }

        spawnTimer++;
    }
    float getDirX() const { return dirX; }
    bool canSpawnSeeker() {
        if (!alive) return false;

        if (spawnTimer >= spawnCooldown) {
            spawnTimer = 0;
            return true;
        }

        return false;
    }
    void shoot(EnemyBullet b[], int n) override {
        // Mothership does not shoot normal bullets
    }
    void specialAttack(Player& p) override {
        if (!laserActive) return;

        // Bigger player hitbox so full ship/wings are detected
        float playerLeft = p.getx() - 1.15f;
        float playerRight = p.getx() + 1.15f;
        float playerTop = p.gety() - 1.05f;
        float playerBottom = p.gety() + 1.05f;

        // Laser vertical area
        float laserTop = y + 0.9f;
        float laserBottom = HEIGHT;

        bool overlapY =
            playerBottom >= laserTop &&
            playerTop <= laserBottom;

        bool insideLaser = false;

        if (dirX < 0) {
            // Mothership moving left: laser covers right side
            float laserLeft = x - 0.4f;
            float laserRight = WIDTH;

            insideLaser =
                playerRight >= laserLeft &&
                playerLeft <= laserRight &&
                overlapY;
        }
        else {
            // Mothership moving right: laser covers left side
            float laserLeft = 0;
            float laserRight = x + 0.4f;

            insideLaser =
                playerRight >= laserLeft &&
                playerLeft <= laserRight &&
                overlapY;
        }

        if (insideLaser) {
            p.takeDamage();  
        }
    }
    void onCollision(GameObject* o) override {
        if (o->getType() == BULLET) {
            Bullet* b = (Bullet*)o;
            if (!b->isPiercing()) b->setAlive(false);  // piercing stays alive
            takeDamage(1);
        }
    }

    ObjectType getType() override {
        return MOTHERSHIP;
    }

    void setTexture(sf::Texture& tex) {
        sprite.setTexture(tex);
        sprite.setOrigin(tex.getSize().x / 2.f, tex.getSize().y / 2.f);
        sprite.setScale(0.20f, 0.20f);
    }

    void setLaserTexture(sf::Texture& tex) {
        laserSprite.setTexture(tex);

        // The image is horizontal, so origin is left-middle.
        // After rotation, it will fire downward from this origin.
        laserSprite.setOrigin(0.f, tex.getSize().y / 2.f);

        // Rotate horizontal beam to downward beam
        laserSprite.setRotation(90.f);
    }

    void draw(sf::RenderWindow& w) override {
        if (!alive) return;

        float bx = cx(x);
        float by = cy(y);
        float hw = CW * 3.2f;
        float hh = CH * 0.55f;

        // Phase 2 aura
        if (phase == 2) {
            float pulse = 0.5f + sinf(frameCount * 0.1f) * 0.5f;

            for (int i = 0; i < 3; i++) {
                float r = CW * (1.5f + i * 0.6f + pulse);

                sf::CircleShape aura(r);
                aura.setOrigin(r, r);
                aura.setPosition(bx, by);

                int alpha = 80 - i * 20;
                aura.setFillColor(sf::Color(255, 0, 120, alpha));

                w.draw(aura);
            }
        }

        // Main mothership sprite
        sprite.setPosition(cx(x), cy(y));
        w.draw(sprite);

        // Dome glow
        float pulse = 0.6f + sinf(frameCount * 0.08f) * 0.4f;

        drawGlow(
            w,
            bx,
            by - hh * 0.8f,
            CW * (0.8f + pulse * 0.25f),
            phase == 2 ? sf::Color(255, 0, 120, 130) : sf::Color(0, 180, 255, 130),
            3
        );

        sf::CircleShape dome(CW * (0.7f + pulse * 0.15f));
        dome.setOrigin(dome.getRadius(), dome.getRadius());
        dome.setPosition(bx, by - hh * 0.8f);
        dome.setFillColor(
            phase == 2
            ? sf::Color(255, 60, 120, 190)
            : sf::Color(0, 180, 255, 180)
        );
        dome.setOutlineColor(sf::Color(220, 240, 255));
        dome.setOutlineThickness(1.5f);
        w.draw(dome);

        // Small engine glows
        for (int i = 0; i < 3; i++) {
            drawGlow(
                w,
                bx - CW * (1.f - i * 0.5f),
                by + hh,
                CW * 0.2f,
                sf::Color(0, 80, 200, 120),
                2
            );

            drawGlow(
                w,
                bx + CW * (1.f - i * 0.5f),
                by + hh,
                CW * 0.2f,
                sf::Color(0, 80, 200, 120),
                2
            );
        }

        // Downward warning line before laser fires
        if (warningActive) {
            float warnStartX;
            float warnW;

            float warnStartY = cy(y) + CH * 0.6f;   // starts below mothership
            float warnH = WIN_H - warnStartY;       // goes down to bottom

            if (dirX < 0) {
                // moving left -> warning on RIGHT side below mothership
                warnStartX = cx(x);
                warnW = WIN_W - warnStartX;
            }
            else {
                // moving right -> warning on LEFT side below mothership
                warnStartX = 0.f;
                warnW = cx(x);
            }

            float flicker = (frameCount % 10 < 5) ? 70.f : 25.f;

            sf::RectangleShape warn(sf::Vector2f(warnW, warnH));
            warn.setPosition(warnStartX, warnStartY);
            warn.setFillColor(sf::Color(255, 80, 180, (sf::Uint8)flicker));
            w.draw(warn);

            // vertical warning stripes, also starting below mothership
            for (int i = 0; i < 8; i++) {
                float stripeX = warnStartX + i * (warnW / 8.f);

                sf::RectangleShape stripe(sf::Vector2f(4.f, warnH));
                stripe.setPosition(stripeX, warnStartY);
                stripe.setFillColor(sf::Color(255, 200, 255, 90));
                w.draw(stripe);
            }

            // charging rings exactly under mothership
            for (int i = 0; i < 5; i++) {
                float r = CW * (0.5f + i * 0.35f + sinf(frameCount * 0.15f) * 0.15f);

                sf::CircleShape ring(r);
                ring.setOrigin(r, r);
                ring.setPosition(cx(x), warnStartY);
                ring.setFillColor(sf::Color(0, 0, 0, 0));
                ring.setOutlineColor(sf::Color(255, 80, 180, 140 - i * 22));
                ring.setOutlineThickness(2.f);

                w.draw(ring);
            }
        }
        if (laserActive) {
            float laserStartX;
            float laserW;

            float laserStartY = cy(y) + CH * 0.6f;  // starts below mothership
            float laserH = WIN_H - laserStartY;     // extends downward only

            if (dirX < 0) {
                // moving left -> laser covers RIGHT side below mothership
                laserStartX = cx(x);
                laserW = WIN_W - laserStartX;
            }
            else {
                // moving right -> laser covers LEFT side below mothership
                laserStartX = 0.f;
                laserW = cx(x);
            }

            // origin glow under mothership
            drawGlow(
                w,
                cx(x),
                laserStartY,
                CW * 2.0f,
                sf::Color(255, 40, 200, 150),
                4
            );

            // huge outer glow covering side below mothership
            sf::RectangleShape glowOuter(sf::Vector2f(laserW, laserH));
            glowOuter.setPosition(laserStartX, laserStartY);
            glowOuter.setFillColor(sf::Color(255, 20, 180, 55));
            w.draw(glowOuter);

            // stronger middle laser area
            sf::RectangleShape glowMid(sf::Vector2f(laserW, laserH));
            glowMid.setPosition(laserStartX, laserStartY);
            glowMid.setFillColor(sf::Color(255, 70, 220, 75));
            w.draw(glowMid);

            // bright vertical core strips
            for (int i = 0; i < 6; i++) {
                float stripX = laserStartX + i * (laserW / 6.f);

                sf::RectangleShape core(sf::Vector2f(18.f, laserH));
                core.setPosition(stripX, laserStartY);
                core.setFillColor(sf::Color(255, 245, 255, 180));
                w.draw(core);
            }

            // animated vertical energy pulses
            for (int i = 0; i < 12; i++) {
                float pulseX = laserStartX + fmod((i * 90.f) + frameCount * 8.f, laserW);

                sf::RectangleShape pulse(sf::Vector2f(35.f, laserH));
                pulse.setPosition(pulseX, laserStartY);
                pulse.setFillColor(sf::Color(255, 255, 255, 90));
                w.draw(pulse);
            }

            // beam mouth / source under mothership
            sf::CircleShape source(CW * 0.9f);
            source.setOrigin(source.getRadius(), source.getRadius());
            source.setPosition(cx(x), laserStartY);
            source.setFillColor(sf::Color(255, 245, 255, 230));
            source.setOutlineColor(sf::Color(255, 80, 220, 200));
            source.setOutlineThickness(3.f);
            w.draw(source);

            // screen flash
            sf::RectangleShape flash(sf::Vector2f(WIN_W, WIN_H));
            flash.setFillColor(sf::Color(255, 20, 180, 20));
            w.draw(flash);
        }
    }
    void drawHealthBar(sf::RenderWindow& w, sf::Font& font) override {
        if (!gFontLoaded) return;

        float bw = 300.f;
        float bh = 14.f;
        float bx = 20.f;
        float by_ = 62.f;

        sf::RectangleShape bg(sf::Vector2f(bw, bh));
        bg.setPosition(bx, by_);
        bg.setFillColor(sf::Color(40, 40, 60, 220));
        w.draw(bg);

        sf::RectangleShape bar(sf::Vector2f(bw * (float)health / maxHp, bh));
        bar.setPosition(bx, by_);
        bar.setFillColor(
            phase == 2
            ? sf::Color(200, 0, 255)
            : sf::Color(255, 50, 50)
        );
        w.draw(bar);

        sf::Text t;
        t.setFont(font);
        t.setString(
            "MOTHERSHIP  " +
            to_string(health) +
            "/" +
            to_string(maxHp) +
            (phase == 2 ? "  [PHASE 2]" : "")
        );

        t.setCharacterSize(10);
        t.setFillColor(sf::Color::White);
        t.setPosition(bx + 4, by_ + 1);
        w.draw(t);
    }
};

void trySpawnSeeker(Mothership& m, Seeker sk[], int n) {
    if (!m.canSpawnSeeker()) return;
    for (int i = 0; i < n; i++) {
        if (!sk[i].Isalive())
        {
            sk[i].reset(rand() % WIDTH, 0.f);
            return;
        }
    }
}

void drawHUDPanel(sf::RenderWindow& w, float x, float y, float width, float height, sf::Color col) {
    // ===== BACK GLASS =====
    sf::RectangleShape bg(sf::Vector2f(width, height));
    bg.setPosition(x, y);
    bg.setFillColor(sf::Color(5, 10, 25, 110));
    w.draw(bg);

    // ===== SOFT GRADIENT (top highlight) =====
    sf::RectangleShape grad(sf::Vector2f(width, height / 2));
    grad.setPosition(x, y);
    grad.setFillColor(sf::Color(255, 255, 255, 15)); // very subtle
    w.draw(grad);

    // ===== THIN OUTLINE =====
    sf::RectangleShape outline(sf::Vector2f(width, height));
    outline.setPosition(x, y);
    outline.setFillColor(sf::Color::Transparent);
    outline.setOutlineThickness(1);
    outline.setOutlineColor(sf::Color(col.r, col.g, col.b, 90));
    w.draw(outline);

    // ===== CORNER CUT STYLE =====
    float cut = 10.f;
    sf::ConvexShape corner;
    corner.setPointCount(3);
    corner.setFillColor(sf::Color(col.r, col.g, col.b, 120));

    // top-left cut
    corner.setPoint(0, sf::Vector2f(x, y + cut));
    corner.setPoint(1, sf::Vector2f(x, y));
    corner.setPoint(2, sf::Vector2f(x + cut, y));
    w.draw(corner);

    // top-right cut
    corner.setPoint(0, sf::Vector2f(x + width - cut, y));
    corner.setPoint(1, sf::Vector2f(x + width, y));
    corner.setPoint(2, sf::Vector2f(x + width, y + cut));
    w.draw(corner);

    // bottom-left cut
    corner.setPoint(0, sf::Vector2f(x, y + height - cut));
    corner.setPoint(1, sf::Vector2f(x, y + height));
    corner.setPoint(2, sf::Vector2f(x + cut, y + height));
    w.draw(corner);

    // bottom-right cut
    corner.setPoint(0, sf::Vector2f(x + width - cut, y + height));
    corner.setPoint(1, sf::Vector2f(x + width, y + height));
    corner.setPoint(2, sf::Vector2f(x + width, y + height - cut));
    w.draw(corner);

    // ===== INNER LINE (gives depth) =====
    sf::RectangleShape inner(sf::Vector2f(width - 6, height - 6));
    inner.setPosition(x + 3, y + 3);
    inner.setFillColor(sf::Color::Transparent);
    inner.setOutlineThickness(1);
    inner.setOutlineColor(sf::Color(255, 255, 255, 20));
    w.draw(inner);
}
void drawHeart(sf::RenderWindow& w, float x, float y, float size, sf::Color color) {
    drawGlow(w, x + size / 2, y + size / 2, size * 0.9f, sf::Color(color.r, color.g, color.b, 60), 3);

    sf::CircleShape left(size / 2);
    sf::CircleShape right(size / 2);

    left.setPosition(x, y);
    right.setPosition(x + size / 2, y);

    left.setFillColor(color);
    right.setFillColor(color);

    sf::ConvexShape bottom;
    bottom.setPointCount(3);
    bottom.setPoint(0, sf::Vector2f(x - 2, y + size / 2));
    bottom.setPoint(1, sf::Vector2f(x + size + 2, y + size / 2));
    bottom.setPoint(2, sf::Vector2f(x + size / 2, y + size + 7));
    bottom.setFillColor(color);

    w.draw(left);
    w.draw(right);
    w.draw(bottom);
}

void drawBar(sf::RenderWindow& w, float x, float y, float width, float height, float fill, sf::Color color) {
    if (fill < 0) fill = 0;
    if (fill > 1) fill = 1;

    sf::RectangleShape bg(sf::Vector2f(width, height));
    bg.setPosition(x, y);
    bg.setFillColor(sf::Color(20, 25, 45, 180));
    bg.setOutlineThickness(1);
    bg.setOutlineColor(sf::Color(120, 160, 255, 90));
    w.draw(bg);

    sf::RectangleShape fg(sf::Vector2f(width * fill, height));
    fg.setPosition(x, y);
    fg.setFillColor(color);
    w.draw(fg);


}

//  HUD

void drawHUD(sf::RenderWindow& w, sf::Font& font, Player& p, int score, int multiplier, int level, bool survival, int highScore) {
    if (!gFontLoaded) return;

    sf::Text t;
    t.setFont(font);

    // top gradient-ish bars
    sf::RectangleShape top(sf::Vector2f((float)WIN_W, 64.f));
    top.setFillColor(sf::Color(0, 0, 20, 120));
    w.draw(top);

    sf::RectangleShape bottom(sf::Vector2f((float)WIN_W, 74.f));
    bottom.setPosition(0, WIN_H - 74.f);
    bottom.setFillColor(sf::Color(0, 0, 20, 145));
    w.draw(bottom);

    // SCORE PANEL
    drawHUDPanel(w, 12, 10, 190, 42, sf::Color(0, 220, 255));

    t.setCharacterSize(11);
    t.setFillColor(sf::Color(120, 220, 255));
    t.setString("SCORE");
    t.setPosition(24, 15);
    w.draw(t);

    t.setCharacterSize(20);
    t.setFillColor(sf::Color(230, 250, 255));
    t.setString(to_string(score));
    t.setPosition(24, 28);
    w.draw(t);

    t.setCharacterSize(10);
    t.setFillColor(sf::Color(255, 220, 120));
    t.setString("HIGH SCORE:  " + to_string(highScore));
    t.setPosition(105, 17);
    w.draw(t);

    // MULTIPLIER
    drawHUDPanel(w, 215, 10, 90, 42, sf::Color(255, 210, 60));

    t.setCharacterSize(11);
    t.setFillColor(sf::Color(255, 230, 120));
    t.setString("BOOST");
    t.setPosition(228, 15);
    w.draw(t);

    t.setCharacterSize(20);
    t.setFillColor(sf::Color(255, 220, 70));
    t.setString("x" + to_string(multiplier));
    t.setPosition(232, 28);
    w.draw(t);

    // LEVEL / WAVE CENTER PANEL
    drawHUDPanel(w, WIN_W / 2.f - 85, 10, 170, 42, sf::Color(140, 100, 255));

    string ls = survival ? "WAVE " + to_string(level) : "LEVEL " + to_string(level);
    t.setCharacterSize(20);
    t.setFillColor(sf::Color(220, 210, 255));
    t.setString(ls);
    sf::FloatRect lb = t.getLocalBounds();
    t.setPosition(WIN_W / 2.f - lb.width / 2.f, 21);
    w.draw(t);

    // LIVES PANEL
    drawHUDPanel(w, WIN_W - 230, 10, 215, 42, sf::Color(255, 70, 110));

    t.setCharacterSize(11);
    t.setFillColor(sf::Color(255, 150, 170));
    t.setString("LIVES");
    t.setPosition(WIN_W - 218, 15);
    w.draw(t);

    int maxLives = 5;
    int lives = p.getLives();

    for (int i = 0; i < maxLives; i++) {
        float hx = WIN_W - 160 + i * 28;
        float hy = 25;

        if (i < lives)
            drawHeart(w, hx, hy, 13, sf::Color(255, 75, 105));
        else
            drawHeart(w, hx, hy, 13, sf::Color(70, 30, 45, 170));
    }

    // BOTTOM LEFT WEAPON CARD
    float by = WIN_H - 62.f;

    string wname = "STANDARD";
    sf::Color wcol(100, 210, 255);

    if (p.getWeapon() == W_SPREAD) {
        wname = "SPREAD";
        wcol = sf::Color(255, 220, 0);
    }
    else if (p.getWeapon() == W_PIERCING) {
        wname = "PIERCING";
        wcol = sf::Color(190, 80, 255);
    }

    drawHUDPanel(w, 12, by, 210, 50, wcol);

    t.setCharacterSize(11);
    t.setFillColor(sf::Color(180, 220, 255));
    t.setString("WEAPON SYSTEM");
    t.setPosition(24, by + 7);
    w.draw(t);

    t.setCharacterSize(19);
    t.setFillColor(wcol);
    t.setString(wname);
    t.setPosition(24, by + 24);
    w.draw(t);

    // SHIELD CARD
    drawHUDPanel(w, 240, by, 220, 50, sf::Color(0, 255, 150));

    t.setCharacterSize(11);
    t.setFillColor(sf::Color(120, 255, 190));
    t.setString("SHIELD");
    t.setPosition(252, by + 7);
    w.draw(t);

    int shield = p.getShieldHits();
    for (int i = 0; i < 3; i++) {
        sf::CircleShape orb(7);
        orb.setPosition(255 + i * 28, by + 28);

        if (i < shield)
            orb.setFillColor(sf::Color(0, 255, 150));
        else
            orb.setFillColor(sf::Color(30, 70, 60));

        w.draw(orb);


    }

    // EMP CARD
    drawHUDPanel(w, 480, by, 145, 50, sf::Color(210, 0, 255));

    t.setCharacterSize(11);
    t.setFillColor(sf::Color(230, 140, 255));
    t.setString("EMP CHARGES");
    t.setPosition(492, by + 7);
    w.draw(t);

    t.setCharacterSize(20);
    t.setFillColor(sf::Color(230, 80, 255));
    t.setString(to_string(p.getEMPCount()));
    t.setPosition(500, by + 24);
    w.draw(t);

    // DASH BAR
    drawHUDPanel(w, WIN_W - 250, by, 235, 50, sf::Color(0, 200, 255));

    t.setCharacterSize(11);
    t.setFillColor(sf::Color(140, 230, 255));
    t.setString("DASH DRIVE");
    t.setPosition(WIN_W - 238, by + 7);
    w.draw(t);

    float dashFill = 1.f - p.getDashCooldown() / 38.f;
    drawBar(w, WIN_W - 238, by + 29, 165, 10, dashFill,
        dashFill >= 1.f ? sf::Color(0, 255, 140) : sf::Color(0, 170, 255));

    t.setCharacterSize(10);
    t.setFillColor(dashFill >= 1.f ? sf::Color(0, 255, 140) : sf::Color(180, 210, 255));
    t.setString(dashFill >= 1.f ? "READY" : "CHARGING");
    t.setPosition(WIN_W - 65, by + 24);
    w.draw(t);
}
int menuIndex = 0;
int modeIndex = 0;
bool hasSavedGame = false;
GameState savedPlayState = ARCADE;
int transitionTimer = 0;
const int TRANSITION_DURATION = 75;   // 2.5 seconds at 30fps
int  transitionNextLevel = 1;
bool transitionIsBoss = false;
bool transitionIsSurvival = false;



//  MENUS
void drawCenteredText(sf::RenderWindow& w, sf::Font& f,
    std::string text, float y, int size, sf::Color col)
{
    sf::Text t;
    t.setFont(f);
    t.setString(text);
    t.setCharacterSize(size);
    t.setFillColor(col);

    sf::FloatRect bounds = t.getLocalBounds();
    t.setOrigin(bounds.left + bounds.width / 2,
        bounds.top + bounds.height / 2);

    t.setPosition(WIN_W / 2.f, y);
    w.draw(t);
}
void drawGameOver(sf::RenderWindow& w,
    sf::Font& fontBold,
    sf::Font& fontMedium,
    int score, int highScore, int lvl, bool surv,
    sf::Texture& bgTex)
{
    w.setView(w.getDefaultView());
    w.clear(sf::Color(5, 0, 10));

    sf::Sprite bg;
    bg.setTexture(bgTex);
    bg.setPosition(0.f, 0.f);
    bg.setScale(
        (float)WIN_W / bgTex.getSize().x,
        (float)WIN_H / bgTex.getSize().y
    );
    w.draw(bg);

    float pulse = 0.7f + sinf(frameCount * 0.08f) * 0.3f;
    sf::Color titleCol(
        255,
        (sf::Uint8)(40 + pulse * 40),
        (sf::Uint8)(120 + pulse * 80)
    );

    // Main title
    drawCenteredText(w, fontBold,
        "GAME OVER",
        275, 60,
        titleCol);
    // High score directly under Game Over
    drawCenteredText(w, fontMedium,
        "HIGH SCORE: " + to_string(highScore),
        340, 28,
        sf::Color(120, 240, 255));

    // Final score under high score
    drawCenteredText(w, fontMedium,
        "FINAL SCORE: " + to_string(score),
        380, 28,
        sf::Color(255, 220, 80));

    // Level / waves under final score
    drawCenteredText(w, fontMedium,
        (surv ? "WAVES SURVIVED: " : "LEVEL REACHED: ") + to_string(lvl),
        430, 24,
        sf::Color(230, 230, 240));

    // Blinking return text
    if ((frameCount / 20) % 2 == 0) {
        drawCenteredText(w, fontMedium,
            "PRESS ENTER TO RETURN TO MENU",
            540, 20,
            sf::Color(180, 220, 255));
    }
}
void drawControls(sf::RenderWindow& w,
    sf::Font& fontBold,
    sf::Font& fontMedium)
{
    w.clear(sf::Color(5, 5, 20));

    // TITLE (Bold)
    drawCenteredText(w, fontBold, "CONTROLS", 80, 52, sf::Color(0, 220, 255));

    struct ControlEntry { string key, action; };
    ControlEntry controls[] = {
        { "LEFT / RIGHT",  "Move Ship (or A/D)" },
        { "SPACEBAR",      "Fire Primary Weapon" },
        { "E + ARROWS",    "Evasive Dash" },
        { "N KEY",         "Screen Nuke (EMP)" },
        { "ESCAPE",        "Pause Game" },
        { "R KEY",         "Resume (when paused)" },
        { "M KEY",         "Back to Menu (paused)" },
    };

    int count = 7;
    float startY = 170.f;
    float rowH = 55.f;

    for (int i = 0; i < count; i++) {
        float rowY = startY + i * rowH;

        // panel
        sf::RectangleShape panel(sf::Vector2f(720.f, 42.f));
        panel.setPosition(WIN_W / 2.f - 360.f, rowY);
        panel.setFillColor(sf::Color(10, 20, 50, 180));
        panel.setOutlineThickness(1);
        panel.setOutlineColor(sf::Color(0, 180, 255, 80));
        w.draw(panel);

        // KEY (left)
        sf::Text keyText;
        keyText.setFont(fontMedium);
        keyText.setString(controls[i].key);
        keyText.setCharacterSize(18);
        keyText.setFillColor(sf::Color(0, 220, 255));
        keyText.setPosition(WIN_W / 2.f - 340.f, rowY + 10.f);
        w.draw(keyText);

        // ACTION (right)
        sf::Text actText;
        actText.setFont(fontMedium);
        actText.setString(controls[i].action);
        actText.setCharacterSize(18);
        actText.setFillColor(sf::Color(220, 220, 230));
        actText.setPosition(WIN_W / 2.f - 40.f, rowY + 10.f);
        w.draw(actText);
    }

    // bottom hint
    drawCenteredText(w, fontMedium,
        "PRESS ANY KEY TO RETURN",
        650, 16,
        sf::Color(150, 170, 210));
}
void drawModeSelect(sf::RenderWindow& w,
    sf::Font& fontBold,
    sf::Font& fontMedium,
    sf::Sprite& background,
    sf::RectangleShape& bgOverlay)
{
    w.clear();
    w.draw(background);
    w.draw(bgOverlay);

    // TITLE (Bold)
    drawCenteredText(w, fontBold, "SELECT MODE", 120, 52, sf::Color(0, 220, 255));

    // subtitle (Medium)
    drawCenteredText(w, fontMedium, "Choose your battle style", 175, 16, sf::Color(120, 160, 200));

    string modes[3] = { "ARCADE", "SURVIVAL", "BACK" };
    string descs[3] = {
        "3 levels with bosses - reach the Mothership",
        "Endless waves - how long can you last?",
        "Return to main menu"
    };

    for (int i = 0; i < 3; i++) {
        float rowY = 270.f + i * 110.f;
        bool selected = (i == modeIndex);

        // panel
        sf::RectangleShape panel(sf::Vector2f(700.f, 80.f));
        panel.setPosition(WIN_W / 2.f - 350.f, rowY);
        panel.setFillColor(selected
            ? sf::Color(0, 40, 80, 200)
            : sf::Color(5, 10, 30, 150));
        panel.setOutlineThickness(selected ? 2.f : 1.f);
        panel.setOutlineColor(selected
            ? sf::Color(0, 220, 255, 220)
            : sf::Color(60, 80, 120, 100));
        w.draw(panel);

        // mode name (Medium but slightly bigger if selected)
        sf::Color nameCol = selected ? sf::Color(255, 220, 80) : sf::Color(200, 200, 220);
        string label = selected ? ("> " + modes[i]) : ("  " + modes[i]);

        drawCenteredText(w, fontMedium, label, rowY + 22,
            selected ? 30 : 26, nameCol);

        // description (small Medium)
        drawCenteredText(w, fontMedium, descs[i], rowY + 57,
            14, sf::Color(140, 160, 190));
    }

    drawCenteredText(w, fontMedium,
        "UP/DOWN to choose   ENTER to confirm",
        600, 14, sf::Color(150, 170, 210));
}
void drawMainMenu(sf::RenderWindow& w,
    sf::Font& fontBold,
    sf::Font& fontMedium,
    sf::Sprite& background,
    sf::RectangleShape& bgOverlay)
{
    w.clear();
    w.draw(background);
    w.draw(bgOverlay);

    drawCenteredText(w, fontBold, "SPACE  INVADERS", 120, 56, sf::Color(0, 220, 255));
    drawCenteredText(w, fontMedium, "Object-Oriented Galactic Defense", 170, 16, sf::Color(120, 160, 200));

    // Build options dynamically
    vector<string> options;
    if (hasSavedGame) options.push_back("RESUME GAME");
    options.push_back("START GAME");
    options.push_back("CONTROLS");
    options.push_back("EXIT");

    int count = (int)options.size();
    float startY = hasSavedGame ? 235.f : 270.f; // shift up slightly if 4 options

    for (int i = 0; i < count; i++) {
        sf::Color col = (i == menuIndex) ? sf::Color(255, 220, 80) : sf::Color(220, 220, 230);
        int size = (i == menuIndex) ? 32 : 26;
        string text = (i == menuIndex ? "> " : "  ") + options[i];

        // Highlight RESUME differently
        if (hasSavedGame && i == 0)
            col = (i == menuIndex) ? sf::Color(80, 255, 160) : sf::Color(60, 200, 130);

        drawCenteredText(w, fontMedium, text, startY + i * 55, size, col);
    }


    drawCenteredText(w, fontMedium, "Use UP/DOWN and ENTER", 570, 14, sf::Color(150, 170, 210));
}
void drawPauseIcon(sf::RenderWindow& w, sf::Font& font)
{
    float x = WIN_W / 2.f + 150.f;
    float y = 32.f;

    sf::RectangleShape box(sf::Vector2f(62.f, 28.f));
    box.setOrigin(31.f, 14.f);
    box.setPosition(x, y);
    box.setFillColor(sf::Color(10, 12, 30, 170));
    box.setOutlineThickness(1.2f);
    box.setOutlineColor(sf::Color(180, 120, 255, 170));
    w.draw(box);

    sf::RectangleShape bar1(sf::Vector2f(4.f, 14.f));
    sf::RectangleShape bar2(sf::Vector2f(4.f, 14.f));

    bar1.setOrigin(2.f, 7.f);
    bar2.setOrigin(2.f, 7.f);

    bar1.setPosition(x - 14.f, y);
    bar2.setPosition(x - 6.f, y);

    bar1.setFillColor(sf::Color(235, 210, 255));
    bar2.setFillColor(sf::Color(235, 210, 255));

    w.draw(bar1);
    w.draw(bar2);

    if (gFontLoaded) {
        sf::Text t;
        t.setFont(font);
        t.setString("ESC");
        t.setCharacterSize(10);
        t.setFillColor(sf::Color(235, 235, 255));

        sf::FloatRect b = t.getLocalBounds();
        t.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
        t.setPosition(x + 13.f, y - 1.f);
        w.draw(t);
    }
}void drawPauseOverlay(sf::RenderWindow& w, sf::Font& f) {
    sf::RectangleShape ov(sf::Vector2f((float)WIN_W, (float)WIN_H));
    ov.setFillColor(sf::Color(0, 0, 0, 160)); w.draw(ov);
    drawCenteredText(w, f, "PAUSED", 200, 50, sf::Color(0, 220, 255));
    drawCenteredText(w, f, "R  -  RESUME", 305, 26, sf::Color(100, 255, 140));
    drawCenteredText(w, f, "M  -  MAIN MENU", 360, 26, sf::Color(255, 160, 80));
}
void drawLevelTransition(sf::RenderWindow& w, sf::Font& fontBold, sf::Font& fontMedium)
{
    string title, sub;
    sf::Color mainCol;

    if (transitionIsBoss) {
        mainCol = sf::Color(255, 90, 50);
        title = "BOSS INCOMING";

        if (transitionNextLevel == 1)
            sub = "The Cruiser approaches...";
        else if (transitionNextLevel == 2)
            sub = "Twin Cannons unleashed...";
        else
            sub = "The Mothership descends...";
    }
    else if (transitionIsSurvival) {
        mainCol = sf::Color(255, 220, 80);
        title = "WAVE " + to_string(transitionNextLevel);
        sub = "Enemies inbound...";
    }
    else {
        mainCol = sf::Color(0, 220, 255);
        title = "LEVEL " + to_string(transitionNextLevel);

        if (transitionNextLevel == 2)
            sub = "Twin Cannons awaits beyond...";
        else
            sub = "The Mothership looms ahead...";
    }

    float progress = 1.f - (float)transitionTimer / (float)TRANSITION_DURATION;

    if (progress < 0.f) progress = 0.f;
    if (progress > 1.f) progress = 1.f;

    // Very light dark tint only, game still visible
    sf::RectangleShape tint(sf::Vector2f((float)WIN_W, (float)WIN_H));
    tint.setFillColor(sf::Color(0, 0, 0, 35));
    w.draw(tint);

    if (gFontLoaded) {
        // Main title shadow
        sf::Text titleShadow;
        titleShadow.setFont(fontBold);
        titleShadow.setString(title);
        titleShadow.setCharacterSize(54);
        titleShadow.setFillColor(sf::Color(0, 0, 0, 180));

        sf::FloatRect tb = titleShadow.getLocalBounds();
        titleShadow.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
        titleShadow.setPosition(WIN_W / 2.f + 3.f, WIN_H / 2.f - 35.f + 3.f);
        w.draw(titleShadow);

        // Main title
        sf::Text titleText;
        titleText.setFont(fontBold);
        titleText.setString(title);
        titleText.setCharacterSize(54);
        titleText.setFillColor(mainCol);

        tb = titleText.getLocalBounds();
        titleText.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
        titleText.setPosition(WIN_W / 2.f, WIN_H / 2.f - 35.f);
        w.draw(titleText);

        // Subtitle shadow
        sf::Text subShadow;
        subShadow.setFont(fontMedium);
        subShadow.setString(sub);
        subShadow.setCharacterSize(22);
        subShadow.setFillColor(sf::Color(0, 0, 0, 170));

        sf::FloatRect sb = subShadow.getLocalBounds();
        subShadow.setOrigin(sb.left + sb.width / 2.f, sb.top + sb.height / 2.f);
        subShadow.setPosition(WIN_W / 2.f + 2.f, WIN_H / 2.f + 25.f + 2.f);
        w.draw(subShadow);

        // Subtitle
        sf::Text subText;
        subText.setFont(fontMedium);
        subText.setString(sub);
        subText.setCharacterSize(22);
        subText.setFillColor(sf::Color(220, 230, 255, 230));

        sb = subText.getLocalBounds();
        subText.setOrigin(sb.left + sb.width / 2.f, sb.top + sb.height / 2.f);
        subText.setPosition(WIN_W / 2.f, WIN_H / 2.f + 25.f);
        w.draw(subText);
    }

    // Simple thin progress line, no box
    float lineW = 420.f;
    float lineH = 4.f;
    float lineX = (WIN_W - lineW) / 2.f;
    float lineY = WIN_H / 2.f + 70.f;

    sf::RectangleShape lineBg(sf::Vector2f(lineW, lineH));
    lineBg.setPosition(lineX, lineY);
    lineBg.setFillColor(sf::Color(255, 255, 255, 45));
    w.draw(lineBg);

    sf::RectangleShape lineFill(sf::Vector2f(lineW * progress, lineH));
    lineFill.setPosition(lineX, lineY);
    lineFill.setFillColor(sf::Color(mainCol.r, mainCol.g, mainCol.b, 220));
    w.draw(lineFill);
}
void clearActiveProjectiles(Bullet bullets[], EnemyBullet enemyBullets[]) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].setAlive(false);
    }

    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        enemyBullets[i].setAlive(false);
    }
}
void clearActivePowerups(PowerUp powerups[], int& powerupCount) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        powerups[i].setAlive(false);
    }

    powerupCount = 0;
}
void drawWin(sf::RenderWindow& w,
    sf::Font& fontBold,
    sf::Font& fontMedium,
    int score,
    sf::Texture& bgTex)
{
    w.setView(w.getDefaultView());
    w.clear();

    // background
    sf::Sprite bg;
    bg.setTexture(bgTex);
    bg.setPosition(0, 0);
    bg.setScale(
        (float)WIN_W / bgTex.getSize().x,
        (float)WIN_H / bgTex.getSize().y
    );
    w.draw(bg);

    // pulse
    float pulse = 0.7f + sinf(frameCount * 0.08f) * 0.3f;
    sf::Color titleCol(
        100,
        (sf::Uint8)(200 + pulse * 55),
        150
    );

    // TITLE (Bold)
    drawCenteredText(w, fontBold, "VICTORY!", 280, 64, titleCol);

    // subtitle (Medium)
    drawCenteredText(w, fontMedium,
        "MOTHERSHIP DESTROYED",
        350, 22,
        sf::Color(180, 255, 200)
    );

    // score (Medium)
    drawCenteredText(w, fontMedium,
        "FINAL SCORE: " + to_string(score),
        400, 28,
        sf::Color(255, 220, 80)
    );

    // blinking text
    if ((frameCount / 20) % 2 == 0) {
        drawCenteredText(w, fontMedium,
            "PRESS ENTER TO CONTINUE",
            520, 20,
            sf::Color(180, 200, 255)
        );
    }
}

//  ASTEROID SPAWN
void respawnAsteroid(Asteroid& a) {
    int size = 1 + rand() % 3;

    float spd = 0.025f + (rand() % 3) * 0.01f;

    a = Asteroid(
        rand() % WIDTH,     // random x
        -2 - rand() % 8,    // start above screen
        0,                  // no horizontal movement
        spd,                // slow downward speed
        1, 1,
        size
    );

    a.setAlive(true);
}

//  POWERUP SPAWN
void showPickupMessage(PowerUpType type, string& msg, int& timer, int& glowTimer) {
    if (type == PU_EMP)
        msg = "EMP COLLECTED!";
    else if (type == PU_SHIELD)
        msg = "SHIELD ACTIVATED!";
    else if (type == PU_SPREAD)
        msg = "SPREAD WEAPON!";
    else if (type == PU_PIERCING)
        msg = "PIERCING WEAPON!";

    timer = 90;
    glowTimer = 25;
}

void spawnPowerUp(int x, int y, PowerUp pu[], int& cnt) {
    if (cnt >= MAX_POWERUPS) return;

    // Separate 5% chance for EMP
    int empRoll = rand() % 100;
    if (empRoll < 5) {
        pu[cnt] = PowerUp(x, y, 0, 0.15f, 1, 1, PU_EMP);
        pu[cnt].setAlive(true);
        cnt++;
        return;
    }

    // Separate 8% chance for weapon/defense upgrade
    int upgradeRoll = rand() % 100;
    if (upgradeRoll < 8) {
        int typeRoll = rand() % 3;
        PowerUpType type;

        if (typeRoll == 0)
            type = PU_SPREAD;
        else if (typeRoll == 1)
            type = PU_PIERCING;
        else
            type = PU_SHIELD;

        pu[cnt] = PowerUp(x, y, 0, 0.15f, 1, 1, type);
        pu[cnt].setAlive(true);
        cnt++;
    }
}

// draw explosion particles
static void spawnExplosion(float gx, float gy, sf::Color col, int count = 18) {
    if (gSounds) gSounds->playExplosion();
    int sp = 0;
    for (int i = 0; i < MAX_PARTICLES && sp < count; i++) {
        if (!gParts[i].alive) {
            gParts[i].alive = true;
            gParts[i].x = cx(gx); gParts[i].y = cy(gy);
            float ang = (rand() % 360) * 3.14159f / 180.f;
            float spd = 1.5f + (rand() % 30) / 10.f;
            gParts[i].vx = cosf(ang) * spd; gParts[i].vy = sinf(ang) * spd;
            gParts[i].life = 25.f + rand() % 20;
            gParts[i].maxLife = gParts[i].life;
            gParts[i].col = col;
            sp++;
        }
    }
}

static void spawnBossExplosion(float gx, float gy, sf::Color mainCol, int intensity = 120) {
    // Big screen shake
    triggerShake(28.f);

    if (gSounds) {
        gSounds->playExplosion();
    }

    // Main outward explosion particles
    int spawned = 0;

    for (int i = 0; i < MAX_PARTICLES && spawned < intensity; i++) {
        if (!gParts[i].alive) {
            gParts[i].alive = true;

            // Start around boss center, with slight random spread
            gParts[i].x = cx(gx) + (rand() % 80 - 40);
            gParts[i].y = cy(gy) + (rand() % 50 - 25);

            float ang = (rand() % 360) * 3.14159f / 180.f;
            float spd = 2.0f + (rand() % 70) / 10.f; // 2.0 to 9.0

            gParts[i].vx = cosf(ang) * spd;
            gParts[i].vy = sinf(ang) * spd;

            gParts[i].life = 45.f + rand() % 45;
            gParts[i].maxLife = gParts[i].life;

            // Mix orange/white/main boss colour
            int c = rand() % 4;

            if (c == 0)
                gParts[i].col = sf::Color(255, 220, 120);
            else if (c == 1)
                gParts[i].col = sf::Color(255, 90, 20);
            else if (c == 2)
                gParts[i].col = sf::Color(255, 255, 255);
            else
                gParts[i].col = mainCol;

            spawned++;
        }
    }

    // Secondary slower smoke/debris burst
    int smokeSpawned = 0;

    for (int i = 0; i < MAX_PARTICLES && smokeSpawned < intensity / 2; i++) {
        if (!gParts[i].alive) {
            gParts[i].alive = true;

            gParts[i].x = cx(gx) + (rand() % 120 - 60);
            gParts[i].y = cy(gy) + (rand() % 80 - 40);

            float ang = (rand() % 360) * 3.14159f / 180.f;
            float spd = 0.8f + (rand() % 30) / 10.f;

            gParts[i].vx = cosf(ang) * spd;
            gParts[i].vy = sinf(ang) * spd + 0.8f;

            gParts[i].life = 70.f + rand() % 50;
            gParts[i].maxLife = gParts[i].life;

            gParts[i].col = sf::Color(90, 80, 75);

            smokeSpawned++;
        }
    }

    // Extra smaller chain explosions around the boss
    for (int b = 0; b < 8; b++) {
        float ox = gx + ((rand() % 7) - 3);
        float oy = gy + ((rand() % 5) - 2);

        spawnExplosionSilent(
            ox,
            oy,
            sf::Color(255, 120 + rand() % 80, 30),
            25
        );
    }
}


static bool circleCollide(float x1, float y1, float r1, float x2, float y2, float r2);

//  COLLISIONS

void handleCollisions(Bullet b[], int bc, Drone d[], int dc, Viper v[], int vc,
    Asteroid a[], int ac, int& score, int& mult, int& kt, PowerUp pu[], int& pc)
{
    for (int i = 0; i < bc; i++) {
        if (!b[i].Isalive()) continue;

        // bullet vs drones
        for (int j = 0; j < dc; j++) {
            if (!d[j].Isalive()) continue;
            if (circleCollide(cx(b[i].getx()), cy(b[i].gety()), CW * 0.2f,
                d[j].getPixelX(), d[j].getPixelY(), d[j].getRadius())) {
                b[i].onCollision(&d[j]);
                if (!d[j].Isalive()) {
                    spawnExplosion(d[j].getx(), d[j].gety(), sf::Color(80, 160, 255));
                    score += d[j].getScore() * mult;
                    mult = (mult < 4) ? mult * 2 : 4;
                    kt = 37;
                    spawnPowerUp((int)d[j].getx(), (int)d[j].gety(), pu, pc);
                }
                break;
            }
        }

        if (!b[i].Isalive()) continue;

        // bullet vs vipers
        for (int j = 0; j < vc; j++) {
            if (!v[j].Isalive()) continue;
            if (circleCollide(cx(b[i].getx()), cy(b[i].gety()), CW * 0.2f,
                v[j].getPixelX(), v[j].getPixelY(), v[j].getRadius())) {
                b[i].onCollision(&v[j]);
                if (!v[j].Isalive()) {
                    spawnExplosion(v[j].getx(), v[j].gety(), sf::Color(255, 80, 80));
                    score += v[j].getScore() * mult;
                    mult = (mult < 4) ? mult * 2 : 4;
                    kt = 37;
                    spawnPowerUp((int)v[j].getx(), (int)v[j].gety(), pu, pc);
                }
                break;
            }
        }

        if (!b[i].Isalive()) continue;

        // bullet vs asteroids - asteroid is indestructible, only bullet dies
        for (int j = 0; j < ac; j++) {
            if (!a[j].Isalive()) continue;
            if (circleCollide(cx(b[i].getx()), cy(b[i].gety()), CW * 0.2f,
                a[j].getPixelX(), a[j].getPixelY(), a[j].getPixelRadius())) {
                b[i].setAlive(false);
                break;
            }
        }
    }
}
void handleEBulletAsteroid(EnemyBullet eb[], int ebc, Asteroid a[], int ac) {
    for (int i = 0; i < ebc; i++) {
        if (!eb[i].Isalive()) continue;
        for (int j = 0; j < ac; j++) {
            if (!a[j].Isalive()) continue;
            if (circleCollide(cx(eb[i].getx()), cy(eb[i].gety()), CW * 0.28f,
                a[j].getPixelX(), a[j].getPixelY(), a[j].getPixelRadius())) {
                eb[i].setAlive(false); break;
            }
        }
    }
}

static bool circleCollide(float x1, float y1, float r1,
    float x2, float y2, float r2) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    return sqrtf(dx * dx + dy * dy) < (r1 + r2);
}

void handleEBulletPlayer(EnemyBullet eb[], int ebc, Player& p, int& mult, int& kt) {
    for (int i = 0; i < ebc; i++) {
        if (!eb[i].Isalive()) continue;
        if (circleCollide(p.getPixelX(), p.getPixelY(), p.getRadius(),
            cx(eb[i].getx()), cy(eb[i].gety()), CW * 0.28f)) {
            eb[i].setAlive(false);
            p.takeDamage();
            mult = 1; kt = 0;

        }
    }
}

void handlePlayerPowerUp(Player& p, PowerUp pu[], int cnt,
    string& pickupMsg, int& pickupMsgTimer, int& pickupGlowTimer) {
    for (int i = 0; i < cnt; i++) {
        if (!pu[i].Isalive()) continue;
        if (circleCollide(p.getPixelX(), p.getPixelY(), p.getRadius(),
            cx(pu[i].getx()), cy(pu[i].gety()), CW * 0.6f)) {
            showPickupMessage(pu[i].getPowerType(), pickupMsg, pickupMsgTimer, pickupGlowTimer);
            p.onCollision(&pu[i]);
            pu[i].setAlive(false);
            if (gSounds) gSounds->playPowerup();
        }
    }
}
void handlePlayerEnemyContact(Player& p, Drone d[], int dc,
    Viper v[], int vc, int& mult, int& kt) {
    for (int i = 0; i < dc; i++) {
        if (!d[i].Isalive()) continue;
        if (circleCollide(p.getPixelX(), p.getPixelY(), p.getRadius(),
            d[i].getPixelX(), d[i].getPixelY(), d[i].getRadius())) {
            p.takeDamage();
            mult = 1; kt = 0;

            return;
        }
    }
    for (int i = 0; i < vc; i++) {
        if (!v[i].Isalive()) continue;
        if (circleCollide(p.getPixelX(), p.getPixelY(), p.getRadius(),
            v[i].getPixelX(), v[i].getPixelY(), v[i].getRadius())) {
            p.takeDamage();
            mult = 1; kt = 0;

            return;
        }
    }
}

void handleSeekerPlayer(Player& p, Seeker sk[], int n, int& mult, int& kt) {
    for (int i = 0; i < n; i++) {
        if (!sk[i].Isalive()) continue;

        if (circleCollide(p.getPixelX(), p.getPixelY(), p.getRadius(),
            sk[i].getPixelX(), sk[i].getPixelY(), sk[i].getRadius())) {

            spawnExplosion(sk[i].getx(), sk[i].gety(), sf::Color(255, 150, 0));
            p.takeDamage();
            sk[i].setAlive(false);
            mult = 1;
            kt = 0;
        }
    }
}
void handlePlayerAsteroid(Player& p, Asteroid a[], int n, int& mult, int& kt) {
    for (int i = 0; i < n; i++) {
        if (!a[i].Isalive()) continue;
        if (circleCollide(p.getPixelX(), p.getPixelY(), p.getRadius(),
            a[i].getPixelX(), a[i].getPixelY(), a[i].getPixelRadius())) {
            p.takeDamage();
            mult = 1; kt = 0;

        }
    }
}

void handlePlayerBossContact(Player& p, Boss* boss, int& mult, int& kt) {
    if (!boss || !boss->Isalive()) return;

    float pr = p.getRadius();
    float pLeft = p.getPixelX() - pr;
    float pRight = p.getPixelX() + pr;
    float pTop = p.getPixelY() - pr;
    float pBottom = p.getPixelY() + pr;

    float bLeft, bRight, bTop, bBot;

    if (boss->getType() == TWINCANNONS) {
        // Wider/lower area because TwinCannons turrets are drawn far from body
        bLeft = cx(boss->getx()) - CW * 5.0f;
        bRight = cx(boss->getx()) + CW * 5.0f;
        bTop = cy(boss->gety()) - CH * 1.5f;
        bBot = cy(boss->gety()) + CH * 4.0f;
    }
    else if (boss->getType() == MOTHERSHIP) {
        // Mothership is visually huge
        bLeft = cx(boss->getx()) - CW * 4.0f;
        bRight = cx(boss->getx()) + CW * 4.0f;
        bTop = cy(boss->gety()) - CH * 1.8f;
        bBot = cy(boss->gety()) + CH * 2.2f;
    }
    else {
        // Cruiser/general boss
        bLeft = cx(boss->getx()) - CW * boss->getwidth();
        bRight = cx(boss->getx()) + CW * boss->getwidth();
        bTop = cy(boss->gety()) - CH * 1.5f;
        bBot = cy(boss->gety()) + CH * 3.0f;
    }

    bool overlap =
        pRight >= bLeft &&
        pLeft <= bRight &&
        pBottom >= bTop &&
        pTop <= bBot;

    if (overlap) {
        p.takeDamage();
        mult = 1;
        kt = 0;
    }
}

void handleBossCollision(Bullet b[], int bc, Boss* boss) {
    if (!boss || !boss->Isalive()) return;

    for (int i = 0; i < bc; i++) {
        if (!b[i].Isalive()) continue;

        float bx = b[i].getx();
        float by = b[i].gety();

        // Special larger hit area for TwinCannons because turrets are wider/lower
        if (boss->getType() == TWINCANNONS) {
            float left = boss->getx() - 5.0f;
            float right = boss->getx() + 5.0f;
            float top = boss->gety() - 1.0f;
            float bottom = boss->gety() + 4.0f;

            if (bx >= left && bx <= right && by >= top && by <= bottom) {
                boss->onCollision(&b[i]);
            }
        }
        else if (boss->getType() == MOTHERSHIP) {
            float bx = b[i].getx();
            float by = b[i].gety();

            // Mothership x,y is the centre of the PNG
            float left = boss->getx() - 3.5f;
            float right = boss->getx() + 3.5f;
            float top = boss->gety() - 1.2f;
            float bottom = boss->gety() + 2.2f;

            if (bx >= left && bx <= right &&
                by >= top && by <= bottom) {
                boss->onCollision(&b[i]);
            }
        }
        else if (boss->getType() == CRUISER) {
            // Cruiser x,y is also the centre of the PNG
            float left = boss->getx() - 3.2f;
            float right = boss->getx() + 3.2f;
            float top = boss->gety() - 0.8f;
            float bottom = boss->gety() + 2.2f;

            if (bx >= left && bx <= right &&
                by >= top && by <= bottom) {
                boss->onCollision(&b[i]);
            }
        }

        else {
            // Normal boss body hitbox
            if (bx >= boss->getx() && bx <= boss->getx() + boss->getwidth() &&
                by >= boss->gety() && by <= boss->gety() + boss->getheight()) {
                boss->onCollision(&b[i]);
            }
        }
    }
}

//  SHOOTING & EMP
void shootWithWeapon(Player& p, Bullet b[], int n) {
    if (!p.canShoot()) return;
    p.resetFireTimer();
    if (gSounds) gSounds->playShoot();              
    WeaponType wp = p.getWeapon();
    if (wp == W_SPREAD) {
        int sc = 0;
        for (int i = 0; i < n && sc < 3; i++) {
            if (!b[i].Isalive()) {
                b[i].setPiercing(false);
                if (sc == 0) b[i].shoot(p.getx(), p.gety());
                else if (sc == 1) b[i].shootSpread(p.getx(), p.gety(), -1);
                else              b[i].shootSpread(p.getx(), p.gety(), 1);
                sc++;
            }
        }
    }
    else if (wp == W_PIERCING) {
        for (int i = 0; i < n; i++) {
            if (!b[i].Isalive()) {
                b[i].setPiercing(true);
                b[i].shoot(p.getx(), p.gety());
                break;
            }
        }
    }
    else {
        for (int i = 0; i < n; i++) if (!b[i].Isalive()) { b[i].setPiercing(false); b[i].shoot(p.getx(), p.gety()); break; }
    }
}


void triggerEMP(Drone d[], int dc, Viper v[], int vc, Seeker sk[], int sc,
    EnemyBullet eb[], int ebc,
    Boss* activeBoss)
{
    for (int i = 0; i < dc; i++)
        if (d[i].Isalive()) {
            spawnExplosion(d[i].getx(), d[i].gety(), sf::Color(200, 0, 255));
            d[i].setAlive(false);
        }

    for (int i = 0; i < vc; i++)
        if (v[i].Isalive()) {
            spawnExplosion(v[i].getx(), v[i].gety(), sf::Color(200, 0, 255));
            v[i].setAlive(false);
        }

    for (int i = 0; i < sc; i++)
        if (sk[i].Isalive()) {
            spawnExplosion(sk[i].getx(), sk[i].gety(), sf::Color(200, 0, 255));
            sk[i].setAlive(false);
        }

    for (int i = 0; i < ebc; i++)
        eb[i].setAlive(false);

    if (activeBoss && activeBoss->Isalive())
        activeBoss->takeDamage(15);
}


//  LEVEL SETUP
float getTopDroneY(Drone d[], int n) {
    float m = (float)HEIGHT;
    for (int i = 0; i < n; i++) if (d[i].Isalive() && d[i].gety() < m) m = d[i].gety();
    return m;
}

void setupLevel(int level, Drone d[], Viper v[], Seeker sk[]) {
    for (int i = 0; i < MAX_DRONES; i++)  d[i].setAlive(false);
    for (int i = 0; i < MAX_VIPERS; i++)  v[i].setAlive(false);
    for (int i = 0; i < MAX_SEEKERS; i++) sk[i].setAlive(false);

    const int dpr = 12, sp = 3;
    int startXD = (WIDTH - (dpr - 1) * sp) / 2;
    const int vpr = 6, spv = 6;
    int startXV = (WIDTH - (vpr - 1) * spv) / 2;

    float droneSpeed = 0.02f + (level - 1) * 0.005f;
    float droneFireCD;
    float viperFireCD;

    if (level == 1) {
        droneFireCD = 65.f;
        viperFireCD = 85.f;
    }
    else if (level == 2) {
        droneFireCD = 55.f;
        viperFireCD = 75.f;
    }
    else {
        droneFireCD = 45.f;
        viperFireCD = 72.f;
    }

    int numDR = 1 + level, numVR = 1 + level, dIdx = 0;
    for (int r = 0; r < numDR && dIdx < MAX_DRONES; r++)
        for (int c = 0; c < dpr && dIdx < MAX_DRONES; c++) {
            d[dIdx].spawn((float)(startXD + c * sp), 4.f + r * 1.f, droneSpeed);
            d[dIdx].setFireCooldown(droneFireCD);
            dIdx++;
        }

    int vIdx = 0;
    for (int r = 0; r < numVR && vIdx < MAX_VIPERS; r++) {
        float offset = -(float)(numVR - r);
        for (int c = 0; c < vpr && vIdx < MAX_VIPERS; c++) {
            v[vIdx].setPosition((float)(startXV + c * spv), 1.f + r * 1.f);
            v[vIdx].setRowOffset(offset);
            v[vIdx].setFireCooldown(viperFireCD); 
            v[vIdx].setAlive(true);
            vIdx++;
        }
    }
    for (int i = 0; i < 6; i++) sk[i].reset((float)(startXV + i * spv), 0.f);
}

void setupSurvivalLevel(int wave, Drone d[], Viper v[], Seeker sk[]) {
    // Clear old wave enemies
    for (int i = 0; i < MAX_DRONES; i++)  d[i].setAlive(false);
    for (int i = 0; i < MAX_VIPERS; i++)  v[i].setAlive(false);
    for (int i = 0; i < MAX_SEEKERS; i++) sk[i].setAlive(false);

    // Requirement: total enemies increase by 2 each wave
    int totalEnemies = 12 + (wave - 1) * 2;

    // Clamp to available array space
    int maxTotal = MAX_DRONES + MAX_VIPERS + MAX_SEEKERS;
    if (totalEnemies > maxTotal) totalEnemies = maxTotal;

    // Requirement: speed +5% each wave
    float spd = 0.02f * (float)pow(1.05, wave - 1);

    // Requirement: fire rate +10% each wave
    // Higher fire rate means lower cooldown
    float fireCD = 35.f / (float)pow(1.10, wave - 1);
    if (fireCD < 8.f) fireCD = 8.f;

    int numVipers = 0;
    int numSeekers = 0;
    int numDrones = totalEnemies;

    // Vipers from wave 5 onward
    if (wave >= 5) {
        numVipers = min(2 + (wave - 5), MAX_VIPERS);
    }

    // Seekers from wave 8 onward
    if (wave >= 8) {
        numSeekers = min(1 + (wave - 8), MAX_SEEKERS);
    }

    // Make sure total enemies still follows +2 rule
    if (numVipers + numSeekers > totalEnemies) {
        numVipers = 0;
        numSeekers = 0;
    }

    numDrones = totalEnemies - numVipers - numSeekers;
    if (numDrones > MAX_DRONES) numDrones = MAX_DRONES;

    // Spawn drones
    const int dpr = 12;
    const int sp = 3;
    int startXD = (WIDTH - (dpr - 1) * sp) / 2;

    int dIdx = 0;
    int rows = (numDrones + dpr - 1) / dpr;

    for (int r = 0; r < rows && dIdx < numDrones; r++) {
        for (int c = 0; c < dpr && dIdx < numDrones; c++) {
            d[dIdx].spawn((float)(startXD + c * sp), 4.f + r, spd);
            d[dIdx].setFireCooldown(fireCD);
            dIdx++;
        }
    }

    // Spawn vipers
    if (wave >= 5) {
        const int vpr = 6;
        const int spv = 6;
        int startXV = (WIDTH - (vpr - 1) * spv) / 2;
        int totalViperRows = (numVipers + vpr - 1) / vpr;

        for (int i = 0; i < numVipers; i++) {
            int row = i / vpr;
            int col = i % vpr;

            v[i].setPosition((float)(startXV + col * spv), 2.f + row);
            float offset = -(float)(totalViperRows - row);
            v[i].setRowOffset(offset);
            v[i].setFireCooldown(fireCD);
            v[i].setAlive(true);
        }
    }

    // Spawn seekers
    if (wave >= 8) {
        for (int i = 0; i < numSeekers; i++) {
            sk[i].reset((float)(5 + i * 5), 0.f);
            sk[i].setAlive(true);
        }
    }
}
void resetGame(Player& p, Drone d[], Viper v[], Seeker sk[],
    Bullet b[], EnemyBullet eb[], Asteroid a[], PowerUp pu[],
    int& score, int& level, bool& bossPhase,
    Cruiser& cr, TwinCannons& tc, Mothership& ms,
    float& syncY, int& fcount, int& puCnt, sf::Texture& cruiserTex,
    sf::Texture& twinBodyTex,
    sf::Texture& twinLeftTex,
    sf::Texture& twinRightTex,
    sf::Texture& mothershipTex,
    sf::Texture& playerTex)
{
    p = Player();
    p.setTexture(playerTex);
    for (int i = 0; i < MAX_DRONES; i++)  d[i].setAlive(false);
    for (int i = 0; i < MAX_VIPERS; i++)  v[i].setAlive(false);
    for (int i = 0; i < MAX_SEEKERS; i++) sk[i].setAlive(false);
    for (int i = 0; i < MAX_BULLETS; i++) { b[i].setAlive(false); eb[i].setAlive(false); }
    for (int i = 0; i < MAX_ASTEROIDS; i++) a[i].setAlive(false);
    for (int i = 0; i < MAX_POWERUPS; i++)  pu[i].setAlive(false);

    // Asteroids: speed intentionally kept low for 60fps visual parity
    int asteroidCount = 2 + rand() % 2;
    for (int i = 0; i < asteroidCount; i++) {
        int size = 1 + rand() % 5;

        float spd = 0.025f + (rand() % 3) * 0.01f;

        a[i] = Asteroid(
            rand() % WIDTH,
            rand() % (HEIGHT / 2),
            0,
            spd,
            1, 1,
            size
        );

        a[i].setAlive(true);
    }

    score = 0; level = 1; bossPhase = false; syncY = 4.f; fcount = 0; puCnt = 0;
    cr = Cruiser(WIDTH / 2, 4);
    tc = TwinCannons(WIDTH / 2, 4);
    ms = Mothership(WIDTH / 2, 4);

    cr.setTexture(cruiserTex);
    tc.setTextures(twinBodyTex, twinLeftTex, twinRightTex);
    ms.setTexture(mothershipTex);
    for (int i = 0; i < MAX_PARTICLES; i++) gParts[i].alive = false;
}


void loadAssets(
    sf::Music& bgMusic,
    sf::Font& fontBold,
    sf::Font& fontMedium,
    sf::Texture& menuBgTex,
    sf::Texture& playerTex,
    sf::Texture& droneTex,
    sf::Texture& viperTex,
    sf::Texture& seekerTex,
    sf::Texture& cruiserTex,
    sf::Texture& twinBodyTex,
    sf::Texture& twinLeftTurretTex,
    sf::Texture& twinRightTurretTex,
    sf::Texture& mothershipTex,
    sf::Texture& bgTexture,
    sf::Sprite& background,
    sf::RectangleShape& bgOverlay,
    sf::Texture& gameOverBgTex,
    sf::Texture& mothershipLaserTex,
    Mothership& mothership
) {
    if (!bgMusic.openFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/bgsong.mpeg")) {
        cout << "Background music failed\n";
    }

    bgMusic.setLoop(true);
    bgMusic.setVolume(4);
    bgMusic.play();

    for (int i = 0; i < MAX_PRETTY_STARS; i++) {
        prettyStars[i].x = (float)(rand() % WIN_W);
        prettyStars[i].y = (float)(rand() % WIN_H);

        int layer = rand() % 3;

        if (layer == 0) {
            prettyStars[i].size = 0.8f;
            prettyStars[i].speed = 0.12f;
        }
        else if (layer == 1) {
            prettyStars[i].size = 1.3f;
            prettyStars[i].speed = 0.28f;
        }
        else {
            prettyStars[i].size = 2.0f;
            prettyStars[i].speed = 0.55f;
        }

        prettyStars[i].phase = (float)(rand() % 100);
    }

    if (!fontBold.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/Orbitron-Bold.ttf"))
        cout << "Bold font failed\n";

    if (!fontMedium.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/Orbitron-Medium.ttf"))
        cout << "Medium font failed\n";

    if (!menuBgTex.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/frontscreen.png"))
        cout << "Menu background failed to load\n";

    if (!playerTex.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/player.png"))
        cout << "Player texture failed to load\n";

    if (!droneTex.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/drone.png"))
        cout << "Drone texture failed\n";

    if (!viperTex.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/viper.png"))
        cout << "Viper texture failed\n";

    if (!seekerTex.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/seeker.png"))
        cout << "Seeker texture failed\n";

    if (!cruiserTex.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/Cruiser.png"))
        cout << "Cruiser texture failed\n";

    if (!twinBodyTex.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/Twincannon.PNG"))
        cout << "Twin body texture failed\n";

    if (!twinLeftTurretTex.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/turret_left.PNG"))
        cout << "Left turret texture failed\n";

    if (!twinRightTurretTex.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/turret_right.PNG"))
        cout << "Right turret texture failed\n";

    if (!mothershipTex.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/mothership.PNG"))
        cout << "Mothership texture failed\n";
    if (!mothershipLaserTex.loadFromFile("C:\\Users\\noor fatima\\Documents\\SMFLREAL\\x64\\Debug\\mothershiplaserbeam.png"))
    {
        cout << "Mothership Laser texture failed\n";
    }

    if (!bgTexture.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/space_bg.jpeg")) {
        cout << "Background image failed\n";
    }

    mothership.setLaserTexture(mothershipLaserTex);
    mothership.setTexture(mothershipTex);
    background.setTexture(bgTexture);
    background.setScale(
        (float)WIN_W / bgTexture.getSize().x,
        (float)WIN_H / bgTexture.getSize().y
    );
    background.setColor(sf::Color(160, 160, 160, 180));

    bgOverlay.setSize(sf::Vector2f((float)WIN_W, (float)WIN_H));
    bgOverlay.setFillColor(sf::Color(0, 0, 25, 120));

    if (gFont.loadFromFile("arial.ttf"))
        gFontLoaded = true;
    else if (gFont.loadFromFile("C:/Windows/Fonts/arial.ttf"))
        gFontLoaded = true;
    else
        cout << "HUD font failed\n";

    if (!gameOverBgTex.loadFromFile("C:/Users/noor fatima/Documents/SMFLREAL/x64/Debug/gameoverscreen.png")) {
        cout << "Game over background failed to load\n";
    }
}
void handleEvents(
    sf::RenderWindow& window,
    GameState& state,
    GameState& prevPlayState,
    SoundManager& sounds,

    Player& player,
    Drone drones[],
    Viper vipers[],
    Seeker seekers[],
    Bullet bullets[],
    EnemyBullet enemyBullets[],
    Asteroid asteroids[],
    PowerUp powerups[],

    int& score,
    int& currentLevel,
    bool& bossPhase,

    Cruiser& cruiser,
    TwinCannons& twinCannons,
    Mothership& mothership,

    float& waveSyncY,
    int& powerupCount,

    sf::Texture& cruiserTex,
    sf::Texture& twinBodyTex,
    sf::Texture& twinLeftTurretTex,
    sf::Texture& twinRightTurretTex,
    sf::Texture& mothershipTex,
    sf::Texture& playerTex,

    float& empFlash
) {
    sf::Event evt;

    while (window.pollEvent(evt)) {
        if (evt.type == sf::Event::Closed) {
            window.close();
        }

        if (evt.type == sf::Event::KeyPressed) {

            // ── MENU ──
            if (state == MENU) {
                int optionCount = hasSavedGame ? 4 : 3;

                if (evt.key.code == sf::Keyboard::Up) {
                    menuIndex--;
                    if (menuIndex < 0) menuIndex = optionCount - 1;
                    sounds.playMenu();
                }
                else if (evt.key.code == sf::Keyboard::Down) {
                    menuIndex++;
                    if (menuIndex >= optionCount) menuIndex = 0;
                    sounds.playMenu();
                }
                else if (evt.key.code == sf::Keyboard::Return) {
                    sounds.playMenu();

                    // Offset indices by 1 if resume option exists
                    int offset = hasSavedGame ? 1 : 0;

                    if (hasSavedGame && menuIndex == 0) {
                        // RESUME — restore the saved play state, nothing else changes
                        state = savedPlayState;
                        prevPlayState = savedPlayState;
                    }
                    else if (menuIndex == 0 + offset) {
                        // START GAME
                        hasSavedGame = false; // clear any old save
                        state = MODE_SELECT;
                        modeIndex = 0;
                    }
                    else if (menuIndex == 1 + offset) {
                        state = CONTROLS;
                    }
                    else if (menuIndex == 2 + offset) {
                        window.close();
                    }
                }
            }


            // ── MODE SELECT ──
            else if (state == MODE_SELECT) {
                if (evt.key.code == sf::Keyboard::Up) {
                    modeIndex--;
                    if (modeIndex < 0) modeIndex = 2;
                    sounds.playMenu();
                }
                else if (evt.key.code == sf::Keyboard::Down) {
                    modeIndex++;
                    if (modeIndex > 2) modeIndex = 0;
                    sounds.playMenu();
                }
                else if (evt.key.code == sf::Keyboard::Return) {
                    sounds.playMenu();

                    if (modeIndex == 0) {
                        hasSavedGame = false;
                        resetGame(player, drones, vipers, seekers, bullets, enemyBullets, asteroids, powerups,
                            score, currentLevel, bossPhase,
                            cruiser, twinCannons, mothership,
                            waveSyncY, frameCount, powerupCount,
                            cruiserTex, twinBodyTex, twinLeftTurretTex, twinRightTurretTex, mothershipTex,
                            playerTex);

                        setupLevel(1, drones, vipers, seekers);
                        state = ARCADE;
                        prevPlayState = ARCADE;
                    }
                    else if (modeIndex == 1) {
                        hasSavedGame = false;
                        resetGame(player, drones, vipers, seekers, bullets, enemyBullets, asteroids, powerups,
                            score, currentLevel, bossPhase,
                            cruiser, twinCannons, mothership,
                            waveSyncY, frameCount, powerupCount,
                            cruiserTex, twinBodyTex, twinLeftTurretTex, twinRightTurretTex, mothershipTex,
                            playerTex);

                        setupSurvivalLevel(1, drones, vipers, seekers);
                        state = SURVIVAL;
                        prevPlayState = SURVIVAL;
                    }
                    else if (modeIndex == 2) {
                        state = MENU;
                        menuIndex = 0;
                    }
                }
                else if (evt.key.code == sf::Keyboard::Escape) {
                    sounds.playMenu();
                    state = MENU;
                    menuIndex = 0;
                }
            }

            // ── CONTROLS ──
            else if (state == CONTROLS) {
                sounds.playMenu();
                state = MENU;
            }

            // ── PAUSED ──
            else if (state == PAUSED) {
                if (evt.key.code == sf::Keyboard::R) {
                    sounds.playMenu();
                    state = prevPlayState;   // resume in-place (unchanged)
                }
                else if (evt.key.code == sf::Keyboard::M) {
                    sounds.playMenu();
                    hasSavedGame = true;          // mark as resumable
                    savedPlayState = prevPlayState; // remember ARCADE or SURVIVAL
                    menuIndex = 0;                // reset cursor to RESUME
                    state = MENU;
      
                }
            }

            // ── GAME OVER / WIN ──
            else if (state == GAMEOVER || state == WIN) {
                if (evt.key.code == sf::Keyboard::Return) {
                    sounds.playMenu();
                    state = MENU;
                }
            }
            // ── IN-GAME ──
            else if (state == ARCADE || state == SURVIVAL) {
                if (evt.key.code == sf::Keyboard::Escape) {
                    sounds.playMenu();
                    prevPlayState = state;
                    state = PAUSED;
                }

                if (evt.key.code == sf::Keyboard::N && player.hasEMP()) {
                    player.activateEMP();

                    Boss* activeBoss = nullptr;

                    if (bossPhase) {
                        if (currentLevel == 1) {
                            activeBoss = &cruiser;
                        }
                        else if (currentLevel == 2) {
                            activeBoss = &twinCannons;
                        }
                        else if (currentLevel == 3) {
                            activeBoss = &mothership;
                        }
                    }

                    triggerEMP(drones, MAX_DRONES, vipers, MAX_VIPERS, seekers, MAX_SEEKERS,
                        enemyBullets, MAX_ENEMY_BULLETS, activeBoss);

                    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
                        enemyBullets[i].setAlive(false);
                    }

                    for (int i = 0; i < MAX_DRONES; i++) {
                        drones[i].setAlive(false);
                    }

                    for (int i = 0; i < MAX_VIPERS; i++) {
                        vipers[i].setAlive(false);
                    }

                    for (int i = 0; i < MAX_SEEKERS; i++) {
                        seekers[i].setAlive(false);
                    }

                    spawnExplosion(player.getx(), player.gety(), sf::Color(200, 0, 255), 60);
                    triggerShake(15);
                    empFlash = 15.f;
                }
            }
        }
    }
}
void updateGame(
    GameState& state,
    GameState& prevPlayState,

    Player& player,
    Bullet bullets[],
    EnemyBullet enemyBullets[],
    PowerUp powerups[],
    Asteroid asteroids[],

    Drone drones[],
    Viper vipers[],
    Seeker seekers[],

    Cruiser& cruiser,
    TwinCannons& twinCannons,
    Mothership& mothership,

    int& score,
    int& multiplier,
    int& killTimer,
    int& currentLevel,
    bool& bossPhase,
    float& waveSyncY,
    int& powerupCount,
    int& asteroidCount,

    string& pickupMsg,
    int& pickupMsgTimer,
    int& pickupGlowTimer,

    float& empFlash
) {
    // ── LEVEL TRANSITION COUNTDOWN ──
    if (state == LEVEL_TRANSITION) {
        clearActiveProjectiles(bullets, enemyBullets);
        if (gShakeTimer > 0) gShakeTimer--;
        updateParticles();
        transitionTimer--;

        if (transitionTimer <= 0) {
            if (transitionIsBoss) {
                bossPhase = true;

                if (currentLevel == 3) {
                    player.resetPosition(mothership.getx(), mothership.getDirX());
                }
                else {
                    player.resetPosition();
                }

                state = prevPlayState;
            }
            else if (transitionIsSurvival) {
                setupSurvivalLevel(transitionNextLevel, drones, vipers, seekers);
                waveSyncY = 4.f;
                player.resetPosition();
                state = SURVIVAL;
            }
            else {
                setupLevel(transitionNextLevel, drones, vipers, seekers);
                waveSyncY = 4.f;
                player.resetPosition();
                state = ARCADE;
            }
        }

        return;
    }

    if (!(state == ARCADE || state == SURVIVAL)) {
        return;
    }

    bool left =
        sf::Keyboard::isKeyPressed(sf::Keyboard::Left) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::A);

    bool right =
        sf::Keyboard::isKeyPressed(sf::Keyboard::Right) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::D);

    bool up =
        sf::Keyboard::isKeyPressed(sf::Keyboard::Up) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::W);

    bool down =
        sf::Keyboard::isKeyPressed(sf::Keyboard::Down) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::S);

    bool dash = sf::Keyboard::isKeyPressed(sf::Keyboard::E);

    if (left) {
        if (dash) {
            player.dash(-1);
        }
        else {
            player.moveLeft();
        }
    }

    if (right) {
        if (dash) {
            player.dash(1);
        }
        else {
            player.moveRight();
        }
    }
    if (up) {
        player.moveUp();
    }

    if (down) {
        player.moveDown();
    }


    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
        shootWithWeapon(player, bullets, MAX_BULLETS);
    }

    player.update();

    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].update();
    }

    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        enemyBullets[i].update();
    }

    for (int i = 0; i < asteroidCount; i++) {
        if (asteroids[i].Isalive()) {
            asteroids[i].update();
        }
        else {
            if (asteroidRespawnTimer[i] > 0) {
                asteroidRespawnTimer[i]--;
            }
            else {
                respawnAsteroid(asteroids[i]);
                asteroidRespawnTimer[i] = 300 + rand() % 300;
            }
        }
    }
  
    for (int i = 0; i < powerupCount; i++) {
        powerups[i].update();
    }

    Viper::globalAngle += 0.08f;

    float topY = getTopDroneY(drones, MAX_DRONES);
    waveSyncY = (topY < HEIGHT) ? topY : (waveSyncY + 0.02f + (currentLevel - 1) * 0.005f);

    for (int i = 0; i < MAX_VIPERS; i++) {
        if (vipers[i].Isalive()) {
            vipers[i].setSyncY(waveSyncY);
        }
    }

    for (int i = 0; i < MAX_SEEKERS; i++) {
        if (seekers[i].Isalive()) {
            if (abs((int)seekers[i].getx() - (int)player.getx()) <= 2) {
                seekers[i].lockPlayer(player.getx());
            }

            seekers[i].update();
        }
    }

    for (int i = 0; i < MAX_PRETTY_STARS; i++) {
        prettyStars[i].y += prettyStars[i].speed;

        if (prettyStars[i].y > WIN_H) {
            prettyStars[i].y = 0;
            prettyStars[i].x = (float)(rand() % WIN_W);
        }
    }

    if (killTimer > 0) {
        killTimer--;
    }
    else {
        multiplier = 1;
    }

    if (gShakeTimer > 0) {
        gShakeTimer--;
    }

    if (empFlash > 0) {
        empFlash--;
    }

    if (playerHitFlash > 0) {
        playerHitFlash--;
    }

    if (shieldBreakFlash > 0) {
        shieldBreakFlash--;
    }

    updateParticles();

    int droneFireChance;

    if (currentLevel == 1)
        droneFireChance = 600;
    else if (currentLevel == 2)
        droneFireChance = 500;
    else
        droneFireChance = 400;

    for (int i = 0; i < MAX_DRONES; i++) {
        drones[i].update();

        if (drones[i].Isalive() && drones[i].canshoot() && rand() % droneFireChance == 0) {
            drones[i].shoot(enemyBullets, MAX_ENEMY_BULLETS);
            drones[i].resetFireTimer();
        }
    }

    int viperFireChance;

    if (currentLevel == 1)
        viperFireChance = 700;
    else if (currentLevel == 2)
        viperFireChance = 600;
    else
        viperFireChance = 500;

    for (int i = 0; i < MAX_VIPERS; i++) {
        if (vipers[i].Isalive()) {
            vipers[i].update();

            if (vipers[i].canshoot() && rand() % viperFireChance == 0) {
                vipers[i].shoot(enemyBullets, MAX_ENEMY_BULLETS);
                vipers[i].resetFireTimer();
            }
        }
    }

    if (state == ARCADE) {
        if (bossPhase) {
            if (currentLevel == 1) {
                cruiser.update();
                cruiser.specialAttack(player);
                cruiser.shoot(enemyBullets, MAX_ENEMY_BULLETS);

                handleBossCollision(bullets, MAX_BULLETS, &cruiser);

                if (!cruiser.Isalive()) {
                    spawnBossExplosion(cruiser.getx(), cruiser.gety(), sf::Color(80, 140, 255), 130);
                    score += 500 * multiplier;
                    bossPhase = false;
                    currentLevel = 2;
                    transitionIsBoss = false;
                    transitionIsSurvival = false;
                    transitionNextLevel = 2;
                    transitionTimer = TRANSITION_DURATION;
                    prevPlayState = ARCADE;
                    clearActiveProjectiles(bullets, enemyBullets);
                    clearActivePowerups(powerups, powerupCount);
                    player.resetPosition();
                    state = LEVEL_TRANSITION;
                }

            }
            else if (currentLevel == 2) {
                twinCannons.update();
                twinCannons.specialAttack(player);
                twinCannons.shoot(enemyBullets, MAX_ENEMY_BULLETS);

                handleBossCollision(bullets, MAX_BULLETS, &twinCannons);

                if (!twinCannons.Isalive()) {
                    spawnBossExplosion(twinCannons.getx(), twinCannons.gety(), sf::Color(255, 160, 40), 160);
                    score += 1000 * multiplier;
                    bossPhase = false;
                    currentLevel = 3;
                    transitionIsBoss = false;
                    transitionIsSurvival = false;
                    transitionNextLevel = 3;
                    transitionTimer = TRANSITION_DURATION;
                    prevPlayState = ARCADE;
                    clearActiveProjectiles(bullets, enemyBullets);
                    clearActivePowerups(powerups, powerupCount);
                    player.resetPosition();
                    state = LEVEL_TRANSITION;

                }
            }
            else if (currentLevel == 3) {
                mothership.update();
                mothership.specialAttack(player);
                mothership.shoot(enemyBullets, MAX_ENEMY_BULLETS);

                trySpawnSeeker(mothership, seekers, MAX_SEEKERS);
                handleBossCollision(bullets, MAX_BULLETS, &mothership);

                if (!mothership.Isalive()) {
                    spawnBossExplosion(mothership.getx(), mothership.gety(), sf::Color(255, 50, 200), 220);
                    score += 1500 * multiplier;
                    state = WIN;
                }
            }
        }
        else {
            bool any = false;

            for (int i = 0; i < MAX_DRONES; i++) {
                if (drones[i].Isalive()) {
                    any = true;
                    break;
                }
            }

            if (!any) {
                for (int i = 0; i < MAX_VIPERS; i++) {
                    if (vipers[i].Isalive()) {
                        any = true;
                        break;
                    }
                }
            }

            if (!any) {
                for (int i = 0; i < MAX_SEEKERS; i++) {
                    if (seekers[i].Isalive()) {
                        any = true;
                        break;
                    }
                }
            }

            if (!any) {
                transitionIsBoss = true;
                transitionIsSurvival = false;
                transitionNextLevel = currentLevel;
                transitionTimer = TRANSITION_DURATION;
                prevPlayState = ARCADE;
                clearActiveProjectiles(bullets, enemyBullets);
                clearActivePowerups(powerups, powerupCount);
                player.resetPosition();
                state = LEVEL_TRANSITION;

                if (gSounds) gSounds->playBossWarning();
            }
        }
    }
    else {
        bool any = false;

        for (int i = 0; i < MAX_DRONES; i++) {
            if (drones[i].Isalive()) {
                any = true;
                break;
            }
        }

        if (!any) {
            for (int i = 0; i < MAX_VIPERS; i++) {
                if (vipers[i].Isalive()) {
                    any = true;
                    break;
                }
            }
        }

        if (!any) {
            for (int i = 0; i < MAX_SEEKERS; i++) {
                if (seekers[i].Isalive()) {
                    any = true;
                    break;
                }
            }
        }

        if (!any) {
            currentLevel++;
            transitionIsBoss = false;
            transitionIsSurvival = true;
            transitionNextLevel = currentLevel;
            transitionTimer = TRANSITION_DURATION;
            prevPlayState = SURVIVAL;
            clearActiveProjectiles(bullets, enemyBullets);
            clearActivePowerups(powerups, powerupCount);
            player.resetPosition();
            state = LEVEL_TRANSITION;
        }

    }

    // Only process damage-dealing collisions during active gameplay
    if (state == ARCADE || state == SURVIVAL) {
        handleCollisions(bullets, MAX_BULLETS, drones, MAX_DRONES, vipers, MAX_VIPERS,
            asteroids, asteroidCount, score, multiplier, killTimer, powerups, powerupCount);

        handleEBulletAsteroid(enemyBullets, MAX_ENEMY_BULLETS, asteroids, asteroidCount);
        handleSeekerPlayer(player, seekers, MAX_SEEKERS, multiplier, killTimer);
        handleEBulletPlayer(enemyBullets, MAX_ENEMY_BULLETS, player, multiplier, killTimer);

        handlePlayerPowerUp(player, powerups, powerupCount,
            pickupMsg, pickupMsgTimer, pickupGlowTimer);

        handlePlayerAsteroid(player, asteroids, asteroidCount, multiplier, killTimer);
        handlePlayerEnemyContact(player, drones, MAX_DRONES, vipers, MAX_VIPERS, multiplier, killTimer);

        if (bossPhase) {
            Boss* activeBoss = nullptr;
            if (currentLevel == 1)      activeBoss = &cruiser;
            else if (currentLevel == 2) activeBoss = &twinCannons;
            else if (currentLevel == 3) activeBoss = &mothership;
            handlePlayerBossContact(player, activeBoss, multiplier, killTimer);
        }

        if (player.getLives() <= 0) {
            prevPlayState = state;
            state = GAMEOVER;
        }

        if (!bossPhase) {
            for (int i = 0; i < MAX_VIPERS; i++)
                if (vipers[i].Isalive() && (int)vipers[i].gety() >= HEIGHT - 2) {
                    prevPlayState = state; state = GAMEOVER;
                }
            for (int i = 0; i < MAX_DRONES; i++)
                if (drones[i].Isalive() && (int)drones[i].gety() >= HEIGHT - 2) {
                    prevPlayState = state; state = GAMEOVER;
                }
        }
    }
    if (score > highScore) {
        highScore = score;
    }
    int newCount = 0;
    for (int i = 0; i < powerupCount; i++) {
        if (powerups[i].Isalive()) {
            if (newCount != i)
                powerups[newCount] = powerups[i];
            newCount++;
        }
    }
    powerupCount = newCount;

}
void renderGame(
    sf::RenderWindow& window,

    GameState state,
    GameState prevPlayState,

    sf::Font& fontBold,
    sf::Font& fontMedium,

    sf::Sprite& background,
    sf::RectangleShape& bgOverlay,
    sf::Texture& gameOverBgTex,

    Player& player,
    Bullet bullets[],
    EnemyBullet enemyBullets[],
    PowerUp powerups[],
    Asteroid asteroids[],

    Drone drones[],
    Viper vipers[],
    Seeker seekers[],

    Cruiser& cruiser,
    TwinCannons& twinCannons,
    Mothership& mothership,

    int score,
    int multiplier,
    int currentLevel,

    bool bossPhase,

    int powerupCount,
    int asteroidCount,

    string& pickupMsg,
    int& pickupMsgTimer,
    int& pickupGlowTimer,

    float empFlash
) {
    window.clear();
    if (state == MENU) {
        drawMainMenu(window, fontBold, fontMedium, background, bgOverlay);
    }
    else if (state == MODE_SELECT) {
        drawModeSelect(window, fontBold, fontMedium, background, bgOverlay);
    }
    else if (state == CONTROLS) {
        drawControls(window, fontBold, fontMedium);
    }
    else if (state == GAMEOVER) {
        drawGameOver(window, fontBold, fontMedium, score, highScore, currentLevel, prevPlayState == SURVIVAL, gameOverBgTex);
    }
    else if (state == WIN) {
        drawWin(window, fontBold, fontMedium, score, gameOverBgTex);
    }

    else {
        window.clear();

        window.setView(window.getDefaultView());
        window.draw(background);
        if (state != LEVEL_TRANSITION) {
            window.draw(bgOverlay);
        }
        sf::View view = window.getDefaultView();
        sf::Vector2f shake = getShakeOffset();

        view.setCenter(WIN_W / 2.f + shake.x, WIN_H / 2.f + shake.y);
        window.setView(view);

        float t = frameCount * 0.03f;

        for (int i = 0; i < MAX_PRETTY_STARS; i++) {
            PrettyStar& s = prettyStars[i];

            float glow = 0.5f + 0.5f * sin(t + s.phase);

            sf::CircleShape glowStar(s.size * 3.0f);
            glowStar.setPosition(s.x - s.size, s.y - s.size);
            glowStar.setFillColor(sf::Color(120, 150, 255, 12));
            window.draw(glowStar);

            sf::CircleShape star(s.size);
            star.setPosition(s.x, s.y);
            star.setFillColor(sf::Color(190, 205, 255, (sf::Uint8)(60 + 45 * glow)));
            window.draw(star);
        }

        for (int i = 0; i < asteroidCount; i++) {
            asteroids[i].draw(window);
        }

        for (int i = 0; i < MAX_DRONES; i++) {
            drones[i].draw(window);
        }

        for (int i = 0; i < MAX_VIPERS; i++) {
            vipers[i].draw(window);
        }

        for (int i = 0; i < MAX_SEEKERS; i++) {
            seekers[i].draw(window);
        }

        if (bossPhase) {
            if (currentLevel == 1) {
                cruiser.draw(window);
            }
            else if (currentLevel == 2) {
                twinCannons.draw(window);
            }
            else if (currentLevel == 3) {
                mothership.draw(window);
            }
        }

        for (int i = 0; i < MAX_BULLETS; i++) {
            bullets[i].draw(window);
        }

        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
            enemyBullets[i].draw(window);
        }

        for (int i = 0; i < powerupCount; i++) {
            powerups[i].draw(window);
        }

        if (pickupGlowTimer > 0) {
            drawGlow(window, cx(player.getx()), cy(player.gety()), CW * 0.8f, sf::Color(0, 255, 180, 120), 4);
            pickupGlowTimer--;
        }

        player.draw(window);
        drawParticles(window);
        if (state != LEVEL_TRANSITION && empFlash > 0) {
            sf::RectangleShape fl(sf::Vector2f((float)WIN_W, (float)WIN_H));
            fl.setFillColor(sf::Color(180, 0, 255, (sf::Uint8)(empFlash * 10)));
            window.draw(fl);
        }

        window.setView(window.getDefaultView());

        drawHUD(window, gFont, player, score, multiplier, currentLevel, state == SURVIVAL,highScore);

        if (state == ARCADE || state == SURVIVAL) {
            drawPauseIcon(window, gFont);
        }

        if (pickupMsgTimer > 0 && gFontLoaded) {
            sf::Text pickupText;
            pickupText.setFont(gFont);
            pickupText.setString(pickupMsg);
            pickupText.setCharacterSize(18);
            pickupText.setFillColor(sf::Color(0, 255, 180));
            pickupText.setPosition(20, 70);
            window.draw(pickupText);

            pickupMsgTimer--;
        }

        if (bossPhase) {
            if (currentLevel == 1 && cruiser.Isalive())
                cruiser.drawHealthBar(window, gFont);
            else if (currentLevel == 2 && twinCannons.Isalive())
                twinCannons.drawHealthBar(window, gFont);
            else if (currentLevel == 3 && mothership.Isalive())
                mothership.drawHealthBar(window, gFont);
        }

        if (state == PAUSED) {
            drawPauseOverlay(window, gFont);
        }
        if (state == LEVEL_TRANSITION) {
            drawLevelTransition(window, fontBold, fontMedium);
        }
    }
}

//  MAIN
int main() {
    srand((unsigned)time(0));

    sf::RenderWindow window(sf::VideoMode(WIN_W, WIN_H), "Space Invaders - SFML", sf::Style::Close);
    window.setFramerateLimit(30);

    asteroidRespawnTimer = new int[MAX_ASTEROIDS]();
    prettyStars = new PrettyStar[MAX_PRETTY_STARS];
    gParts = new Particle[MAX_PARTICLES];

    for (int i = 0; i < MAX_PARTICLES; i++) gParts[i].alive = false;

    // sounds
    SoundManager sounds;
    sounds.load();
    gSounds = &sounds;

    sf::Music bgMusic;

    Mothership mothership(57, 1);
    // fonts/textures
    sf::Font fontBold, fontMedium;
    sf::Texture menuBgTex, playerTex;
    sf::Texture droneTex, viperTex, seekerTex;
    sf::Texture cruiserTex, twinBodyTex, twinLeftTurretTex, twinRightTurretTex, mothershipTex;
    sf::Texture bgTexture, gameOverBgTex;

    sf::Sprite background;
    sf::RectangleShape bgOverlay;
    sf::Texture mothershipLaserTex;


    loadAssets(
        bgMusic,
        fontBold,
        fontMedium,
        menuBgTex,
        playerTex,
        droneTex,
        viperTex,
        seekerTex,
        cruiserTex,
        twinBodyTex,
        twinLeftTurretTex,
        twinRightTurretTex,
        mothershipTex,
        bgTexture,
        background,
        bgOverlay,
        gameOverBgTex,
        mothershipLaserTex,
        mothership
    );

    string pickupMsg = "";
    int pickupMsgTimer = 0;
    int pickupGlowTimer = 0;

    Player player;
    player.setTexture(playerTex);

    Bullet* bullets = new Bullet[MAX_BULLETS];
    EnemyBullet* enemyBullets = new EnemyBullet[MAX_ENEMY_BULLETS];
    PowerUp* powerups = new PowerUp[MAX_POWERUPS];
    Asteroid* asteroids = new Asteroid[MAX_ASTEROIDS];
    Drone* drones = new Drone[MAX_DRONES];
    Viper* vipers = new Viper[MAX_VIPERS];
    Seeker* seekers = new Seeker[MAX_SEEKERS];

    Cruiser cruiser(WIDTH / 2, 4);
    TwinCannons twinCannons(WIDTH / 2, 4);


    for (int i = 0; i < MAX_DRONES; i++)
        drones[i].setTexture(droneTex);

    for (int i = 0; i < MAX_VIPERS; i++)
        vipers[i].setTexture(viperTex);

    for (int i = 0; i < MAX_SEEKERS; i++)
        seekers[i].setTexture(seekerTex);

    cruiser.setTexture(cruiserTex);
    twinCannons.setTextures(twinBodyTex, twinLeftTurretTex, twinRightTurretTex);
    mothership.setTexture(mothershipTex);

    GameState state = MENU;
    GameState prevPlayState = ARCADE;

    int currentLevel = 1;
    bool bossPhase = false;
    float waveSyncY = 4.f;
    int powerupCount = 0;
    int asteroidCount = 3;
    int score = 0;
    int multiplier = 1;
    int killTimer = 0;
    float empFlash = 0.f;

    while (window.isOpen()) {
        handleEvents(
            window, state, prevPlayState,
            sounds,
            player,
            drones, vipers, seekers,
            bullets, enemyBullets, asteroids, powerups,
            score, currentLevel, bossPhase,
            cruiser, twinCannons, mothership,
            waveSyncY, powerupCount,
            cruiserTex, twinBodyTex, twinLeftTurretTex, twinRightTurretTex, mothershipTex,
            playerTex,
            empFlash
        );

        updateGame(
            state, prevPlayState,
            player,
            bullets, enemyBullets, powerups, asteroids,
            drones, vipers, seekers,
            cruiser, twinCannons, mothership,
            score, multiplier, killTimer,
            currentLevel, bossPhase,
            waveSyncY, powerupCount, asteroidCount,
            pickupMsg, pickupMsgTimer, pickupGlowTimer,
            empFlash
        );

        renderGame(
            window,
            state, prevPlayState,
            fontBold, fontMedium,
            background, bgOverlay,
            gameOverBgTex,
            player,
            bullets, enemyBullets, powerups, asteroids,
            drones, vipers, seekers,
            cruiser, twinCannons, mothership,
            score, multiplier, currentLevel,
            bossPhase,
            powerupCount, asteroidCount,
            pickupMsg, pickupMsgTimer, pickupGlowTimer,
            empFlash
        );

        window.display();
        frameCount++;
    }

    // Memory Cleanup
    delete[] bullets;
    delete[] enemyBullets;
    delete[] powerups;
    delete[] asteroids;
    delete[] drones;
    delete[] vipers;
    delete[] seekers;
    delete[] asteroidRespawnTimer;
    delete[] prettyStars;
    delete[] gParts;
    return 0;
}