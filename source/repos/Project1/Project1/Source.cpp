#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <ctime>
#include <vector>
#include <cmath>

using namespace std;
using namespace sf;

Event event;
int rwindowx = VideoMode::getDesktopMode().width;
int rwindowy = VideoMode::getDesktopMode().height;
const double pi = 3.141592;

enum species { LIZARD, BIRD, MAMMAL, AMPHIBIAN };
enum stats {HP, RUNSPD, JUMPH};

struct Entity;
struct Surface;
vector<Surface> surfaces;

struct Surface {
	RectangleShape rect;

	Surface(Vector2f location, Vector2f size) {
		rect = RectangleShape(location);
		rect.setSize(size);
		rect.setFillColor(Color(0, 0, 0, 255));
	}

	bool intersects(Entity * character);
};

struct Entity {
	Texture texture;
	Sprite sprite;

	double x = 0;
	double y = 0;
	double xv = 0;
	double yv = 0;

	bool inAir = false;
	int timeInAir;

	int hp;
	vector<int> stats;

	Entity(Vector2f position, vector<int> statistics) {
		texture.loadFromFile("assets/met13.png");
		sprite.setPosition(position);
		sprite.setTexture(texture);

		stats = statistics;
	}

	void update() {
		inAir = true;
		for (int i = 0; i < surfaces.size(); i++) {
			if (surfaces[i].intersects(this)) {
				inAir = false;
				yv = 0;
			}
		}

		if (timeInAir > 0) {
			timeInAir--;
		}
		if (yv > -15 && inAir) {
			yv += .1;
		}

		x += xv;
		y += yv;
		sprite.move(x, y);
	}

	void jump() {
		if (!inAir || timeInAir > 0) {
			yv = -1;
		}
		if (!inAir) {
			timeInAir = 50;
		}
	}
};

bool Surface::intersects(Entity* character){
	return rect.getGlobalBounds().intersects(character->sprite.getGlobalBounds());
}

int main() {
	surfaces.push_back(Surface(Vector2f(5.f, 5.f), Vector2f(10.f, 20.f)));

	Entity monster = Entity(Vector2f(10.f, 50.f), vector<int>({ 1,2,3 }));

	RenderWindow renderWindow;
	renderWindow.create(VideoMode(rwindowx, rwindowy), "Platformer"/*, Style::Fullscreen*/);

	while (renderWindow.isOpen()) {
		renderWindow.clear(Color(150,170,200,255));
		monster.update();
		renderWindow.draw(monster.sprite);
		for (int i = 0; i < surfaces.size(); i++) {
			renderWindow.draw(surfaces[i].rect);
		}
		renderWindow.display();
		while (renderWindow.pollEvent(event)) {
			switch (event.type) {
			case Event::KeyPressed:
				switch (event.key.code) {
					case Keyboard::Escape:	renderWindow.close(); break;
					case Keyboard::Up: monster.jump(); break;
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