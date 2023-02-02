#include "particle.h"

void save_batch(const char *file_name, const std::vector<Particle> &particles) {
	std::fstream file;
	file.open(file_name, std::ios_base::out | std::ios_base::binary | std::ios_base::in);

	set_batch_size(file, get_batch_size(file) + particles.size());

	file.seekp(0, std::ios::end);

	for (auto &p : particles) {
		p.save_to_file(file);
		//p->print();
	}

	file.close();
}

void new_batch_file(const char *file_name) {
	std::ofstream file;
	file.open(file_name);
	set_batch_size(file, 0);
	file.close();
}

void append_batch(const char *destination, const char *source) {
	std::fstream  dst(destination, std::ios_base::out | std::ios_base::binary | std::ios_base::in);
	std::ifstream src(source, std::ios::binary);

	set_batch_size(dst, get_batch_size(dst) + get_batch_size(src));

	dst.seekp(0, std::ios::end);

	auto buf = src.rdbuf();
	buf->pubseekpos(sizeof(uint32_t));
	dst << buf;
}