#include <SFML/Graphics.hpp>
#include <iostream>
#include <sstream>
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

double gravity = .1;

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
	RectangleShape rect;
	bool against;

	Hitbox(Vector2f location, Vector2f size, bool againstEnemy) {
		rect.setPosition(location);
		rect.setSize(size);
		rect.setFillColor(Color(0, 0, 0, 150));
		against = againstEnemy;
	}
};

struct Entity {
	Texture texture;
	Sprite sprite;

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

	bool right;
	bool left;
	bool up;

	bool playable;
	bool alive = true;
	int hp;
	string filename;
	vector<int> stats;
	double maxSpeedX = 6;
	double maxSpeedY = 5;
	double instantSpeedX = 2;
	double holdSpeedX = .04;
	double instantSpeedY = 6;
	double holdSpeedY = .06;

	Entity(string title, Vector2f position, Vector2f size, vector<int> statistics, bool player) {
		filename = title;
		texture.loadFromFile(title);
		texture.setSmooth(true);
		sprite.setPosition(position);
		sprite.setTexture(texture);
		sprite.setOrigin(Vector2f(texture.getSize().x / 2, texture.getSize().y / 2));

		x = position.x;
		y = position.y;
		
		width = size.x;
		height = size.y;

		playable = player;
		stats = statistics;
		hp = stats[HP];
	}

	void movement() {
		if (playable) {
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
		alive = false;
	}

	Sprite update() {
		movement();

		inAir = true;
		collideRight = false;
		collideLeft = false;
		for (int i = 0; i < surfaces.size(); i++) {
			bool exit = false;
			if (surfaces[i].intersectsTop(this)) {
				yv = max(0.0, yv);
				//exit = true;
			}
			if (surfaces[i].intersectsBottom(this)) {
				inAir = false;
				yv = min(0.0, yv);
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
			yv -= .05 + holdSpeedY;
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
		sprite.setPosition(x, y);

		if (forwards) {
			sprite.setScale(Vector2f(width, height));
		}
		else {
			sprite.setScale(Vector2f(-width, height));
		}

		if (frame % 100 == 0) {
			cout << " pos: (" + to_string(x) + "," + to_string(y) + ") v: (" + to_string(xv) + "," + to_string(yv) + ")" << endl;
		}

		if (!playable) {
			hitboxes.push_back(Hitbox(Vector2f((float)(sprite.getGlobalBounds().left), (float)(sprite.getGlobalBounds().top)), 
				Vector2f(sprite.getGlobalBounds().width, sprite.getGlobalBounds().height), false));
		}

		sprite.setTexture(texture);
		return sprite;
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
		hp -= 1;
		return true;
	}
};
vector<Entity> enemies;

bool Surface::intersectsTop(Entity* character) {
	RectangleShape fake(Vector2f(character->sprite.getGlobalBounds().width, character->maxSpeedY));
	fake.setPosition(Vector2f(character->sprite.getGlobalBounds().left, character->sprite.getGlobalBounds().top - character->maxSpeedY));
	return rect.getGlobalBounds().intersects(fake.getGlobalBounds());
}

bool Surface::intersectsBottom(Entity* character) {
	RectangleShape fake(Vector2f(character->sprite.getGlobalBounds().width, character->maxSpeedY));
	fake.setPosition(Vector2f(character->sprite.getGlobalBounds().left, character->sprite.getGlobalBounds().top + character->sprite.getGlobalBounds().height));
	return rect.getGlobalBounds().intersects(fake.getGlobalBounds());
}

bool Surface::intersectsLeft(Entity* character) {
	RectangleShape fake(Vector2f(character->maxSpeedX, character->sprite.getGlobalBounds().height));
	fake.setPosition(Vector2f(character->sprite.getGlobalBounds().left - character->maxSpeedX, character->sprite.getGlobalBounds().top));
	return rect.getGlobalBounds().intersects(fake.getGlobalBounds());
}

bool Surface::intersectsRight(Entity* character) {
	RectangleShape fake(Vector2f(character->maxSpeedX, character->sprite.getGlobalBounds().height));
	fake.setPosition(Vector2f(character->sprite.getGlobalBounds().left + character->sprite.getGlobalBounds().width, character->sprite.getGlobalBounds().top));
	return rect.getGlobalBounds().intersects(fake.getGlobalBounds());
}

int main() {

	arial.loadFromFile("Arial.ttf");
	sf::Text text;
	text.setFont(arial);
	text.setCharacterSize(24); 
	text.setFillColor(sf::Color::White);
	text.setStyle(sf::Text::Bold);
	text.setPosition(Vector2f(500.f, 200.f));
	view.setSize(Vector2f(2000.f, 1400.f));

	surfaces.push_back(Surface(Vector2f(0.f, 0.f), Vector2f(400.f, 30.f)));
	surfaces.push_back(Surface(Vector2f(0.f, 800.f), Vector2f(400.f, 50.f)));
	surfaces.push_back(Surface(Vector2f(500.f, 800.f), Vector2f(1400.f, 50.f)));
	surfaces.push_back(Surface(Vector2f(800.f, 710.f), Vector2f(200.f, 100.f)));
	surfaces.push_back(Surface(Vector2f(1300.f, 630.f), Vector2f(200.f, 150.f)));
	surfaces.push_back(Surface(Vector2f(1700.f, 500.f), Vector2f(400.f, 20.f)));

	Entity monster = Entity("assets/lizardmodel.png", Vector2f(10.f, 500.f), Vector2f(.04, .05), vector<int>({ 1,2,3 }), true);
	enemies.push_back(Entity("assets/met13.png", Vector2f(1400.f, 500.f), Vector2f(.6, .75), vector<int>({ 1,2,3 }), false));

	RenderWindow renderWindow;
	renderWindow.create(VideoMode(rwindowx, rwindowy), "Platformer"/*, Style::Fullscreen*/);

	while (renderWindow.isOpen()) {
		frame++;

		renderWindow.clear(Color(150,170,200,255));

		hitboxes = vector<Hitbox>();
		view.setCenter(max(10.f, monster.sprite.getPosition().x), 500.f);
		renderWindow.setView(view);
		if (monster.alive) {
			renderWindow.draw(monster.update());
		}
		for (int i = 0; i < enemies.size(); i++) {
			if (enemies[i].alive) {
				renderWindow.draw(enemies[i].update());
			}
		}
		for (int i = 0; i < surfaces.size(); i++) {
			renderWindow.draw(surfaces[i].rect);
		}
		text.setString(to_string(monster.xv));
		renderWindow.draw(text);
		for (int i = 0; i < hitboxes.size(); i++) {
			renderWindow.draw(hitboxes[i].rect);
			if (!hitboxes[i].against && monster.sprite.getGlobalBounds().intersects(hitboxes[i].rect.getGlobalBounds())) {
				monster.hurt(&hitboxes[i]);
			}
			if (hitboxes[i].against) {
				for (int e = 0; e < enemies.size(); e++) {
					if (enemies[e].sprite.getGlobalBounds().intersects(hitboxes[i].rect.getGlobalBounds())) {
						enemies[e].hurt(&hitboxes[i]);
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