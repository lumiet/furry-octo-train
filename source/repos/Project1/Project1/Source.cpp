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

Event event;
int rwindowx = VideoMode::getDesktopMode().width;
int rwindowy = VideoMode::getDesktopMode().height;
const double pi = 3.141592;

enum species { LIZARD, BIRD, MAMMAL, AMPHIBIAN };
enum stats { HP, RUNSPD, JUMPH };

struct Entity;
struct Surface;
struct Hitbox;
vector<Surface> surfaces;
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

struct Surface {
	RectangleShape rect;

	Surface(Vector2f location, Vector2f size) {
		rect.setPosition(location);
		rect.setSize(size);
		rect.setFillColor(Color(0, 0, 0, 255));
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

	Hitbox(Entity * character, Vector2f location, Vector2f size, bool againstEnemy, set<string> attr) {
		origin = character;
		rect.setPosition(location);
		rect.setSize(size);
		rect.setFillColor(Color(90, 0, 0, 90));
		against = againstEnemy;
		attributes = attr;
	}
};

struct Entity {
	Texture texture;
	Sprite sprite;
	Text label;

	double x = 0;
	double y = 0;
	double xv = 0;
	double yv = 0;

	float width;
	float height;

	bool inAir = false;
	int timeInAir = 0;
	bool forwards = true; 
	bool collideRight;
	bool collideLeft;
	int invulnerable = 0;

	bool right;
	bool left;
	bool up;

	bool alive = true;
	int hp;
	string filename;
	vector<int> stats;
	set<string> attributes;
	double maxSpeedX = 6;
	double maxSpeedY = 8;
	double instantSpeedX = 2.1;
	double holdSpeedX = .05;
	double instantSpeedY = 6.8;
	double holdSpeedY = .2;

	Entity(string title, Vector2f position, Vector2f size, vector<int> statistics, set<string> attr) {
		filename = title;
		texture.loadFromFile(title);
		texture.setSmooth(true);
		sprite.setPosition(position);
		sprite.setTexture(texture);
		sprite.setOrigin(Vector2f((float)(texture.getSize().x / 2), (float)(texture.getSize().y / 2)));

		x = position.x;
		y = position.y;
		
		width = size.x;
		height = size.y;

		attributes = attr;
		stats = statistics;
		hp = stats[HP];

		label.setFont(arial);
		label.setCharacterSize(20);
		label.setFillColor(sf::Color::White);
		label.setStyle(sf::Text::Bold);
	}

	void movement() {
		if (includes(attributes, "playable")) {
			left = Keyboard::isKeyPressed(Keyboard::Left);
			right = Keyboard::isKeyPressed(Keyboard::Right);
			up = Keyboard::isKeyPressed(Keyboard::Up);
		}
		else {
			up = false;
			if (collideRight) {
				right = false;
				left = true;
			}
			else if(collideLeft){
				right = true;
				left = false;
			}
		}
	}

	void die() {
		hp = 0;
		alive = false;
	}

	void update(RenderWindow& window) {
		movement();
		
		if (invulnerable > 0) {
			invulnerable--;
		}
		inAir = true;
		collideRight = false;
		collideLeft = false;
		for (int i = 0; i < surfaces.size(); i++) {
			bool exit = false;
			if (surfaces[i].intersectsTop(this)) {
				yv = max(0.0, yv);
				if (inAir) {
					set<string> attr;
					attr.insert("headhit");
					hitboxes.push_back(Hitbox(this, Vector2f((float)(sprite.getGlobalBounds().left), (float)(sprite.getGlobalBounds().top - surfaces[i].rect.getGlobalBounds().height - 30)),
						Vector2f(sprite.getGlobalBounds().width, 30), true, attr));
				}
			}
			if (surfaces[i].intersectsBottom(this)) {
				if (yv != 0.0 && includes(attributes, "playable")) {
					set<string> attr;
					attr.insert("headhit");
					hitboxes.push_back(Hitbox(this, Vector2f((float)(sprite.getGlobalBounds().left),
						(float)(sprite.getGlobalBounds().top + sprite.getGlobalBounds().height + surfaces[i].rect.getGlobalBounds().height)),
						Vector2f(min(sprite.getGlobalBounds().width, 
							surfaces[i].rect.getGlobalBounds().width + surfaces[i].rect.getGlobalBounds().left - sprite.getGlobalBounds().left), 30), true, attr));
				}
				inAir = false;
				yv = min(0.0, yv);
				if (includes(attributes, "turn_at_corner")) {
					collideLeft = surfaces[i].rect.getGlobalBounds().left > sprite.getGlobalBounds().left;
					collideRight = (surfaces[i].rect.getGlobalBounds().left + surfaces[i].rect.getGlobalBounds().width) < (sprite.getGlobalBounds().left + sprite.getGlobalBounds().width);
				}
				//exit = true;
			}
			if (surfaces[i].intersectsRight(this) && !exit) {
				collideRight = true;
			}
			if (surfaces[i].intersectsLeft(this) && !exit) {
				collideLeft = true;
			}
		}

		if (timeInAir > 0) {
			timeInAir--;
		}
		if (inAir) {
			yv += gravity;
		}
		if (timeInAir > 5 && up) {
			yv -= .05 + holdSpeedY * (1 + xv / maxSpeedX);
		}

		if (right || left) {
			run();
		}

		if (xv > 0) {
			xv -= holdSpeedX / 1.5;
			if (collideRight) {
				xv = 0;
			}
		}
		if (xv < 0) {
			xv += holdSpeedX / 1.5;
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

		if (yv > maxSpeedY) {
			yv = maxSpeedY;
		}
		if (yv < -maxSpeedY) {
			yv = -maxSpeedY;
		}

		if (abs(xv) < .5) {
			xv = 0;
		}

		x += xv;
		y += yv;
		sprite.setPosition((float)(x), (float)(y));

		if (forwards) {
			sprite.setScale(Vector2f(width, height));
		}
		else {
			sprite.setScale(Vector2f(-width, height));
		}

		if (frame % 100 == 0) {
			cout << " pos: (" + to_string(x) + "," + to_string(y) + ") v: (" + to_string(xv) + "," + to_string(yv) + ")" << endl;
		}

		if (!includes(attributes, "playable")) {
			set<string> attr;
			hitboxes.push_back(Hitbox(this, Vector2f((float)(sprite.getGlobalBounds().left), (float)(sprite.getGlobalBounds().top)), 
				Vector2f(sprite.getGlobalBounds().width, sprite.getGlobalBounds().height), false, attr));
		}
		else if(invulnerable < 1){
			set<string> attr;
			attr.insert("jump");
			hitboxes.push_back(Hitbox(this, Vector2f((float)(sprite.getGlobalBounds().left), (float)(sprite.getGlobalBounds().top + sprite.getGlobalBounds().height)),
				Vector2f(sprite.getGlobalBounds().width, maxSpeedY), true, attr));
		}

		sprite.setColor(Color(255,255 - invulnerable,255 - invulnerable,255));
		sprite.setTexture(texture);
		label.setOrigin(Vector2f((float)(label.getGlobalBounds().width / 2), 0.f));
		label.setPosition(Vector2f((float)(sprite.getGlobalBounds().left + sprite.getGlobalBounds().width / 2), (float)(sprite.getGlobalBounds().top - 20)));
		label.setString(to_string(hp) + "HP");
		window.draw(label);
		window.draw(sprite);
	}

	void jump() {
		if (!inAir) {
			yv -= instantSpeedY;
			timeInAir = 25;
		}
	}

	void run() {
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

	bool hurt(Hitbox * hurter) {
		if (invulnerable > 0 || hp <= 0) {
			return false;
		}
		hp -= 1;
		if (hp <= 0) {
			die();
		}
		else {
			invulnerable = 200;
		}
		return true;
	}
};
vector<Entity> enemies;

bool Surface::intersectsTop(Entity* character) {
	RectangleShape fake(Vector2f((float)(character->sprite.getGlobalBounds().width), (float)(character->maxSpeedY)));
	fake.setPosition(Vector2f((float)(character->sprite.getGlobalBounds().left), (float)(character->sprite.getGlobalBounds().top - character->maxSpeedY)));
	return rect.getGlobalBounds().intersects(fake.getGlobalBounds());
}

bool Surface::intersectsBottom(Entity* character) {
	RectangleShape fake(Vector2f((float)(character->sprite.getGlobalBounds().width), (float)(character->maxSpeedY)));
	fake.setPosition(Vector2f((float)(character->sprite.getGlobalBounds().left), (float)(character->sprite.getGlobalBounds().top + character->sprite.getGlobalBounds().height)));
	return rect.getGlobalBounds().intersects(fake.getGlobalBounds());
}

bool Surface::intersectsLeft(Entity* character) {
	RectangleShape fake(Vector2f((float)(character->maxSpeedX), (float)(character->sprite.getGlobalBounds().height)));
	fake.setPosition(Vector2f((float)(character->sprite.getGlobalBounds().left - character->maxSpeedX), (float)(character->sprite.getGlobalBounds().top)));
	return rect.getGlobalBounds().intersects(fake.getGlobalBounds());
}

bool Surface::intersectsRight(Entity* character) {
	RectangleShape fake(Vector2f((float)(character->maxSpeedX), (float)(character->sprite.getGlobalBounds().height)));
	fake.setPosition(Vector2f((float)(character->sprite.getGlobalBounds().left + character->sprite.getGlobalBounds().width), (float)(character->sprite.getGlobalBounds().top)));
	return rect.getGlobalBounds().intersects(fake.getGlobalBounds());
}

set<string> play;
set<string> mook;
set<string> mook_turn;

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

	surfaces.push_back(Surface(Vector2f(0.f, 0.f), Vector2f(400.f, 30.f)));
	surfaces.push_back(Surface(Vector2f(0.f, 800.f), Vector2f(400.f, 50.f)));
	surfaces.push_back(Surface(Vector2f(500.f, 800.f), Vector2f(1400.f, 50.f)));
	surfaces.push_back(Surface(Vector2f(800.f, 710.f), Vector2f(200.f, 100.f)));
	surfaces.push_back(Surface(Vector2f(1400.f, 600.f), Vector2f(200.f, 150.f)));
	surfaces.push_back(Surface(Vector2f(1750.f, 495.f), Vector2f(400.f, 20.f)));

	surfaces.push_back(Surface(Vector2f(2300.f, 480.f), Vector2f(40.f, 200.f)));
	surfaces.push_back(Surface(Vector2f(2340.f, 600.f), Vector2f(300.f, 80.f)));
	surfaces.push_back(Surface(Vector2f(2640.f, 550.f), Vector2f(40.f, 130.f)));

	surfaces.push_back(Surface(Vector2f(3400.f, -600.f), Vector2f(60.f, 1350.f)));
	surfaces.push_back(Surface(Vector2f(3350.f, 900.f), Vector2f(850.f, 600.f)));
	surfaces.push_back(Surface(Vector2f(4200.f, -600.f), Vector2f(60.f, 690.f)));
	surfaces.push_back(Surface(Vector2f(3460.f, 630.f), Vector2f(620.f, 40.f)));
	surfaces.push_back(Surface(Vector2f(3630.f, 325.f), Vector2f(620.f, 40.f)));
	surfaces.push_back(Surface(Vector2f(4200.f, 300.f), Vector2f(60.f, 1200.f)));

	// ENEMY TYPES //
	play.insert("playable");
	mook_turn.insert("turn_at_corner");

	Entity monster = Entity("assets/lizardmodel.png", Vector2f(60.f, 500.f), Vector2f((float)(.04), (float)(.05)), vector<int>({ 3,2,3 }), play);
	enemies.push_back(Entity("assets/met13.png", Vector2f(1400.f, 500.f), Vector2f((float)(.6), (float)(.75)), vector<int>({ 1,2,3 }), mook));
	enemies.push_back(Entity("assets/met93.png", Vector2f(1800.f, 450.f), Vector2f((float)(.6), (float)(.75)), vector<int>({ 2,2,3 }), mook_turn));
	enemies.push_back(Entity("assets/met13.png", Vector2f(2360.f, 550.f), Vector2f((float)(.6), (float)(.75)), vector<int>({ 1,2,3 }), mook));
	enemies.push_back(Entity("assets/met93.png", Vector2f(3490.f, 590.f), Vector2f((float)(.6), (float)(.75)), vector<int>({ 2,2,3 }), mook_turn));
	enemies.push_back(Entity("assets/met93.png", Vector2f(3650.f, 290.f), Vector2f((float)(.6), (float)(.75)), vector<int>({ 2,2,3 }), mook_turn));

	RenderWindow renderWindow;
	renderWindow.create(VideoMode(rwindowx, rwindowy), "Platformer"/*, Style::Fullscreen*/);

	while (renderWindow.isOpen()) {
		frame++;

		renderWindow.clear(Color(150,170,200,255));

		hitboxes = vector<Hitbox>();
		view.setCenter(max(view.getSize().x / 2 - 50, monster.sprite.getPosition().x), 500.f);
		renderWindow.setView(view);
		if (monster.y > view.getCenter().y + view.getSize().y / 2 + monster.sprite.getGlobalBounds().height * 2) {
			monster.die();
		}
		for (int i = 0; i < enemies.size(); i++) {
			if (enemies[i].alive) {
				enemies[i].update(renderWindow);
			}
		}
		for (int i = 0; i < surfaces.size(); i++) {
			renderWindow.draw(surfaces[i].rect);
		}
		text.setString(to_string(monster.invulnerable));
		renderWindow.draw(text);

		if (monster.alive) {
			monster.update(renderWindow);
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
			//renderWindow.draw(hitboxes[i].rect);
			if (!hitboxes[i].against && monster.sprite.getGlobalBounds().intersects(hitboxes[i].rect.getGlobalBounds())) {
				monster.hurt(&hitboxes[i]);
			}
			if (hitboxes[i].against) {
				for (int e = 0; e < enemies.size(); e++) {
					if (enemies[e].sprite.getGlobalBounds().intersects(hitboxes[i].rect.getGlobalBounds())) {
						bool hit = enemies[e].hurt(&hitboxes[i]);
						if (hit && includes(hitboxes[i].attributes, "jump")) {
							hitboxes[i].origin->yv = -hitboxes[i].origin->instantSpeedY;
							hitboxes[i].origin->timeInAir = 25;
						}
						if (hit && includes(hitboxes[i].attributes, "headhit")) {
							enemies[e].yv = -hitboxes[i].origin->instantSpeedY;
						}
					}
				}
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