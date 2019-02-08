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

double jump(int height, int cframe, int tframe) {
	return height * sin((cframe / tframe) * pi);
}
enum species { LIZARD, BIRD, MAMMAL, AMPHIBIAN };
enum stats {HP, RUNSPD, JUMPH};



int speciesStats[4][3] = { 
	{1,2,3}, //LIZARD
	{1,2,3}, //BIRD
	{1,2,3}, //MAMMAL
	{1,2,3} //AMPHIBIAN
};



struct Character {
	int x;
	int y;
	Texture texture;
	Sprite sprite;
	bool inAir = false;
	int hp;
	int maxHp;
	void setUp() {

	}


};

void main() {
	Texture chara;
	IntRect charaselect(0, 0, 500, 500);
	Sprite character(chara, charaselect);
	chara.loadFromFile("lizardmodel.png");
	character.setTexture(chara);
	RenderWindow renderWindow;
	renderWindow.create(VideoMode(rwindowx, rwindowy), "Platformer"/*, Style::Fullscreen*/);



	while (renderWindow.isOpen()) {
		character.move(1, 0);
		renderWindow.clear(Color(25,25,25,255));
		renderWindow.draw(character);
		renderWindow.display();
		while (renderWindow.pollEvent(event)) {
			switch (event.type) {
			case Keyboard::Escape:
				renderWindow.close();
				break;
			case Event::EventType::Closed:
				renderWindow.close();
				break;
			}
		
		
		}
	}



	return;
}