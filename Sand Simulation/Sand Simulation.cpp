#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

enum block {
	none,
	sand,
	dirt,
	water,
	last
};

struct ParticleData {
	olc::Pixel col;
	std::string name;
	int density = 0;
	int gravity = 0;
	int spread = 0;
};

std::array<ParticleData, last> dataMap{
  ParticleData{ olc::Pixel(137, 207, 240), "None", 0, 0, 0 },
  ParticleData{ olc::YELLOW, "Sand", 2, 10, 1 },
  ParticleData{ olc::DARK_GREY, "Dirt", 2, 0, 0 },
  ParticleData{ olc::BLUE, "Water", 1, 10, 5 },
};

struct Particle {
	olc::vi2d vel;
	block type;

	Particle() : Particle(none) { }
	Particle(block _type) {
		reset(_type);
	}

	bool isEmpty() {
		return type == none;
	}
	void reset(block _type = none) {
		type = _type;
		vel = { 0, 0 };
	}
};

struct Map {
	Particle* map;
	Particle* mapCpy;
	int w, h, size;

	int cnt = 0, cntNone = 0;

	Map(int _w, int _h) {
		w = _w;
		h = _h;
		size = w * h * sizeof(Particle);

		map = new Particle[w * h];
		mapCpy = new Particle[w * h];
		memset(map, none, size);
	}

	bool isEmptyCpy(int x, int y) {
		if (!inside(x, y))
			return false;
		return /*map[x + w * y].isEmpty() && */mapCpy[x + w * y].isEmpty(); // TODO correct?
	}
	bool isEmpty(int x, int y) {
		if (!inside(x, y))
			return false;
		return map[x + w * y].isEmpty() && mapCpy[x + w * y].isEmpty();
	}
	bool isLessDense(int mx, int my, int ox, int oy) {
		return dataMap[get(mx, my).type].density > dataMap[get(ox, oy).type].density;
	}
	void swap(int mx, int my, int ox, int oy) {
		Particle temp = get(mx, my);
		setCpy(mx, my, get(ox, oy));
		setCpy(ox, oy, temp);
		//set(ox, oy, temp); // TODO correct?
	}
	bool swapIfLessDense(const olc::vi2d& s, const olc::vi2d& o) {
		if (isLessDense(s.x, s.y, o.x, o.y)) {
			swap(s.x, s.y, o.x, o.y);
			return true;
		}
		return false;
	}
	Particle get(int x, int y) {
		if (!inside(x, y))
			return none;
		return map[x + w * y];
	}
	Particle getCpy(int x, int y) {
		if (!inside(x, y))
			return none;
		return mapCpy[x + w * y];
	}
	void setCircle(int px, int py, int r, const Particle& p) {
		for (int y = -r; y <= r; y++)
			for (int x = -r; x <= r; x++)
				if (x * x + y * y <= r * r + r)
					if ((p.type != none && rand() % 10 > 8) || get(px + x, py + y).type == dirt)
						set(px + x, py + y, p);
	}
	void set(int x, int y, const Particle& p) {
		if (!inside(x, y))
			return;
		map[x + w * y] = p;

		if (p.type == none) //TODO
			cntNone++;
		else
			cnt++;
	}
	void setCpy(int x, int y, const Particle& p) {
		if (!inside(x, y))
			return;
		mapCpy[x + w * y] = p;

		if (p.type == none) //TODO
			cntNone++;
		else
			cnt++;
	}
	bool inside(int x, int y) {
		return !(x < 0 || y < 0 || x >= w || y >= h);
	}
	void copyMap() {

		cnt = 0; //TODO
		cntNone = 0;

		memcpy(mapCpy, map, size);
	}
	void saveMapCopy() {

		if (abs(cnt - cntNone) > 0) //TODO
			std::cout << cnt - cntNone << " diff" << std::endl;

		memcpy(map, mapCpy, size);
	}
};

class SandSimulation : public olc::PixelGameEngine
{
public:
	SandSimulation()
	{
		sAppName = "Sand Simulation";
	}

	Map* map = nullptr;
	int radius = 10;
	bool run = false;
	block blockToPlace = sand;

public:
	bool OnUserCreate() override
	{
		map = new Map(ScreenWidth(), ScreenHeight());
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		Input();
		Update(fElapsedTime);
		Render();

		return true;
	}

	void Input() {

		if (GetKey(olc::UP).bPressed)
			blockToPlace = (block)((blockToPlace + 1) % last);
		if (GetKey(olc::DOWN).bPressed)
			blockToPlace = (block)((blockToPlace - 1) % last);

		if (GetMouse(0).bPressed || GetMouse(0).bHeld)
			map->setCircle(GetMouseX(), GetMouseY(), radius, blockToPlace);
		if (GetMouse(1).bPressed || GetMouse(1).bHeld)
			map->setCircle(GetMouseX(), GetMouseY(), radius, none);
	}

	void Update(float dt) {

		run = !run;

		map->copyMap();

		for (int y = ScreenHeight() - 1; y >= 0; y--)
			for (int x = run ? 0 : ScreenWidth() - 1; run ? x < ScreenWidth() : x >= 0; run ? x++ : x--)
				switch (map->get(x, y).type)
				{
				case none: break;
				case sand: updateSand(x, y); break;
				case dirt: break;
				case water: updateWater(x, y); break;
				default: break;
				}

		map->saveMapCopy();
	}

	void Render() {

		for (int y = 0; y < ScreenHeight(); y++)
			for (int x = 0; x < ScreenWidth(); x++)
				Draw(x, y, dataMap[map->get(x, y).type].col);

		DrawString(5, 5, "Selected block is: " + dataMap[blockToPlace].name);
	}

	olc::vi2d updateGravity(Particle& current, int x, int y) {
		current.vel.y = clamp(current.vel.y + (dataMap[current.type].gravity), -10, 10);
		current.vel.x += dataMap[current.type].spread;
		if (!map->isLessDense(x, y, x, y + 1))
			current.vel.y /= 2;
		return current.vel;
	}

	void updateSand(int x, int y) {
		if (y == ScreenHeight() - 1) 
			return;

		int dir = rand() % 2 ? 1 : -1;

		if (map->swapIfLessDense({ x, y }, { x, y + 1 }))
			;
		else if (!map->swapIfLessDense({ x, y }, { x + dir, y + 1 }))
			;
		else if (!map->swapIfLessDense({ x, y }, { x - dir, y + 1 }))
			;
	}

	void updateWater(int x, int y) {
		if (y == ScreenHeight() - 1)
			return;

		/*int spread = 10;
		
		auto godown = [&](int nx, int d) {
			bool ret = false;
			int ny = y;
			for (int o = 1; o <= d; o++)
				if (map->isEmptyCpy(nx, y + o)) {
					ny = y + o;
					ret = true;
				}
				else
					break;
			if (ret)
				map->setCpy(nx, ny, map->get(x, y));
			return ret;
		};
		
		auto tryout = [&](int nx, int d) {
			if (map->isEmptyCpy(nx, y)) {
				if (map->isEmptyCpy(nx, y + 1)) {
					if (godown(nx, d))
						return true;
				}
				else {
					map->setCpy(nx, y, map->get(x, y));
					return true;
				}
			}
			return false;
		};
		
		int dir = rand() % 2 ? 1 : -1;
		for (int i = 0; i <= spread; i++) 
			if (tryout(x - i * dir, spread - i) || tryout(x + i * dir, spread - i)) {
				map->setCpy(x, y, none);
				return;
			}
		
		return;*/

		int spread = 10;

		auto godown = [&](int nx, int d) {
			bool ret = false;
			int ny = y;
			for (int o = 1; o <= d; o++)
				if (map->isEmptyCpy(nx, y + o)) {
					ny = y + o;
					ret = true;
				}
				else
					break;
			if (ret)
				map->setCpy(nx, ny, map->get(x, y));
			return ret;
		};

		auto tryout = [&](int nx, int d) {
			if (map->isEmptyCpy(nx, y)) {
				if (map->isEmptyCpy(nx, y + 1)) {
					if (godown(nx, d))
						return true;
				}
				else {
					map->setCpy(nx, y, map->get(x, y));
					return true;
				}
			}
			return false;
		};

		int dir = rand() % 2 ? 1 : -1;
		for (int i = 0; i <= spread; i++)
			if (tryout(x - i * dir, spread - i) || tryout(x + i * dir, spread - i)) {
				map->setCpy(x, y, none);
				return;
			}

		return;

		//if (map->isEmpty(x - 1, y)) {
		//	if (map->isEmpty(x - 1, y + 1)) {
		//		if (!godown(x - 1, 4))

		//			if (map->isEmpty(x - 2, y)) {
		//				if (map->isEmpty(x - 2, y + 1)) {
		//					if (!godown(x - 2, 3))
		//						;
		//					else
		//						return;
		//				}
		//				else
		//					map->swap(x, y, x - 2, y);
		//			}


		//		else
		//			return;
		//	}
		//	else
		//		map->swap(x, y, x - 1, y);
		//}


		////Particle current = map->get(x, y);
		////olc::vi2d maxOffset = updateGravity(current, x, y);
		////
		////std::cout << maxOffset.x << "X " << maxOffset.y << "Y " << std::endl;
		////
		////if (map->isEmpty(x, y + 1))
		////	for (int oy = 1; oy < maxOffset.y; oy++)
		////		if (map->isEmpty(x, y + oy)) {
		////			map->setCpy(x, y + oy - 1, none);
		////			map->setCpy(x, y + oy, current);
		////		}
		////		else
		////			break;
		////else {
		////	bool canR = true, canL = true;
		////	bool L = 0, R = 0;
		////	int dir = run ? 1 : -1;
		////
		////	for (int ox = 1; ox < maxOffset.x; ox++) {
		////
		////		if (!canR || !map->isEmpty(x + (dir * ox), y))
		////			canR = false;
		////		else
		////			R++;
		////		if (!canL || !map->isEmpty(x + (-dir * ox), y))
		////			canL = false;
		////		else
		////			L++;
		////
		////		if (!canR && !canL) {
		////			int off = (R > L) ? R * dir : -dir * L;
		////
		////			current.vel.x = min(current.vel.x, max(L, R));
		////
		////			map->swap(x, y, x + off, y);
		////			//map->setCpy(x, y, none);
		////			//map->setCpy(x + off, y, current);
		////		}
		////	}
		////
		////}
		////
		////return;

		///*Particle current = map->get(x, y);

		//if (map->isEmpty(x, y + 1))
		//	map->setCpy(x, y + 1, current);

		//else {
		//	bool found = false;
		//	bool canL = true, canR = true;
		//	for (int off = 1; off < 10; off++) {
		//		int dir = (run ? 1 : -1) * off;

		//		if (map->isEmpty(x + dir, y + 1)) {
		//			map->setCpy(x + dir, y + 1, current);
		//			found = true;
		//			break;
		//		}
		//		else


		//		else if (map->isEmpty(x - dir, y + 1) && canL) {
		//			map->setCpy(x - dir, y + 1, current);
		//			found = true;
		//			break;
		//		}
		//	}
		//	if (!found)
		//		for (int off = 1; off < 50; off++) {
		//			int dir = (run ? 1 : -1) * off;

		//			if (map->isEmpty(x + dir, y)) {
		//				map->setCpy(x + dir, y, current);
		//				found = true;
		//				break;
		//			}

		//			else if (map->isEmpty(x - dir, y)) {
		//				map->setCpy(x - dir, y, current);
		//				found = true;
		//				break;
		//			}
		//		}
		//	if (!found)
		//		return;
		//}		

		//map->setCpy(x, y, none);*/

		//Particle current = map->get(x, y);
		//
		//int dir = run ? 1 : -1;
		//
		//if (map->isEmptyCpy(x, y + 1))
		//	map->setCpy(x, y + 1, current);
		//
		//else if (map->isEmptyCpy(x + dir, y + 1))
		//	map->setCpy(x + dir, y + 1, current);
		//
		//else if (map->isEmptyCpy(x - dir, y + 1))
		//	map->setCpy(x - dir, y + 1, current);
		//
		//else if (map->isEmptyCpy(x + dir, y))
		//	map->setCpy(x + dir, y, current);
		//
		//else if (map->isEmptyCpy(x - dir, y))
		//	map->setCpy(x - dir, y, current);
		//
		//else
		//	return;
		//
		//
		//map->setCpy(x, y, none);
	}

	int clamp(int v, int l, int h) {
		if (h < l) std::swap(l, h);
		return max(min(v, h), l);
	}
}; 

int main()
{
	SandSimulation app;
	if (app.Construct(500, 300, 2, 2, false, true))
		app.Start();
	return 0;
}