#include <SFML/Graphics.hpp>
#include <iostream>
#include <sstream>
#include <set>
#include <string>
#include <fstream>
#include <ctime>
#include <vector>
#include <cmath>

using namespace std;
using namespace sf; 

Font arial;
View view;

int frame = 0;
int sinceDied = 0;

double gravity = .21;
float blockSize = 75.f;
float partialSize = blockSize / 3;

Event event;
int rwindowx = VideoMode::getDesktopMode().width;
int rwindowy = VideoMode::getDesktopMode().height;
const double pi = 3.141592;

enum species { LIZARD, BIRD, MAMMAL, AMPHIBIAN };
enum stats { HP, RUNSPD, JUMPH };


struct Entity;
struct Surface;
struct Hitbox;
struct Fluid;
struct Enemy;

//Enemy play, mook, mook_turn, mook_spike, mook_spike_turn, mook_chase, mook_jumper, mook_floater, mook_ghoster, crate, red_crate, red_potion;
vector<Fluid> fluids;
vector<Entity> enemies;
vector<Surface> surfaces;
vector<Surface> tempSurfaces;
vector<Hitbox> hitboxes;

bool includes(set<string> checkSet, string value) {
	set <string, greater <string> > ::iterator itr;
	bool final = false;
	for (itr = checkSet.begin(); itr != checkSet.end(); ++itr) {
		if (*itr == value) {
			final = true;
		}
	}
	return final;
}

struct Fluid {
	RectangleShape rect;
	set<string> attributes;

	Fluid(Vector2f location, Vector2f size, set<string> attr) {
		rect.setPosition(location);
		rect.setSize(size);
		analyzeAttr(attr);
	}

	Fluid(int x, int y, int w, int h, set<string> attr) {
		rect.setPosition(Vector2f((float)(x * partialSize), (float)((32 - y) * partialSize)));
		rect.setSize(Vector2f((float)(w * partialSize), (float)(h * partialSize)));
		analyzeAttr(attr);
	}

	void analyzeAttr(set<string> attr){
		attributes = attr;
		if (includes(attributes, "water")) {
			rect.setFillColor(Color(0, 0, 40, 180));
		}
	}

	bool intersects(Entity * character);
};

struct Surface {
	RectangleShape rect;
	bool canIntersectTop;
	bool canIntersectBottom;
	bool canIntersectRight;
	bool canIntersectLeft;
	bool canTremble;
	set<string> attributes;

	Surface(Vector2f location, Vector2f size, set<string> attr) {
		rect.setPosition(location);
		rect.setSize(size);
		analyzeAttr(attr);
	}

	Surface(int x, int y, int w, int h, set<string> attr) {
		rect.setPosition(Vector2f((float)(x * partialSize), (float)((32 - y) * partialSize)));
		rect.setSize(Vector2f((float)(w * partialSize), (float)(h * partialSize)));
		analyzeAttr(attr);
	}

	void analyzeAttr(set<string> attr){
		attributes = attr;

		if (includes(attributes, "no_bottom")) {
			rect.setFillColor(Color(0, 0, 10, 180));
		} else {
			rect.setFillColor(Color(0, 0, 0, 255));
		}

		if (includes(attributes, "no_bottom")) {
			canIntersectBottom = false;
		}
		if (includes(attributes, "no_top")) {
			canIntersectTop = false;
		}
		if (includes(attributes, "no_left")) {
			canIntersectLeft = false;
		}
		if (includes(attributes, "no_right")) {
			canIntersectRight = false;
		}

		canTremble = !includes(attributes, "no_tremble");
	}


	bool intersectsTop(Entity * character);
	bool intersectsBottom(Entity * character);
	bool intersectsRight(Entity * character);
	bool intersectsLeft(Entity * character);
};

struct Hitbox {
	Entity * origin;
	RectangleShape rect;
	bool against;
	set<string> attributes;
	int damage;

	Hitbox(Entity * character, Vector2f location, Vector2f size, bool againstEnemy, int damageVal, set<string> attr) {
		origin = character;
		rect.setPosition(location);
		rect.setSize(size);
		if(includes(attr, "red")) {
			rect.setFillColor(Color(90, 0, 0, 90));
		}
		else {
			rect.setFillColor(Color(0, 0, 0, 0));
		}
		damage = damageVal;
		against = againstEnemy;
		attributes = attr;
	}
};

struct Enemy {
	set<string> attributes;
	string filename;
	Vector2f scale;
	vector<int> stats;

	Enemy(string title, vector<int> size, vector<int> statistics, set<string> attr) {
		filename = title;
		Texture fake;
		fake.loadFromFile(title);
		float width = (float)(fake.getSize().x);
		float height = (float)(fake.getSize().y);

		if (includes(attr, "animate-6") || includes(attr, "animate-jump")) {
			width /= 6;
		}
		if (includes(attr, "animate-4")) {
			width /= 4;
		}
		if (includes(attr, "animate-2")) {
			width /= 2;
		}

		scale = Vector2f((float)(size[0] * blockSize / width), (float)(size[1] * blockSize / height));
		stats = statistics;
		attributes = attr;
	}
};

struct Entity {
	Texture texture;
	Sprite sprite;
	Text label;
	int animationState = 0;
	int animationFrame = 0;

	double x = 0;
	double y = 0;
	double xv = 0;
	double yv = 0;

	float width;
	float height;

	bool inAir = false;
	int timeInAir = 0;
	bool forwards = true;
	bool inFluid = false;
	bool collideRight;
	bool collideLeft;
	int invulnerable = 0;
	int lastAttack = 0;
	int summonCounter = 20;

	bool right;
	bool left;
	bool up;
	bool attack = false;
	bool quake = false;

	bool alive = true;
	int hp;
	string filename;
	vector<int> stats;
	set<string> attributes;
	double personalGravity = gravity;
	double maxSpeedX = 6;
	double maxSpeedY = 12;
	double instantSpeedX = 2.1;
	double holdSpeedX = .05;
	double instantSpeedY = 7;
	double holdSpeedY = .1;

	Entity(Enemy type, int xo, int yo) {
		x = xo * partialSize;
		y = (32 - yo) * partialSize;
		init(type);
	}

	Entity(Enemy type, Vector2f pos) {
		x = pos.x;
		y = pos.y;
		init(type);
	}

	void init(Enemy type){
		filename = type.filename;
		attributes = type.attributes;
		texture.loadFromFile(filename);
		texture.setSmooth(true);
		sprite.setTexture(texture);
		sprite.setPosition(Vector2f((float)(x), (float)(y)));
		if (includes(attributes, "animate-6") || includes(attributes, "animate-jump")) {
			sprite.setTextureRect(IntRect(256, 0, 128, 128));
			sprite.setOrigin(Vector2f((float)(64), (float)(64)));
		}
		else if (includes(attributes, "animate-2")) {
			sprite.setTextureRect(IntRect(0, 0, 64, 128));
			sprite.setOrigin(Vector2f((float)(32), (float)(64)));
		}
		else if (includes(attributes, "animate-4")) {
			sprite.setTextureRect(IntRect(0, 0, 142, 128));
			sprite.setOrigin(Vector2f((float)(71), (float)(64)));
		}
		else {
			sprite.setOrigin(Vector2f((float)(texture.getSize().x / 2), (float)(texture.getSize().y / 2)));
		}

		if (includes(attributes, "jumper")) {
			instantSpeedY /= 2;
			maxSpeedY /= 2;
		}
		
		if (includes(attributes, "potion")) {
			invulnerable = 5;
		}

		
		width = type.scale.x;
		height = type.scale.y;

		stats = type.stats;
		hp = stats[HP];

		label.setFont(arial);
		label.setCharacterSize(20);
		label.setFillColor(sf::Color::White);
		label.setStyle(sf::Text::Bold);
	}

	void movement(Entity * player) {
		if (includes(attributes, "playable")) {
			left = Keyboard::isKeyPressed(Keyboard::Left);
			right = Keyboard::isKeyPressed(Keyboard::Right);
			up = Keyboard::isKeyPressed(Keyboard::Up);
			attack = Keyboard::isKeyPressed(Keyboard::Space);
			quake = Keyboard::isKeyPressed(Keyboard::Down);
		}
		else {
			up = false;
			if (collideRight) {
				right = false;
				left = true;
			}
			else if (collideLeft) {
				right = true;
				left = false;
			}
			attack = false;
		}
	}

	void summon(set<string> conditions);

	void die(){
		hp = 0;
		alive = false;
		summon({ "death" });
	}

	void update(RenderWindow& window, Entity * player) {

		animationFrame += (includes(attributes, "animate-6") || includes(attributes, "animate-2")) ? 1 : 0;
		animationFrame += (includes(attributes, "animate-jump") && inAir) ? 4 : 0;
		if ((includes(attributes, "animate-6") || (animationState < 5 && includes(attributes, "animate-jump"))) && animationFrame % 10 == 0) {
			animationState = (animationState + 1) % 6;
			sprite.setTextureRect(IntRect(128 * animationState, 0, 128, 128));
		}

		if (includes(attributes, "animate-2")) {
			animationState = (animationState + 1) % 2;
			sprite.setTextureRect(IntRect(64 * animationState, 0, 64, 128));
		}

		if (includes(attributes, "animate-4")) {
			animationState = (animationState + 1) % 4;
			sprite.setTextureRect(IntRect(142 * animationState, 0, 142, 128));
		}

		if (invulnerable > 0) {
			invulnerable--;
		}
		if (lastAttack > 0) {
			lastAttack--;
		}

		

		if (timeInAir > 0) {
			timeInAir--;
		}
		if (inAir) {
			yv += personalGravity;
		}
		if (timeInAir > 5 && up) {
			yv -= .05 + holdSpeedY;
		}

		if (right || left) {
			run();
		}
		if (attack) {
			doAttack();
		}

		if (!includes(attributes, "no_movement")) {
			movement(player);
			inAir = true;
			collideRight = false;
			collideLeft = false;
			for (int i = 0; i < tempSurfaces.size(); i++) {
				bool exit = false;
				if (tempSurfaces[i].intersectsTop(this)) {
					yv = max(0.0, yv);
					if (inAir && tempSurfaces[i].canTremble) {
						if (includes(attributes, "playable")) {
							set<string> attr;
							attr.insert("headhit");
							hitboxes.push_back(Hitbox(this, Vector2f((float)(sprite.getGlobalBounds().left), (float)(sprite.getGlobalBounds().top - tempSurfaces[i].rect.getGlobalBounds().height - 30)),
								Vector2f(sprite.getGlobalBounds().width, 30), true, 1, attr));
						}
						else {
							hurtPreset(1);
						}
					}
				}
				if (tempSurfaces[i].intersectsBottom(this)) {
					if (yv != 0.0 && includes(attributes, "playable") && quake && tempSurfaces[i].canTremble) {
						set<string> attr;
						attr.insert("butthit");
						hitboxes.push_back(Hitbox(this, Vector2f((float)(sprite.getGlobalBounds().left),
							(float)(sprite.getGlobalBounds().top + sprite.getGlobalBounds().height + tempSurfaces[i].rect.getGlobalBounds().height)),
							Vector2f(min(sprite.getGlobalBounds().width,
								tempSurfaces[i].rect.getGlobalBounds().width + tempSurfaces[i].rect.getGlobalBounds().left - sprite.getGlobalBounds().left), 30), true, 1, attr));
					}
					inAir = false;
					yv = min(0.0, yv);
					if (includes(attributes, "turn_at_corner")) {
						collideLeft = tempSurfaces[i].rect.getGlobalBounds().left > sprite.getGlobalBounds().left;
						collideRight = (tempSurfaces[i].rect.getGlobalBounds().left + tempSurfaces[i].rect.getGlobalBounds().width) < (sprite.getGlobalBounds().left + sprite.getGlobalBounds().width);
					}
					//exit = true;
				}
				if (tempSurfaces[i].intersectsRight(this) && !exit) {
					collideRight = true;
				}
				if (tempSurfaces[i].intersectsLeft(this) && !exit) {
					collideLeft = true;
				}
			}

			if (collideLeft && collideRight) {
				hurtPreset(1);
			}

			inFluid = false;
			for (int i = 0; i < fluids.size(); i++) {
				if (fluids[i].intersects(this)) {
					inFluid = true;
				}
			}

			if (includes(attributes, "no_gravity")) {
				personalGravity = 0;
			}

			if (xv > 0) {
				xv -= holdSpeedX / 3;
				if (collideRight) {
					xv = 0;
				}
			}
			if (xv < 0) {
				xv += holdSpeedX / 3;
				if (collideLeft) {
					xv = 0;
				}
			}

			if (!right && xv > instantSpeedX) {
				xv = instantSpeedX;
			}
			if (!left && xv < -instantSpeedX) {
				xv = -instantSpeedX;
			}

			if (xv > maxSpeedX) {
				xv = maxSpeedX;
			}
			if (xv < -maxSpeedX) {
				xv = -maxSpeedX;
			}

			if (abs(xv) < .2) {
				xv = 0;
			}
		}

		if (yv > maxSpeedY) {
			yv = maxSpeedY;
		}
		if (yv < -maxSpeedY) {
			yv = -maxSpeedY;
		}

		if (quake && inAir) {
			xv = 0;
			if (yv < 0) {
				yv = 0;
			}
			if (inAir && yv < maxSpeedY) {
				yv = maxSpeedY;
			}
		}

		if (includes(attributes, "floatup")) {
			yv = -3;
		}

		x += xv;
		y += inFluid ? (yv / 4) : yv;
		sprite.setPosition((float)(x), (float)(y));

		if (forwards) {
			sprite.setScale(Vector2f(width, height));
		}
		else {
			sprite.setScale(Vector2f(-width, height));
		}

		if (!includes(attributes, "no_hurt") && !includes(attributes, "playable") && invulnerable == 0) {
			set<string> attr;
			hitboxes.push_back(Hitbox(this, Vector2f((float)(sprite.getGlobalBounds().left), (float)(sprite.getGlobalBounds().top)),
				Vector2f(sprite.getGlobalBounds().width, sprite.getGlobalBounds().height), false, 1, attr));
		}
		else if (!includes(attributes, "no_hurt")) {
			set<string> attr;
			attr.insert("collection");
			hitboxes.push_back(Hitbox(this, Vector2f((float)(sprite.getGlobalBounds().left), (float)(sprite.getGlobalBounds().top)),
				Vector2f(sprite.getGlobalBounds().width, sprite.getGlobalBounds().height), true, 1, attr));
		}

		if (includes(attributes, "playable") && invulnerable < 1) {
			set<string> attr;
			attr.insert("jump");
			hitboxes.push_back(Hitbox(this, Vector2f((float)(sprite.getGlobalBounds().left), (float)(sprite.getGlobalBounds().top + sprite.getGlobalBounds().height)),
				Vector2f((float)(sprite.getGlobalBounds().width), (float)(maxSpeedY)), true, (quake ? 2 : 1), attr));
		}

		if (includes(attributes, "playable")) {
			set<string> pushLeft;
			pushLeft.insert("push_left");
			hitboxes.push_back(Hitbox(this, Vector2f((float)(sprite.getGlobalBounds().left - 5), (float)(sprite.getGlobalBounds().top)),
				Vector2f(2, sprite.getGlobalBounds().height), true, 0, pushLeft));
			set<string> pushRight;
			pushRight.insert("push_right");
			hitboxes.push_back(Hitbox(this, Vector2f((float)(sprite.getGlobalBounds().left + sprite.getGlobalBounds().width + 5), (float)(sprite.getGlobalBounds().top)),
				Vector2f(2, sprite.getGlobalBounds().height), true, 0, pushRight));
		}

		if(includes(attributes, "spike")){
			set<string> attr;
			attr.insert("headhit");
			hitboxes.push_back(Hitbox(this, Vector2f((float)(sprite.getGlobalBounds().left), (float)(sprite.getGlobalBounds().top - 5)),
				Vector2f(sprite.getGlobalBounds().width, 5), false, 1, attr));
		}

		if (includes(attributes, "jumper")) {
			jump();
		}

		if (summonCounter == 0) {
			summonCounter = 350;
		}
		summonCounter--;
		

		sprite.setColor(Color(255, 255 - invulnerable * 4, 255 - invulnerable * 4, 255));
		sprite.setTexture(texture);

		label.setOrigin(Vector2f((float)(label.getGlobalBounds().width / 2), 0.f));
		label.setPosition(Vector2f((float)(sprite.getGlobalBounds().left + sprite.getGlobalBounds().width / 2), (float)(sprite.getGlobalBounds().top - 20)));
		label.setString("HP: " + to_string(hp) + " / " + to_string(stats[HP]));
		if (!includes(attributes, "no_health")) {
			window.draw(label);
		}

		sprite.move(0, (float)(maxSpeedY));
		window.draw(sprite);
		sprite.move(0, (float)(-maxSpeedY));
	}

	void jump() {
		if (!inAir) {
			yv -= instantSpeedY;
			timeInAir = 25;
			animationState = 0;
		}
		if (inFluid) {
			yv -= instantSpeedY * 2;
		}
	}

	void run() {
		if (includes(attributes, "dont_move") || quake) {
			return;
		}
		double val = instantSpeedX;
		if (xv >= instantSpeedX - .1) {
			val = xv + holdSpeedX;
		}
		else if (xv <= -instantSpeedX + .1) {
			val = -xv + holdSpeedX;
		}
		if (right && !left) {
			xv = abs(val);
			forwards = true;
			return;
		}
		if (left && !right) {
			xv = -abs(val);
			forwards = false;
			return;
		}
		xv = 0;
	}

	void doAttack() {
		if (lastAttack == 0) {
			set<string> attr;
			attr.insert("red");
			if (forwards) {
				hitboxes.push_back(Hitbox(this, Vector2f((float)(sprite.getGlobalBounds().left + sprite.getGlobalBounds().width), (float)(sprite.getGlobalBounds().top)),
					Vector2f(sprite.getGlobalBounds().width, sprite.getGlobalBounds().height), true, 2, attr));
			}
			else {
				hitboxes.push_back(Hitbox(this, Vector2f((float)(sprite.getGlobalBounds().left - sprite.getGlobalBounds().width), (float)(sprite.getGlobalBounds().top)),
					Vector2f(sprite.getGlobalBounds().width, sprite.getGlobalBounds().height), true, 2, attr));
			}
			lastAttack = 100;
		}
	}

	bool hurt(Hitbox * hurter) {
		if (includes(attributes, "no_damage")) {
			return false;
		}
		if (includes(hurter->attributes, "push_left") && (invulnerable > 0 || includes(attributes, "can_be_pushed"))) {
			xv = -1;
			return false;
		}
		if (includes(hurter->attributes, "push_right") && (invulnerable > 0 || includes(attributes, "can_be_pushed"))) {
			xv = 1;
			return false;
		}
		if (invulnerable > 0 || hp <= 0 ||
			(includes(hurter->attributes, "jump") && !includes(attributes, "jumpable")) ||
			(includes(hurter->attributes, "collection") && !includes(attributes, "collectable"))) {
			return false;
		}
		hp -= hurter->damage;
		if (hp <= 0) {
			die();
		} else if(hurter->damage > 0){
			invulnerable = 50;
		}
		return true;
	}

	void hurtPreset(int damage) {
		if (invulnerable > 0) {
			return;
		}
		hp -= damage;
		if (hp <= 0) {
			die();
		}
		else if (damage > 0) {
			invulnerable = 50;
		}
	}

	void createSurface() {
		if (includes(attributes, "surface") && alive) {
			set<string> attr{"no_display"};
			tempSurfaces.push_back(Surface(Vector2f((float)(sprite.getGlobalBounds().left), (float)(sprite.getGlobalBounds().top)),
				Vector2f(sprite.getGlobalBounds().width, sprite.getGlobalBounds().height), attr));
		}
	}
};

bool Fluid::intersects(Entity* character) {
	return rect.getGlobalBounds().intersects(character->sprite.getGlobalBounds());
}

bool Surface::intersectsTop(Entity* character) {
	RectangleShape fake(Vector2f((float)(character->sprite.getGlobalBounds().width), (float)(character->maxSpeedY)));
	fake.setPosition(Vector2f((float)(character->sprite.getGlobalBounds().left), (float)(character->sprite.getGlobalBounds().top - character->maxSpeedY)));
	return rect.getGlobalBounds().intersects(fake.getGlobalBounds()) && canIntersectBottom;
}

bool Surface::intersectsBottom(Entity* character) {
	RectangleShape fake(Vector2f((float)(character->sprite.getGlobalBounds().width), (float)(character->maxSpeedY)));
	fake.setPosition(Vector2f((float)(character->sprite.getGlobalBounds().left), (float)(character->sprite.getGlobalBounds().top + character->sprite.getGlobalBounds().height)));
	return rect.getGlobalBounds().intersects(fake.getGlobalBounds()) && canIntersectTop;
}

bool Surface::intersectsLeft(Entity* character) {
	RectangleShape fake(Vector2f((float)(character->maxSpeedX), (float)(character->sprite.getGlobalBounds().height)));
	fake.setPosition(Vector2f((float)(character->sprite.getGlobalBounds().left - character->maxSpeedX), (float)(character->sprite.getGlobalBounds().top)));
	return rect.getGlobalBounds().intersects(fake.getGlobalBounds()) && canIntersectLeft;
}

bool Surface::intersectsRight(Entity* character) {
	RectangleShape fake(Vector2f((float)(character->maxSpeedX), (float)(character->sprite.getGlobalBounds().height)));
	fake.setPosition(Vector2f((float)(character->sprite.getGlobalBounds().left + character->sprite.getGlobalBounds().width), (float)(character->sprite.getGlobalBounds().top)));
	return rect.getGlobalBounds().intersects(fake.getGlobalBounds()) && canIntersectRight;
}

// FLUID TYPES //
set<string> water{ "water" };

// SURFACE TYPES //
set<string> solid;
set<string> semitrans{ "no_bottom", "no_left", "no_right" };

// ENEMY TYPES //
Enemy play = Enemy("assets/lizardmodel.png",             {1, 2}, { 3, 1, 1 }, { "playable" });
Enemy mook_ghoster = Enemy("assets/ghoster.png",         {1, 1}, { 1, 1, 1 }, { "ghost", "animate-4" });
Enemy mook = Enemy("assets/jump-noturn.png",             {1, 1}, { 1, 1, 1 }, { "jumpable", "animate-6" });
Enemy mook_turn = Enemy("assets/jump-turn.png",          {1, 1}, { 2, 1, 1 }, { "animate-6", "jumpable", "turn_at_corner" });
Enemy mook_spike = Enemy("assets/spiketop-noturn.png",   {1, 1}, { 1, 1, 1 }, { "spike", "animate-6" });
Enemy mook_spike_turn = Enemy("assets/spiketop-turn.png",{1, 1}, { 1, 1, 1 }, { "spike", "animate-6", "turn_at_corner" });
Enemy mook_jumper = Enemy("assets/jumper.png",           {1, 1}, { 1, 1, 1 }, { "jumper", "jumpable", "animate-jump" });
Enemy mook_floater = Enemy("assets/floater.png",         {1, 2}, { 3, 1, 1 }, { "floatup", "jumpable", "animate-2", "dont_move" });
Enemy mook_spawner = Enemy("assets/spawner.png",         {2, 3}, { 3, 1, 1 }, { "no_movement", "no_health", "no_hurt", "no_gravity",  "surface", "no_damage", "summon_floater" });
Enemy crate = Enemy("assets/crate.png",                  {2, 2}, { 4, 1, 1 }, { "dont_move", "no_health", "no_hurt", "can_be_pushed", "surface" });
Enemy red_crate = Enemy("assets/red-crate.png",          {1, 1}, { 1, 1, 1 }, { "dont_move", "no_health", "no_hurt", "can_be_pushed", "surface", "drop_red_potion" });
Enemy red_potion = Enemy("assets/red-potion.png",        {1, 1}, { 1, 1, 1 }, { "dont_move", "no_health", "no_hurt", "collectable", "potion", "heal" });

void Entity::summon(set<string> conditions) {
	if (includes(conditions, "death")) {
		if (includes(attributes, "drop_red_potion")) {
			enemies.push_back(Entity(red_potion, Vector2f((float)(x), (float)(y))));
		}
	}

	if (includes(conditions, "update")) {
		if (includes(attributes, "summon_floater")) {
			enemies.push_back(Entity(mook_floater, Vector2f((float)(x + blockSize / 8), (float)(y - blockSize * 3))));
		}
	}
}

int main() {
	arial.loadFromFile("Arial.ttf");
	Text text;
	text.setFont(arial);
	text.setCharacterSize(24); 
	text.setFillColor(Color::White);
	text.setStyle(Text::Bold);
	text.setPosition(Vector2f(500.f, 200.f));

	Text gameover_text;
	gameover_text.setFont(arial);
	gameover_text.setCharacterSize(40);
	gameover_text.setFillColor(Color::Black);
	gameover_text.setStyle(Text::Bold);
	gameover_text.setString("YOU DIED...");
	view.setSize(Vector2f(2000.f, 1400.f));

	RectangleShape gameover_rect;
	gameover_rect.setSize(view.getSize());

	// PLACEMENTS //
	fluids.push_back(Fluid(188, -4, 200, 18, water));

	Entity monster = Entity(play, 2, 20);
	surfaces.push_back(Surface(-4, 0, 20, 18, solid));
	surfaces.push_back(Surface(20, 0, 56, 18, solid));
	surfaces.push_back(Surface(32, 4, 8, 4, solid));
	surfaces.push_back(Surface(56, 8, 8, 6, solid));
	surfaces.push_back(Surface(70, 12, 16, 1, solid));

	surfaces.push_back(Surface(92, 13, 2, 8, solid));
	surfaces.push_back(Surface(94, 8, 12, 3, solid));
	surfaces.push_back(Surface(105, 10, 2, 5, solid));

	surfaces.push_back(Surface(132, 24, 5, 45, solid));
	surfaces.push_back(Surface(134, -5, 54, 24, solid));
	surfaces.push_back(Surface(138, 19, 24, 2, semitrans));
	surfaces.push_back(Surface(146, 6, 24, 2, semitrans));
	surfaces.push_back(Surface(168, 55, 2, 51, solid));
	surfaces.push_back(Surface(170, 19, 14, 2, semitrans));
	surfaces.push_back(Surface(170, 6, 14, 2, semitrans));
	surfaces.push_back(Surface(188, 28, 2, 48, solid));

	enemies.push_back(Entity(mook, 56, 12));
	enemies.push_back(Entity(mook_turn, 72, 16));
	enemies.push_back(Entity(mook_spike, 96, 12));
	enemies.push_back(Entity(mook_spawner, 115, -12));
	enemies.push_back(Entity(mook_spawner, 125, -12));
	enemies.push_back(Entity(mook_spike_turn, 155, 32));
	enemies.push_back(Entity(mook_spike_turn, 155, 14));
	enemies.push_back(Entity(red_crate, 12, 12));
	enemies.push_back(Entity(crate, 32, 12));

	RenderWindow renderWindow;
	renderWindow.create(VideoMode(rwindowx, rwindowy), "Platformer"/*, Style::Fullscreen*/);

	while (renderWindow.isOpen()) {
		frame++;

		renderWindow.clear(Color(150, 170, 200, 255));
		view.setCenter(max(view.getSize().x / 2 - 50, monster.sprite.getPosition().x), 500.f);
		renderWindow.setView(view);

		RectangleShape tempView = RectangleShape(Vector2f(view.getSize().x, view.getSize().y));
		tempView.setPosition(Vector2f((view.getCenter().x - view.getSize().x / 2), view.getCenter().y - view.getSize().y / 2));

		int enemySizeTemp = enemies.size();

		hitboxes = vector<Hitbox>();
		tempSurfaces = vector<Surface>();
		for (int i = 0; i < enemySizeTemp; i++) {
			enemies[i].createSurface();
		}
		for (int i = 0; i < surfaces.size(); i++) {
			if (tempView.getGlobalBounds().intersects(surfaces[i].rect.getGlobalBounds())) {
				tempSurfaces.push_back(surfaces[i]);
			}
		}

		if (monster.y > view.getCenter().y + view.getSize().y / 2 + monster.sprite.getGlobalBounds().height * 2) {
			monster.die();
		}

		for (int i = 0; i < enemySizeTemp; i++) {
			if (enemies[i].alive && tempView.getGlobalBounds().intersects(enemies[i].sprite.getGlobalBounds())) {
				enemies[i].update(renderWindow, &monster);
			}
		}

		for (int i = 0; i < tempSurfaces.size(); i++) {
			if (!includes(tempSurfaces[i].attributes, "no_display")) {
				renderWindow.draw(tempSurfaces[i].rect);
			}
		}

		text.setString(to_string(monster.invulnerable));
		renderWindow.draw(text);

		if (monster.alive) {
			monster.update(renderWindow, &monster);

			for (int i = 0; i < fluids.size(); i++) {
				renderWindow.draw(fluids[i].rect);
			}
		}
		else {
			if (sinceDied < 255) {
				sinceDied += 5;
			}
			gameover_text.setPosition(Vector2f((float)(monster.sprite.getGlobalBounds().left - gameover_text.getGlobalBounds().width / 2), 500.f));
			gameover_rect.setPosition(Vector2f((float)(view.getCenter().x - view.getSize().x / 2), (float)(view.getCenter().y - view.getSize().y / 2)));
			gameover_rect.setFillColor(Color(255,255,255,sinceDied));
			renderWindow.draw(gameover_rect);
			renderWindow.draw(gameover_text);
		}

		for (int i = 0; i < hitboxes.size(); i++) {
			renderWindow.draw(hitboxes[i].rect);
			if (!hitboxes[i].against && monster.sprite.getGlobalBounds().intersects(hitboxes[i].rect.getGlobalBounds())) {
				monster.hurt(&hitboxes[i]);

				if (includes(hitboxes[i].attributes, "headhit")) {
					monster.yv = -hitboxes[i].origin->instantSpeedY;
				}
			}
			if (hitboxes[i].against) {
				for (int e = 0; e < enemySizeTemp; e++) {
					if (enemies[e].sprite.getGlobalBounds().intersects(hitboxes[i].rect.getGlobalBounds())) {
						bool hit = enemies[e].hurt(&hitboxes[i]);
						if (hit && includes(hitboxes[i].attributes, "jump")) {
							hitboxes[i].origin->yv = -hitboxes[i].origin->instantSpeedY;
							hitboxes[i].origin->timeInAir = 25;
						}
						if (hit && includes(hitboxes[i].attributes, "headhit")) {
							enemies[e].yv = -hitboxes[i].origin->instantSpeedY;
						}
						if (hit && includes(hitboxes[i].attributes, "collection")) {
							if (includes(enemies[e].attributes, "heal")) {
								hitboxes[i].origin->hp++;
							}
						}
					}
				}
			}
		}

		for (int i = 0; i < enemySizeTemp; i++) {
			if (enemies[i].summonCounter == 1) {
				enemies[i].summon({ "update" });
			}
		}

		set<int> deadIndexes = {};
		for (int i = 0; i < enemies.size(); i++) {
			if (!enemies[i].alive || enemies[i].sprite.getGlobalBounds().top + enemies[i].sprite.getGlobalBounds().height < tempView.getGlobalBounds().top) {
				deadIndexes.insert(i);
			}
		}

		for (int i = enemies.size() - 1; i >= 0; i--) {
			if (deadIndexes.count(i)) {
				deadIndexes.erase(i);
			}
		}

		renderWindow.display();
		while (renderWindow.pollEvent(event)) {
			switch (event.type) {
			case Event::KeyPressed:
				switch (event.key.code) {
					case Keyboard::Escape:	renderWindow.close(); break;
				}
				if (Keyboard::isKeyPressed(Keyboard::Up)) {
					monster.jump();
				}
				break;
			case Event::EventType::Closed:
				renderWindow.close();
				break;
			}
		}
	}

	return 0;
}