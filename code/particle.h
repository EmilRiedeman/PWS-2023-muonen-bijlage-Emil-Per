#pragma once

#include <vector>
#include <fstream>
#include <iostream>
#include <chrono>
#include <array>
#include <algorithm>
#include <memory>

constexpr int WIDTH = 256;
constexpr int HEIGHT = 256;
constexpr int AREA = WIDTH * HEIGHT;
using CANVAS = std::array<int, AREA>;

struct Particle {
	typedef std::chrono::system_clock sys_clock;

	sys_clock::time_point time_point = sys_clock::now();
	std::vector<int> data{};
	int width{};
	int height{};
	int start_x{};
	int start_y{};

	Particle(const CANVAS &canvas, const std::vector<int> &included, int from, int width, int height) : data(width * height, 0), width(width), height(height), start_x(from % WIDTH), start_y(from / WIDTH) {
		int i_index = 0;
		for (int y = 0; y < height; ++y) {
			int h_from = from + y * WIDTH;
			for (int x = 0; x < width; ++x) {
				int c_index = h_from + x;
				if (i_index < (int)included.size() && included[i_index] == c_index) {
					data[y * width + x] = canvas[c_index];
					++i_index;
				}
			}
		}
	}

	Particle() = default;

	int area() const {
		return width * height;
	}

	bool touches_border() const {
		return start_x == 0 || start_y == 0 || start_x + width >= WIDTH || start_y + height >= HEIGHT;
	}

	void imprint_on_canvas(CANVAS &canvas) const {
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				auto &c_i = canvas[start_x + x + (start_y + y) * WIDTH];
				c_i = std::max(c_i, data[y * width + x]);
			}
		}
	}

	template <typename T>
	void save_to_file(T &file) const {
		std::vector<std::pair<int, int>> zeros;
		std::vector<int> included;
		int z_count = 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int index = y * width + x;
				if (data[index]) {
					if (z_count) zeros.emplace_back(index - z_count, z_count);
					z_count = 0;
					included.push_back(data[index]);
				} else ++z_count;
			}
		}

		if (z_count) zeros.emplace_back(width * height - z_count, z_count);

		auto write = [&file](int x) {
			//std::cout << x << '\n';
			file.seekp(0, std::ios_base::end);
			auto y = (uint16_t)x;
			file.write(reinterpret_cast<char*>(&y), 2);
		};

		file.seekp(0, std::ios_base::end);
		auto t = time_point.time_since_epoch().count();
		file.write(reinterpret_cast<char*>(&t), sizeof t);

		write(start_x);
		write(start_y);

		write(width);
		write(height);

		write(zeros.size());
		for (auto &x : zeros) {
			write(x.first);
			write(x.second);
		}

		for (auto x : data) {
			if (x) write(x);
		}
	}

	static Particle read_from_file(std::ifstream &file) {
		auto next = [&file]() {
			uint16_t x;
			file.read(reinterpret_cast<char*>(&x), sizeof(x));
			//std::cout << x << '\n';
			return (int)x;
		};

		Particle result{};
		sys_clock::rep t;
		file.read(reinterpret_cast<char*>(&t), sizeof(t));
		result.time_point = sys_clock::time_point{sys_clock::duration{t}};

		result.start_x = next();
		result.start_y = next();
		result.width = next();
		result.height = next();

		std::vector<std::pair<int, int>> zeros(next());
		for (auto &x : zeros) {
			x.first = next();
			x.second = next();
		}

		result.data.assign(result.width * result.height, 0);

		int z = 0;
		for (int i = 0; i < result.width * result.height;) {
			if (z < (int)zeros.size() && zeros[z].first == i) i += zeros[z++].second;
			else result.data[i++] = next();
		}
		return result;
	}

	void print() const {
		auto sorted = data;
		std::sort(sorted.begin(), sorted.end());
		std::cout << "---\n";
		std::vector<int> delta;
		int min_v = 0, max_v = sorted.back();
		int prev{};
		for (auto x : sorted) {
			if (x) std::cout << x << ';';
			if (!prev && x) min_v = x;
			prev = x;
		}

		auto t = sys_clock::to_time_t(time_point);
		std::cout << "\n" <<
			"time: " << std::ctime(&t) <<
			"start pos: (" << start_x << ',' << start_y << "); " <<
			"size: " << width << 'x' << height << "; " <<
			"min value: " << min_v << "; " <<
			"max value: " << max_v << "; " <<
			"\n";

		constexpr const char SHADE[] = " .:-=+*#%@";
		int id = 0;
		std::cout << "---\n";
		std::cout << '+' << std::string(width, '-') << "+\n";
		for (int i = 0; i < height; ++i) {
			std::cout << '|';
			for (int j = 0; j < width; ++j, ++id) {
				char c = SHADE[std::min((data[id] * (sizeof(SHADE) - 1) + max_v - 1) / max_v, sizeof(SHADE) - 2)];
				std::cout << c;
			}
			std::cout << "|\n";
		}
		std::cout << '+' << std::string(width, '-') << "+\n";
	}
};


inline CANVAS read_canvas(const std::string &file_name) {
	std::ifstream file;
	file.open(file_name.data());
	CANVAS arr;
	for (int i = 0; i < AREA; ++i) {
		file >> arr[i];
	}
	file.close();
	arr[(91-1) * WIDTH + 70-1] = 0;
	return arr;
}

inline void write_canvas_to_file(const CANVAS &c, const char *dst) {
	std::ofstream f(dst);
	int i = 0;
	for (int y = 0; y < HEIGHT; ++y) {
		for (int x = 0; x < WIDTH; ++x) f << c[i++] << (x + 1 == WIDTH? '\n': ' ');
	}
}

template <typename T, typename Function>
inline CANVAS convert_to_canvas(const std::vector<T> &vec, Function f) {
	CANVAS c{};

	for (const auto &t : vec) {
		auto x = f(t);
		if (x) x->imprint_on_canvas(c);
	}

	return c;
}

template <typename T>
uint32_t get_batch_size(T &file) {
	uint32_t n;
	file.seekg(0, std::ios_base::beg);
	file.read(reinterpret_cast<char*>(&n), sizeof(n));
	return n;
}

template <typename T>
void set_batch_size(T &file, uint32_t x) {
	file.seekp(0, std::ios_base::beg);
	file.write(reinterpret_cast<char*>(&x), sizeof(x));
}

void new_batch_file(const char *file_name);

void save_batch(const char *file_name, const std::vector<Particle> &particles);

void append_batch(const char *destination, const char *source);

template <typename Tuple = std::tuple<Particle>, typename Function>
inline std::vector<Tuple> read_batch_filtered(const char *file_name, Function f) {
	std::ifstream file;
	file.open(file_name, std::ios_base::in | std::ios_base::binary);

	uint32_t n = get_batch_size(file);

	std::cerr << file_name << " contains " << n << " particles\n";

	std::vector<Tuple> result;
	result.reserve(n);
	for (uint32_t i = 0; i < n; ++i) {
		result.push_back(Tuple());
		std::get<0>(result.back()) = Particle::read_from_file(file);
		if (!f(result.back())) result.pop_back();
	}

	return result;
}