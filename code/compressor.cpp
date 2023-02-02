/*

	Compileer met: 
	g++ -Wall -O2 -std=c++17 -o compressor compressor.cpp particle.cpp

	Leeg bestand maken:
	compressor new [bestand]

	Deeltjes uit rauwe data bestanden toevoegen aan gecomprimeerde bestand:
	compressor batch [gecomprimeerd] [data betand beginletters] [aantal] (cijfers)

	Deeltjes uit rauwe data bestanden toevoegen aan gecomprimeerde bestand tijdens meting en rauw verwijderen:
	compressor auto [gecomprimeerd] [data betand beginletters] [aantal cijfers] [tijdsinterval bestand check in ms] [maximale wachttijd in ms]

	Bestanden toevoegen aan 1 groot bestand
	compressor append [bestemming] {bestanden...}
*/

#include "particle.h"

#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <memory>

decltype(auto) find_particles(const std::string &canvas_file) {
	constexpr int VISIT[] = {-1, 1, -WIDTH, WIDTH, -WIDTH-1, -WIDTH+1, WIDTH-1, WIDTH+1};
	std::vector<Particle> r;

	auto arr = read_canvas(canvas_file);

	bool visited[AREA]{};
	for (int i = 0; i < AREA; ++i) {
		if (arr[i] && !visited[i]) {
			int xmin = WIDTH, ymin = HEIGHT, xmax = -1, ymax = -1;
			std::vector<int> next{i};
			std::vector<int> included;
			while (!next.empty()) {
				int b = next.back();
				next.pop_back();

				if (b >= 0 && b < AREA && arr[b] && !visited[b]) {
					included.push_back(b);

					visited[b] = true;
					int x = b % WIDTH;
					int y = b / HEIGHT;
					xmin = std::min(x, xmin);
					ymin = std::min(y, ymin);
					xmax = std::max(x, xmax);
					ymax = std::max(y, ymax);

					for (int v : VISIT) {
						if (std::abs(v) % WIDTH != 0) {
							int nx = (x + v) % WIDTH;
							if (nx < 0 || nx >= WIDTH) {
								continue;
							}
						}
						next.push_back(b + v);
					}
				}
			}
			std::sort(included.begin(), included.end());

			r.emplace_back(arr, included, xmin + ymin * WIDTH, xmax - xmin + 1, ymax - ymin + 1);
		}
	}
	std::cout << canvas_file << " contains " << r.size() << " particles\n";
	return r;
}

std::string to_string(int x, int n) {
	std::ostringstream out;
    out << std::setfill('0') << std::setw(n) << x;
    return out.str();
}

void remove_file(const std::string &file_name) {
	if (remove(file_name.data())) std::cerr << strerror(errno) << ':' << file_name << '\n';
}

bool does_file_exist(const char *file_name) {
    std::ifstream f(file_name);
    return f.good();
}

void compress_batch(const char *dest, const char *data, int amount, int digits) {
	std::string file_name_start = std::string(data) + "_";

	for (int i = 0; i < amount; ++i) {
		auto file_name = file_name_start + to_string(i, digits) + ".txt";
		if (!does_file_exist(file_name.data())) {
			std::cerr << file_name << " does not exist\n";
			break;
		}
		save_batch(dest, find_particles(file_name));
	}
}

void auto_compress(const char *dest, const char *data, int digits, int check_interval, int max_time) {
	std::string file_name_start = std::string(data) + "_";

	for (int i = 0;; ++i) {
		auto file_name = file_name_start + to_string(i, digits) + ".txt";
		auto time_stamp = std::chrono::steady_clock::now();
		for (int c = 0; !does_file_exist(file_name.data()); c++) {
			if (time_stamp + std::chrono::milliseconds(max_time) <= std::chrono::steady_clock::now()) {
				std::cerr << "Could not find '" << file_name << "' in max time. Compressing stopped\n";
				return;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(check_interval));
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		save_batch(dest, find_particles(file_name));
		remove_file(file_name);
		remove_file(file_name + ".dsc");
	}
}

int main(int argc, char const *argv[]) {
	if (argc >= 2) {
		if (!strcmp(argv[1], "new")) {
			if (argc == 3) {
				new_batch_file(argv[2]);
				return 0;
			}
		} else if (!strcmp(argv[1], "batch")) {
			if (argc == 5 || argc == 6) {
				compress_batch(argv[2], argv[3], std::stoi(argv[4]), (argc == 6)? std::stoi(argv[5]): strlen(argv[4]));
				return 0;
			}
		} else if (!strcmp(argv[1], "auto")) {
			if (argc == 7) {
				auto_compress(argv[2], argv[3], std::stoi(argv[4]), std::stoi(argv[5]), std::stoi(argv[6]));
				return 0;
			}
		} else if (!strcmp(argv[1], "append")) {
			if (argc >= 3) {
				auto dest = argv[2];

				for (int i = 3; i < argc; ++i) append_batch(dest, argv[i]);

				std::ifstream f(dest);
				std::cout << dest << " now contains " << get_batch_size(f) << " particles\n";
				f.close();

				return 0;
			}
		}
	}
	std::cerr << "unknown command\n";
	return 1;
}